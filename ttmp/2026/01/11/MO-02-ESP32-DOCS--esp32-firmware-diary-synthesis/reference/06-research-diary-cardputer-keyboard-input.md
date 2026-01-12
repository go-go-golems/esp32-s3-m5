---
Title: 'Research Diary: Cardputer Keyboard Input'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - keyboard
    - gpio
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Original keyboard scanning implementation
    - Path: esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp
      Note: Reusable keyboard scanner component
    - Path: esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/layout.h
      Note: Key layout and legend definitions
    - Path: esp32-s3-m5/components/cardputer_kb/REFERENCE.md
      Note: Component reference documentation
ExternalSources: []
Summary: 'Research diary documenting investigation of Cardputer keyboard scanning and scancode mapping.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Capture research trail for the Cardputer keyboard input guide.'
WhenToUse: 'Reference when writing or updating keyboard documentation.'
---

# Research Diary: Cardputer Keyboard Input

## Goal

Document the research process for understanding Cardputer keyboard matrix scanning, from raw GPIO operations to semantic action bindings.

## Step 1: Understand the hardware

Started by reading the README and source code for `0007-cardputer-keyboard-serial`.

### What I learned

The Cardputer keyboard is a **passive matrix** with:
- **3 output pins** (active scan drivers): GPIO8, GPIO9, GPIO11
- **7 input pins** (read sense lines): GPIO13, GPIO15, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7

This creates a **4×14 logical matrix** (56 keys), though the physical layout has 4 rows.

**Hardware variant alert**: Some Cardputer revisions use GPIO1/GPIO2 instead of GPIO13/GPIO15 for IN0/IN1. The firmware includes autodetection for this.

## Step 2: Trace the scan algorithm

Read `hello_world_main.c` and `matrix_scanner.cpp` to understand the scan loop.

### Scan state encoding

The 3 output pins are driven as a 3-bit counter (0-7), creating 8 scan states:

```
scan_state  OUT2  OUT1  OUT0   Purpose
    0         0     0     0    First half of matrix scan
    1         0     0     1
    2         0     1     0
    3         0     1     1
    4         1     0     0    Second half of matrix scan
    5         1     0     1
    6         1     1     0
    7         1     1     1
```

For each scan state, the 7 input pins are read. A pressed key grounds the corresponding input line (active-low), so `gpio_get_level() == 0` means pressed.

### Position derivation

The mapping from (scan_state, input_bit) to (x, y) position:

```c
int x = (scan_state > 3) ? (2 * j) : (2 * j + 1);
int y_base = (scan_state > 3) ? (scan_state - 4) : scan_state;
int y = (-y_base) + 3;  // flip to match vendor "picture" layout
```

This produces coordinates in a 4-row × 14-column grid where:
- x ∈ [0, 13] (column)
- y ∈ [0, 3] (row)

### Key number (keyNum) derivation

The vendor convention uses 1-indexed `keyNum`:

```c
uint8_t keyNum = (y * 14) + (x + 1);  // range 1..56
```

This is important for binding tables and cross-referencing vendor documentation.

## Step 3: Understand debounce and edge detection

Read the scan loop in `hello_world_main.c` to understand event handling.

### Debounce strategy

```c
bool pressed = pos_is_pressed(keys, n, x, y);
bool was_pressed = prev_pressed[y][x];

if (pressed && !was_pressed) {
    uint32_t last = last_emit_ms[y][x];
    if ((now_ms - last) < DEBOUNCE_MS) {
        // Ignore bouncy edge
    } else {
        last_emit_ms[y][x] = now_ms;
        emit_keypress_event(x, y);
    }
}
prev_pressed[y][x] = pressed;
```

**Key insight**: Debounce is per-key, not global. Each key has its own `last_emit_ms` timestamp.

### Edge detection vs level

The firmware tracks **edges**, not levels:
- `key_down` = transition from not-pressed to pressed
- The key remains in `prev_pressed` until released
- `key_up` = transition from pressed to not-pressed (not used in this firmware but conceptually available)

## Step 4: Modifier key handling

Read how Shift/Ctrl/Alt are handled.

### Modifier detection pass

Before processing key events, scan for modifier states:

```c
bool shift = false;
bool ctrl = false;
bool alt = false;

for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 14; x++) {
        if (!pos_is_pressed(keys, n, x, y)) continue;
        const char *label = s_key_map[y][x].first;
        if (strcmp(label, "shift") == 0) shift = true;
        else if (strcmp(label, "ctrl") == 0) ctrl = true;
        else if (strcmp(label, "alt") == 0) alt = true;
    }
}
```

### Character mapping with Shift

The legend table has two values per key:

```c
static const key_value_t s_key_map[4][14] = {
    {{"`", "~"}, {"1", "!"}, ...},
    ...
};

const char *s = shift ? s_key_map[y][x].second : s_key_map[y][x].first;
```

## Step 5: Understand the scancode calibrator (0023)

Read the diary and README for the calibrator firmware.

### Why calibration is needed

The Cardputer keyboard doesn't have dedicated arrow keys. Instead, navigation is implemented as **Fn-key chords**:
- `NavUp` = Fn + `;` (keyNum 29 + 40)
- `NavDown` = Fn + `.` (keyNum 29 + 54)
- `NavLeft` = Fn + `,` (keyNum 29 + 53)
- `NavRight` = Fn + `/` (keyNum 29 + 55)

These are **semantic bindings**, not hardware features. The calibrator helps discover/verify them.

### Calibration workflow

1. **Live matrix mode**: Shows all currently pressed keys
2. **Wizard mode** (toggle with GPIO0 button):
   - Prompts for specific actions (Up, Down, Left, Right, Back, etc.)
   - User holds the corresponding keys
   - After stable 400ms, captures the chord
   - Enter=accept, Del=redo, Tab=skip
3. **Export**: JSON config blob for binding tables

### Captured bindings

From the calibrator output:

| Action | keyNum chord | Physical keys |
|--------|--------------|---------------|
| NavUp | [29, 40] | Fn + `;` |
| NavDown | [29, 54] | Fn + `.` |
| NavLeft | [29, 53] | Fn + `,` |
| NavRight | [29, 55] | Fn + `/` |
| Back | [29, 2] | Fn + `1` |
| Enter | [42] | `enter` |
| Tab | [15] | `tab` |
| Space | [56] | `space` |

## Step 6: Understand the reusable component

Read `components/cardputer_kb/` to understand the abstraction.

### API structure

```cpp
namespace cardputer_kb {
    class MatrixScanner {
    public:
        void init();
        ScanSnapshot scan();
    private:
        bool use_alt_in01_ = false;
    };
    
    struct ScanSnapshot {
        std::vector<KeyPos> pressed;
        std::vector<uint8_t> pressed_keynums;
        bool use_alt_in01;
    };
}
```

### Binding decoder

For semantic actions:

```cpp
const Binding *best = decode_best(pressed_keynums, bindings, count);
if (best) {
    handle_action(best->action);
}
```

The decoder uses **most-specific-wins**: if both `[29]` (Fn alone) and `[29, 40]` (Fn+`;`) match, the longer chord wins.

## Step 7: Cardputer-ADV contrast

Read 0038 README to understand the I2C keyboard variant.

### Key differences

| Aspect | Cardputer | Cardputer-ADV |
|--------|-----------|---------------|
| Keyboard type | GPIO matrix | TCA8418 I2C controller |
| Connection | Direct GPIO | I2C bus (SDA=GPIO8, SCL=GPIO9, INT=GPIO11) |
| Scanning | Firmware polls | TCA8418 handles scan, reports via interrupt |
| Output format | Raw GPIO levels | Key events via I2C registers |

**Critical**: The same GPIOs (8, 9, 11) are used but for completely different purposes!
- Cardputer: scan outputs for keyboard matrix
- Cardputer-ADV: I2C bus for keyboard controller

## Quick Reference

### GPIO Pin Map (Cardputer keyboard)

| Function | GPIO | Notes |
|----------|------|-------|
| OUT0 | GPIO8 | Scan output bit 0 |
| OUT1 | GPIO9 | Scan output bit 1 |
| OUT2 | GPIO11 | Scan output bit 2 |
| IN0 | GPIO13 (or GPIO1) | Sense input, variant-dependent |
| IN1 | GPIO15 (or GPIO2) | Sense input, variant-dependent |
| IN2 | GPIO3 | Sense input |
| IN3 | GPIO4 | Sense input |
| IN4 | GPIO5 | Sense input |
| IN5 | GPIO6 | Sense input |
| IN6 | GPIO7 | Sense input |

### Scan timing

| Parameter | Default | Configurable |
|-----------|---------|--------------|
| Scan period | 10ms | Yes (menuconfig) |
| Debounce window | 30ms | Yes (menuconfig) |
| Settle delay | 30µs | Yes (menuconfig) |

### Key legend (4×14 matrix)

```
Row 0: `  1  2  3  4  5  6  7  8  9  0  -  =  del
Row 1: tab q  w  e  r  t  y  u  i  o  p  [  ]  \
Row 2: fn shift a  s  d  f  g  h  j  k  l  ;  '  enter
Row 3: ctrl opt alt z  x  c  v  b  n  m  ,  .  /  space
```

### Modifier keyNums

| Modifier | keyNum | Position |
|----------|--------|----------|
| Fn | 29 | Row 2, Col 0 |
| Shift | 30 | Row 2, Col 1 |
| Ctrl | 43 | Row 3, Col 0 |
| Opt | 44 | Row 3, Col 1 |
| Alt | 45 | Row 3, Col 2 |
