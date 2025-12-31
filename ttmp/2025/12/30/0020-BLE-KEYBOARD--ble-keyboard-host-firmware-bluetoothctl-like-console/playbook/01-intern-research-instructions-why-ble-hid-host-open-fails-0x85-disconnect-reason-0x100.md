---
Title: 'Intern research instructions: why BLE HID host open fails (0x85) + disconnect reason 0x100'
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Research plan for finding the meaning/root cause of GATTC disconnect reason 0x100 and BLE_HIDH open status 0x85 in ESP-IDF 5.4.1 Bluedroid HID host.
LastUpdated: 2025-12-30T20:46:16.395818444-05:00
WhatFor: ""
WhenToUse: ""
---

# Intern research instructions: why BLE HID host open fails (0x85) + disconnect reason 0x100

## Purpose

Find authoritative explanations (preferably from ESP-IDF docs/source, GitHub issues, or Bluetooth spec references) for the current failure mode in ticket `0020-BLE-KEYBOARD`:

- connecting to keyboard address `DC:2C:26:FD:03:B7` fails
- GATTC disconnect reason shows `0x100`
- BLE HID host open fails with status `0x85` (generic `ESP_GATT_ERROR`)

Deliverables:

1) A short “what it means” mapping for `0x100` and `0x85` in this context (with sources/links).
2) The most likely root-cause hypotheses ranked by likelihood.
3) Concrete mitigation options (configuration changes, API usage changes, or code changes) with pros/cons.

## Environment Assumptions

- Firmware project: `esp32-s3-m5/0020-cardputer-ble-keyboard-host/`
- ESP-IDF version: `v5.4.1` (Bluedroid)
- Keyboard is expected to be a BLE HID device (HOGP).
- Console commands exist: `scan`, `devices`, `connect`, `pair`, `keylog`.

## Commands

You do not need to run commands on hardware; these are for understanding reproduction and log context.

Firmware run steps (context):

```bash
source /home/manuel/esp/esp-idf-5.4.1/export.sh
cd esp32-s3-m5/0020-cardputer-ble-keyboard-host
idf.py -p /dev/ttyACM0 flash monitor
```

Console session (context):

```text
ble> scan on 15
ble> devices
ble> connect DC:2C:26:FD:03:B7 pub
ble> connect DC:2C:26:FD:03:B7 rand
```

Observed failure (key lines):

```text
W (...) BT_APPL: gattc_conn_cb: if=1 st=0 id=1 rsn=0x100
I (...) ble_host: gattc disconnect: reason=0x100
E (...) BLE_HIDH: OPEN failed: 0x85
E (...) BLE_HIDH: dev open failed! status: 0x85
```

## Exit Criteria

You can send back a short report that includes:

- A clear mapping:
  - what component emits `rsn=0x100` and what does that numeric code mean (exact symbol name if possible)
  - what conditions cause `BLE_HIDH: OPEN failed: 0x85` in `esp_hid` (function names and line references if you find them)
- A “most likely” explanation that matches observed behavior: scanning does not show target address, and direct connect fails.
- At least 2 mitigation paths we can try next (address/RPA handling, scan mode changes, pairing mode requirements, etc.).

---

## Research plan (internet)

### 1) Decode `reason=0x100`

Goal: find whether `0x100` is:

- an HCI status code,
- a Bluedroid internal “reason” enum,
- or a vendor-specific wrapper used by ESP-IDF.

Search queries:

- `ESP-IDF bluedroid gattc_conn_cb rsn 0x100`
- `btc_gattc disconnect reason 0x100`
- `esp_ble_gattc_cb_param_t disconnect reason 0x100`
- `BT_APPL gattc_conn_cb rsn 0x100 esp32`

What to look for:

- source-level enums or defines for disconnect reason in Bluedroid (e.g., `btm_ble_api.h`, `bta_gatt_api.h`, or `stack/gatt`).
- issue threads where someone logs `rsn=0x100` on ESP32 BLE.

### 2) Decode `OPEN failed: 0x85` in BLE HID host

Goal: find where `0x85` originates in the HID host open path.

Facts:

- `0x85` maps to `ESP_GATT_ERROR` (“generic error”) in `esp_gatt_defs.h`.

Search queries:

- `BLE_HIDH OPEN failed 0x85`
- `esp_hidh_dev_open 0x85`
- `ble_hidh.c OPEN failed`
- `ESP_GATT_ERROR 0x85 BLE_HIDH`

What to look for:

- a specific upstream GATT or HCI status that gets mapped to `ESP_GATT_ERROR`.
- conditions such as “remote not connectable”, “service discovery failed”, “insufficient authentication”, etc.

### 3) Check if the keyboard address can rotate (RPA)

Goal: confirm whether it’s plausible that the device does not advertise as `DC:2C:26:FD:03:B7` due to privacy.

Search queries:

- `BLE keyboard resolvable private address rotates`
- `HOGP keyboard address randomization RPA`
- `bluetoothctl identity address vs current address`

What to look for:

- explanation of identity address vs current resolvable private address
- how a Central should connect when the peer uses privacy (must connect to the current advertising address)

### 4) Cross-reference with ESP-IDF examples

Goal: determine whether ESP-IDF’s `esp_hid_host` example expects callers to:

- use `esp_hid_scan()` (HID-specific scan logic) and connect by scan result address,
- rather than connecting by a manually known address.

Search queries:

- `esp-idf esp_hid_host example esp_hid_scan esp_hidh_dev_open`
- `esp_hid_gap_init esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler)`

What to deliver:

- If the example or documentation implies “you must discover the current address via scan”, call that out explicitly.

---

## Reporting template (what to send back)

1) **Meaning of codes**
   - `0x100`:
     - meaning:
     - source link(s):
   - `0x85`:
     - meaning:
     - source link(s):

2) **Likely root cause (ranked)**
   - #1 …
   - #2 …
   - #3 …

3) **Mitigations to try next**
   - Option A (minimal code/config change) …
   - Option B (use HID scan helper / change scan mode) …
   - Option C (security/pairing mode adjustments) …

## Notes

- The target address `DC:2C:26:FD:03:B7` was *not* present in scan results during local testing.
- The firmware can scan and print many devices; the failing part is reliably reproducing a successful connect/open to the keyboard.
