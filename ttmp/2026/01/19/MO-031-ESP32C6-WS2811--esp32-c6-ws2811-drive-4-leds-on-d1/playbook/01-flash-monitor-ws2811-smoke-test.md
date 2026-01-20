---
Title: flash/monitor + WS2811 smoke test
Ticket: MO-031-ESP32C6-WS2811
Status: active
Topics:
    - esp-idf
    - esp32
    - gpio
    - tooling
    - tmux
    - flashing
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-19T22:13:23.089957866-05:00
WhatFor: ""
WhenToUse: ""
---

# flash/monitor + WS2811 smoke test

## Purpose

Flash the WS2811 test firmware and confirm that all 4 LEDs respond to indexed updates (pattern changes over time), while the console logs confirm the device is running and what GPIO/timing configuration is active.

## Environment Assumptions

- ESP-IDF v5.4.1 installed at `~/esp/esp-idf-5.4.1/`
- WS2811 strip powered from a stable 5V supply
- Common ground between XIAO and the strip (XIAO GND ↔ strip GND)
- Data line connected to XIAO D1 (GPIO1) ideally through a 5V level shifter

## Commands

Set variables:

```bash
export IDF_SH=~/esp/esp-idf-5.4.1/export.sh
export PROJ=/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0043-xiao-esp32c6-ws2811-4led-d1
export PORT=/dev/ttyACM0
```

Build + configure:

```bash
source "$IDF_SH"
cd "$PROJ"

idf.py set-target esp32c6
idf.py menuconfig
idf.py build
```

Flash + monitor:

```bash
source "$IDF_SH"
cd "$PROJ"

idf.py -p "$PORT" flash monitor
```

## Exit Criteria

- `idf.py monitor` prints a startup banner with the configured GPIO and timing values
- The 4 LEDs show a repeating pattern where each index can be distinguished (e.g., a moving “on” LED or rotating colors)

## Notes

- If LEDs don’t light:
  - Re-check 5V/GND wiring (and that grounds are shared)
  - Try enabling a level shifter (3.3V data can be marginal at 5V VDD)
  - Confirm color order (GRB vs RGB) and timing settings in `menuconfig`
