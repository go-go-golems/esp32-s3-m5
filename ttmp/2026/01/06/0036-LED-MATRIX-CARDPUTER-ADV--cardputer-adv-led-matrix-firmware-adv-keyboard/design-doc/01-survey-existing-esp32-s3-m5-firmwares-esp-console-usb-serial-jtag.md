---
Title: 'Survey: existing esp32-s3-m5 firmwares + esp_console (USB Serial/JTAG)'
Ticket: 0036-LED-MATRIX-CARDPUTER-ADV
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - console
    - usb-serial-jtag
    - keyboard
    - sensors
    - serial
    - debugging
    - flashing
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Cardputer-UserDemo-ADV/main/hal/hal_config.h
      Note: Cardputer-ADV pin definitions (keyboard INT
    - Path: 0013-atoms3r-gif-console/main/console_repl.cpp
      Note: Example of project-Kconfig switchable UART vs USB Serial/JTAG REPL
    - Path: 0025-cardputer-lvgl-demo/main/console_repl.cpp
      Note: USB Serial/JTAG only esp_console pattern + command registration
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: Backend selection pattern + log-noise reduction while REPL active
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Reference pattern for esp_console REPL startup and backend selection
    - Path: 0030-cardputer-console-eventbus/sdkconfig.defaults
      Note: Example of enforcing USB Serial/JTAG console backend in sdkconfig.defaults
    - Path: docs/01-playbook-esp-console-keyboard-ui-esp-event-nanopb-protobuf-0030.md
      Note: Long-form esp_console playbook and pitfalls used as supporting reference
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-06T19:32:59.736648436-05:00
WhatFor: ""
WhenToUse: ""
---


# Survey: existing esp32-s3-m5 firmwares + esp_console (USB Serial/JTAG)

## Executive Summary

This ticket aims to build a simple Cardputer-ADV firmware that drives an external LED matrix module (MAX7219-style 8×8) and eventually uses the Cardputer-ADV keyboard (TCA8418 over I²C) as the input device.

In this repo (`esp32-s3-m5/`), we already have multiple proven “console-first” firmwares. The most relevant pattern for this ticket is: **run an `esp_console` REPL over USB Serial/JTAG** (not UART), register a small set of commands, and treat the REPL as the “control plane” while the rest of the firmware runs as normal tasks.

## Problem Statement

We want to iterate safely and quickly on an embedded hardware integration:

1) prove the LED matrix wiring + driver first (SPI + MAX7219 register programming), controlled interactively via a console, and only then
2) add Cardputer-ADV keyboard input (TCA8418 via I²C + INT) to drive LED matrix behavior.

On Cardputer/ESP32-S3 boards, UART pins are frequently repurposed for peripherals and protocol UARTs, so the interactive console should default to **USB Serial/JTAG** to avoid corrupting traffic or stealing pins.

## Proposed Solution

### A. Reuse existing repo patterns for a console-driven firmware

Use the existing “USB Serial/JTAG + `esp_console` REPL” pattern (see `esp32-s3-m5/0030-cardputer-console-eventbus/` and `esp32-s3-m5/0025-cardputer-lvgl-demo/`):

- `sdkconfig.defaults` selects `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` and disables UART backends.
- Firmware starts the REPL via `esp_console_new_repl_usb_serial_jtag(...)` and registers commands via `esp_console_cmd_register(...)`.
- Console commands are small and do not block for long; they enqueue work or update state.

#### Existing firmwares to copy patterns from (quick survey)

| Firmware | Board | Console approach | Why it matters for this ticket |
|---|---|---|---|
| `0030-cardputer-console-eventbus/` | Cardputer | `esp_console` with backend selected via `#if CONFIG_ESP_CONSOLE_*` | Minimal, readable “REPL + command → event/state update” pattern and `sdkconfig.defaults` that forces USB Serial/JTAG. |
| `0025-cardputer-lvgl-demo/` | Cardputer | USB Serial/JTAG only REPL | Shows a “hard fail” pattern when USB REPL isn’t enabled; also demonstrates lots of commands and keeping the prompt usable. |
| `0029-mock-zigbee-http-hub/` | ESP32 | `#if CONFIG_ESP_CONSOLE_*` backend selection | Shows the same backend-selection approach plus optional log quieting while REPL is active. |
| `0013-atoms3r-gif-console/` | AtomS3R | REPL can bind to UART or USB Serial/JTAG via project Kconfig | Good reference when you need a “switchable backend” (we likely won’t for ADV, but it’s a useful template). |
| `0016-atoms3r-grove-gpio-signal-tester/` | AtomS3R | manual USB Serial/JTAG REPL (no `esp_console`) | Useful contrast: shows the minimum you must implement yourself if you skip `esp_console`. |

### B. Phase 1: LED matrix control (console-only)

Implement a minimal MAX7219 driver (SPI writes of register+data pairs), and expose a few REPL commands:

- `matrix init|clear|test|intensity <0..15>`
- `matrix set <x> <y> <0|1>` and `matrix fill <hex rows...>` (or similar)
- optional `matrix rotate <0|90|180|270>` to calibrate module orientation

For the current hardware setup, the LED matrix is **4 chained 8×8 modules** (a 32×8 strip). Phase 1 should treat “module order” and “orientation” as calibration variables, because MAX7219 modules are often rotated/flipped and the chain order depends on how the ribbon/jumpers are wired.

### C. Phase 2: Cardputer-ADV keyboard input (I²C)

Port (or re-implement) the essential parts of TCA8418 event handling:

- configure the device, enable key-event interrupts, and wire the INT pin to a GPIO ISR
- drain the FIFO (`KEY_LCK_EC` → count, then repeated reads of `KEY_EVENT_A`)
- map key numbers to ASCII (including Shift/Fn semantics), and render to LED matrix

This phase should not require any UART console or protocol changes; it should extend the same console-controlled firmware.

## Design Decisions

### Default console backend: USB Serial/JTAG

- Rationale: avoids UART pin conflicts and UART protocol corruption; consistent with existing projects and this repo’s console guidance.
- Concrete config baseline for new firmwares that use a console:
  - `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
  - `# CONFIG_ESP_CONSOLE_UART is not set` (or `CONFIG_ESP_CONSOLE_UART_DEFAULT=n` + `CONFIG_ESP_CONSOLE_UART_CUSTOM=n`)

### Prefer `esp_console` REPL over a custom line parser

- Rationale: tab-completion/history/editing, consistent lifecycle, and fewer footguns than hand-rolled parsing.
- Exception: a “manual REPL” is acceptable for tiny tools (see `0016`) but is not the default for this ticket.

### Keep console handlers small and non-blocking

- Rationale: SPI/I²C transfers are short, but any “draw / scroll / animation” should live in a task so the REPL stays responsive.

## How To Use `esp_console` Over USB Serial/JTAG (Host Workflow)

This section is the “minimal muscle memory” for interacting with `esp_console` REPL-based firmwares in this repo.

### 1) Ensure the firmware enables the backend

- Prefer `sdkconfig.defaults` to enforce:
  - `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
  - `CONFIG_ESP_CONSOLE_UART_DEFAULT=n`
  - `CONFIG_ESP_CONSOLE_UART_CUSTOM=n`

In code, prefer compile-time selection (examples: `0030`, `0029`) so the firmware doesn’t accidentally reference a backend that’s not compiled:

- `#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` → `esp_console_new_repl_usb_serial_jtag(...)`

### 2) Flash + monitor

From a tutorial directory (example):

- `idf.py flash monitor`

If multiple devices are connected, specify a port explicitly:

- Linux: `idf.py -p /dev/ttyACM0 flash monitor`
- macOS: `idf.py -p /dev/cu.usbmodem* flash monitor`

### 3) Expected “it works” signals

- You should see a REPL prompt (e.g. `bus>`, `lvgl>`, `gif>` depending on firmware).
- `help` should list the registered commands (after `esp_console_register_help_command()`).

### 4) Common pitfalls in this repo

- **No prompt / console not started**: backend disabled in `sdkconfig` (e.g. missing `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`), or the code returns early due to `esp_console_new_repl_*` error. See `0025` and `0030` for explicit log messages.
- **UART conflicts**: on Cardputer/ADV, avoid running an interactive console over UART unless you have explicitly reserved the pins and verified they don’t overlap with any peripheral/protocol UART traffic.

## Alternatives Considered

### Use UART console (default UART / custom UART)

Rejected as the default because UART pins are frequently used for peripherals/protocols on Cardputer/ADV setups. If UART must be used, it should be explicitly documented which UART/pins are reserved and verified non-overlapping with any protocol UART.

### Manual REPL (no `esp_console`)

Viable for very small tools (example: `0016-atoms3r-grove-gpio-signal-tester/`), but rejected here because we expect this firmware to grow (MAX7219 commands → keyboard integration → mapping/font/orientation tools).

## Implementation Plan

See the ticket task list for an incremental build-up plan split into:

- Phase 1: console-driven MAX7219 LED matrix control
- Phase 2: TCA8418 keyboard input and key→glyph rendering

## Open Questions

### Hardware/pinout verification

- Cardputer-ADV keyboard INT is `GPIO11` (see `M5Cardputer-UserDemo-ADV/main/hal/hal_config.h`).
- Cardputer-ADV SPI clock/mosi pins appear to be `GPIO40`/`GPIO14` in the M5 demo (same file).
- Confirm the correct **chip select** pin for the external MAX7219 module on the intended header/cable (avoid conflicts with on-board peripherals; the M5 demo uses `GPIO5` for LoRa NSS).
- Confirm I²C SDA/SCL pins for the keyboard controller on Cardputer-ADV (assumed `GPIO8/GPIO9` per external notes, but should be verified against board docs and/or the M5 demo codebase).

### LED matrix orientation

- Many MAX7219 8×8 modules are rotated/flipped relative to “row/col” expectations; plan to include quick calibration patterns.
- The current setup is **4 chained modules**; include a pattern that makes module order obvious, plus a “reverse chain order” switch.

### Keyboard keymap

- Need a definitive “key number → character” mapping for Cardputer-ADV’s matrix wiring (including Shift/Fn behavior).

## References

### In-repo firmware patterns (esp32-s3-m5)

- `0030-cardputer-console-eventbus/`: USB Serial/JTAG `esp_console` REPL + `#if CONFIG_ESP_CONSOLE_*` backend selection.
- `0025-cardputer-lvgl-demo/`: “USB Serial/JTAG only” REPL pattern with a large command set.
- `0029-mock-zigbee-http-hub/`: REPL backend selection + “quiet logs while console” option.
- `0013-atoms3r-gif-console/`: REPL start for both UART and USB Serial/JTAG, selected by project Kconfig.
- `0016-atoms3r-grove-gpio-signal-tester/`: manual line-based REPL over USB Serial/JTAG (no `esp_console`) for a tiny control plane.

### Existing docs in this repo

- `docs/01-playbook-esp-console-keyboard-ui-esp-event-nanopb-protobuf-0030.md`: long-form “how to use `esp_console`” playbook and known pitfalls.

### External codebase (Cardputer-ADV demo firmware)

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5Cardputer-UserDemo-ADV/` (especially `main/hal/hal_config.h` for pin definitions).
