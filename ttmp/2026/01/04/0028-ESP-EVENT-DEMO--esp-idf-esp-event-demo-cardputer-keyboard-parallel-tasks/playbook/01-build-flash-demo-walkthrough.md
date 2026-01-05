---
Title: Build/flash + demo walkthrough
Ticket: 0028-ESP-EVENT-DEMO
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - keyboard
    - display
    - ui
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0028-cardputer-esp-event-demo/README.md
      Note: Build/flash instructions target this firmware
    - Path: 0028-cardputer-esp-event-demo/main/app_main.cpp
      Note: Firmware behavior the playbook validates
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-04T23:56:44.311711888-05:00
WhatFor: Repeatable steps to build/flash the 0028 Cardputer esp_event demo and sanity-check behavior on-device.
WhenToUse: Use when validating the demo firmware locally, or when reviewing the expected on-screen behavior of event producers/consumers.
---


# Build/flash + demo walkthrough

## Purpose

Build and flash the Cardputer `esp_event` demo firmware (`0028-cardputer-esp-event-demo`) and verify:
- keyboard presses generate `esp_event` events and render on screen
- parallel producer tasks generate random/heartbeat events and render on screen
- UI controls (scroll/pause/clear) work

## Environment Assumptions

- ESP-IDF environment is exported (this repo uses `source ~/esp/esp-idf-5.4.1/export.sh` via `.envrc`).
- Target is `esp32s3`.
- You have a Cardputer connected over USB and can run `idf.py flash monitor`.
- `EXTRA_COMPONENT_DIRS` paths resolve (M5GFX + `components/cardputer_kb`).

## Commands

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0028-cardputer-esp-event-demo

# One-time (or if switching targets)
idf.py set-target esp32s3

# Configure (optional)
idf.py menuconfig

# Build + flash + monitor
idf.py build flash monitor
```

## Exit Criteria

- Device boots and the display shows: `0028: esp_event demo (Cardputer)`.
- Pressing keys appends `KB keynum=...` lines to the on-screen log.
- Every ~1s a `HEARTBEAT heap=... dma=...` line appears.
- Random producer lines appear (`RAND#N value=...`).
- `Del` clears the log.
- `Fn+Space` pauses/resumes appending new lines (overlay shows “PAUSED” when paused).
- `Fn+W/S` scrolls the log (via `NavUp/NavDown`).

## Notes

- If the log is noisy, reduce `Tutorial 0028: Random event producers` or increase the random periods in `menuconfig`.
- If you see increasing `drops=...`, increase `Tutorial 0028: Event loop queue size` or reduce producer rates.
