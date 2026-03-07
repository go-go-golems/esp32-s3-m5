# Tutorial 0073 - M5Dial Film Developer Timer

This project is a working M5Dial film developer timer app for `esp32-s3-m5`.

It started as a copy of `0072-m5dial-timer-demo`, but it now boots into a film-selection flow backed by a curated runtime catalog generated from `film_dev_times.json`.

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

## Current behavior

The app now:

- selects a film from the curated starter catalog
- auto-skips single-choice developer, dilution, temperature, or push/pull stages
- enters a ready screen for the resolved recipe
- runs a countdown for the resolved development time
- supports pause, resume, and rerun for the current recipe

The runtime catalog currently contains:

- `1548` normalized recipe rows
- `19` starter films
- `9` developers

The app currently uses the best available source time in this order:

1. `time_35mm`
2. `time_120`
3. `time_sheet`

It does not yet expose film format as a separate selector.

## Controls

While selecting:

- Turn clockwise or counter-clockwise: change the current field value
- Short press: confirm / advance
- Long press: go back one selector stage
- Swipe left or up: next color theme
- Swipe right or down: previous color theme

While on the recipe ready or timer screen:

- Short press: start, pause, resume, or rerun the current recipe timer
- Long press while running or paused: reset the current recipe timer to ready
- Long press while ready or done: return to the selector
- Swipe left or up: next color theme
- Swipe right or down: previous color theme

## Notes

- The first `idf.py build` for a fresh checkout should be preceded by `idf.py set-target esp32s3` because `.envrc` now only exports the ESP-IDF environment.
- The project uses the repaired vendored `LovyanGFX` component from `M5Dial-UserDemo/components/LovyanGFX`.
- The current v1 still shows raw oddities from the source dataset in a few push/pull labels, for example fractional `pull-*` variants.
- The current implementation plan and research artifacts are in:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/06/ESP-27-M5DIAL-FILM-DEVELOPER--m5dial-film-developer-timer-app-design-and-implementation-guide`
