---
Title: 'Setup + architecture: Cardputer typewriter (keyboard -> on-screen text)'
Ticket: 006-CARDPUTER-TYPEWRITER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - keyboard
    - display
    - m5gfx
    - ui
    - text
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/apps/app_texteditor/app_texteditor.cpp
      Note: Vendor text editor patterns (scroll/cursor/backspace)
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h
      Note: Key labels + matrix pin mapping
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/005-CARDPUTER-SYNTH--cardputer-synthesizer-audio-and-keyboard-integration/analysis/02-cardputer-graphics-with-m5gfx-text-sprites-ui-patterns.md
      Note: Text rendering APIs + sprite UI composition
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Keyboard scan/debounce baseline we will port
    - Path: esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: M5GFX autodetect + full-screen sprite present baseline
ExternalSources: []
Summary: 'Architecture + setup checklist for Cardputer typewriter app: keyboard scan + M5GFX sprite text rendering + build/flash guidance.'
LastUpdated: 2025-12-22T22:48:33.637223499-05:00
WhatFor: ""
WhenToUse: ""
---



## Executive summary

We’re building a Cardputer “typewriter” app: **press keys on the built-in keyboard → text appears on the LCD**, with basic editor behavior (cursor, backspace, enter/newline, and scrolling).

This ticket reuses existing proven building blocks already in this repo:

- **Keyboard scanning (ESP-IDF)**: `esp32-s3-m5/0007-cardputer-keyboard-serial/` has a robust matrix scan + debounce + optional IN0/IN1 autodetect.
- **Display bring-up (M5GFX autodetect)**: `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/` proves that `M5GFX display; display.init();` works and that full-frame sprite presents are stable.
- **Text UI patterns (vendor)**: `M5Cardputer-UserDemo/main/apps/app_texteditor/` shows how the vendor firmware does scrolling/cursor/backspace in a sprite-based text UI.

The implementation will live as a new “chapter project” under:

- `esp32-s3-m5/0012-cardputer-typewriter/`

and the ticket docs live under:

- `echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/006-CARDPUTER-TYPEWRITER--.../`

## Goal and scope

### User-visible behavior (MVP)

- **Typing**: printable characters appear where the cursor is.
- **Backspace**: removes last character on the current line; joins previous line if at column 0.
- **Enter**: inserts a newline (starts a new line).
- **Scrolling**: when text reaches the bottom, old lines scroll off.
- **Status line** (optional, but nice): show current line/column or buffer usage.

### Non-goals (for this ticket / MVP)

- Full text editor features (selection, clipboard, cursor navigation with arrows, save/load).
- Unicode/IME.
- Fancy UI layout (system bar / keyboard bar compositor).

## Hardware + “ground truth”

### Keyboard hardware model

From vendor + ticket `005` analysis:

- **3 output GPIOs** select the scan state (3-bit value → 8 states).
- **7 input GPIOs** are pulled-up; pressed keys read low.
- The physical keys map onto a **4 rows × 14 columns** coordinate grid.

Known-good Cardputer defaults from tutorial `0007`:

- OUT: `GPIO8, GPIO9, GPIO11`
- IN: `GPIO13, GPIO15, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7`

### Display

We rely on **M5GFX autodetect** for Cardputer:

- `m5gfx::M5GFX display; display.init();`
- panel is ST7789 on SPI3 with a visible window offset; autodetect config is the golden path (see `M5GFX.cpp`).

## Architecture (what we will build)

### High-level loop

One FreeRTOS task is enough:

- Scan keyboard at a fixed period (e.g. 10ms).
- Detect “new press” edges (debounce).
- Translate key press → token (character, backspace, enter, modifiers).
- If token changed the document, render into an offscreen canvas sprite and present it.

### Data model (typewriter buffer)

Use a simple “terminal-like” structure:

- `std::vector<std::string> lines` (bounded by max lines) or a ring buffer.
- `size_t cursor_row`, `size_t cursor_col`.

Rules:

- Insert character at cursor → append to current line (MVP can be “always at end of current line”).
- Backspace → erase last char; if at start of line, merge with previous line.
- Enter → push new line; if buffer too long, drop oldest lines.

### Rendering strategy (simple and robust)

Copy the stable “sprite/canvas present” pattern:

- Create a full-screen `M5Canvas` sprite sized to `display.width()/height()`.
- On each edit event:
  - `canvas.fillScreen(TFT_BLACK)`
  - `canvas.setCursor(0,0)` + `print/printf` lines
  - `canvas.pushSprite(0,0)` (optional `display.waitDMA()`)

This is easy to reason about and fast enough for typing rates.

### Font + layout

We’ll use the default font first (or the vendor `FONT_REPL` equivalent later). For layout:

- `int line_h = canvas.fontHeight();`
- `int max_rows = canvas.height() / line_h;`
- To reduce complexity, wrap only at “max columns” computed from `canvas.textWidth("M")` or just rely on `print` clipping for MVP.

## Configuration knobs (Kconfig for chapter 0012)

Recommended Kconfig options (match existing style from `0007` and `0011`):

- **Keyboard**
  - scan period ms
  - debounce ms
  - settle delay µs
  - optional IN0/IN1 autodetect
  - GPIO pin assignments (defaults to known-good Cardputer values)
- **Display/present**
  - `waitDMA` on/off (default on)
  - sprite buffer in PSRAM (default off)
- **UI**
  - maximum stored lines (ring buffer size)
  - heartbeat logging every N key events/frames

## Build + flash workflow (important gotcha)

On this machine, flashing via `/dev/ttyACM0` can fail because the device can re-enumerate mid-flash. Use the stable by-id path:

```bash
idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash monitor
```

## Implementation checklist (what must exist in the new chapter)

In `esp32-s3-m5/0012-cardputer-typewriter/`:

- `CMakeLists.txt`
  - `EXTRA_COMPONENT_DIRS` pointing at `../../M5Cardputer-UserDemo/components/M5GFX`
- `main/`
  - `CMakeLists.txt` registering `hello_world_main.cpp` (C++)
  - `Kconfig.projbuild` defining `CONFIG_TUTORIAL_0012_*`
  - `hello_world_main.cpp` implementing:
    - M5GFX display init via autodetect
    - M5Canvas full-screen sprite
    - keyboard scan loop + text buffer
    - redraw/present on edit events
- `sdkconfig.defaults`
  - flash size 8MB
  - custom `partitions.csv` (same as other Cardputer chapters)
  - main task stack 8000
- `README.md`
  - build/flash commands
  - expected behavior and key bindings (enter/backspace/space)

## Key references (what to open first)

- **Keyboard scan + mapping**: `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c`
- **Display init + full-screen sprite present**: `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
- **Vendor text editor patterns**: `M5Cardputer-UserDemo/main/apps/app_texteditor/app_texteditor.cpp`
- **Vendor key map labels**: `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.h`
- **M5GFX Cardputer autodetect config**: `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`

