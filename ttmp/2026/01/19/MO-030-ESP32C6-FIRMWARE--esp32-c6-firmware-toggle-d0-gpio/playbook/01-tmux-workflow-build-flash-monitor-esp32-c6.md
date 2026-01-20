---
Title: 'tmux workflow: build/flash/monitor (ESP32-C6)'
Ticket: MO-030-ESP32C6-FIRMWARE
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
RelatedFiles:
    - Path: esp32-s3-m5/0042-xiao-esp32c6-d0-gpio-toggle/README.md
      Note: Project commands mirror this playbook
    - Path: esp32-s3-m5/0042-xiao-esp32c6-d0-gpio-toggle/sdkconfig.defaults
      Note: USB Serial/JTAG console defaults
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-19T21:37:21.251554309-05:00
WhatFor: ""
WhenToUse: ""
---


# tmux workflow: build/flash/monitor (ESP32-C6)

## Purpose

Provide a repeatable tmux workflow for building, flashing, and monitoring the ESP32-C6 GPIO-toggle firmware.

## Environment Assumptions

- ESP-IDF v5.4.1 installed at `~/esp/esp-idf-5.4.1/`
- `tmux` installed
- Device attached and visible as a serial port (often `/dev/ttyACM0` for ESP32-C6 USB Serial/JTAG)

## Commands

Set variables once (adjust `PORT` if needed):

```bash
export IDF_SH=~/esp/esp-idf-5.4.1/export.sh
export PROJ=/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0042-xiao-esp32c6-d0-gpio-toggle
export PORT=/dev/ttyACM0
```

Interactive tmux session (recommended):

```bash
tmux new-session -s mo030-esp32c6 -c "$PROJ"
```

In the first pane (build/config):

```bash
source "$IDF_SH"
idf.py set-target esp32c6
idf.py menuconfig
idf.py build
```

Split the window and run flash/monitor in the second pane:

```bash
source "$IDF_SH"
idf.py -p "$PORT" flash monitor
```

Non-interactive “build inside tmux, capture logs” (useful for remote runs):

```bash
tmux has-session -t mo030-esp32c6 2>/dev/null && tmux kill-session -t mo030-esp32c6 || true
tmux new-session -d -s mo030-esp32c6 -c "$PROJ" \
  "bash -lc 'source \"$IDF_SH\" && idf.py set-target esp32c6 && idf.py build; sleep 10'"
tmux capture-pane -pt mo030-esp32c6:0 -S -4000 > /tmp/mo030-esp32c6-build.log
tmux kill-session -t mo030-esp32c6
tail -n 40 /tmp/mo030-esp32c6-build.log
```

## Exit Criteria

- Build output includes: `Project build complete.`
- After flashing, `idf.py monitor` shows periodic logs like `gpio=<n> level=<0|1>`

## Notes

- XIAO ESP32C6 defaults:
  - D0 = GPIO0 (this project’s default)
  - On-board user LED is GPIO15 and is active-low (set `GPIO number to toggle = 15` and enable active-low)
