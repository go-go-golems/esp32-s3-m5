# Tutorial 0072 - M5Dial Timer Demo

This tutorial is a new, smaller M5Dial example for `esp32-s3-m5`.

The project is being built in stages:

- stage 1: board bring-up and hardware smoke test
- stage 2: timer model and LVGL timer UI
- stage 3: polish, validation, and ticket documentation

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
source .envrc
cd 0072-m5dial-timer-demo
idf.py build
```

## Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
source .envrc
cd 0072-m5dial-timer-demo
idf.py -p /dev/ttyACM0 -b 115200 flash monitor
```

## Current behavior

The first milestone is a hardware smoke test:

- power hold is asserted
- display is initialized with direct `LovyanGFX`
- backlight is enabled
- encoder delta is polled in software
- center button presses are debounced
- touch coordinates are read through `LovyanGFX`

Later milestones will replace the smoke-test screen with the actual timer UI.
