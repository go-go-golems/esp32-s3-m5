---
Title: 'Cardputer Keyboard Input: GPIO Matrix Scanning and Scancode Mapping'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - keyboard
    - gpio
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/
      Note: Basic keyboard → serial echo demo
    - Path: esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/
      Note: Scancode calibration tool
    - Path: esp32-s3-m5/components/cardputer_kb/
      Note: Reusable keyboard scanner component
ExternalSources:
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-reference/peripherals/gpio.html
      Note: ESP-IDF GPIO documentation
Summary: 'Complete guide to Cardputer keyboard input: matrix scanning, scancode mapping, and semantic bindings.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Help developers implement keyboard input in Cardputer projects.'
WhenToUse: 'Reference when building Cardputer applications that need keyboard input.'
---

# Understanding Cardputer Keyboard Input

The M5Stack Cardputer features a compact 56-key keyboard that connects directly to ESP32-S3 GPIOs through a passive scan matrix. Unlike USB keyboards or dedicated controller ICs, this keyboard requires firmware to actively scan the matrix, detect pressed keys, and translate raw hardware signals into usable key events.

This guide walks through the complete keyboard input pipeline: from GPIO-level matrix scanning to semantic action bindings. By the end, you'll understand exactly how the hardware works, how to implement reliable key detection, and how to handle special keys like Fn-combos for navigation.

## The Matrix: Hardware Architecture

The Cardputer keyboard is organized as a **4×14 matrix** (56 keys), scanned using a technique common in embedded keyboards: row-column multiplexing.

```
                              INPUT PINS (7 sense lines)
                    ┌───────────────────────────────────────────┐
                    │  IN0   IN1   IN2   IN3   IN4   IN5   IN6  │
                    │ (G13) (G15)  (G3)  (G4)  (G5)  (G6)  (G7) │
                    └───────────────────────────────────────────┘
                        │     │     │     │     │     │     │
                        ▼     ▼     ▼     ▼     ▼     ▼     ▼
    ┌───────────────────────────────────────────────────────────────┐
    │  Row 0:  `   1   2   3   4   5   6   7   8   9   0   -   =  del │
    │  Row 1: tab  q   w   e   r   t   y   u   i   o   p   [   ]   \ │
    │  Row 2: fn shft  a   s   d   f   g   h   j   k   l   ;   ' ent │
    │  Row 3: ctl opt alt  z   x   c   v   b   n   m   ,   .   / spc │
    └───────────────────────────────────────────────────────────────┘
                        ▲     ▲     ▲
                        │     │     │
                    ┌───┴─────┴─────┴───┐
                    │ OUT0  OUT1  OUT2  │
                    │ (G8)  (G9) (G11)  │
                    └───────────────────┘
                       OUTPUT PINS (3 scan drivers)
```

### How Matrix Scanning Works

The idea is simple: with 10 GPIO pins (3 outputs + 7 inputs), we can detect up to 56 unique key positions. Here's the principle:

1. **Drive outputs**: Set the 3 output pins to a specific pattern (0-7 in binary)
2. **Read inputs**: Check which of the 7 input pins are pulled low
3. **Map to position**: The output pattern + input bit uniquely identifies a key
4. **Repeat**: Cycle through all 8 output patterns to scan the entire matrix

Each key is a simple switch that, when pressed, connects its row wire to its column wire. By driving rows sequentially and reading columns, we can detect exactly which switches are closed.

### GPIO Pin Assignments

The Cardputer uses these specific GPIOs:

| Function | GPIO | Direction | Notes |
|----------|------|-----------|-------|
| OUT0 | GPIO8 | Output | Scan select bit 0 |
| OUT1 | GPIO9 | Output | Scan select bit 1 |
| OUT2 | GPIO11 | Output | Scan select bit 2 |
| IN0 | GPIO13 | Input | Sense line 0 (pulled up) |
| IN1 | GPIO15 | Input | Sense line 1 (pulled up) |
| IN2 | GPIO3 | Input | Sense line 2 (pulled up) |
| IN3 | GPIO4 | Input | Sense line 3 (pulled up) |
| IN4 | GPIO5 | Input | Sense line 4 (pulled up) |
| IN5 | GPIO6 | Input | Sense line 5 (pulled up) |
| IN6 | GPIO7 | Input | Sense line 6 (pulled up) |

> **Hardware Variant Alert**: Some Cardputer revisions use GPIO1/GPIO2 instead of GPIO13/GPIO15 for IN0/IN1. The firmware includes autodetection for this—more on that later.

---

## GPIO Configuration

Before scanning, configure the GPIOs appropriately:

```c
void keyboard_gpio_init(void) {
    // Output pins: push-pull, initially low
    const int out_pins[] = {8, 9, 11};
    for (int i = 0; i < 3; i++) {
        gpio_reset_pin(out_pins[i]);
        gpio_set_direction(out_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_pull_mode(out_pins[i], GPIO_PULLUP_PULLDOWN);
        gpio_set_level(out_pins[i], 0);
    }
    
    // Input pins: input with pull-up
    // Keys are active-low: pressed = pulled to ground
    const int in_pins[] = {13, 15, 3, 4, 5, 6, 7};
    for (int i = 0; i < 7; i++) {
        gpio_reset_pin(in_pins[i]);
        gpio_set_direction(in_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(in_pins[i], GPIO_PULLUP_ONLY);
    }
}
```

The pull-up configuration is essential: without it, unpressed keys would float and produce random readings. With pull-ups, unpressed keys read as `1` (high), and pressed keys read as `0` (low, connected to ground through the matrix wiring).

---

## The Scan Loop

The heart of keyboard input is the scan loop, which runs periodically (typically every 10ms) and captures the current state of all keys.

### Setting the Output Pattern

```c
static inline void kb_set_output(uint8_t scan_state) {
    scan_state &= 0x07;  // 3 bits only
    gpio_set_level(GPIO_NUM_8,  (scan_state & 0x01) ? 1 : 0);
    gpio_set_level(GPIO_NUM_9,  (scan_state & 0x02) ? 1 : 0);
    gpio_set_level(GPIO_NUM_11, (scan_state & 0x04) ? 1 : 0);
}
```

### Reading the Input Mask

```c
static inline uint8_t kb_read_inputs(void) {
    const int in_pins[] = {13, 15, 3, 4, 5, 6, 7};
    uint8_t mask = 0;
    
    for (int i = 0; i < 7; i++) {
        int level = gpio_get_level(in_pins[i]);
        if (level == 0) {  // Active-low: pressed = 0
            mask |= (1U << i);
        }
    }
    return mask;
}
```

### The Complete Scan Function

```c
typedef struct {
    int x;  // Column (0-13)
    int y;  // Row (0-3)
} KeyPos;

int kb_scan_pressed(KeyPos *out_keys, int max_keys) {
    int count = 0;
    
    for (int scan_state = 0; scan_state < 8; scan_state++) {
        // 1. Set output pattern
        kb_set_output(scan_state);
        
        // 2. Brief settle delay (10-30µs)
        esp_rom_delay_us(30);
        
        // 3. Read input mask
        uint8_t in_mask = kb_read_inputs();
        if (in_mask == 0) continue;  // No keys in this scan state
        
        // 4. Map each pressed input bit to a key position
        for (int j = 0; j < 7; j++) {
            if ((in_mask & (1U << j)) == 0) continue;
            
            // Position derivation (vendor formula)
            int x = (scan_state > 3) ? (2 * j) : (2 * j + 1);
            int y_base = (scan_state > 3) ? (scan_state - 4) : scan_state;
            int y = 3 - y_base;  // Flip to match visual layout
            
            if (x >= 0 && x <= 13 && y >= 0 && y <= 3) {
                if (count < max_keys) {
                    out_keys[count].x = x;
                    out_keys[count].y = y;
                    count++;
                }
            }
        }
    }
    
    // 5. Return outputs to idle
    kb_set_output(0);
    
    return count;
}
```

### Understanding the Position Derivation

The mapping from `(scan_state, input_bit)` to `(x, y)` follows the vendor's wiring scheme:

| Scan State | OUT2:OUT1:OUT0 | y_base | y (flipped) | x formula |
|------------|----------------|--------|-------------|-----------|
| 0 | 0:0:0 | 0 | 3 | 2j + 1 (odd columns) |
| 1 | 0:0:1 | 1 | 2 | 2j + 1 (odd columns) |
| 2 | 0:1:0 | 2 | 1 | 2j + 1 (odd columns) |
| 3 | 0:1:1 | 3 | 0 | 2j + 1 (odd columns) |
| 4 | 1:0:0 | 0 | 3 | 2j (even columns) |
| 5 | 1:0:1 | 1 | 2 | 2j (even columns) |
| 6 | 1:1:0 | 2 | 1 | 2j (even columns) |
| 7 | 1:1:1 | 3 | 0 | 2j (even columns) |

The formula interleaves even and odd columns across the two halves of the scan range. This is determined by the physical PCB routing, not by any standard—different keyboards may use different mappings.

---

## keyNum: The Vendor Scancode

Beyond (x, y) positions, the vendor firmware uses a 1-indexed "keyNum" that's useful for binding tables and configuration:

```c
// Convert position to keyNum (1-indexed, row-major)
static inline uint8_t keynum_from_xy(int x, int y) {
    if (x < 0 || x > 13 || y < 0 || y > 3) return 0;
    return (uint8_t)((y * 14) + (x + 1));
}

// Convert keyNum back to position
static inline void xy_from_keynum(uint8_t keynum, int *x, int *y) {
    if (keynum < 1 || keynum > 56) {
        *x = *y = -1;
        return;
    }
    uint8_t idx = keynum - 1;
    *x = idx % 14;
    *y = idx / 14;
}
```

This gives each key a unique number from 1 to 56:

```
Row 0:  1   2   3   4   5   6   7   8   9  10  11  12  13  14
Row 1: 15  16  17  18  19  20  21  22  23  24  25  26  27  28
Row 2: 29  30  31  32  33  34  35  36  37  38  39  40  41  42
Row 3: 43  44  45  46  47  48  49  50  51  52  53  54  55  56
```

---

## Edge Detection and Debounce

Raw key scans tell you what's currently pressed, but applications usually want **key events**: "key just pressed" or "key just released." This requires tracking state across scans.

### The Edge Detection Pattern

```c
static bool prev_pressed[4][14] = {0};
static uint32_t last_emit_ms[4][14] = {0};

void process_keyboard(void) {
    KeyPos keys[24];
    int n = kb_scan_pressed(keys, 24);
    
    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 14; x++) {
            bool is_pressed = key_in_list(keys, n, x, y);
            bool was_pressed = prev_pressed[y][x];
            
            // Detect key-down edge
            if (is_pressed && !was_pressed) {
                // Debounce: ignore if too soon after last event
                if ((now_ms - last_emit_ms[y][x]) >= DEBOUNCE_MS) {
                    last_emit_ms[y][x] = now_ms;
                    emit_key_down(x, y);
                }
            }
            
            // Detect key-up edge (if needed)
            if (!is_pressed && was_pressed) {
                emit_key_up(x, y);
            }
            
            prev_pressed[y][x] = is_pressed;
        }
    }
}
```

### Why Per-Key Debounce?

Mechanical key switches don't transition cleanly—they "bounce" for a few milliseconds, potentially generating multiple false edges. The per-key debounce timestamp ensures each key has its own cooldown period:

```
Time:     0ms   5ms   10ms   15ms   20ms   25ms   30ms
Key A:    [press]
          ↓     [bounce]         [real release]
          emit! (ignore) (ignore) (emit release)
                ↑        ↑
                within debounce window → ignored
```

A typical debounce window is 20-50ms. The Cardputer firmware defaults to 30ms.

---

## Modifier Key Handling

Keys like Shift, Ctrl, Alt, and Fn don't produce characters—they modify other keys. Handle them in a separate pass before processing character keys:

```c
bool shift = false, ctrl = false, alt = false, fn = false;

// First pass: detect modifiers
for (int i = 0; i < n; i++) {
    const char *label = key_legend[keys[i].y][keys[i].x];
    if (strcmp(label, "shift") == 0) shift = true;
    else if (strcmp(label, "ctrl") == 0) ctrl = true;
    else if (strcmp(label, "alt") == 0) alt = true;
    else if (strcmp(label, "fn") == 0) fn = true;
}

// Second pass: process other keys with modifier state
for each key_down_event {
    if (is_modifier_key) continue;
    
    char c = shift ? key_map[y][x].shifted : key_map[y][x].normal;
    handle_character(c, ctrl, alt);
}
```

### The Key Map with Shift Variants

```c
typedef struct {
    const char *normal;
    const char *shifted;
} KeyLabel;

static const KeyLabel key_map[4][14] = {
    {{"`","~"}, {"1","!"}, {"2","@"}, {"3","#"}, {"4","$"}, {"5","%"}, 
     {"6","^"}, {"7","&"}, {"8","*"}, {"9","("}, {"0",")"}, {"-","_"}, 
     {"=","+"}, {"del","del"}},
    
    {{"tab","tab"}, {"q","Q"}, {"w","W"}, {"e","E"}, {"r","R"}, {"t","T"},
     {"y","Y"}, {"u","U"}, {"i","I"}, {"o","O"}, {"p","P"}, {"[","{"},
     {"]","}"}, {"\\","|"}},
    
    {{"fn","fn"}, {"shift","shift"}, {"a","A"}, {"s","S"}, {"d","D"}, 
     {"f","F"}, {"g","G"}, {"h","H"}, {"j","J"}, {"k","K"}, {"l","L"},
     {";",":"}, {"'","\""}, {"enter","enter"}},
    
    {{"ctrl","ctrl"}, {"opt","opt"}, {"alt","alt"}, {"z","Z"}, {"x","X"},
     {"c","C"}, {"v","V"}, {"b","B"}, {"n","N"}, {"m","M"}, {",","<"},
     {".",">"},  {"/","?"}, {"space","space"}},
};
```

---

## Fn-Key Combos: Navigation Bindings

The Cardputer keyboard doesn't have dedicated arrow keys. Instead, navigation is implemented as **Fn-key chords**: holding Fn while pressing another key triggers a navigation action.

### The Binding Concept

A binding maps a **chord** (set of simultaneously pressed keyNums) to a **semantic action**:

```cpp
enum class Action {
    NavUp, NavDown, NavLeft, NavRight,
    Back, Enter, Tab, Space
};

struct Binding {
    Action action;
    uint8_t n;           // Number of required keys
    uint8_t keynums[4];  // The chord (e.g., {29, 40} for Fn + `;`)
};
```

### Captured Bindings (M5Cardputer)

These were discovered using the scancode calibrator:

| Action | keyNum Chord | Physical Keys |
|--------|--------------|---------------|
| NavUp | `[29, 40]` | Fn + `;` |
| NavDown | `[29, 54]` | Fn + `.` |
| NavLeft | `[29, 53]` | Fn + `,` |
| NavRight | `[29, 55]` | Fn + `/` |
| Back/Esc | `[29, 2]` | Fn + `1` |
| Delete | `[29, 14]` | Fn + `del` |
| Enter | `[42]` | `enter` |
| Tab | `[15]` | `tab` |
| Space | `[56]` | `space` |

### The Binding Decoder

```cpp
const Binding *decode_best(const std::vector<uint8_t> &pressed_keynums,
                           const Binding *bindings, size_t count) {
    const Binding *best = nullptr;
    
    for (size_t i = 0; i < count; i++) {
        const Binding &b = bindings[i];
        
        // Check if all required keynums are pressed
        bool matches = true;
        for (uint8_t j = 0; j < b.n; j++) {
            if (!contains(pressed_keynums, b.keynums[j])) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            // Most-specific wins: prefer longer chords
            if (best == nullptr || b.n > best->n) {
                best = &b;
            }
        }
    }
    
    return best;
}
```

The **most-specific-wins** rule handles cases where bindings overlap. For example, if you have both:
- `[29]` → some action for Fn alone
- `[29, 40]` → NavUp (Fn + `;`)

When pressing Fn + `;`, the decoder returns NavUp because `[29, 40]` is more specific than `[29]`.

---

## The Scancode Calibrator

How do you know which physical keys correspond to which navigation actions? The **scancode calibrator** (project 0023) is a dedicated firmware tool for this:

### How It Works

1. **Live Matrix Mode** (default)
   - Shows a 4×14 grid on the display
   - Highlights currently pressed keys in real-time
   - Logs pressed keyNums to serial

2. **Wizard Mode** (toggle with GPIO0 button)
   - Prompts for specific actions: "Press UP arrow"
   - User holds the intended keys (e.g., Fn + `;`)
   - After 400ms stability, captures the chord
   - `Enter` = accept, `Del` = redo, `Tab` = skip
   - After all prompts, exports config as JSON

### Running the Calibrator

```bash
cd esp32-s3-m5/0023-cardputer-kb-scancode-calibrator
./build.sh build
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash
./build.sh tmux-flash-monitor
```

### Export Format

After completing the wizard, the firmware outputs:

```
CFG_BEGIN
{
  "bindings": [
    {"action": "NavUp", "keynums": [29, 40]},
    {"action": "NavDown", "keynums": [29, 54]},
    ...
  ]
}
CFG_END
```

Convert to a C++ header with:

```bash
python3 tools/cfg_to_header.py cfg.json > bindings.h
```

---

## Hardware Variant: IN0/IN1 Autodetection

Some Cardputer units use GPIO1/GPIO2 instead of GPIO13/GPIO15 for the first two input pins. The firmware handles this automatically:

```c
static bool use_alt_in01 = false;

uint8_t get_input_mask(void) {
    if (use_alt_in01) {
        return read_from_pins({1, 2, 3, 4, 5, 6, 7});
    } else {
        uint8_t mask = read_from_pins({13, 15, 3, 4, 5, 6, 7});
        
        // If primary shows nothing, check alternate
        if (mask == 0) {
            uint8_t alt = read_from_pins({1, 2, 3, 4, 5, 6, 7});
            if (alt != 0) {
                use_alt_in01 = true;
                ESP_LOGW(TAG, "Switching to alt IN0/IN1 pins [1,2]");
                return alt;
            }
        }
        return mask;
    }
}
```

**Key insight**: The switch only happens when activity is detected on the alternate pins. When no keys are pressed, both configurations read as "all high" (idle), so we can't distinguish them without user input.

---

## The Reusable Component: `cardputer_kb`

For production code, use the reusable component instead of copy-pasting the scan logic:

### Installation

Add to your project's `CMakeLists.txt`:

```cmake
set(EXTRA_COMPONENT_DIRS "../components/cardputer_kb")
```

### Usage

```cpp
#include "cardputer_kb/scanner.h"
#include "cardputer_kb/layout.h"
#include "cardputer_kb/bindings.h"

cardputer_kb::MatrixScanner scanner;

void app_main() {
    scanner.init();
    
    while (true) {
        auto snap = scanner.scan();
        
        // Get pressed positions
        for (const auto &pos : snap.pressed) {
            const char *label = cardputer_kb::legend_for_xy(pos.x, pos.y);
            ESP_LOGI(TAG, "Key: %s", label);
        }
        
        // Check for semantic actions
        const auto *binding = cardputer_kb::decode_best(
            snap.pressed_keynums,
            kCapturedBindings,
            kCapturedBindingsCount);
        
        if (binding) {
            handle_action(binding->action);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

## Cardputer-ADV: A Different Keyboard

The Cardputer-ADV variant uses a completely different keyboard architecture:

| Aspect | Cardputer | Cardputer-ADV |
|--------|-----------|---------------|
| Keyboard IC | None (passive matrix) | TCA8418 I2C controller |
| Connection | Direct GPIO (8,9,11 + 7 inputs) | I2C bus (SDA=8, SCL=9, INT=11) |
| Scanning | Firmware scans matrix | TCA8418 handles scanning internally |
| Events | Raw GPIO levels | Key events via I2C register reads |
| Ghosting | Possible with 3+ keys | N-key rollover via controller |

**Critical GPIO conflict**: The same GPIOs (8, 9, 11) serve completely different purposes:
- Cardputer: Scan output drivers
- Cardputer-ADV: I2C SDA, SCL, and interrupt

Code written for one variant will not work on the other without modification. The ADV keyboard requires I2C communication with the TCA8418 at address `0x34`.

---

## Troubleshooting

### "Keys don't register at all"

1. **Check GPIO configuration**: Are output pins set as outputs? Input pins as inputs with pull-ups?
2. **Verify pin numbers**: Are you using the correct GPIOs for your Cardputer variant?
3. **Enable raw scan logging**: Set `CONFIG_TUTORIAL_0007_DEBUG_RAW_SCAN=y` to see scan state and input masks

### "Serial echo doesn't appear immediately"

This is often **host-side line buffering**, not firmware buffering:

```
Terminal receives:  H e l l o (no newline)
Terminal displays:  (nothing, buffered)
User presses Enter:  \r\n
Terminal displays:  Hello (all at once)
```

Solutions:
- Use `idf.py monitor` (usually unbuffered)
- Enable per-key logging: `CONFIG_TUTORIAL_0007_LOG_KEY_EVENTS=y`
- Use `screen /dev/ttyACM0 115200` for raw byte display

### "Some keys work, others don't"

- **IN0/IN1 variant mismatch**: Enable autodetect (`CONFIG_TUTORIAL_0007_AUTODETECT_IN01=y`)
- **Settle delay too short**: Increase `CONFIG_TUTORIAL_0007_SCAN_SETTLE_US`
- **Physical issue**: Check for debris or damage on the keyboard membrane

### "Keys repeat too fast or bounce"

- Increase debounce: `CONFIG_TUTORIAL_0007_DEBOUNCE_MS` (default 30ms)
- Check that debounce is per-key, not global

---

## Configuration Reference

### Kconfig Options

| Option | Default | Description |
|--------|---------|-------------|
| `SCAN_PERIOD_MS` | 10 | How often to scan (ms) |
| `DEBOUNCE_MS` | 30 | Per-key debounce window (ms) |
| `SCAN_SETTLE_US` | 30 | Delay after output change (µs) |
| `AUTODETECT_IN01` | y | Auto-switch IN0/IN1 pins |
| `DEBUG_RAW_SCAN` | n | Log raw scan masks |
| `LOG_KEY_EVENTS` | n | Log each key event |

### Default Pin Assignments

| Symbol | GPIO | Can Override |
|--------|------|--------------|
| `KB_OUT0_GPIO` | 8 | Yes |
| `KB_OUT1_GPIO` | 9 | Yes |
| `KB_OUT2_GPIO` | 11 | Yes |
| `KB_IN0_GPIO` | 13 | Yes |
| `KB_IN1_GPIO` | 15 | Yes |
| `KB_IN2_GPIO` | 3 | Yes |
| `KB_IN3_GPIO` | 4 | Yes |
| `KB_IN4_GPIO` | 5 | Yes |
| `KB_IN5_GPIO` | 6 | Yes |
| `KB_IN6_GPIO` | 7 | Yes |
| `KB_ALT_IN0_GPIO` | 1 | Yes |
| `KB_ALT_IN1_GPIO` | 2 | Yes |

---

## Quick Start: Build and Flash

### Keyboard Serial Echo Demo

```bash
cd esp32-s3-m5/0007-cardputer-keyboard-serial
idf.py set-target esp32s3
idf.py build flash monitor
```

Type on the keyboard—characters appear on the serial console. Press Enter to see the buffered line logged.

### Scancode Calibrator

```bash
cd esp32-s3-m5/0023-cardputer-kb-scancode-calibrator
./build.sh build
./build.sh -p /dev/ttyACM0 flash monitor
```

Press GPIO0 button to toggle between live matrix view and calibration wizard.
