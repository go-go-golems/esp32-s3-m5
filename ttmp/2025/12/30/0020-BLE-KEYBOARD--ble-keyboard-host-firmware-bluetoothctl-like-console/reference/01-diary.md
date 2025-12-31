---
Title: Diary
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Implementation diary tracking research and design for BLE HID keyboard host firmware + bluetoothctl-like esp_console.
LastUpdated: 2025-12-30T20:04:39.868197534-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Track research, decisions, and next actions while designing an ESP32-S3 BLE HID keyboard “host” firmware plus a USB Serial/JTAG `esp_console` REPL for scan/pair/bond management.

## Context

This workspace already contains prior BLE experiments in ticket `0019-BLE-TEST` (BLE GATT server / peripheral) and multiple console experiments (manual REPL and `esp_console`).
This ticket extends that work to the **Central** role (BLE host) for connecting to a **BLE HID keyboard** (HOGP).

## Quick Reference (living)

### Key ESP-IDF headers / symbols (BLE HID host)

- `components/esp_hid/include/esp_hidh.h`
  - `esp_hidh_init()`, `esp_hidh_deinit()`
  - `ESP_HIDH_EVENTS` + `esp_hidh_event_t` (`ESP_HIDH_OPEN_EVENT`, `ESP_HIDH_INPUT_EVENT`, `ESP_HIDH_CLOSE_EVENT`, …)
  - `esp_hidh_dev_open()`, `esp_hidh_dev_close()`, `esp_hidh_dev_free()`
  - `esp_hidh_dev_dump()`, `esp_hidh_dev_name_get()`, `esp_hidh_dev_bda_get()`, …
- `components/esp_hid/include/esp_hidh_gattc.h`
  - `esp_hidh_gattc_event_handler(...)` (must be attached or called from your GATTC callback)

### Key in-repo reference implementations

- BLE init + callback style (peripheral/server): `esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c`
  - `ble_init()` shows `nvs_flash_init()`, `esp_bt_controller_*`, `esp_bluedroid_*`, and callback registration.
- `esp_console` REPL + command registration: `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp`
  - `esp_console_new_repl_usb_serial_jtag()`, `esp_console_cmd_register()`, `esp_console_start_repl()`.

## Usage Examples

Use this diary as the “why did we do it this way?” log when implementing the firmware and CLI from the analysis docs.

## Related

- Ticket `0019-BLE-TEST` diary + analysis (existing BLE experiments): `esp32-s3-m5/ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/`

---

## Step 1: Ticket creation + initial scoping

### What I did

- Created ticket `0020-BLE-KEYBOARD`.
- Reviewed prior BLE experiment patterns in ticket `0019-BLE-TEST`:
  - Confirmed common BLE init pattern (`nvs_flash_init` → controller init/enable → `esp_bluedroid_init/enable` → register callbacks).
- Reviewed an in-repo `esp_console` implementation that is close to what we want:
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp` uses `esp_console_new_repl_usb_serial_jtag()` and `esp_console_cmd_register()`.
- Located the ESP-IDF HID Host public headers to anchor the design:
  - `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh.h`
  - `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh_gattc.h`

### Why

The goal is to avoid “hand-rolling” HOGP parsing if ESP-IDF already offers a supported HID Host component (`esp_hidh_*`). If `esp_hidh` can manage discovery, subscription,
and dispatch input reports, we can focus on the UX and the console.

### Notes / decisions

- Target feature set:
  - BLE scan with filtering (show address, RSSI, name, appearance/UUID hints).
  - Connect + pair/bond + list/remove bonds.
  - “Trust” semantics implemented as “bonded + auto-reconnect policy”.

---

## Step 2: API surface mapping for scanning + bonding + HID host glue

### What I did

- Mapped the specific ESP-IDF Bluedroid APIs needed for a `bluetoothctl`-like UX:
  - Scanning: `esp_ble_gap_set_scan_params()`, `esp_ble_gap_start_scanning()`, and ADV parsing via `esp_ble_resolve_adv_data_by_type()`.
  - Pairing/security: `esp_ble_gap_set_security_param()`, `esp_ble_set_encryption()`, and request responses like `esp_ble_gap_security_rsp()`.
  - Bond list management: `esp_ble_get_bond_device_num()`, `esp_ble_get_bond_device_list()`, `esp_ble_remove_bond_device()`.
- Confirmed the critical HID host “glue” requirement:
  - The app must forward GATTC events into HID host via `esp_hidh_gattc_event_handler(...)`.

### Why

The console command set can only be designed once we know which operations can be triggered directly (e.g., bond list queries) versus those that are driven by asynchronous GAP events (e.g., security requests).

### Key files consulted

- `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h`
- `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/api/include/api/esp_gattc_api.h`
- `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh_gattc.h`

---

## Step 3: Console (esp_console) design anchored on USB Serial/JTAG

### What I did

- Identified a close-to-target `esp_console` pattern in this repo:
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/console_repl.cpp` (USB Serial/JTAG REPL + multi-command registration).
- Confirmed which `sdkconfig.defaults` settings are consistently used for USB Serial/JTAG control planes:
  - `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, and disable other console transports.
- Drafted a minimal `bluetoothctl`-like command set and the required “interactive security” escape hatches (`passkey`, `confirm`, `sec-accept`).

### Why

Pairing and scanning are callback-driven, so the console must provide a stable and non-blocking way to:

- trigger actions (`scan`, `connect`, `pair`)
- respond to asynchronous prompts (`passkey`, `confirm`)
- query state (`devices`, `bonds`, `info`)

---

## Step 4: Authored starting analysis documents

### What I did

- Wrote the firmware architecture/build analysis:
  - `analysis/01-ble-hid-keyboard-host-firmware-architecture-files-and-build-plan.md`
- Wrote the console design analysis:
  - `analysis/02-usb-esp-console-bluetoothctl-like-ble-scan-pair-trust-cli.md`

### Why

These documents are intended to be “human-readable starting maps” that point directly to the ESP-IDF symbols and in-repo examples we will reuse when implementing the actual firmware.

---

## Step 5: Implemented the firmware skeleton + console + key logging

### What I did

- Created a new ESP-IDF project:
  - `esp32-s3-m5/0020-cardputer-ble-keyboard-host/`
- Implemented the core modules:
  - `main/ble_host.c` — Bluedroid init, scan registry, scan control, bond list helpers
  - `main/hid_host.c` — `esp_hidh` init + HID input handling + GATTC forwarding glue
  - `main/bt_console.c` — USB Serial/JTAG `esp_console` REPL with bluetoothctl-like commands
  - `main/keylog.c` — queue-based key logging + minimal HID usage → ASCII (US layout first pass)
- Built successfully with ESP-IDF v5.4.1 after updating config defaults (flash size + BLE feature flags):
  - `idf.py set-target esp32s3 && idf.py build`

### Commands implemented

- Discovery: `scan on [seconds]`, `scan off`, `devices`
- Connection: `connect <index|addr>`, `disconnect`
- Pairing/bonds: `pair <addr>`, `bonds`, `unpair <addr>`
- Key output: `keylog on|off`
- Security replies (when needed): `sec-accept`, `passkey`, `confirm`

### Commits (regular intervals)

- `025343acfa4da274528bcec5bafb70480f324c2a` — initial ticket docs + new firmware project
- `18ef2e364e2498bf8288cb23b394ec3ef31cd8f6` — config + HID host glue fixes; project builds
