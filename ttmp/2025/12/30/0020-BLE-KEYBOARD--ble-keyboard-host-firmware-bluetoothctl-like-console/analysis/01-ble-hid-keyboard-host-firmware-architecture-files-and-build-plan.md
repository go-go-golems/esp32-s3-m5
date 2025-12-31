---
Title: BLE HID Keyboard Host Firmware — architecture, files, and build plan
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: In-depth map of ESP-IDF components, files, and key symbols needed to build an ESP32-S3 BLE Central (host) that connects to a BLE keyboard (HOGP) and consumes HID input reports.
LastUpdated: 2025-12-30T20:04:33.070654847-05:00
WhatFor: ""
WhenToUse: ""
---

# BLE HID Keyboard Host Firmware — architecture, files, and build plan

## Executive summary

This firmware is the inverse of ticket `0019-BLE-TEST`: instead of the ESP32 acting as a BLE **Peripheral** (GATT server), the ESP32-S3 will act as a BLE **Central** (“host”) that:

1. Scans for BLE keyboards (HID over GATT / HOGP).
2. Connects to a selected device.
3. Pairs/bonds (optionally with Secure Connections).
4. Subscribes to HID input reports.
5. Translates incoming HID key reports into “key events” (and optionally text), then prints/logs them and/or forwards them to another subsystem.

The fastest path to correctness is to **reuse ESP-IDF’s HID Host component** (`esp_hidh_*`) rather than implementing the HOGP profile entirely “by hand” with raw GATTC calls.

## Primary reference points in this mono-repo

### Prior BLE experiment: “peripheral/server” pattern

- `esp32-s3-m5/0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c`
  - `ble_init()` shows Bluedroid BLE bring-up (NVS → controller → bluedroid → callback registration).

### ESP-IDF HID Host surface area (Bluedroid + GATTC)

The HID host API lives in ESP-IDF’s `esp_hid` component:

- `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh.h`
  - `esp_hidh_init()`, `esp_hidh_deinit()`
  - `esp_hidh_dev_open()`, `esp_hidh_dev_close()`, `esp_hidh_dev_free()`
  - `ESP_HIDH_EVENTS` + `esp_hidh_event_t` (`ESP_HIDH_OPEN_EVENT`, `ESP_HIDH_INPUT_EVENT`, `ESP_HIDH_CLOSE_EVENT`, …)
- `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh_gattc.h`
  - `esp_hidh_gattc_event_handler(...)` (required glue: call from your GATTC callback)
- `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hid_common.h`
  - `esp_hid_transport_t` (`ESP_HID_TRANSPORT_BLE`)
  - `esp_hid_usage_t` (`ESP_HID_USAGE_KEYBOARD`, `ESP_HID_USAGE_CCONTROL`, …)
  - `ESP_HID_APPEARANCE_KEYBOARD` (`0x03C1`) for scan filtering heuristics

## Firmware architecture (recommended)

### High-level module decomposition

Keep responsibilities explicit so `esp_console` commands don’t fight callback logic:

- **`ble_host`**: BLE init, scanning, “device registry”, connect/pair/disconnect operations.
- **`hid_host`**: ESP-IDF HID host integration (`esp_hidh_*`), translating `ESP_HIDH_INPUT_EVENT` into keyboard events.
- **`bt_console`**: `esp_console` command handlers (scan/connect/pair/unpair/trust/info/bonds).
- **`app_main`**: wires everything together and sets default security policy.

Diagram (data flow + control):

```
                 USB Serial/JTAG
                      |
                      v
               +--------------+
               |  esp_console |
               |  bt_console  |  (scan/connect/pair/bonds/…)
               +------+-------+
                      |
                      v
               +--------------+        Bluedroid callbacks
               |   ble_host   |<----------------------------------+
               | (GAP/GATTC)  |                                   |
               +------+-------+                                   |
                      |                                           |
                      v                                           |
               +--------------+    ESP_HIDH_EVENTS + input reports |
               |   hid_host   |<----------------------------------+
               | (esp_hidh)   |
               +------+-------+
                      |
                      v
               +--------------+
               | key events   |
               | (log/route)  |
               +--------------+
```

### State machine (minimal)

Central-mode workflows are asynchronous. A small state machine prevents “half actions”:

```
  +-------+    scan on     +----------+   connect    +-----------+
  | IDLE  |--------------->| SCANNING |------------->| CONNECTING|
  +---+---+                +-----+----+              +-----+-----+
      ^                          |                         |
      | stop scan                | scan result             | open evt
      |                          v                         v
      |                     +----------+              +-----------+
      |                     | CANDIDATE|              |  CONNECTED|
      |                     +----------+              +-----+-----+
      |                                                      |
      | disconnect/close                                     | pair/encrypt
      +------------------------------------------------------v
                                                   +----------------+
                                                   | BONDED/TRUSTED |
                                                   +--------+-------+
                                                            |
                                                            | HID input events
                                                            v
                                                     +--------------+
                                                     | KEYSTREAMING |
                                                     +--------------+
```

## ESP-IDF APIs you will touch (file/symbol anchors)

### BLE bring-up (Bluedroid)

Same idea as `0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c:ble_init()`, but for Central you will also register a **GATTC** callback:

- `nvs_flash_init()` (+ erase fallback)
- `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)` (optional, BLE-only)
- `esp_bt_controller_init()`, `esp_bt_controller_enable(ESP_BT_MODE_BLE)`
- `esp_bluedroid_init()`, `esp_bluedroid_enable()`
- `esp_ble_gap_register_callback(gap_cb)`
  - header: `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/api/include/api/esp_gap_ble_api.h`
- `esp_ble_gattc_register_callback(gattc_cb)`
  - header: `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/api/include/api/esp_gattc_api.h`
- `esp_ble_gattc_app_register(APP_ID)`

### Scanning (discover keyboards)

Key functions (esp_gap_ble_api.h):

- `esp_ble_gap_set_scan_params(esp_ble_scan_params_t *scan_params)`
- `esp_ble_gap_start_scanning(uint32_t duration_seconds)`
- `esp_ble_gap_stop_scanning()`
- `esp_ble_resolve_adv_data_by_type(uint8_t *adv_data, uint16_t adv_data_len, esp_ble_adv_data_type type, uint8_t *length)`

Recommended “keyboard candidate” heuristics (use as filters, not proofs):

- Appearance equals `ESP_HID_APPEARANCE_KEYBOARD` (`0x03C1`).
- ADV includes HID service UUID `0x1812` (if present).
- Device name indicates keyboard (vendor/branding heuristic only).

### Connecting (two viable strategies)

1) **Preferred: connect via HID Host component**

- `esp_hidh_dev_open(esp_bd_addr_t bda, esp_hid_transport_t transport, uint8_t remote_addr_type)`
  - transport should be `ESP_HID_TRANSPORT_BLE` (from `esp_hid_common.h`)

2) **Low-level: connect via GATTC and implement HOGP manually**

- `esp_ble_gattc_open(gattc_if, remote_bda, remote_addr_type, true)`
  - header: `/home/manuel/esp/esp-idf-5.4.1/components/bt/host/bluedroid/api/include/api/esp_gattc_api.h`
- discover HID service + report characteristic(s)
- enable notifications by CCCD writes

This ticket focuses on (1) because it is a much smaller surface area to maintain.

### HID host integration (glue required)

To use `esp_hidh` on BLE, the app must:

1) Initialize HID host:

- `esp_hidh_init(const esp_hidh_config_t *config)`
  - you provide an `esp_event_handler_t callback` which receives `ESP_HIDH_EVENTS`

2) Forward GATTC events into HID host:

- In `gattc_cb`, call `esp_hidh_gattc_event_handler(event, gattc_if, param)`
  - header: `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh_gattc.h`

3) Handle HID host events:

- `ESP_HIDH_OPEN_EVENT`: device is opened (connected + ready)
- `ESP_HIDH_INPUT_EVENT`: HID input report received
- `ESP_HIDH_CLOSE_EVENT`: device closed; must call `esp_hidh_dev_free(dev)` here to release resources

## Pairing, bonding, and “trust”

### Security configuration knobs (esp_gap_ble_api.h)

- `esp_ble_gap_set_security_param(esp_ble_sm_param_t param_type, void *value, uint8_t len)`
- `esp_ble_set_encryption(esp_bd_addr_t bd_addr, esp_ble_sec_act_t sec_act)`
- `esp_ble_gap_security_rsp(esp_bd_addr_t bd_addr, bool accept)`
- `esp_ble_passkey_reply(...)`, `esp_ble_confirm_reply(...)`

Bond database management:

- `esp_ble_get_bond_device_num()`
- `esp_ble_get_bond_device_list(int *dev_num, esp_ble_bond_dev_t *dev_list)`
- `esp_ble_remove_bond_device(esp_bd_addr_t bd_addr)`

### What “trust” should mean (recommended)

ESP-IDF exposes a bond database (pairing keys) but does not have a separate “trust store”. To emulate `bluetoothctl trust`, implement trust as an app-level policy bit:

- bonded: the device is in Bluedroid’s bond DB
- trusted: the app will auto-reconnect to the device (NVS flag keyed by address)

## HID keyboard input parsing (first implementation)

Many BLE keyboards use the classic 8-byte “boot keyboard input report” layout:

```
byte 0: modifier bits (Ctrl/Shift/Alt/GUI)
byte 1: reserved
byte 2..7: up to 6 simultaneous keycodes (HID usage IDs)
```

But HOGP devices can also use Report Protocol with report IDs and variable report lengths. Therefore:

- treat `report_id` and `length` as real signals (don’t hardcode “8 bytes always”)
- if an unknown report arrives, log it and dump device info with `esp_hidh_dev_dump(dev, stdout)`

Pseudocode (robust-enough first pass):

```c
on_hidh_event(ESP_HIDH_INPUT_EVENT, e):
  if e.input.usage != ESP_HID_USAGE_KEYBOARD:
    return

  if e.input.length < 2:
    log("short report")
    return

  modifiers = e.input.data[0]
  keycodes = e.input.data[2..e.input.length-1]

  pressed = { kc in keycodes | kc != 0 }
  released = prev_pressed - pressed
  newly_pressed = pressed - prev_pressed

  for kc in newly_pressed:
    emit_key_down(kc, modifiers)
  for kc in released:
    emit_key_up(kc)

  prev_pressed = pressed
```

## Proposed new firmware project layout (inside this mono-repo)

If we add a new numbered project, mirror existing ones:

```
0020-cardputer-ble-keyboard-host/
  CMakeLists.txt
  sdkconfig.defaults
  partitions.csv
  main/
    CMakeLists.txt
    app_main.c
    ble_host.h / ble_host.c
    hid_host.h / hid_host.c
    bt_console.h / bt_console.c
    hid_keymap.h / hid_keymap.c   (optional; HID usage IDs -> ASCII/keys)
```

## Build and flash (human steps)

Assuming the new project directory is `esp32-s3-m5/0020-cardputer-ble-keyboard-host`:

1. `idf.py set-target esp32s3`
2. `idf.py menuconfig`
   - enable USB console transport (see analysis `analysis/02-usb-esp-console-bluetoothctl-like-ble-scan-pair-trust-cli.md`)
   - ensure Bluedroid BLE + GATTC are enabled
3. `idf.py build`
4. `idf.py -p /dev/ttyACM0 flash monitor`

## Debugging checklist (what usually bites)

- Missing GATTC glue: if you forget `esp_hidh_gattc_event_handler(...)`, HID host will not function correctly.
- Address types: carry `remote_addr_type` from scan results into connect/open calls.
- Bond DB confusion: `unpair` should call `esp_ble_remove_bond_device(bda)` and also disconnect if currently connected.
- Console concurrency: avoid heavy printing inside GAP callbacks; enqueue messages to a logger task if needed.

