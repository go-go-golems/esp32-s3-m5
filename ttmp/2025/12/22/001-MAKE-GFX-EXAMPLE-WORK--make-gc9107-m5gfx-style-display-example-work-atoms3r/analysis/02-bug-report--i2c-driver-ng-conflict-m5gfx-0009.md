---
Title: 'Bug report: ESP-IDF I2C driver_ng conflict aborts at boot (tutorial 0009 + M5GFX)'
Ticket: 001-MAKE-GFX-EXAMPLE-WORK
Status: active
Topics:
    - esp32s3
    - esp-idf
    - display
    - gc9107
    - m5gfx
    - atoms3r
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/driver/i2c/i2c.c
      Note: Source of constructor-based conflict check (check_i2c_driver_conflict)
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/common.cpp
      Note: M5GFX/LovyanGFX uses legacy I2C driver symbols (driver/i2c.h
    - Path: 0009-atoms3r-m5gfx-display-animation/main/hello_world_main.cpp
      Note: Backlight I2C helper switched to legacy driver APIs to avoid driver_ng conflict
ExternalSources:
    - https://docs.espressif.com/projects/esp-idf/en/stable/esp32/migration-guides/release-5.x/5.2/peripherals.html
Summary: ""
LastUpdated: 2025-12-23T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


## Summary

Tutorial `esp32-s3-m5/0009-atoms3r-m5gfx-display-animation` can **build**, but the flashed app **crashes immediately at boot** on ESP-IDF `v5.4.1` with:

> `CONFLICT! driver_ng is not allowed to be used with this old driver`

This abort is raised **during global constructors** (before `app_main()`), making it look like “random early boot failure”, but it is a deterministic runtime conflict between:

- **Legacy I2C driver** (`driver/i2c.h`, e.g. `i2c_driver_install`, `i2c_set_pin`, low-level register pokes), and
- **New I2C driver (driver_ng)** (`driver/i2c_master.h`, e.g. `i2c_new_master_bus`).

In our repo, the vendored **M5GFX/LovyanGFX component still compiles against the legacy driver**, while tutorial `0009`’s backlight helper originally used the new `i2c_master` APIs → both linked → ESP-IDF aborts at startup.

## Environment

- **Board**: AtomS3R (ESP32-S3)
- **ESP-IDF**: `v5.4.1`
- **Project**: `atoms3r_m5gfx_display_animation`
- **Tutorial**: `esp32-s3-m5/0009-atoms3r-m5gfx-display-animation`
- **Display stack**: `M5GFX` / LovyanGFX

## Impact

- App cannot reach `app_main()`, so we cannot use tutorial `0009` as a baseline until this conflict is resolved.
- The failure happens very early and can be confusing if you don’t know about the constructor-based conflict check.

## Reproduction

1) Build and flash tutorial `0009` using ESP-IDF 5.4.1.

2) Observe boot output (Manuel provided):

```text
E (257) i2c: CONFLICT! driver_ng is not allowed to be used with this old driver

abort() was called at PC 0x4201d533 on core 0

--- 0x4201d533: check_i2c_driver_conflict at .../components/driver/i2c/i2c.c:1723
--- 0x42002b6b: do_global_ctors at .../components/esp_system/startup.c:104
```

## Root cause analysis

### 1) ESP-IDF enforces “no legacy + new driver mix” at startup

In ESP-IDF 5.4.1, the legacy driver defines a constructor:

- `components/driver/i2c/i2c.c: check_i2c_driver_conflict()`
- It checks for the presence of `i2c_acquire_bus_handle` (weak symbol implemented by the new driver)
- If found, it logs the conflict and calls `abort()`

This check is compiled unless `CONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK` is enabled.

### 2) Our binary links both drivers

Why both end up linked:

- `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/common.cpp` includes `driver/i2c.h` and uses legacy symbols like `i2c_set_pin(...)`.
- Tutorial `0009`’s backlight helper originally used the new driver (`driver/i2c_master.h` via `i2c_new_master_bus()` / `i2c_master_bus_add_device()`).

So the final link pulls in:

- legacy driver translation unit (which brings the constructor check), and
- new driver translation unit (which defines `i2c_acquire_bus_handle`)

→ constructor sees new driver → abort.

## Fix strategy options

### Option A (recommended for now): Standardize tutorial `0009` on the legacy I2C driver

Change `0009` backlight I2C helper to use legacy APIs:

- `i2c_param_config()`
- `i2c_driver_install()`
- `i2c_master_write_to_device()`

This avoids linking the new driver entirely, matching what M5GFX expects today.

Pros:
- Minimal change
- Works with current vendored M5GFX without invasive patching

Cons:
- Does not advance us toward full driver_ng usage

### Option B: Migrate vendored M5GFX to new `driver/i2c_master.h`

This is a larger port:
- Remove reliance on `driver/i2c.h` APIs and direct register access
- Use `i2c_master` bus/device handles

Pros:
- Aligns with ESP-IDF direction; allows using driver_ng everywhere

Cons:
- Non-trivial refactor; higher risk of regressions across M5GFX features

### Option C: Set `CONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK=y`

This disables the abort but does **not** guarantee correctness: both drivers may still fight for the peripheral or assume incompatible state.

Pros:
- Fast “unblock”

Cons:
- Risky / undefined behavior; not recommended as a durable fix

## Current repo status (as of this write-up)

- A patch has been applied to tutorial `0009` to use legacy I2C in the backlight helper to avoid driver_ng linkage.
- `idf.py build` succeeds locally after the patch (see Verification below).
- Next step is to flash and verify the resulting firmware boots past constructors on real hardware.

## Verification

### Build (local)

Command:

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh && \
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0009-atoms3r-m5gfx-display-animation && \
idf.py build
```

Result: **success** (ESP-IDF 5.4.1).

### Boot (device)

Status: **not yet re-verified** after the patch (requires flashing and confirming we no longer abort in `do_global_ctors`).

## References

- ESP-IDF migration notes (I2C driver refactor): `https://docs.espressif.com/projects/esp-idf/en/stable/esp32/migration-guides/release-5.x/5.2/peripherals.html`


