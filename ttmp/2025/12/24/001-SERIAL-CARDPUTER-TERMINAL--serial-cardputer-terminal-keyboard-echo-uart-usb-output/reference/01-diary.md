---
Title: Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-24T08:39:36.150496575-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary — 001-SERIAL-CARDPUTER-TERMINAL

## Goal

Capture the investigation trail and implementation notes for building a **Cardputer serial terminal**:

- You type on the **built-in keyboard**
- You **see what you type** on the LCD immediately
- The same bytes are also **transmitted** to a **menuconfig-selectable** serial backend (**USB-Serial-JTAG** or a hardware **UART**)

## Context

This ticket is intentionally “assembled” from proven parts already in this repo:

- **Keyboard scan + debounce + IN0/IN1 autodetect + inline serial echo**: `esp32-s3-m5/0007-cardputer-keyboard-serial/`
- **On-screen typewriter buffer + M5GFX sprite-based rendering**: `esp32-s3-m5/0012-cardputer-typewriter/`
- **Existing implementation diaries / design docs** that explain the gotchas and milestones:
  - Ticket `003-CARDPUTER-KEYBOARD-SERIAL` (scope pivot notes + keyboard echo UX gotchas)
  - Ticket `006-CARDPUTER-TYPEWRITER` (chapter `0012` implementation steps and configs)

## Step 1: Create ticket + locate prior art (0007/0012 diaries)

This step established the documentation workspace for the new terminal ticket and gathered the “already solved once” context so we don’t reinvent keyboard scanning or on-screen rendering. The key outcome is that we have both **narrative diaries** and **concrete source files** to reference while specifying the new serial-terminal behavior.

### What I did
- Created docmgr ticket `001-SERIAL-CARDPUTER-TERMINAL`
- Used `docmgr doc search` to locate related diaries/design docs for `0007` and `0012`
- Identified the two most relevant “prior art” tickets:
  - `003-CARDPUTER-KEYBOARD-SERIAL` (`design-doc/01-cardputer-keyboard-serial-echo-esp-idf.md`, `reference/01-diary.md`)
  - `006-CARDPUTER-TYPEWRITER` (`analysis/01-setup-architecture-cardputer-typewriter...md`, `reference/01-diary.md`)

### Why
- We want the new terminal to be **implementation-ready**: precise config knobs, file paths, and API references.
- The sharp edges are already documented (e.g. host line-buffering and inline echo semantics); we should reuse those lessons.

### What I learned
- The “keyboard is broken” symptom is often just **inline echo + host-side line buffering** (no newline → some viewers don’t show partial lines).
- It’s valuable to have a fallback where each keypress produces an unmistakable newline log line (`CONFIG_TUTORIAL_0007_LOG_KEY_EVENTS`).

### What should be done in the future
- N/A (this was a research/orientation step).

## Step 2: Read the key source files (0007 keyboard/serial + 0012 display/typewriter)

This step mapped the written diaries to the actual code surfaces we’ll reuse. The main “aha” is that `0007` already implements a pragmatic “dual-path write”: it outputs to `stdout` *and* directly to the USB-Serial-JTAG driver so that inline echo is visible even when console routing is confusing. Separately, `0012` has a minimal-but-solid text buffer + full-screen redraw loop that is perfect for a terminal MVP.

### What I did
- Read `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c`
  - Noted the keyboard scan helpers (`kb_scan_pressed`, mapping to the 4×14 “picture” key map)
  - Noted the serial helpers:
    - `serial_write_str()` and `serial_write_ch()`
    - `usb_serial_jtag_write_bytes()` usage behind `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG_ENABLED`
- Read `esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild`
  - Captured which scan/debounce/autodetect knobs already exist (we should mirror them)
- Read `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp`
  - Noted the core loop pattern:
    - keyboard scan → token → buffer edit → `dirty=true`
    - redraw only when `dirty`
  - Noted the rendering approach:
    - `m5gfx::M5GFX display; display.init();`
    - `M5Canvas canvas(&display); canvas.createSprite(w,h); canvas.pushSprite(0,0);`
    - optional `display.waitDMA()` controlled by Kconfig
- Read `esp32-s3-m5/0012-cardputer-typewriter/main/Kconfig.projbuild`
  - Confirmed it already includes the same keyboard scan knobs as `0007` plus UI knobs like `CONFIG_TUTORIAL_0012_MAX_LINES`

### Why
- The new ticket’s analysis doc needs to be concrete about:
  - Which functions to reuse/copy
  - Which Kconfig symbols to create
  - Which ESP-IDF APIs to call for UART vs USB-Serial-JTAG

### What warrants a second pair of eyes
- The terminal behavior choice around **local echo vs remote echo**:
  - If we show typed characters immediately (local echo) *and* also append incoming serial RX to the screen, we can get duplicates when the remote device echoes input. This should be an explicit, menuconfig-controlled behavior.

### What should be done in the future
- Implement the new terminal as a chapter project that composes:
  - `0012`’s on-screen buffer + redraw path
  - a pluggable serial backend (USB-Serial-JTAG vs UART)
  - (optional) UART RX reader to show incoming bytes on-screen

## Step 3: Implement chapter 0015 scaffold + serial terminal loop (keyboard → screen + UART/USB)

This step turned the “build plan” into actual firmware code by creating a new chapter project under `esp32-s3-m5/0015-cardputer-serial-terminal/`. The chapter is a direct composition of the two proven pieces we identified earlier: the `0012` sprite/canvas UI loop and the `0007` keyboard scan model. On top of that, we added a small serial backend layer (USB-Serial-JTAG vs UART) selected via `menuconfig`.

The key design choice is to treat both typed keys and received bytes as the same thing: a stream of bytes applied to a terminal buffer. That keeps “local echo” and “RX-to-screen” easy to reason about.

### What I did
- Created `esp32-s3-m5/0015-cardputer-serial-terminal/` by cloning `0012` and removing generated artifacts.
- Renamed build identity (`project(cardputer_serial_terminal)`), README, and Kconfig surfaces to `0015`.
- Implemented `main/hello_world_main.cpp`:
  - M5GFX autodetect + full-screen `M5Canvas` redraw-on-dirty (from `0012`)
  - Keyboard scan + debounce + key mapping (from `0012`/`0007`)
  - Serial backend selection:
    - USB-Serial-JTAG: `usb_serial_jtag_write_bytes` / `usb_serial_jtag_read_bytes`
    - UART: `uart_driver_install` / `uart_param_config` / `uart_set_pin` / `uart_write_bytes` / `uart_read_bytes`
  - Local echo + optional RX-to-screen

### Why
- The ticket goal is a concrete serial terminal client, not just a plan.
- Doing this as chapter `0015` keeps tutorial organization consistent and makes it easy to copy/paste into future work.

### What worked
- The chapter now has the full “keyboard → screen + TX to backend” loop implemented with Kconfig knobs.

### What warrants a second pair of eyes
- USB-Serial-JTAG driver installation assumptions: we currently rely on the driver being installed elsewhere (console). If the console layer is disabled, reads/writes may be a no-op; reviewers should confirm whether we should explicitly install the driver in this tutorial.

### What should be done in the future
- Add a short hardware note in the `0015` README about which GPIOs are sensible defaults for external UART wiring on Cardputer, since TX/RX pins are board-specific.

## Quick Reference

### Key prior art
- `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c`
  - Keyboard scan + debounce + modifier detection + inline echo
  - Direct USB-Serial-JTAG writes: `usb_serial_jtag_write_bytes(...)`
- `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp`
  - On-screen buffer (`std::deque<std::string>`) + redraw-on-dirty
  - M5GFX + `M5Canvas` full-screen sprite present

## Usage Examples

N/A (this diary is narrative; the implementation recipe lives in the analysis doc for this ticket).

## Related

- Ticket `003-CARDPUTER-KEYBOARD-SERIAL` diary/design doc (keyboard echo UX gotchas + motivation)
- Ticket `006-CARDPUTER-TYPEWRITER` diary/analysis doc (chapter `0012` implementation milestones)
