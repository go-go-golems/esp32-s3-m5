---
Title: 'Cardputer Serial Terminal: USB vs Grove UART Backend Guide'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - cardputer
    - uart
    - usb-serial
    - terminal
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/
      Note: Main serial terminal implementation
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Terminal source with backend abstraction
    - Path: esp32-s3-m5/0015-cardputer-serial-terminal/main/Kconfig.projbuild
      Note: Menuconfig options for backend and terminal behavior
    - Path: esp32-s3-m5/0038-cardputer-adv-serial-terminal/
      Note: Advanced variant with REPL and fan control
ExternalSources:
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-reference/peripherals/usb_serial_jtag_console.html
      Note: ESP-IDF USB Serial/JTAG documentation
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-reference/peripherals/uart.html
      Note: ESP-IDF UART driver documentation
Summary: 'Complete guide for configuring and using the Cardputer serial terminal with USB Serial/JTAG or Grove UART backends.'
LastUpdated: 2026-01-12T10:00:00-05:00
WhatFor: 'Help developers choose and configure the correct serial backend for their use case.'
WhenToUse: 'Reference when building or configuring the Cardputer serial terminal firmware.'
---

# Building a Portable Serial Terminal with the M5Stack Cardputer

The M5Stack Cardputer—with its integrated keyboard, display, and ESP32-S3 brain—makes an ideal platform for building a portable serial terminal. In this guide, we'll walk through the architecture of the Cardputer serial terminal firmware, explore the critical choice between USB and UART backends, and show you how to configure the terminal for your specific use case.

Whether you're debugging an Arduino in the field, talking to a GPS module, or need a dedicated serial console without hauling a laptop, this firmware transforms your Cardputer into a capable serial communication tool.

## What We're Building

The serial terminal firmware creates a complete data path from keyboard to serial output:

```
┌────────────────────────────────────────────────────────────────────────┐
│                         CARDPUTER TERMINAL                              │
│                                                                         │
│   ┌─────────────┐                                                      │
│   │  KEYBOARD   │  You type on the physical keyboard                   │
│   │  (4×14 matrix)                                                     │
│   └──────┬──────┘                                                      │
│          │                                                              │
│          ▼                                                              │
│   ┌─────────────┐                                                      │
│   │  KEY SCAN   │  Matrix scan → debounce → key token                  │
│   │  LOOP       │  ("a", "enter", "space", etc.)                       │
│   └──────┬──────┘                                                      │
│          │                                                              │
│          ├────────────────────┬────────────────────┐                   │
│          │                    │                    │                   │
│          ▼                    ▼                    ▼                   │
│   ┌─────────────┐      ┌─────────────┐      ┌─────────────┐           │
│   │  TOKEN TO   │      │  LOCAL      │      │  BACKEND    │           │
│   │  BYTES      │      │  ECHO       │      │  WRITE      │           │
│   │             │      │  (optional) │      │             │           │
│   │ "enter"→\r\n│      │  Show on    │      │  USB or     │──────────►│ Serial
│   │ "a" → 0x61  │      │  screen     │      │  UART       │           │ Output
│   └─────────────┘      └──────┬──────┘      └─────────────┘           │
│                               │                                        │
│                               ▼                                        │
│                        ┌─────────────┐                                 │
│   ┌─────────────┐      │  SCREEN     │      ┌─────────────┐           │
│   │  BACKEND    │      │  BUFFER     │◄─────│  RX PARSE   │◄──────────│ Serial
│   │  READ       │─────►│  (lines[])  │      │  (optional) │           │ Input
│   └─────────────┘      └──────┬──────┘      └─────────────┘           │
│                               │                                        │
│                               ▼                                        │
│                        ┌─────────────┐                                 │
│                        │  LCD        │                                 │
│                        │  DISPLAY    │                                 │
│                        │  (M5GFX)    │                                 │
│                        └─────────────┘                                 │
└────────────────────────────────────────────────────────────────────────┘
```

The firmware handles everything: scanning the keyboard matrix, debouncing key presses, converting key tokens to serial bytes, displaying characters on the LCD, and managing bidirectional communication with the serial backend.

## The Backend Decision: USB or UART?

The terminal supports two mutually exclusive serial backends. This is the most important architectural decision you'll make, and it's selected at compile time through `menuconfig`. Let's understand each option deeply before choosing.

### Option 1: USB Serial/JTAG Backend

The ESP32-S3 includes a built-in USB Serial/JTAG peripheral that appears as a CDC-ACM device on your host computer—typically `/dev/ttyACM0` on Linux or `COM3` on Windows. When you select the USB backend, your Cardputer becomes a USB serial device that your computer can talk to directly.

**How it works under the hood:**

The USB Serial/JTAG peripheral is part of the ESP32-S3 silicon. It doesn't require any external components—just the USB-C cable you're already using for flashing. The firmware uses ESP-IDF's `usb_serial_jtag` driver to read and write bytes:

```c
// Writing to USB Serial/JTAG
usb_serial_jtag_write_bytes(data, length, timeout_ticks);

// Reading from USB Serial/JTAG
int bytes_read = usb_serial_jtag_read_bytes(buffer, max_length, timeout_ticks);
```

**When to use USB backend:**

- You want the Cardputer to act as a USB serial device connected to your PC
- You're testing the terminal firmware itself
- You want to capture terminal output on your computer
- You don't need `esp_console` REPL access simultaneously (more on this critical constraint later)

**The catch:** USB Serial/JTAG is a shared resource. The same physical USB connection and the same driver instance handle both your terminal data *and* the ESP-IDF console/REPL if you have one enabled. This leads to the most important constraint in the entire firmware, which we'll explore in detail below.

### Option 2: Grove UART Backend

The Cardputer's Grove port exposes two GPIO pins (G1 and G2) that can be configured as a hardware UART. When you select the UART backend, the terminal communicates through these pins instead of USB. You'll need an external USB↔UART adapter to connect to your computer, but you gain complete separation between terminal data and USB debugging.

**How it works under the hood:**

The firmware uses ESP-IDF's UART driver with configurable pins and baud rate:

```c
// UART configuration
uart_config_t cfg = {
    .baud_rate = CONFIG_TUTORIAL_0015_UART_BAUD,  // Default: 115200
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};

// Install driver and configure pins
uart_driver_install(UART_NUM_1, rx_buffer_size, tx_buffer_size, 0, NULL, 0);
uart_param_config(UART_NUM_1, &cfg);
uart_set_pin(UART_NUM_1,
             CONFIG_TUTORIAL_0015_UART_TX_GPIO,  // Default: GPIO2 (G2)
             CONFIG_TUTORIAL_0015_UART_RX_GPIO,  // Default: GPIO1 (G1)
             UART_PIN_NO_CHANGE,  // No RTS
             UART_PIN_NO_CHANGE); // No CTS
```

**When to use UART backend:**

- You're connecting to an external serial device (Arduino, GPS module, modem, another microcontroller)
- You want to keep USB free for debugging and REPL access
- You need a specific baud rate (1200 to 3,000,000 baud supported)
- You're building a standalone serial tool that doesn't need USB at all

### Backend Comparison at a Glance

| Aspect | USB Serial/JTAG | Grove UART |
|--------|-----------------|------------|
| Host device path | `/dev/ttyACM*` | External adapter (`/dev/ttyUSB*`) |
| Physical connection | Built-in USB-C | Grove port (G1/G2) |
| Baud rate | Fixed (USB protocol) | Configurable (115200 default) |
| Compatible with esp_console REPL | ❌ No—they compete for same transport | ✅ Yes—completely separate |
| Extra hardware needed | None | USB↔UART adapter for PC connection |
| Best for | Talking to host PC | Talking to external devices |

## The USB Serial/JTAG vs REPL Conflict (Critical)

This is the single most important constraint to understand. If you skip this section, you'll spend hours debugging why your REPL stopped working or why your terminal isn't receiving data.

### The Problem

USB Serial/JTAG on the ESP32-S3 provides a **single transport**—one pipe for bytes in, one pipe for bytes out. When you enable `esp_console` (the interactive REPL that gives you a command prompt over USB), it starts a task that continuously reads from USB Serial/JTAG looking for commands.

If your terminal firmware is *also* trying to read from USB Serial/JTAG to display incoming data on the LCD, you have two consumers fighting over the same byte stream. The result is unpredictable: sometimes the REPL gets your input, sometimes the terminal does, and the user experience is broken.

### Visualizing the Conflict

```
                              USB Serial/JTAG
                              (single transport)
                                    │
                        ┌───────────┴───────────┐
                        │                       │
                        ▼                       ▼
                ┌───────────────┐       ┌───────────────┐
                │ esp_console   │       │ Terminal RX   │
                │ REPL task     │       │ polling loop  │
                │               │       │               │
                │ Waiting for   │       │ Waiting for   │
                │ commands...   │       │ serial data...│
                └───────────────┘       └───────────────┘
                        │                       │
                        └───────────┬───────────┘
                                    │
                                    ▼
                        ┌───────────────────────┐
                        │  WHO GETS THE BYTES?  │
                        │  (Race condition!)    │
                        └───────────────────────┘
```

### The Solution: Choose One

You have two clean options:

**Option A: Use UART backend + USB REPL (recommended for development)**

This is the configuration we recommend during development. Your terminal talks to external devices through the Grove UART port, while USB Serial/JTAG remains dedicated to `esp_console`. You get full REPL access for debugging while the terminal does its job.

```
USB-C ─────► esp_console REPL (debugging, commands)
Grove ─────► Terminal data (to/from external device)
```

**Option B: Use USB backend + disable REPL**

If you specifically want the Cardputer to act as a USB serial device (talking to your PC, not an external device), select the USB backend. The advanced terminal firmware (`0038`) detects this and automatically skips starting the REPL:

```c
// From 0038-cardputer-adv-serial-terminal/main/console_repl.cpp
void console_start(void) {
#if defined(CONFIG_TUTORIAL_0038_BACKEND_USB_SERIAL_JTAG)
    // The on-screen terminal binds USB-Serial-JTAG for raw byte I/O.
    // Avoid fighting over the same transport.
    ESP_LOGW(TAG, "USB backend selected; skipping esp_console to avoid USB RX conflicts");
    return;
#endif
    // ... REPL startup code ...
}
```

### Quick Decision Guide

Ask yourself: "Do I need to type commands to the firmware itself (REPL), or am I only using the terminal to talk to something else?"

- **Need REPL** → Use UART backend (default in `0038`)
- **No REPL needed, want USB serial device** → Use USB backend

## Wiring the Grove UART

If you've chosen the UART backend, you'll need to wire the Grove port to your target device. The Cardputer's Grove port is a standard 4-pin connector with the following pinout:

### Grove Port Pinout

```
┌─────────────────────────────────────┐
│         CARDPUTER GROVE PORT        │
│                                     │
│    ┌─────┐                         │
│    │  1  │  G1 (GPIO1) ─── RX      │
│    ├─────┤                         │
│    │  2  │  G2 (GPIO2) ─── TX      │
│    ├─────┤                         │
│    │  3  │  5V (Power out)         │
│    ├─────┤                         │
│    │  4  │  GND                    │
│    └─────┘                         │
│                                     │
│    Looking at the Cardputer        │
│    with screen facing you          │
└─────────────────────────────────────┘
```

By default, the firmware configures:
- **G1 (GPIO1)** as UART RX (data *into* the Cardputer)
- **G2 (GPIO2)** as UART TX (data *out of* the Cardputer)

### The TX/RX Crossing Rule

UART is a point-to-point protocol. Each device has a transmit (TX) pin that sends data and a receive (RX) pin that receives data. For two devices to communicate, **you must cross the signals**: one device's TX connects to the other's RX, and vice versa.

This is one of the most common wiring mistakes. Here's the correct configuration:

```
┌─────────────────────┐                    ┌─────────────────────┐
│     CARDPUTER       │                    │    USB↔UART         │
│                     │                    │    ADAPTER          │
│                     │                    │                     │
│   G1 (RX) ●─────────┼────────────────────┼─────● TX            │
│           ◄─────────┼── Data flows in ───┼─────                │
│                     │                    │                     │
│   G2 (TX) ●─────────┼────────────────────┼─────● RX            │
│           ─────────►┼── Data flows out ──┼─────►               │
│                     │                    │                     │
│   GND    ●──────────┼────────────────────┼─────● GND           │
│                     │                    │                     │
└─────────────────────┘                    └─────────────────────┘

CORRECT: TX → RX, RX → TX (crossed)
```

**What happens if you wire TX→TX and RX→RX?**

Both devices will be trying to transmit on the same wire while both are listening on a wire that has no signal. You'll see nothing—no data in either direction.

### Connecting to Common Devices

**USB↔UART Adapter (FTDI, CP2102, CH340):**

These adapters let you connect the Cardputer's UART to your computer:

| Cardputer | Adapter |
|-----------|---------|
| G1 (RX) | TX |
| G2 (TX) | RX |
| GND | GND |

On your computer, the adapter will appear as `/dev/ttyUSB0` (Linux) or `COM4` (Windows). Use your favorite terminal program (`screen`, `minicom`, PuTTY) at 115200 baud to communicate.

**Arduino or other microcontroller:**

| Cardputer | Arduino |
|-----------|---------|
| G1 (RX) | TX (or a SoftwareSerial TX) |
| G2 (TX) | RX (or a SoftwareSerial RX) |
| GND | GND |

Make sure both devices are configured for the same baud rate (default: 115200).

**3.3V vs 5V: Level Considerations**

The ESP32-S3 operates at 3.3V logic levels. Most modern USB↔UART adapters support 3.3V (check the jumper or switch on your adapter). If you're connecting to a 5V device like an Arduino Uno, you may need a level shifter—though in practice, the ESP32-S3's inputs are often 5V-tolerant for receive, and 3.3V output is usually sufficient for 5V devices to recognize as logic high.

## Configuring the Terminal

All configuration happens through ESP-IDF's `menuconfig` system. The settings are baked into the firmware at compile time.

### Accessing the Configuration Menu

```bash
cd 0015-cardputer-serial-terminal
source ~/esp/esp-idf-5.4.1/export.sh
idf.py menuconfig
```

Navigate to: **Tutorial 0015: Cardputer Serial Terminal**

You'll see a menu with all configurable options:

```
┌──────────────────────────────────────────────────────────────────────┐
│                Tutorial 0015: Cardputer Serial Terminal              │
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│    [*] Wait for DMA completion after present (avoid tearing)         │
│    [ ] Allocate M5Canvas sprite in PSRAM (disable for DMA)           │
│    (0) Log heartbeat every N edit events (0=disabled)                │
│    (256) Maximum stored lines in buffer                              │
│                                                                      │
│    Serial backend (USB (USB-Serial-JTAG / ttyACM*))  --->           │
│                                                                      │
│    [*] Local echo typed bytes to screen                              │
│    [*] Show incoming bytes on screen                                 │
│    [*] Transmit CRLF on Enter                                        │
│    Backspace transmit byte (0x7f (DEL))  --->                       │
│                                                                      │
│    (10) Keyboard scan period (ms)                                    │
│    (30) Per-key debounce (ms)                                        │
│    ...                                                               │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

### Serial Backend Selection

The most important setting. Select **Serial backend** to see your options:

```
Serial backend
    ( ) USB (USB-Serial-JTAG / ttyACM*)
    (X) GROVE UART (G1/G2)
```

If you select **GROVE UART**, additional options appear:

```
(1) UART port number (controller)        ← Which UART peripheral (0, 1, or 2)
(115200) GROVE UART baud rate            ← 1200 to 3000000
(2) GROVE UART TX GPIO (default: G2)     ← Output pin
(1) GROVE UART RX GPIO (default: G1)     ← Input pin
```

The defaults (UART1, 115200 baud, TX=GPIO2, RX=GPIO1) work for the standard Grove port configuration. Only change these if you've wired things differently.

### Terminal Behavior Settings

These settings control how the terminal handles input and output:

**Local echo typed bytes to screen (`CONFIG_TUTORIAL_0015_LOCAL_ECHO`)**

When enabled (default), every character you type appears on the LCD immediately. This provides instant visual feedback—you see what you're typing without waiting for a response from the remote device.

*When to disable:* If the device you're talking to echoes back every character you send (like most interactive shells), you'll see each character twice. Disable local echo to let the remote device's echo be your only feedback.

**Show incoming bytes on screen (`CONFIG_TUTORIAL_0015_SHOW_RX`)**

When enabled (default), bytes received from the serial backend are displayed on the LCD. This is what makes it a terminal—you see both what you type and what the device sends back.

*When to disable:* If you're using the Cardputer purely as a one-way serial transmitter and don't need to see responses.

**Transmit CRLF on Enter (`CONFIG_TUTORIAL_0015_NEWLINE_CRLF`)**

When enabled (default), pressing Enter sends `\r\n` (carriage return + line feed). This is what most systems expect for "end of line."

*When to disable:* Some Unix systems prefer just `\n`. Disable this if you're seeing double-spaced output or other newline weirdness.

**Backspace transmit byte**

Choose between:
- **0x7F (DEL)** — The ASCII DEL character. Most modern systems interpret this as "delete character to the left."
- **0x08 (BS)** — The ASCII backspace character. Some older systems or specific protocols expect this.

### Echo Behavior Matrix

Understanding echo modes is crucial for a good terminal experience. Here's how different combinations behave:

| Your Setting | Remote Device | What You See |
|--------------|---------------|--------------|
| LOCAL_ECHO=on | Echoes input | Double characters (type 'a', see 'aa') |
| LOCAL_ECHO=on | Silent (no echo) | Single characters (correct) |
| LOCAL_ECHO=off | Echoes input | Single characters (correct) |
| LOCAL_ECHO=off | Silent (no echo) | Nothing until response arrives |

**The rule of thumb:** Enable LOCAL_ECHO when talking to silent devices. Disable LOCAL_ECHO when talking to devices that echo.

## Understanding the Terminal Buffer

The terminal maintains a text buffer that stores the displayed content. Understanding how this buffer works helps explain the terminal's behavior and limitations.

### Buffer Architecture

The buffer is implemented as a `std::deque<std::string>`, where each string represents one line of text:

```cpp
std::deque<std::string> lines;  // Each element is one line
size_t dropped_lines = 0;       // Count of lines scrolled out of buffer
```

New characters are appended to the last line. When a newline is received, a new empty string is pushed to the deque. When the buffer exceeds `CONFIG_TUTORIAL_0015_MAX_LINES` (default: 256), the oldest lines are removed from the front.

### How Bytes Become Display Content

The `term_apply_byte()` function processes each incoming byte:

```cpp
static void term_apply_byte(std::deque<std::string> &lines, 
                            size_t max_lines,
                            size_t &dropped_lines,
                            term_rx_state_t &st, 
                            uint8_t b) {
    // Handle carriage return
    if (b == '\r') {
        st.pending_cr = true;
        buf_newline(lines, max_lines, dropped_lines);
        return;
    }
    
    // Handle line feed (skip if immediately after CR to handle CRLF)
    if (b == '\n') {
        if (st.pending_cr) {
            st.pending_cr = false;
            return;  // Already created newline for the CR
        }
        buf_newline(lines, max_lines, dropped_lines);
        return;
    }
    st.pending_cr = false;

    // Handle backspace/delete
    if (b == 0x08 || b == 0x7f) {
        buf_backspace(lines);
        return;
    }
    
    // Handle tab (expand to 4 spaces)
    if (b == '\t') {
        buf_insert_char(lines, ' ');
        buf_insert_char(lines, ' ');
        buf_insert_char(lines, ' ');
        buf_insert_char(lines, ' ');
        return;
    }
    
    // Printable ASCII characters
    if (b >= 32 && b <= 126) {
        buf_insert_char(lines, (char)b);
        return;
    }
    
    // Non-printable: ignored
}
```

Note that the terminal currently handles only ASCII. Extended characters and ANSI escape sequences are ignored. This is a deliberate simplification for the MVP.

### Display Rendering

The display is redrawn only when the `dirty` flag is set (after any buffer change). The render loop draws a header with status information, then the visible portion of the buffer, then a cursor:

```cpp
if (dirty) {
    dirty = false;
    canvas.fillScreen(TFT_BLACK);
    
    // Header rows
    canvas.printf("Cardputer Serial Terminal (0015)\n");
    canvas.printf("Backend: UART1 115200 baud\n");
    // ... more header lines ...
    
    // Calculate visible lines
    int text_rows = (canvas.height() / line_height) - header_rows;
    size_t visible = min(lines.size(), text_rows);
    size_t start = lines.size() - visible;
    
    // Draw visible lines
    for (size_t i = start; i < lines.size(); i++) {
        canvas.print(lines[i].c_str());
    }
    
    // Draw cursor
    int cursor_x = canvas.textWidth(lines.back().c_str());
    canvas.fillRect(cursor_x, cursor_y, cursor_width, 2, TFT_WHITE);
    
    // Push to display
    canvas.pushSprite(0, 0);
    display.waitDMA();
}
```

## The Serial Backend Abstraction

The firmware uses a simple abstraction layer to handle both USB and UART backends through the same interface. This makes the main loop backend-agnostic.

### Backend Data Structure

```cpp
typedef enum {
    BACKEND_USB_SERIAL_JTAG = 0,
    BACKEND_UART = 1,
} backend_kind_t;

typedef struct {
    backend_kind_t kind;
    uart_port_t uart_num;
    bool usb_driver_ok;
} serial_backend_t;
```

### Initialization

The `backend_init()` function configures the appropriate driver based on the compile-time selection:

```cpp
static void backend_init(serial_backend_t &b) {
#if CONFIG_TUTORIAL_0015_BACKEND_UART
    b.kind = BACKEND_UART;
    b.uart_num = (uart_port_t)CONFIG_TUTORIAL_0015_UART_NUM;

    uart_config_t cfg = {
        .baud_rate = CONFIG_TUTORIAL_0015_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(b.uart_num, 1024, 0, 0, NULL, 0);
    uart_param_config(b.uart_num, &cfg);
    uart_set_pin(b.uart_num,
                 CONFIG_TUTORIAL_0015_UART_TX_GPIO,
                 CONFIG_TUTORIAL_0015_UART_RX_GPIO,
                 UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);
#else
    b.kind = BACKEND_USB_SERIAL_JTAG;
    
    // Install USB driver if not already installed
    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        cfg.rx_buffer_size = 1024;
        cfg.tx_buffer_size = 1024;
        usb_serial_jtag_driver_install(&cfg);
    }
    b.usb_driver_ok = usb_serial_jtag_is_driver_installed();
#endif
}
```

### Reading and Writing

The `backend_read()` and `backend_write()` functions dispatch to the appropriate driver:

```cpp
static int backend_write(serial_backend_t &b, const uint8_t *data, size_t len) {
    if (b.kind == BACKEND_UART) {
        return uart_write_bytes(b.uart_num, (const char *)data, len);
    }
    return usb_serial_jtag_write_bytes(data, len, 0);
}

static int backend_read(serial_backend_t &b, uint8_t *data, size_t max_len) {
    if (b.kind == BACKEND_UART) {
        int n = uart_read_bytes(b.uart_num, data, max_len, 0);
        return n < 0 ? 0 : n;
    }
    return usb_serial_jtag_read_bytes(data, max_len, 0);
}
```

The `0` timeout parameter means non-blocking: the function returns immediately with whatever bytes are available (possibly zero).

## Building and Flashing

### Prerequisites

Before building, ensure you have:

- **ESP-IDF 5.4.x** installed (5.4.1 or later recommended)
- **M5Stack Cardputer** connected via USB-C
- **Linux/macOS/WSL** environment (Windows native requires additional setup)

### Build Commands

```bash
# Navigate to the project
cd esp32-s3-m5/0015-cardputer-serial-terminal

# Source the ESP-IDF environment
source ~/esp/esp-idf-5.4.1/export.sh

# Set the target chip
idf.py set-target esp32s3

# Build the firmware
idf.py build
```

The build process takes 1-3 minutes depending on your machine. You'll see output like:

```
...
Project build complete. To flash, run:
 idf.py flash
or
 idf.py -p (PORT) flash
```

### Flashing and Monitoring

```bash
idf.py flash monitor
```

This flashes the firmware and immediately opens the serial monitor. You should see boot output like:

```
I (324) cardputer_serial_terminal_0015: boot; free_heap=278432 dma_free=253952
I (334) cardputer_serial_terminal_0015: bringing up M5GFX display via autodetect...
I (544) cardputer_serial_terminal_0015: display ready: width=240 height=135
I (554) cardputer_serial_terminal_0015: canvas ok: 64800 bytes; free_heap=213456
I (564) cardputer_serial_terminal_0015: serial backend: USB-Serial-JTAG (installed=1)
I (574) cardputer_serial_terminal_0015: keyboard scan pins: OUT=[8,9,11] IN=[13,15,3,4,5,6,7]
I (584) cardputer_serial_terminal_0015: ready: local_echo=1 show_rx=1 newline_crlf=1
```

Press `Ctrl+]` to exit the monitor.

### Dealing with Device Enumeration Issues

Sometimes the USB device path (`/dev/ttyACM0`) changes during flashing as the device resets. If you see errors like "device not found" or "permission denied," try these solutions:

**Use the stable by-id path:**

```bash
# List available devices
ls -la /dev/serial/by-id/

# Look for something like:
# usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00

# Flash using the stable path
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...-if00' flash monitor
```

**Lower the flash baud rate:**

```bash
idf.py -b 115200 flash
```

**Use bootloader mode:**

Hold the BOOT button (G0) while pressing and releasing the RESET button. This forces the chip into download mode where device enumeration is more stable.

## What You'll See on the Display

Once running, the Cardputer LCD shows the terminal interface:

```
┌────────────────────────────────────────┐
│ Cardputer Serial Terminal (0015)       │  ← Title
│ Backend: UART1 115200 baud             │  ← Active backend
│ Last: a @(5,2) shift=0 ctrl=0 alt=0    │  ← Last keypress info
│ Lines: 12 (dropped=0)  Events: 45      │  ← Buffer stats
│────────────────────────────────────────│
│ > ls -la                               │  ← Terminal content
│ total 156                              │  │
│ drwxr-xr-x 12 user user  4096 Jan 1    │  │
│ -rw-r--r--  1 user user   256 Jan 1    │  │
│ _                                      │  ← Cursor
└────────────────────────────────────────┘
```

**Header information:**
- **Line 1:** Firmware name and version
- **Line 2:** Current backend and baud rate (for UART) or just "USB-Serial-JTAG"
- **Line 3:** Debug info about the last key pressed, including matrix position and modifier states
- **Line 4:** Buffer statistics (lines in buffer, dropped lines, edit events)

**Terminal area:**
- Scrolling text content (most recent lines visible)
- Simple underscore cursor at the insertion point

## Keyboard Mapping

The Cardputer's keyboard is a 4×14 matrix matching the layout printed on the keys. The firmware scans this matrix and converts physical key positions to ASCII characters or control tokens.

### The Key Map

```cpp
static const key_value_t s_key_map[4][14] = {
    // Row 0: Number row
    {{"`","~"}, {"1","!"}, {"2","@"}, {"3","#"}, {"4","$"}, {"5","%"}, 
     {"6","^"}, {"7","&"}, {"8","*"}, {"9","("}, {"0",")"}, {"-","_"}, 
     {"=","+"}, {"del","del"}},
     
    // Row 1: QWERTY row
    {{"tab","tab"}, {"q","Q"}, {"w","W"}, {"e","E"}, {"r","R"}, {"t","T"},
     {"y","Y"}, {"u","U"}, {"i","I"}, {"o","O"}, {"p","P"}, {"[","{"}, 
     {"]","}"}, {"\\","|"}},
     
    // Row 2: Home row
    {{"fn","fn"}, {"shift","shift"}, {"a","A"}, {"s","S"}, {"d","D"}, 
     {"f","F"}, {"g","G"}, {"h","H"}, {"j","J"}, {"k","K"}, {"l","L"}, 
     {";",":"}, {"'","\""}, {"enter","enter"}},
     
    // Row 3: Bottom row
    {{"ctrl","ctrl"}, {"opt","opt"}, {"alt","alt"}, {"z","Z"}, {"x","X"},
     {"c","C"}, {"v","V"}, {"b","B"}, {"n","N"}, {"m","M"}, {",","<"}, 
     {".",">"},  {"/","?"}, {"space","space"}},
};
```

Each entry has two values: `{normal, shifted}`. When Shift is held, the second value is used.

### Special Keys

| Key | Token | Serial Bytes Sent |
|-----|-------|-------------------|
| Enter | `enter` | `\r\n` (or `\n` if CRLF disabled) |
| Space | `space` | `0x20` |
| Tab | `tab` | `0x09` |
| Delete/Backspace | `del` | `0x7F` (or `0x08` if BS mode) |
| Shift | `shift` | Modifier only (not sent) |
| Ctrl | `ctrl` | Modifier only (not sent) |
| Alt | `alt` | Modifier only (not sent) |

## Troubleshooting Guide

### Problem: Characters Appear Twice

**Symptom:** You type "hello" and see "hheelllloo" on the screen.

**Cause:** Both local echo and remote echo are enabled. When you type 'h', the local echo immediately shows 'h', then the remote device echoes 'h' back, which is also displayed.

**Solution:** Disable local echo in menuconfig:
1. Run `idf.py menuconfig`
2. Navigate to **Tutorial 0015: Cardputer Serial Terminal**
3. Disable **Local echo typed bytes to screen**
4. Rebuild and flash

### Problem: No Response from Remote Device (UART)

**Symptom:** You type but nothing appears from the device, even though you know it should respond.

**Diagnostic steps:**

1. **Check wiring:** Are TX and RX crossed correctly? (Cardputer G2 → Device RX, Cardputer G1 → Device TX)

2. **Check GND:** Is ground connected between devices?

3. **Check baud rate:** Do both devices use the same baud rate? Mismatched baud rates produce garbage or nothing.

4. **Check in logs:** The boot log shows:
   ```
   I (xxx) ...: serial backend: UART1 baud=115200 tx=2 rx=1
   ```
   Verify this matches your configuration.

5. **Enable debug logging:** Set `CONFIG_TUTORIAL_0015_LOG_KEY_EVENTS=y` in menuconfig to confirm keys are being detected.

### Problem: USB Backend Not Working

**Symptom:** USB backend selected, but host can't communicate with the device.

**Diagnostic steps:**

1. **Check driver installation:** Boot log should show:
   ```
   I (xxx) ...: serial backend: USB-Serial-JTAG (installed=1)
   ```
   If `installed=0`, the driver failed to install.

2. **Check device enumeration:** On Linux, run `dmesg | tail` after plugging in. You should see:
   ```
   usb 1-2: new full-speed USB device number 12 using xhci_hcd
   usb 1-2: New USB device found, idVendor=303a, idProduct=1001
   cdc_acm 1-2:1.0: ttyACM0: USB ACM device
   ```

3. **Check permissions:** On Linux, you may need to add yourself to the `dialout` group:
   ```bash
   sudo usermod -a -G dialout $USER
   ```
   Then log out and back in.

### Problem: Keyboard Not Responding

**Symptom:** The terminal boots but nothing happens when you press keys.

**Diagnostic steps:**

1. **Check boot log for keyboard pins:**
   ```
   I (xxx) ...: keyboard scan pins: OUT=[8,9,11] IN=[13,15,3,4,5,6,7]
   ```

2. **Enable key event logging:** Set `CONFIG_TUTORIAL_0015_LOG_KEY_EVENTS=y` and check for log output when pressing keys.

3. **Check for pin conflicts:** If using UART backend on GPIO1/GPIO2, the firmware should log:
   ```
   W (xxx) ...: keyboard autodetect disabled: UART pins conflict with alt IN0/IN1
   ```
   This is expected behavior—the keyboard continues to work on the primary pins.

### Problem: Flash Keeps Failing

**Symptom:** `idf.py flash` fails with timeout errors or device not found.

**Solutions:**

1. **Use stable device path** (see "Building and Flashing" section)

2. **Enter bootloader manually:** Hold BOOT (G0) while pressing RESET

3. **Check USB cable:** Some cables are charge-only without data lines

4. **Reduce flash speed:**
   ```bash
   idf.py -b 115200 flash
   ```

## API Reference

### Key Functions

| Function | Purpose |
|----------|---------|
| `backend_init()` | Initialize USB or UART backend based on config |
| `backend_write()` | Send bytes to the active backend |
| `backend_read()` | Read available bytes from the active backend |
| `term_apply_byte()` | Process one byte for display (handles CR, LF, BS, printables) |
| `buf_insert_char()` | Append a character to the current line |
| `buf_newline()` | Start a new line in the buffer |
| `buf_backspace()` | Remove last character or join with previous line |
| `kb_scan_pressed()` | Scan keyboard matrix, return pressed key positions |
| `token_to_tx_bytes()` | Convert key token ("enter", "space", "a") to serial bytes |

### Configuration Symbols

| Kconfig Symbol | Default | Description |
|----------------|---------|-------------|
| `CONFIG_TUTORIAL_0015_BACKEND_UART` | n | Use Grove UART backend |
| `CONFIG_TUTORIAL_0015_BACKEND_USB_SERIAL_JTAG` | y | Use USB backend |
| `CONFIG_TUTORIAL_0015_UART_BAUD` | 115200 | UART baud rate |
| `CONFIG_TUTORIAL_0015_UART_TX_GPIO` | 2 | UART TX pin (G2) |
| `CONFIG_TUTORIAL_0015_UART_RX_GPIO` | 1 | UART RX pin (G1) |
| `CONFIG_TUTORIAL_0015_LOCAL_ECHO` | y | Echo typed chars locally |
| `CONFIG_TUTORIAL_0015_SHOW_RX` | y | Display received bytes |
| `CONFIG_TUTORIAL_0015_NEWLINE_CRLF` | y | Send CRLF on Enter |
| `CONFIG_TUTORIAL_0015_BS_SEND_DEL` | y | Backspace sends 0x7F |
| `CONFIG_TUTORIAL_0015_MAX_LINES` | 256 | Buffer size in lines |

## Next Steps

With the serial terminal working, you might want to explore:

- **The advanced terminal (`0038`):** Adds a TCA8418 I²C keyboard, fan relay control, and `esp_console` REPL integration
- **Custom protocols:** Modify `term_apply_byte()` to handle custom escape sequences or binary protocols
- **ANSI color support:** Extend the terminal to parse and render ANSI color codes
- **History and editing:** Add command history and line editing like a real terminal

The serial terminal serves as both a practical tool and a template for building more sophisticated serial communication projects on the Cardputer platform.
