---
Title: 'Build plan: Cardputer serial terminal (keyboard echo + UART/USB backend)'
Ticket: 001-SERIAL-CARDPUTER-TERMINAL
Status: active
Topics:
    - esp32
    - esp32-s3
    - m5stack
    - cardputer
    - uart
    - usb-serial
    - console
    - keyboard
    - ui
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild
      Note: Reference for keyboard scan Kconfig naming and defaults
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Reference for scan mapping and USB-Serial-JTAG echo strategy
    - Path: esp32-s3-m5/0012-cardputer-typewriter/main/Kconfig.projbuild
      Note: Reference for present knobs + buffer knobs
    - Path: esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp
      Note: Reference for M5GFX+M5Canvas redraw-on-dirty typewriter buffer
    - Path: esp32-s3-m5/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md
      Note: Explains host line-buffering confusion + why to flush and optionally log per-key
    - Path: esp32-s3-m5/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/analysis/01-setup-architecture-cardputer-typewriter-keyboard-on-screen-text.md
      Note: Shows how the typewriter chapter was decomposed and validated
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-24T08:39:36.02293956-05:00
WhatFor: ""
WhenToUse: ""
---


# Executive summary

Build a **serial console client** for the **M5Stack Cardputer** under ESP-IDF where:

- The built-in **keyboard** produces text input events.
- The Cardputer LCD shows **what you type** immediately (typewriter/terminal feel).
- Those same bytes are also transmitted to a **menuconfig-selectable serial backend**:
  - **USB-Serial-JTAG** (device shows up as `/dev/ttyACM*` on the host), or
  - a hardware **UART** (`UART0/1/2` with configurable pins + baud), typically wired via the Cardputer **GROVE** port (G1/G2).

This ticket explicitly reuses two “known-good” building blocks already in this repo:

- `esp32-s3-m5/0007-cardputer-keyboard-serial`: keyboard scan + debounce + **pragmatic serial echo** (stdout + USB-Serial-JTAG direct write)
- `esp32-s3-m5/0012-cardputer-typewriter`: on-screen text buffer + **M5GFX/M5Canvas** full-screen redraw loop

# Scope / behavior contract

## MVP UX

- **Typing**: printable characters appear on-screen as you type.
- **Transmit**: typed characters are emitted to the configured backend immediately.
- **Enter**:
  - updates the on-screen buffer (newline),
  - transmits a newline sequence (configurable, typically `\r\n` for “terminal” devices).
- **Backspace**:
  - updates the on-screen buffer (delete previous char / join line),
  - transmits a backspace sequence (configurable: `0x08` vs `0x7f`, and optionally `"\b \b"` as a “local terminal erase” behavior).

## Recommended (still small) additions

Even though the initial request focuses on “send what I type”, a terminal is dramatically more useful if it can **display incoming serial bytes** too. The clean MVP extension is:

- **RX display**: read bytes from the selected backend and append them to the on-screen buffer.
  - Suggested: treat `\r`, `\n`, and `\r\n` sanely; display other bytes verbatim (or show hex for non-printables behind a debug flag).

## Important gotcha: “inline echo” + host line buffering

The `0007` tutorial documents the classic confusion mode:

- the device writes characters **without a newline**
- some host consoles only “flush” display on `\n`

That’s why `0007` does two things that we should preserve:

- `setvbuf(stdout, NULL, _IONBF, 0)` and `fflush(stdout)` after writes
- and when `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED` is set, also writes directly via:
  - `usb_serial_jtag_write_bytes(...)`

This makes “realtime” echo visible even if ESP-IDF console routing is set up in a surprising way.

# Where this should live (code layout)

Create a new chapter project:

- `esp32-s3-m5/0015-cardputer-serial-terminal/`

Rationale: this matches the existing “chapter tutorial” structure (`0007`, `0012`, `0013`, …) and keeps the code close to the hardware-specific building blocks.

Also remember the workspace layout gotcha from earlier tickets:

- Code commits happen from the `esp32-s3-m5/` git repo.
- Ticket docs live under `esp32-s3-m5/ttmp/...` (separate doc workspace).

# Architecture (what to build)

## Conceptual components

- **KeyboardScanner**
  - Reuse the scan loop + mapping from `0007` / `0012` (`kb_scan_pressed`, 4×14 map).
- **TerminalModel**
  - A terminal-like buffer (lines + cursor), reusing the `std::deque<std::string>` strategy from `0012`.
- **Renderer**
  - M5GFX autodetect + `M5Canvas` full-screen sprite redraw-on-dirty (from `0012`).
- **SerialBackend**
  - A small abstraction that can `write()` and (optionally) `read()` from either:
    - USB-Serial-JTAG, or
    - hardware UART.

## Event-driven loop (recommended)

Keep one owner for UI state (buffer + rendering) and feed it events from producers.

Pseudo-structures:

```c
enum EventType { EVT_KEY_TX_BYTES, EVT_SERIAL_RX_BYTES };

struct Event {
  EventType type;
  uint8_t bytes[64];
  size_t n;
  // optional: metadata (ctrl/alt/shift, key token name)
};
```

Pseudo-control flow:

```c
init_display_canvas();              // from 0012
init_keyboard_gpio_and_state();     // from 0007/0012
backend = init_serial_backend_from_kconfig();

QueueHandle_t q = xQueueCreate(...);
start_task(serial_rx_task, q, backend);     // optional but recommended

while (true) {
  // 1) scan keyboard periodically
  if (keyboard_has_new_token(&tok)) {
    bytes = token_to_tx_bytes(tok, kconfig_newline_mode, kconfig_backspace_mode);

    if (kconfig_local_echo) {
      terminal_model_apply_bytes(&model, bytes);   // updates buffer/cursor
      dirty = true;
    }

    backend.write(bytes);
  }

  // 2) drain RX events (if enabled)
  while (xQueueReceive(q, &ev, 0) == pdTRUE) {
    terminal_model_apply_bytes(&model, ev.bytes);
    dirty = true;
  }

  // 3) render on dirty
  if (dirty) {
    render_fullscreen_canvas(&model);
    canvas.pushSprite(0,0);
    if (CONFIG_PRESENT_WAIT_DMA) display.waitDMA();
    dirty = false;
  }

  vTaskDelay(pdMS_TO_TICKS(CONFIG_SCAN_PERIOD_MS));
}
```

## Simpler single-thread alternative (acceptable MVP)

If we want zero extra tasks, we can poll UART/USB reads in the same loop with a 0-timeout:

- `uart_read_bytes(uart_num, buf, sizeof(buf), 0)`
- `usb_serial_jtag_read_bytes(buf, sizeof(buf), 0)`

This keeps the system simple, and is likely sufficient at keyboard typing rates.

# Serial backend implementation details (ESP-IDF APIs)

## Hardware UART backend

Minimal UART bring-up sequence:

- `uart_driver_install(uart_num, rx_buf_size, tx_buf_size, event_queue_size, &queue, intr_alloc_flags)`
- `uart_param_config(uart_num, &uart_config)`
  - configure `baud_rate`, `data_bits`, `parity`, `stop_bits`, `flow_ctrl`
- `uart_set_pin(uart_num, tx_gpio, rx_gpio, rts_gpio, cts_gpio)`
- transmit: `uart_write_bytes(uart_num, data, len)`
- receive: `uart_read_bytes(uart_num, data, max_len, timeout_ticks)`

Notes:

- For a “terminal” you typically want **8N1** and no flow control unless the wiring supports it.
- For Cardputer external UART pins, make TX/RX GPIOs explicit Kconfig choices (defaults should be conservative / “unconnected” safe).

## USB-Serial-JTAG backend

The `0007` chapter uses:

- `usb_serial_jtag_is_driver_installed()`
- `usb_serial_jtag_write_bytes(ptr, len, timeout_ticks)`

For a real terminal, also support RX:

- `usb_serial_jtag_read_bytes(ptr, max_len, timeout_ticks)`

Depending on how the rest of the project is configured, you may also want to install the driver explicitly:

- `usb_serial_jtag_driver_install(&usb_serial_jtag_driver_config_t{...})`

(In some setups the console layer installs it; `0007` defensively checks `usb_serial_jtag_is_driver_installed()`.)

# menuconfig / Kconfig surface (what knobs to expose)

Reuse the keyboard scan-related options that already exist in `0007`/`0012`:

- scan period / debounce / settle delay
- optional IN0/IN1 autodetect with alternate GPIO pair
- pin assignments for OUT0/OUT1/OUT2 and IN0..IN6

Add a new “Serial backend” section:

- **Backend selection**
  - `USB-Serial-JTAG`
  - `GROVE UART (G1/G2)`
- **UART configuration**
  - UART port number (0/1/2)
  - baud rate
  - default GROVE mapping: **RX=G1 (GPIO1)**, **TX=G2 (GPIO2)**
  - TX GPIO / RX GPIO
  - optional RTS/CTS GPIOs (or fixed to `UART_PIN_NO_CHANGE`)
- **Terminal semantics**
  - local echo: on/off (important if remote device also echoes)
  - newline mode: `\n` vs `\r\n`
  - backspace mode: send `0x08` vs `0x7f` (and optionally whether to also send the “erase sequence”)
- **RX display**
  - enabled/disabled
  - optional “render non-printable as hex” debug knob

Pseudo-Kconfig sketch (names should match the chapter number you choose):

```kconfig
menu "Tutorial 0015: Cardputer Serial Terminal"

choice
  prompt "Serial backend"
  default TUTORIAL_0015_BACKEND_USB_SERIAL_JTAG
config TUTORIAL_0015_BACKEND_USB_SERIAL_JTAG
  bool "USB (USB-Serial-JTAG / ttyACM*)"
config TUTORIAL_0015_BACKEND_UART
  bool "GROVE UART (G1/G2)"
endchoice

config TUTORIAL_0015_UART_NUM
  int "UART port number"
  range 0 2
  default 1
  depends on TUTORIAL_0015_BACKEND_UART

config TUTORIAL_0015_UART_BAUD
  int "UART baud rate"
  range 1200 3000000
  default 115200
  depends on TUTORIAL_0015_BACKEND_UART

config TUTORIAL_0015_UART_TX_GPIO
  int "UART TX GPIO"
  range 0 48
  default 43
  depends on TUTORIAL_0015_BACKEND_UART

config TUTORIAL_0015_UART_RX_GPIO
  int "UART RX GPIO"
  range 0 48
  default 44
  depends on TUTORIAL_0015_BACKEND_UART

config TUTORIAL_0015_LOCAL_ECHO
  bool "Local echo typed bytes to screen"
  default y

config TUTORIAL_0015_SHOW_RX
  bool "Show incoming bytes on screen"
  default y

config TUTORIAL_0015_NEWLINE_CRLF
  bool "Transmit CRLF on Enter"
  default y

choice
  prompt "Backspace transmit byte"
  default TUTORIAL_0015_BS_SEND_DEL
config TUTORIAL_0015_BS_SEND_BS
  bool "0x08 (BS)"
config TUTORIAL_0015_BS_SEND_DEL
  bool "0x7f (DEL)"
endchoice

endmenu
```

# Implementation notes (how to reuse 0007 + 0012 without surprises)

## Keyboard scan + mapping

Both `0007` and `0012` rely on the same “picture coordinate system” key mapping:

- `static const key_value_t s_key_map[4][14] = { ... }`
- scan 8 states (3 output bits), read 7 inputs (active-low), map to x/y

Recommendation: copy the `0012` version because it already cleanly supports primary vs alt input arrays.

## On-screen rendering

Reuse the exact pattern from `0012`:

- `m5gfx::M5GFX display; display.init();`
- `M5Canvas canvas(&display); canvas.createSprite(w,h);`
- redraw the entire canvas only when `dirty`
- `display.waitDMA()` (controlled by Kconfig) after each present

This is stable and “fast enough” for typing + serial RX at typical UART speeds (115200).

# Validation checklist (manual)

- **Keyboard only**
  - type letters, numbers, punctuation
  - press `shift` and confirm map switches
  - `enter` makes a new line and scrolls
  - `del` backspaces and joins lines as expected
- **USB backend**
  - open `screen`/`idf.py monitor` and confirm transmitted bytes arrive
  - confirm characters appear immediately (no “only shows up on Enter” confusion)
- **UART backend**
  - loopback test (TX→RX) if supported, or connect to another serial device
  - verify baud/pins correctness via `ESP_LOGI` printout of the selected config
- **Echo duplication check**
  - connect to a remote device that echoes
  - toggle “local echo” off/on to confirm behavior is controllable

# References (start here)

## Source code (chapters)

- `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c`
  - keyboard scan + debounce + modifier detection
  - inline echo helpers and direct USB-Serial-JTAG write path
- `esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild`
  - keyboard scan Kconfig knobs (period/debounce/settle/autodetect/pins)
- `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp`
  - on-screen typewriter buffer + redraw loop using M5GFX + M5Canvas
- `esp32-s3-m5/0012-cardputer-typewriter/main/Kconfig.projbuild`
  - display present knobs (waitDMA, PSRAM) + keyboard scan knobs + max lines

## Prior ticket docs (diaries/analysis)

- `esp32-s3-m5/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/design-doc/01-cardputer-keyboard-serial-echo-esp-idf.md`
- `esp32-s3-m5/ttmp/2025/12/22/003-CARDPUTER-KEYBOARD-SERIAL--cardputer-keyboard-input-with-serial-echo-esp-idf/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/analysis/01-setup-architecture-cardputer-typewriter-keyboard-on-screen-text.md`
- `esp32-s3-m5/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--cardputer-typewriter-keyboard-on-screen-text/reference/01-diary.md`

## ESP-IDF APIs (by symbol name)

- UART driver: `uart_driver_install`, `uart_param_config`, `uart_set_pin`, `uart_write_bytes`, `uart_read_bytes`
- USB-Serial-JTAG: `usb_serial_jtag_driver_install`, `usb_serial_jtag_is_driver_installed`, `usb_serial_jtag_write_bytes`, `usb_serial_jtag_read_bytes`
