---
Title: 'ESP32-C6 IDF notes: analysis'
Ticket: MO-030-ESP32C6-FIRMWARE
Status: active
Topics:
    - esp-idf
    - esp32
    - gpio
    - tooling
    - tmux
    - flashing
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-19T21:28:51.77469331-05:00
WhatFor: ""
WhenToUse: ""
---

# ESP32-C6 IDF notes: analysis

## Executive Summary

The imported ESP-IDF notes describe a minimal GPIO toggle demo for the Seeed Studio XIAO ESP32C6, including the key board-specific detail that the on-board user LED is on GPIO15 and is active-low.

This ticket will produce a new ESP-IDF firmware project targeting `esp32c6` that toggles the board’s **D0 pin** (XIAO “D0” is `GPIO0`) with a configurable period. The project will be buildable in CI/locally and easy to flash + monitor via a tmux workflow.

## Problem Statement

We need a small, known-good ESP-IDF firmware for ESP32-C6 to validate the toolchain, build workflow, and a simple GPIO output on a board pin labeled **D0**.

The notes we imported focus on toggling the on-board LED (GPIO15), but the requested target is **D0**, which uses Arduino-style pin naming and must be mapped to a concrete ESP32-C6 GPIO number.

## Proposed Solution

Create a new minimal ESP-IDF project (in this repo) that:

- Uses `idf.py set-target esp32c6`
- Configures and toggles a single GPIO output in a loop (FreeRTOS delay)
- Defaults to toggling **XIAO ESP32C6 D0 = GPIO0**
- Exposes the GPIO number, active-low inversion, and blink period via `menuconfig` (Kconfig), so the same project can be reused on other ESP32-C6 boards/pins without code edits
- Includes a simple tmux workflow for build/flash/monitor

## Design Decisions

- **Pin mapping:** Treat “D0” as the board-labeled pin on Seeed XIAO ESP32C6 and map it to `GPIO0`. If the target board is not XIAO ESP32C6, this must be verified and changed via `menuconfig`.
- **Configuration via Kconfig:** Use `main/Kconfig.projbuild` for the toggle parameters to avoid hard-coding the pin and to document the mapping in `menuconfig`.
- **No interactive console:** Keep this firmware minimal and rely on standard logging + `idf.py monitor` rather than building a custom REPL/console.

## Alternatives Considered

- **Use ESP-IDF’s built-in `blink` example:** Quick, but the request is to toggle D0 specifically; `blink` defaults to on-board LED behavior and requires per-project `menuconfig` anyway.
- **Toggle the on-board LED (GPIO15) instead of D0:** Useful sanity check, but does not validate the requested external pin.

## Implementation Plan

1. Import the provided notes into ticket `sources/` and summarize key points (done).
2. Add tasks to the ticket for pin mapping validation, firmware scaffolding, build, and smoke test.
3. Create a new ESP-IDF project targeting `esp32c6` that toggles a configurable GPIO output.
4. Add a tmux helper script (or playbook) to standardize build/flash/monitor commands.
5. Build locally with `idf.py build` and record results in the diary (and commit in small steps).

## Open Questions

- Is the target board definitely **Seeed XIAO ESP32C6** (vs another ESP32-C6 dev board with a “D0” silkscreen)?
- If it is XIAO ESP32C6: do we want the default to be D0 (`GPIO0`) as requested, or the on-board LED (`GPIO15`, active-low) for immediate visible confirmation?
- Any constraints on D0 usage for this board (boot straps, peripherals, external wiring)?

## References

- Imported notes: `sources/local/esp32c6-idf.md.md`
- Seeed XIAO ESP32C6 getting started (pin map: D0 = GPIO0; BOOT = GPIO9; user LED/light = GPIO15): https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/
- Seeed XIAO ESP32C6 ESP-IDF guide (set-target + LED on GPIO15): https://wiki.seeedstudio.com/xiao_idf/
- ESP-IDF GPIO driver docs (ESP32-C6): https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/gpio.html
