# Tutorial 0073 - M5Dial Film Developer Timer

This project is the scaffold for a new M5Dial film developer timer app for `esp32-s3-m5`.

It currently starts as a copy of `0072-m5dial-timer-demo` so the working M5Dial board, LVGL, and input-event plumbing can be reused while the film-specific data model and UI are implemented.

The detailed design and implementation plan lives in ticket `ESP-27-M5DIAL-FILM-DEVELOPER`.

## Build

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
source .envrc
cd 0073-m5dial-film-developer-timer
idf.py set-target esp32s3
idf.py build
```

## Flash + Monitor

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
source .envrc
cd 0073-m5dial-film-developer-timer
idf.py -p /dev/ttyACM0 -b 115200 flash monitor
```

## Current scaffold behavior

Right now the scaffold still boots the inherited round LVGL timer demo from `0072` until the film-specific UI is implemented.

Planned behavior:

- select a film
- select developer when there is more than one option
- select dilution when there is more than one option
- select temperature
- select push or pull
- run the resulting development timer

## Current inherited controls

- Turn clockwise: increase duration
- Turn counter-clockwise: decrease duration
- Short press: start or pause
- Long press: reset
- Swipe left or up: next color style
- Swipe right or down: previous color style

## Notes

- The first `idf.py build` for a fresh checkout should be preceded by `idf.py set-target esp32s3` because `.envrc` now only exports the ESP-IDF environment.
- The project uses the repaired vendored `LovyanGFX` component from `M5Dial-UserDemo/components/LovyanGFX`.
- The current implementation plan and research artifacts are in:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide`
