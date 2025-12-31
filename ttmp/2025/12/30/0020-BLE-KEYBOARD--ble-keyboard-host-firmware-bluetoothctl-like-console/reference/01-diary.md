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
RelatedFiles:
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: |-
        Crash-loop forwarding path captured in Step 6
        Implements decoded logging + scan-stop→open sequencing (commit 6a96946)
    - Path: 0020-cardputer-ble-keyboard-host/main/hid_host.c
      Note: Crash-loop and fix captured in Step 6
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/sources/ble-gatt-bug-report-research.md
      Note: Source material for Step 7 mapping (0x85/0x0100)
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

---

## Step 6: First on-device run, crash-loop, and current connect failure

### Goal

Get to the “happy path” for the target keyboard address `DC:2C:26:FD:03:B7`:

1) scan → 2) connect → 3) pair/bond → 4) enable `keylog` → 5) see typed keys over USB serial.

This step covers the first “real hardware” run and the two major outcomes: (1) an initial crash-loop that prevented any interactive work, and (2) after fixing the crash-loop, a second blocker where we still can’t connect/open the target keyboard. The key theme is ordering and ownership: `esp_hidh` has internal assumptions about who registers GATTC and when its callback queue exists.

After stabilizing boot, we reached a consistent REPL prompt (`ble>`) and could scan, enumerate devices, and attempt to connect. However, the target keyboard address still isn’t observed in the scan list and direct open attempts fail with a disconnect reason and generic GATT error, so we don’t yet reach the pairing or key-report stage.

**Commit (code):** `18ef2e364e2498bf8288cb23b394ec3ef31cd8f6` — "Fix 0020 config and HID host glue"

### What I did

- Flashed and monitored the device from tmux:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `idf.py -p /dev/ttyACM0 flash monitor`
- Captured the crash-loop logs and backtrace.
- Changed init and callback forwarding so `esp_hidh` owns GATTC registration and only receives forwarded events when its internal state is valid.
- Re-flashed and verified boot stability (`ble>` prompt).
- Ran scan/connect attempts for `DC:2C:26:FD:03:B7` from the console.

### Why

This firmware is intended to be “bluetoothctl for an ESP32”: it must never crash-loop in the common “boot + scan + connect” flow, and it must expose enough surface area to debug pairing/security in the field. The crash-loop prevented all of that.

### What worked

- After the init-order + ownership fix, the device boots reliably and the console becomes usable.
- Scanning and device table output works as a baseline “RF is alive” check.

### What didn't work

- Initial boot crashed repeatedly with:

  - `E (...) BLE_HIDH: esp_ble_hidh_init(629): esp_ble_gattc_app_register failed!`
  - `E (...) ESP_HIDH: esp_hidh_init(111): BLE HIDH init failed`
  - `E (...) hid_host: esp_hidh_init failed: ESP_ERR_INVALID_STATE`

  followed by:

  - `assert failed: xQueueGenericSend queue.c:936 (pxQueue)`

  and a backtrace implicating the HID host callback queue path:

  - `SEND_CB` in `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c`
  - `esp_hidh_gattc_event_handler` in `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c`
  - `hid_host_forward_gattc_event` in `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/hid_host.c`
  - our GATTC callback in `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`

- After boot stability was fixed:
  - `scan on 15` + `devices` does not show `DC:2C:26:FD:03:B7`.
  - Direct connect/open attempts fail:
    - `connect DC:2C:26:FD:03:B7 pub`
    - `connect DC:2C:26:FD:03:B7 rand`
  - Failure pattern:
    - `gattc disconnect: reason=0x100`
    - `BLE_HIDH: OPEN failed: 0x85` (`ESP_GATT_ERROR`)
    - `hid_host: esp_hidh_dev_open failed`

### What I learned

- The BLE HID host path (`esp_hidh`) is sensitive to initialization order and assumes it controls certain pieces of the GATTC lifecycle.
- Forwarding GATTC events is required, but forwarding them “too early” can exercise internal paths before their queue/dispatch machinery exists.

### What was tricky to build

- The “obvious” fix (disable GATTC forwarding until HID init completes) conflicts with the fact that `esp_hidh_init()` itself depends on GATTC registration/callback paths; there’s a narrow window where forwarding must be enabled for init to succeed, but unsafe if init fails.
- Error surfaces are split across multiple components (Bluedroid GAP/GATTC + esp_hid), so the most actionable artifact is the backtrace rather than the top-level `ESP_ERR_INVALID_STATE`.

### What warrants a second pair of eyes

- Review the ownership contract for GATTC registration:
  - confirm we never call `esp_ble_gattc_app_register()` in the app path for this HID host use-case,
  - confirm forwarding into `esp_hidh_gattc_event_handler()` is only done when safe, and we never call it after a failed `esp_hidh_init()`.

### What should be done in the future

- Add a “boot invariant” check path (even if only logs) that prints whether the HID host is initialized and whether GATTC forwarding is enabled; this makes future regressions obvious from the first 20 lines of boot.
- Keep the crash-loop backtrace and logs in a dedicated bug report so we don’t lose the exact signature if it reappears.

### Code review instructions

- Start at `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/hid_host.c` and inspect:
  - the `esp_hidh_init()` call site and gating,
  - `hid_host_forward_gattc_event(...)`.
- Then check `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`:
  - the GATTC callback (`gattc_cb`) and how/when it forwards events.

### Technical details

- Environment:
  - ESP-IDF: `/home/manuel/esp/esp-idf-5.4.1/`
  - Monitor port: `/dev/ttyACM0` (USB Serial/JTAG)
- Target device (desired): `DC:2C:26:FD:03:B7`

---

## Step 7: Decode connect failures and tighten scan→connect sequencing

This step is about turning opaque “it failed” signals into actionable diagnostics, and removing a known Bluedroid failure mode. The intern research clarified that `0x85` is a generic GATT error (`ESP_GATT_ERROR`, widely known as “133”) and that `0x0100` is `ESP_GATT_CONN_CONN_CANCEL` (“L2CAP connection cancelled”), which is consistent with attempting to connect to a device that is not currently connectable/advertising at that address.

The practical upshot is that two issues can stack: (1) we may be trying to connect to a stale “identity” address while the keyboard is actually advertising with a privacy/RPA address, and (2) Bluedroid is known to return `0x85` if you attempt to open/connect while scanning is still active. I implemented better logging and a safer connect sequence (“stop scan → wait for scan stop complete → open”) to help you drive the experiments interactively.

**Commit (code):** `6a96946c2133900eb8511b61fc15ffeb0186daf7` — "BLE: decode GATTC status/reason and connect after scan stop"

### What I did

- Read the intern research report and extracted the two key mappings:
  - `0x85` → `ESP_GATT_ERROR`
  - `0x0100` → `ESP_GATT_CONN_CONN_CANCEL` (connection cancelled at the GATT/L2CAP layer)
- Updated BLE host logging to print decoded strings for:
  - `ESP_GATTC_OPEN_EVT` (`status=... (ESP_GATT_...)`)
  - `ESP_GATTC_DISCONNECT_EVT` (`reason=... (ESP_GATT_CONN_...)`)
  - `ESP_GATTC_CLOSE_EVT` (status + reason)
- Changed connect sequencing so a `connect ...` request made during scanning is deferred until `ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT` fires (instead of “stop scan and immediately open”).

### Why

We can’t efficiently debug the connect failure when the output is only hex codes. Also, if scanning is still running, Bluedroid has a known behavior where open/connect can fail with `0x85`, which produces confusing noise if we’re simultaneously fighting privacy/RPA or “not in pairing mode” issues.

### What worked

- The firmware now prints a line that decodes the GATT open status and disconnect reason, which should directly confirm whether we are hitting:
  - “scan/connect overlap” (often `ESP_GATT_ERROR` at open), and/or
  - “connect cancelled” (`ESP_GATT_CONN_CONN_CANCEL`) consistent with “not connectable at this address”.

### What didn't work

- We still do not have a confirmed successful connect to the keyboard, so the root cause remains open (privacy/RPA vs device not advertising vs other stack constraints).

### What I learned

- In ESP-IDF’s Bluedroid layer, `ESP_GATTC_DISCONNECT_EVT.reason` is an `esp_gatt_conn_reason_t`, which includes “special” 16-bit values like `0x0100` and cannot be treated as a plain 8-bit HCI reason.

### What was tricky to build

- A naive “stop scanning” before connect isn’t sufficient; to actually remove the race you need to wait for `ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT` before starting the GATT open.

### What warrants a second pair of eyes

- Confirm the new “pending connect” state machine can’t be confused by overlapping scan/scan-stop/connect commands (especially if the user spams `scan on` while a connect is pending).

### What should be done in the future

- Improve scanning UX so we can identify the keyboard more reliably:
  - increase `MAX_DEVICES` or add a “pin”/“watch” list so it doesn’t get evicted,
  - parse scan response (`scan_rsp_len`) in addition to advertising data so names show up more often,
  - consider printing HID-related hints (UUID 0x1812 / appearance) when present.

### Code review instructions

- Start at `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`:
  - `gatt_status_to_str(...)`
  - `gatt_conn_reason_to_str(...)`
  - connect deferral in `ble_host_connect(...)` and `ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT`.

### Technical details

- Intern report (source artifact):
  - `ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/sources/ble-gatt-bug-report-research.md`
