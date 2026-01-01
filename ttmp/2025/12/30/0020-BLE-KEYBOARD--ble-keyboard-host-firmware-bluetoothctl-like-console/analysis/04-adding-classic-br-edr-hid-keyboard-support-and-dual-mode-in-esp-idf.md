---
Title: Adding Classic (BR/EDR) HID keyboard support (and dual-mode) in ESP-IDF
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
RelatedFiles:
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: Shows BLE-only controller init; would change to BTDM for dual-mode
    - Path: 0020-cardputer-ble-keyboard-host/main/hid_host.c
      Note: Already uses esp_hidh events and could be reused for Classic transport
    - Path: 0020-cardputer-ble-keyboard-host/sdkconfig.defaults
      Note: Current config disables Classic; would enable BT_CLASSIC + HID host
ExternalSources: []
Summary: Explains Classic BR/EDR vs BLE for keyboards, why a BR/EDR-only keyboard is invisible to the current BLE-only ESP32 firmware, and which ESP-IDF APIs/configs are needed to add Classic HID host (and optionally dual-mode).
LastUpdated: 2025-12-31T18:51:52.543742671-05:00
WhatFor: ""
WhenToUse: ""
---



## Summary

Your keyboard is discoverable via Classic (BR/EDR), not LE advertising, so our current BLE-only firmware will never see it. Supporting it means enabling the BT dual-mode controller (or Classic-only), adding Classic inquiry-based discovery (`esp_bt_gap_start_discovery`) and Classic pairing UX, and opening HID using `esp_hidh` with `ESP_HID_TRANSPORT_BT` (Classic HIDP).

# Adding Classic (BR/EDR) HID keyboard support (and dual-mode) in ESP-IDF

## Goal

Explain what it means that the keyboard is discoverable only via BR/EDR (`bluetoothctl scan bredr`), why our current firmware cannot see it, and what parts of the ESP-IDF stack we would need to use/change to support:

- Classic HID host (BR/EDR HIDP), and optionally
- dual-mode (support both BR/EDR and BLE HOGP in one firmware).

## Mental model: BLE vs BR/EDR is not “two ways to say Bluetooth”

Bluetooth has two transports that can coexist on the same radio:

- BR/EDR (“Classic”): inquiry/page, classic profiles (SDP/HIDP/etc), classic HID keyboards are very common.
- BLE (“Low Energy”, LE): advertising + scanning, GATT services, HID over GATT (HOGP) is common for newer BLE keyboards.

Linux `bluetoothctl` can show devices from both worlds. Our ESP32 firmware currently runs in BLE-only controller mode, so it can only see LE advertisements.

## What we observed: your keyboard is Classic-discoverable

You observed the keyboard appears in `bluetoothctl scan bredr`.

That implies one of these is true:

- the keyboard is Classic-only (BR/EDR HIDP, no LE at all), or
- the keyboard is dual-mode but, in the current pairing mode/state, it is only discoverable on BR/EDR.

Either way, a BLE-only scanner will not see it.

## Where we are today: this firmware is BLE-only by design

In `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`, init explicitly disables Classic:

- `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)`
- `esp_bt_controller_enable(ESP_BT_MODE_BLE)`

And the tutorial defaults in `esp32-s3-m5/0020-cardputer-ble-keyboard-host/sdkconfig.defaults` set:

- `CONFIG_BT_BLE_ENABLED=y`
- `CONFIG_BT_CLASSIC_ENABLED=n`

So even though ESP32-S3 hardware supports BR/EDR, this project currently builds and boots as BLE-only.

## What “discovery” means in Classic vs BLE

### BLE discovery (what our firmware does)

- Devices are discovered by LE advertising packets.
- ESP-IDF event: `ESP_GAP_BLE_SCAN_RESULT_EVT` via `esp_ble_gap_register_callback(...)`.
- Name is often split across advertisement and scan response; we already had to fix this by parsing `adv_data_len + scan_rsp_len`.

### Classic discovery (what `bluetoothctl scan bredr` does)

- Devices are discovered by Classic inquiry (not advertising).
- ESP-IDF API: `esp_bt_gap_start_discovery(mode, inq_len, num_rsps)`.
- ESP-IDF events (via `esp_bt_gap_register_callback(...)`):
  - `ESP_BT_GAP_DISC_RES_EVT` (per device)
  - `ESP_BT_GAP_DISC_STATE_CHANGED_EVT` (start/stop)
- Classic discovery can also do “name discovery” (connect briefly to fetch remote name) depending on cached results (documented in `esp_gap_bt_api.h`).

This difference is the core reason Linux “sees it immediately” while our ESP32 LE scanner does not: the keyboard is not emitting LE advertising traffic.

## HID keyboard profiles: HIDP (Classic) vs HOGP (BLE)

### Classic HID (HIDP over BR/EDR)

High-level flow:

1) Discover device via inquiry (`esp_bt_gap_start_discovery`).
2) Pair/bond (Classic pairing: legacy PIN or SSP).
3) Connect HID profile (Classic HID host).
4) Receive HID input reports over the Classic HID interrupt channel.

Key characteristics:

- Not GATT-based.
- Uses Classic pairing events (`ESP_BT_GAP_*`).
- Device classification can use Class of Device (CoD) (keyboard/mouse bits) and/or SDP.

### BLE HID (HOGP over GATT)

High-level flow:

1) Discover via LE scan.
2) Connect as GATT client.
3) Pair/bond (LE SMP) + enable notifications.
4) Receive HID input reports via GATT notifications.

Key characteristics:

- Fully GATT-based.
- Uses LE pairing/security events (`ESP_GAP_BLE_*`).

## ESP-IDF building blocks we can reuse: `esp_hidh` supports both transports

ESP-IDF provides an HID Host abstraction in `components/esp_hid` that supports:

- BLE transport: `ESP_HID_TRANSPORT_BLE`
- Classic transport: `ESP_HID_TRANSPORT_BT`

The key API is:

- `esp_hidh_dev_open(bda, ESP_HID_TRANSPORT_BLE, addr_type)` → BLE HOGP path
- `esp_hidh_dev_open(bda, ESP_HID_TRANSPORT_BT, 0)` → Classic HIDP path (dispatches to `esp_bt_hidh_dev_open`)

So we do not have to implement HIDP ourselves, but we do need to enable Classic in config/runtime and add Classic discovery + pairing UX.

## What it takes to add Classic HID support (rough implementation plan)

### 1) Enable Classic + HID Host in sdkconfig (build-time)

Update `esp32-s3-m5/0020-cardputer-ble-keyboard-host/sdkconfig.defaults`:

- `CONFIG_BT_CLASSIC_ENABLED=y`
- `CONFIG_BT_BLE_ENABLED=y` (if dual-mode; or `n` if Classic-only build)
- `CONFIG_BT_HID_ENABLED=y`
- `CONFIG_BT_HID_HOST_ENABLED=y`

This increases flash/RAM usage and typically requires more heap headroom.

### 2) Enable dual-mode controller at runtime (boot-time)

Update `ble_host_init()` in `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`:

- remove `esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)`
- change controller mode from BLE-only to dual-mode:
  - `esp_bt_controller_enable(ESP_BT_MODE_BTDM)`

This is the key switch to “support both modes” at the controller level.

### 3) Add Classic GAP callback + inquiry-based device registry

Add:

- `esp_bt_gap_register_callback(bt_gap_cb)`
- `esp_bt_gap_start_discovery(...)` for “scan bredr”

Then parse `ESP_BT_GAP_DISC_RES_EVT` into a Classic device registry with at least:

- `bda` (address)
- `cod` (Class of Device) to filter keyboards
- remote name (from EIR or name discovery)

This is a parallel discovery pipeline to the existing BLE scan registry.

### 4) Add Classic pairing UX

Classic pairing events you may need to surface in the console:

- `ESP_BT_GAP_PIN_REQ_EVT` (legacy PIN)
- `ESP_BT_GAP_CFM_REQ_EVT` (SSP numeric comparison confirm)
- `ESP_BT_GAP_KEY_REQ_EVT` / `ESP_BT_GAP_KEY_NOTIF_EVT` (SSP passkey entry / notification)
- `ESP_BT_GAP_AUTH_CMPL_EVT` (bond result)

The operator UX differs from BLE. Many keyboards use “type this code on the keyboard then press Enter” style flows.

### 5) Open the HID device via Classic transport

Once discovered and (if necessary) paired:

- call `esp_hidh_dev_open(bda, ESP_HID_TRANSPORT_BT, 0)`
- continue using existing `esp_hidh` events:
  - `ESP_HIDH_INPUT_EVENT` for key reports
  - `ESP_HIDH_OPEN_EVENT` / `ESP_HIDH_CLOSE_EVENT`

Your current `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/hid_host.c` is already based around these `esp_hidh` events, so much of the “decode keyboard reports” path can remain unchanged.

## Dual-mode vs either/or: what’s realistic?

### Can we support both modes in one firmware?

Yes, conceptually:

- set controller mode to `ESP_BT_MODE_BTDM`,
- keep two discovery mechanisms (LE scan + Classic inquiry),
- open with the correct HID transport (`ESP_HID_TRANSPORT_BLE` vs `ESP_HID_TRANSPORT_BT`).

### Practical constraints / sharp edges

- Radio time-sharing: Classic inquiry and LE scanning share the same controller/radio; running both concurrently can reduce reliability and complicate debugging.
- State explosion: you now have two security stacks (LE SMP vs Classic pairing/SSP) and two discovery registries.
- Memory: dual-mode consumes more heap and stack; some defaults may need tuning.

## ESP-IDF code and config entry points (where to learn “how it works”)

### Classic GAP (BR/EDR)

- `/home/manuel/esp/esp-idf-5.3.4/components/bt/host/bluedroid/api/include/api/esp_gap_bt_api.h`
  - `esp_bt_gap_register_callback`
  - `esp_bt_gap_start_discovery`
  - pairing events: `ESP_BT_GAP_PIN_REQ_EVT`, `ESP_BT_GAP_CFM_REQ_EVT`, `ESP_BT_GAP_AUTH_CMPL_EVT`, etc.
  - helper: `esp_bt_gap_resolve_eir_data` for parsing Classic EIR fields (name, etc.)

### HID host abstraction

- `/home/manuel/esp/esp-idf-5.3.4/components/esp_hid/include/esp_hid_common.h`
  - `esp_hid_transport_t` includes `ESP_HID_TRANSPORT_BT` and `ESP_HID_TRANSPORT_BLE`
- `/home/manuel/esp/esp-idf-5.3.4/components/esp_hid/src/esp_hidh.c`
  - `esp_hidh_dev_open(...)` dispatches to BLE vs BT based on transport
- `/home/manuel/esp/esp-idf-5.3.4/components/esp_hid/src/bt_hidh.c`
  - `esp_bt_hidh_dev_open(...)` connects via Classic HID host (`esp_bt_hid_host_connect`)

### Kconfig knobs for Classic HID host

- `/home/manuel/esp/esp-idf-5.3.4/components/bt/host/bluedroid/Kconfig.in`
  - `BT_CLASSIC_ENABLED`
  - `BT_HID_ENABLED` / `BT_HID_HOST_ENABLED`

## Difficulty assessment (what will take time)

If your keyboard is Classic-only and the goal is “make it work on ESP32”:

- Enabling Classic + HID host: straightforward (config + controller mode).
- Implementing Classic inquiry registry + console UX: moderate.
- Pairing UX: usually the hardest part (PIN vs SSP flows vary by keyboard).

Supporting both BLE and Classic in one firmware is doable but meaningfully more complex, mostly because it forces you to unify two discovery and security models into a single operator experience.

## Next steps (recommended)

1) Decide scope:
   - Classic-only keyboard host (simpler), or
   - dual-mode host (more general).
2) Capture `bluetoothctl info <addr>` for the keyboard and save it into a ticket doc (it often includes the “Class” and HID-related UUIDs that confirm Classic HID).
3) If we proceed:
   - implement `scan bredr` in the console,
   - add a Classic device registry,
   - open HID devices with `ESP_HID_TRANSPORT_BT`.
