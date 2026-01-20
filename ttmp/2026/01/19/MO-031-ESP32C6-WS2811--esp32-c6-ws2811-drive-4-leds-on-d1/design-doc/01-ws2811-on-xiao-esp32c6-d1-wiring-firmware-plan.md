---
Title: 'WS2811 on XIAO ESP32C6 D1: wiring + firmware plan'
Ticket: MO-031-ESP32C6-WS2811
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
LastUpdated: 2026-01-19T22:13:21.843631448-05:00
WhatFor: ""
WhenToUse: ""
---

# WS2811 on XIAO ESP32C6 D1: wiring + firmware plan

## Executive Summary

Drive a 4‑LED WS2811 (1‑wire) chain from a Seeed Studio XIAO ESP32C6 using the board pin **D1**, which maps to **GPIO1**. The firmware uses ESP-IDF’s RMT TX driver to generate WS2811‑compatible waveforms and sends 4×RGB frames (12 bytes) repeatedly (simple color chase) so it’s easy to confirm the full chain is addressed.

Because the LED strip is powered from **5V**, the data line often needs a **3.3V→5V level shifter** for reliability; wiring guidance and a smoke-test playbook are included in this ticket.

## Problem Statement

We have a WS2811 LED chain with three wires (**GND**, **5V**, **DAT**) and want to control 4 LEDs from XIAO ESP32C6 using the D1 pin.

Key constraints:
- WS2811 is timing-sensitive (sub‑microsecond pulse widths).
- The strip’s data input is typically referenced to the strip’s supply voltage (5V), so 3.3V logic may be marginal without level shifting.
- The strip must share a common ground with the MCU.

## Proposed Solution

### Hardware / Wiring

Connect:
- XIAO **GND** → LED strip **GND**
- External **5V supply +5V** → LED strip **5V**
- XIAO **D1 (GPIO1)** → LED strip **DAT** (preferably through a 5V level shifter)

Recommended extras (common WS281x best-practices):
- **Level shifter:** `74AHCT125` / `74HCT14` / `SN74HCT` family powered from 5V (3.3V in, 5V out). Place near the strip.
- **Series resistor:** 220–470Ω in series with DAT near the strip input to reduce ringing.
- **Bulk capacitor:** ~470–1000µF across 5V/GND at strip input to absorb inrush / transients.

Power note:
- Budget ~60mA per RGB LED at full white. For 4 LEDs, plan for ~240mA plus margin.

### Firmware

Create a new ESP-IDF project targeting `esp32c6` that:
- Uses an RMT TX channel at a high resolution (e.g., 10MHz, 0.1µs ticks).
- Uses an “LED strip encoder” (bytes encoder + reset code) adapted from ESP-IDF’s `examples/peripherals/rmt/led_strip`.
- Sends 4×RGB frames repeatedly (e.g., chase pattern), logging startup config to make it obvious the firmware is alive even if LEDs are dark.
- Exposes configuration via `menuconfig`:
  - Data GPIO (default **GPIO1** for XIAO D1)
  - LED count (default **4**)
  - Color order (start with GRB; make it configurable only if needed)
  - Timing parameters (defaults tuned for WS2811; override if the strip variant needs different timings)

## Design Decisions

- **D1 pin mapping:** For Seeed XIAO ESP32C6, D1 → GPIO1 (from the Seeed pin map).
- **RMT instead of bit-banging:** RMT produces stable waveforms without tight CPU timing loops.
- **Configurable timings:** WS2811 variants differ; making timings configurable reduces “works on my strip” surprises.

## Alternatives Considered

- **Use ESP-IDF `led_strip` component:** This IDF tree doesn’t ship a standalone `led_strip` component; the reference implementation lives in the RMT examples, so we vendor a small encoder in-project.
- **Bit-bang with `gpio_set_level` + delay loops:** fragile timing and steals CPU; RMT is the intended tool.
- **Use SPI “fake NRZ” trick:** possible on some MCUs, but RMT is simpler and more direct here.

## Implementation Plan

1. Create a new ticket and capture wiring + constraints (this doc).
2. Scaffold a new firmware project `0043-*` for XIAO ESP32C6 WS2811.
3. Vendor the LED strip encoder from ESP-IDF’s RMT example and adapt timings/defaults for WS2811.
4. Build locally (`idf.py set-target esp32c6 && idf.py build`).
5. Smoke test on hardware (flash + monitor + confirm all 4 LEDs respond) and record findings.

## Open Questions

- Confirm the strip is WS2811 (not WS2812/WS2812B) and the expected color order (often GRB).
- Does the strip accept 3.3V data at 5V VDD reliably, or do we require a level shifter?
- Desired animation: simple chase vs “set all 4 to fixed colors” vs a test pattern that uniquely identifies each LED index.

## References

- Seeed XIAO ESP32C6 pin map (D1 = GPIO1): https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/
- ESP-IDF RMT LED strip example (encoder implementation): `$IDF_PATH/examples/peripherals/rmt/led_strip`
