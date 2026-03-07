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
idf.py set-target esp32s3
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

The current firmware boots into a round LVGL timer screen:

- rotary encoder changes the timer duration
- short press starts, pauses, and resumes the timer
- long press resets the timer to the selected duration
- the timer ring and status text change color by state
- the board layer still exposes touch, but touch is not part of v1 interaction yet

## Controls

- Turn clockwise: increase duration
- Turn counter-clockwise: decrease duration
- Short press: start or pause
- Long press: reset

## Notes

- The first `idf.py build` for a fresh checkout should be preceded by `idf.py set-target esp32s3` because `.envrc` now only exports the ESP-IDF environment.
- The project uses the repaired vendored `LovyanGFX` component from `M5Dial-UserDemo/components/LovyanGFX`.
