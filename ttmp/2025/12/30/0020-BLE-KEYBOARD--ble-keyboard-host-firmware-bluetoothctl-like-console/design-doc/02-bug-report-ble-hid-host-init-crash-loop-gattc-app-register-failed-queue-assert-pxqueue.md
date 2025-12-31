---
Title: 'Bug report: BLE HID host init crash-loop (gattc_app_register failed, queue assert pxQueue)'
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh_gattc.h
      Note: Defines esp_hidh_gattc_event_handler contract
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c
      Note: Backtrace source for SEND_CB / pxQueue assert
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: GATTC callback that forwards into esp_hidh
    - Path: 0020-cardputer-ble-keyboard-host/main/hid_host.c
      Note: Gates esp_hidh init + GATTC forwarding to prevent crash-loop
ExternalSources: []
Summary: Crash-loop on boot when BLE HID host is initialized before BLE stack or when app competes for GATTC registration; fixed by init-order/ownership changes.
LastUpdated: 2025-12-30T20:50:09.97165015-05:00
WhatFor: ""
WhenToUse: ""
---


# Bug report: BLE HID host init crash-loop (gattc_app_register failed, queue assert pxQueue)

## Executive Summary

The initial version of the BLE HID keyboard host firmware crash-looped immediately after boot. Logs showed `esp_hidh_init()` failing with `ESP_ERR_INVALID_STATE` after `esp_ble_gattc_app_register` failed, followed by an assert in `xQueueGenericSend()` originating in `components/esp_hid/src/ble_hidh.c` (`SEND_CB()`).

Root cause: the application’s GATTC callback was forwarding GATTC events into `esp_hidh_gattc_event_handler()` while the HID host internal callback queue (`pxQueue`) was not initialized, and the app was also competing with the HID host for GATTC “app id” registration. This combination leads to “init failed” and then an unsafe event delivery path that asserts.

This is fixed in `18ef2e364e2498bf8288cb23b394ec3ef31cd8f6` by enforcing init order (BLE stack before HID host), removing the app’s direct `esp_ble_gattc_app_register(...)` usage, and gating GATTC event forwarding so it only occurs when safe.

## Problem Statement

### Symptoms

On boot, the device resets repeatedly with the following signature (verbatim from monitor output):

- HID host init fails:
  - `E (...) BLE_HIDH: esp_ble_hidh_init(629): esp_ble_gattc_app_register failed!`
  - `E (...) ESP_HIDH: esp_hidh_init(111): BLE HIDH init failed`
  - `E (...) hid_host: esp_hidh_init failed: ESP_ERR_INVALID_STATE`
- Then an assert:
  - `assert failed: xQueueGenericSend queue.c:936 (pxQueue)`
- Backtrace includes:
  - `SEND_CB` at `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c:47`
  - `esp_hidh_gattc_event_handler` at `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c:341`
  - `hid_host_forward_gattc_event` at `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/hid_host.c:124`
  - `gattc_cb` at `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c:221`

### Impact

- The crash-loop prevents reaching the `esp_console` REPL, so scanning/pairing/debugging cannot proceed.
- It makes the firmware unusable for iterative development: each boot ends in reset with no stable state.

### Environment

- Target: ESP32-S3 (Cardputer)
- ESP-IDF: `/home/manuel/esp/esp-idf-5.4.1/`
- Project: `esp32-s3-m5/0020-cardputer-ble-keyboard-host/`
- Monitor: `idf.py -p /dev/ttyACM0 flash monitor`

## Proposed Solution

Treat `esp_hidh` as the owner of “GATTC registration + event consumption”, and structure initialization so that:

1) The BLE stack is initialized first (controller + Bluedroid).
2) `esp_hidh_init()` is called after BLE is ready.
3) The application does not separately call `esp_ble_gattc_app_register()` for the HID host use-case.
4) The application forwards GATTC events into `esp_hidh_gattc_event_handler()` only when the HID host is in a state that can receive them.

Conceptual flow:

```text
boot
 ├─ init NVS
 ├─ init/enable BT controller
 ├─ init/enable Bluedroid
 ├─ register GAP callbacks (scan, security)
 ├─ register GATTC callback (forwarding shim)
 ├─ esp_hidh_init()        <-- HID host registers/uses GATTC internally
 └─ start console
```

Forwarding shim (pseudocode):

```c
void gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    if (hid_host_can_receive_gattc_events()) {
        esp_hidh_gattc_event_handler(event, gattc_if, param);
    }
    // Also handle any app-specific gattc events here if needed (currently none).
}
```

## Design Decisions

### Don’t call `esp_ble_gattc_app_register()` from the app

Rationale: `esp_hidh_init()` uses the BLE HID host component which manages its own registration. Competing registrations can produce `ESP_ERR_INVALID_STATE` and put the HID host in a half-initialized state.

### Forward GATTC events, but gate them carefully

Rationale: the HID host depends on GATTC callbacks for its own bring-up, so “only forward after init returns” is too simplistic. The chosen design ensures forwarding is enabled during the internal registration window, and disables forwarding if `esp_hidh_init()` fails so we never deliver events into a broken host.

### Prefer “fail closed” (stop forwarding) after init failure

Rationale: the observed assert indicates unsafe behavior when forwarding events into an uninitialized internal queue. If init fails, we should treat the HID subsystem as unavailable and avoid calling into it.

## Alternatives Considered

### Alternative A: Always forward GATTC events unconditionally

Rejected because it reproduces the assert when the HID host’s internal state is not initialized.

### Alternative B: Never forward GATTC events; use only GAP events

Rejected because `esp_hidh` explicitly requires GATTC event forwarding; without it, HOGP will not function.

### Alternative C: Defer registration of the app GATTC callback until after `esp_hidh_init()`

Not used because we still want a single “central point” for GATTC event dispatch and logging. Also, in practice `esp_hidh_init()` can trigger events immediately, and late registration would drop them.

## Implementation Plan

This is implemented already; the “plan” here is the minimal checklist for keeping it fixed:

1) Keep BLE stack init (controller + Bluedroid) before calling `esp_hidh_init()`.
2) Ensure no other module calls `esp_ble_gattc_app_register()` for this HID host firmware.
3) Ensure the GATTC callback forwards events into `esp_hidh_gattc_event_handler()` only when safe.
4) Add/keep a single boot log line that prints whether HID host init succeeded and whether forwarding is enabled (future regression detection).

Key code locations:

- `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/hid_host.c`
  - HID host init and forwarding gate
- `esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/ble_host.c`
  - GATTC callback (the forwarding shim)

## Open Questions

- Is this failure mode considered “expected if misused” by ESP-IDF, or is it a bug that should be hardened (no assert) in `esp_hid`?
- Is there an official documented contract for when it is safe to call `esp_hidh_gattc_event_handler()` (especially around init failures)?
- Does ESP-IDF 5.4.1 differ from newer versions in how `esp_hidh_init()` registers GATTC, and is there an upstream fix or example we should mirror exactly?

## References

- Diary: `ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md`
- Prior and current bug (connect/open): `ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/design-doc/01-bug-report-ble-hid-host-cannot-connect-open-dc-2c-26-fd-03-b7-disconnect-0x100-open-0x85.md`
- ESP-IDF sources referenced by the backtrace:
  - `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c`
  - `/home/manuel/esp/esp-idf-5.4.1/components/esp_hid/include/esp_hidh_gattc.h`
