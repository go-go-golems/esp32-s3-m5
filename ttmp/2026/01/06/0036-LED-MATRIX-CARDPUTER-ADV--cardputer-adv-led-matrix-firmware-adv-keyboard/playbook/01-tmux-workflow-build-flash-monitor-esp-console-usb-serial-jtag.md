---
Title: 'tmux workflow: build/flash/monitor + esp_console (USB Serial/JTAG)'
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
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0022-cardputer-m5gfx-demo-suite/build.sh
      Note: Reference implementation for auto-port-pick + tmux flash-monitor session
    - Path: 0025-cardputer-lvgl-demo/build.sh
      Note: Reference implementation for tmux flash-monitor with stable port selection
    - Path: docs/01-websocket-over-wi-fi-esp-idf-playbook.md
      Note: Existing tmux flash+monitor example and serial-port lock guidance
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-06T19:56:02.378767759-05:00
WhatFor: ""
WhenToUse: ""
---


# tmux workflow: build/flash/monitor + esp_console (USB Serial/JTAG)

## Purpose

Run ESP-IDF build/flash/monitor in a repeatable `tmux` session while interacting with an `esp_console` REPL over **USB Serial/JTAG** (preferred for Cardputer/ESP32-S3).

## Environment Assumptions

- `tmux` installed (`tmux -V`)
- ESP-IDF installed and working (you can run `idf.py --version` after sourcing `export.sh`)
- Device enumerates as a serial port (common examples):
  - `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*` (preferred, stable)
  - `/dev/ttyACM0` (often USB Serial/JTAG or USB CDC)
  - `/dev/ttyUSB0` (UART bridge)
- Firmware uses `esp_console` over USB Serial/JTAG:
  - `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`
  - UART console backends disabled unless explicitly needed

## Commands

### 0) Identify which serial port to use

```bash
ls -1 /dev/serial/by-id/ 2>/dev/null | rg -n "Espressif_USB_JTAG_serial_debug_unit" || true
ls -1 /dev/ttyACM* 2>/dev/null || true
ls -1 /dev/ttyUSB* 2>/dev/null || true
```

Pick a port (example):

```bash
export PORT=/dev/ttyACM0
```

### 1) Kill any existing monitor session holding the port

If flashing fails with “port busy” / “device or resource busy”, a monitor is often holding the port.

```bash
tmux kill-session -t 0036-matrix || true
```

### 2) One-pane flash+monitor (recommended: avoids port-lock conflicts)

Run `flash monitor` in a single pane (one process) to avoid “flash needs port but monitor holds it”.

```bash
tmux new-session -d -s 0036-matrix \
  "source \"$HOME/esp/esp-idf-5.4.1/export.sh\" >/dev/null && idf.py -C 0036-cardputer-adv-led-matrix-console -p \"$PORT\" flash monitor"
tmux attach -t 0036-matrix
```

### 3) Two-pane layout (optional)

Use only if you’re careful about port contention and you *want* a dedicated monitor pane.

```bash
tmux new-session -d -s 0036-matrix "bash -lc 'source \"$HOME/esp/esp-idf-5.4.1/export.sh\" >/dev/null; cd 0036-cardputer-adv-led-matrix-console; idf.py -p \"$PORT\" flash'"
tmux split-window -h -t 0036-matrix "bash -lc 'source \"$HOME/esp/esp-idf-5.4.1/export.sh\" >/dev/null; cd 0036-cardputer-adv-led-matrix-console; idf.py -p \"$PORT\" monitor'"
tmux select-layout -t 0036-matrix even-horizontal
tmux attach -t 0036-matrix
```

### 4) Interact with `esp_console`

Once `idf.py monitor` is attached and the firmware prints a prompt (example `matrix>`):

```text
help
matrix help
matrix init
matrix clear
matrix intensity 4
matrix row 0 0xff
```

## Exit Criteria

- `idf.py flash monitor` runs without “port busy” errors.
- You see boot logs followed by an `esp_console` prompt.
- Typing `help` prints a command list and the REPL remains responsive.

## Notes

- If interactive input is flaky in `idf.py monitor` (especially inside tmux), try:
  - a different terminal emulator
  - running `idf.py monitor` outside tmux
  - as a last resort, use a raw serial tool (e.g. `picocom`) if the firmware is configured for that device (you’ll lose monitor hotkeys/reset tooling).
- Prefer **USB Serial/JTAG** for Cardputer/ADV interactive consoles; avoid UART console unless pins are explicitly reserved and verified non-overlapping with peripherals/protocol UARTs.
