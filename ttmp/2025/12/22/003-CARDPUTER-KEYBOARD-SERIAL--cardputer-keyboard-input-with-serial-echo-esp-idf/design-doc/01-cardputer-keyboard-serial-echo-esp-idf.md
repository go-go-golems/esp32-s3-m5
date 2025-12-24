---
Title: Cardputer typewriter: keyboard → on-screen text (ESP-IDF)
Ticket: 003-CARDPUTER-KEYBOARD-SERIAL
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - keyboard
    - display
    - m5gfx
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Design for a Cardputer 'typewriter' demo: keyboard matrix input rendered on the Cardputer display using M5GFX/M5Canvas, including basic editing and cursor."
LastUpdated: 2025-12-22T19:47:17.386217575-05:00
WhatFor: ""
WhenToUse: ""
---

# Cardputer typewriter: keyboard → on-screen text (ESP-IDF)

## Executive Summary

Build a minimal-but-fun **typewriter** demo for M5Cardputer under ESP-IDF:

- Scan the built-in **keyboard matrix** (GPIO)
- Translate key presses into characters (respecting **shift**, plus special keys like **enter** and **del/backspace**)
- Render the text live on the Cardputer’s **ST7789 LCD** using **M5GFX**, optionally via an offscreen **M5Canvas** sprite

This becomes a “known-good” reference for Cardputer keyboard + display bring-up and a reusable building block for future on-device REPL/UIs.

## Problem Statement

We want a Cardputer project where “what I type on the keyboard” appears on the device’s screen immediately, with basic editing. This requires:

- Correct **keyboard scan** wiring and debouncing
- A stable **display bring-up** for Cardputer (ST7789 + backlight)
- A small **line editor** / buffer model (insert/backspace/newline, wrap, scroll)
- Rendering that is readable and doesn’t flicker

## Proposed Solution

### High-level architecture

Create a new ESP-IDF tutorial project (planned: `esp32-s3-m5/0011-cardputer-typewriter`) with:

- **KeyboardScanner**: polling GPIO matrix scan (derived from `esp32-s3-m5/0007-cardputer-keyboard-serial` and vendor `KEYBOARD::Keyboard` logic)
- **EditorState**: a tiny terminal-like buffer with cursor position
- **Renderer (M5GFX)**: monospaced font grid, draw characters into a backbuffer/canvas and present; optionally a blinking cursor

### Display initialization strategy

Use M5GFX’s Cardputer path:

- `M5GFX display; display.init();`

`M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp` contains an autodetect branch for `board_M5Cardputer` that:

- Uses SPI3 and **3-wire** mode
- Configures ST7789 with `panel_width=135`, `panel_height=240`, `offset_x=52`, `offset_y=40`, and sets `rotation=1`
- Sets PWM backlight on GPIO38

This is preferable to hardcoding the ST7789 config in the tutorial because it matches the vendor’s bring-up path.

### Input model (typewriter UX)

- **Printable characters**: insert at cursor, advance cursor
- **Backspace (`del`)**: delete previous character, move cursor left
- **Enter**: new line; if at bottom, scroll content up
- **Shift**: choose alternate map (uppercase/symbols)
- **Optional**: a blinking cursor (`_` or inverted block)

### Rendering approach

Two viable implementations:

- **Direct-to-display**: for each key event, update one cell and redraw just that rectangle.
- **Canvas-backed** (recommended): render into `M5Canvas`/`LGFX_Sprite` and `pushSprite()` to the display. This mirrors the vendor apps (e.g. `app_texteditor.cpp`) and avoids tearing.

For simplicity and robustness, the first iteration can keep a fixed-size text grid (rows × cols) and redraw either:

- the changed cell (fast), or
- the full screen on each key event (simpler, still fine at typing speed).

## Design Decisions

### Base keyboard scan on existing known-good code

- **Why**: `esp32-s3-m5/0007-cardputer-keyboard-serial` already matches the vendor pinout and scan mapping; it’s a reliable starting point.

### Use M5GFX for display, optionally via M5Canvas

- **Why**: Vendor firmware uses M5GFX for Cardputer display and uses sprites/canvases for UI, reducing flicker.

## Alternatives Considered

### Use `esp_lcd` directly

- Viable, but higher bring-up cost; M5GFX already contains a working Cardputer configuration.

### Reuse vendor `KEYBOARD::Keyboard` class directly

- Viable; may be adopted later. First iteration can keep the smaller, tutorial-friendly scan loop from `0007`.

## Implementation Plan

1. Create new tutorial project folder under `esp32-s3-m5/` (Cardputer).
2. Bring up display via `M5GFX display.init()`; clear screen; choose font + size.
3. Port the keyboard scan loop and key map; emit key events on press edges.
4. Implement `EditorState` (grid + cursor + scroll).
5. Render typed content + cursor on screen (direct draw or canvas).
6. Add README + menuconfig options (scan period, debounce, font size, cursor blink period).
7. Validate on device: type, backspace, enter, long text scroll.

## Open Questions

- Do we want to support key repeat (press-and-hold) for backspace/arrow keys, or only edge-triggered presses?
- Should the cursor be a blinking underscore, or an inverted block?

## References

- Known-good keyboard scan tutorial: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0007-cardputer-keyboard-serial/`
- Vendor text input patterns (typewriter-like):
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/apps/app_texteditor/app_texteditor.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/main/apps/app_chat/app_chat.cpp`
- Vendor display autodetect/config: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`
