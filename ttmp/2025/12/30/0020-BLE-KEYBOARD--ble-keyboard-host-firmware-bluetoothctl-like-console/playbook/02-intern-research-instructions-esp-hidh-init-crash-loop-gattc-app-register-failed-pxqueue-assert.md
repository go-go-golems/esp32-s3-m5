---
Title: 'Intern research instructions: esp_hidh init crash-loop (gattc_app_register failed, pxQueue assert)'
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
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c
      Note: Upstream source to correlate with internet findings
    - Path: 0020-cardputer-ble-keyboard-host/main/hid_host.c
      Note: Local implementation context for the bug signature
ExternalSources: []
Summary: Internet research playbook to confirm root cause and best-practice init/forwarding contract for esp_hidh crash-loop signature (gattc_app_register failed, pxQueue assert).
LastUpdated: 2025-12-30T20:50:12.136223306-05:00
WhatFor: ""
WhenToUse: ""
---


# Intern research instructions: esp_hidh init crash-loop (gattc_app_register failed, pxQueue assert)

## Purpose

Collect external evidence (issues, docs, examples) explaining why this crash-loop happens and what the official “correct” initialization and GATTC forwarding contract is for ESP-IDF’s BLE HID Host (`esp_hidh`) on ESP32-S3.

This is explicitly an internet research task: we want citations/links and a summary that we can use to either (a) lock down our current fix as “the correct pattern”, or (b) adjust our code to match upstream best practice.

## Environment Assumptions

- You have internet access (GitHub, Espressif docs, forums).
- You can read this repo locally, but do not need hardware access.
- Target stack and version:
  - ESP-IDF 5.4.1
  - BLE HID host component: `esp_hidh` / `ble_hidh`
- The crash signature to match is:
  - `esp_ble_gattc_app_register failed!`
  - `esp_hidh_init failed: ESP_ERR_INVALID_STATE`
  - `assert failed: xQueueGenericSend ... (pxQueue)`
  - backtrace frames include `SEND_CB` in `components/esp_hid/src/ble_hidh.c`

## Commands

Use these commands to orient yourself in the repo and pinpoint the exact signature (optional but helpful for accuracy):

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5

# Find the app forwarding path
rg -n "esp_hidh_gattc_event_handler|hid_host_forward_gattc_event|esp_hidh_init" 0020-cardputer-ble-keyboard-host/main

# Find the referenced ESP-IDF source (paths exist on the dev machine)
rg -n "SEND_CB\\b" /home/manuel/esp/esp-idf-5.4.1/components/esp_hid/src/ble_hidh.c
```

## Exit Criteria

Deliver a short report (1–2 pages) with:

1) **At least 3 authoritative links** (prefer: ESP-IDF GitHub issues/PRs, official examples, or Espressif docs) that discuss:
   - `esp_ble_gattc_app_register failed` in `esp_hidh` / `ble_hidh`,
   - the `SEND_CB` / `pxQueue` assert (or equivalent crash),
   - the correct init order and who should register GATTC.
2) A clear statement of the **official recommended pattern** for BLE HID host apps:
   - do we call `esp_ble_gattc_app_register()` ourselves or not?
   - when (if ever) is it safe to forward events to `esp_hidh_gattc_event_handler()`?
3) If there is an upstream fix, include:
   - the first ESP-IDF version that contains it,
   - the commit/PR link,
   - what changed (one paragraph).
4) A concrete recommendation:
   - “keep our current fix as-is” or “change X to match upstream”.

Store your report as a new `reference/` document under ticket `0020-BLE-KEYBOARD` and link it from the ticket index.

## Notes

### Primary in-repo context (read first)

- Crash-loop details and our local fix are documented in:
  - `ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md`
  - `ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/design-doc/02-bug-report-ble-hid-host-init-crash-loop-gattc-app-register-failed-queue-assert-pxqueue.md`
- Current firmware code lives in:
  - `esp32-s3-m5/0020-cardputer-ble-keyboard-host/`

### Suggested search queries (copy/paste)

- `esp_ble_hidh_init esp_ble_gattc_app_register failed`
- `ble_hidh SEND_CB xQueueGenericSend pxQueue`
- `esp_hidh_gattc_event_handler init order`
- `ESP_ERR_INVALID_STATE esp_hidh_init gattc_app_register`
- `ESP32 HID host example esp_hidh_gattc_event_handler`

### What we think is happening (hypothesis to validate)

- The app competes with `esp_hidh` for GATTC registration (app calls `esp_ble_gattc_app_register` when `esp_hidh` expects to own it), causing init to fail.
- After init failure, the app still forwards GATTC events into `esp_hidh_gattc_event_handler`, which can invoke `SEND_CB` with a null/invalid internal queue (`pxQueue`), producing an assert.

Your job is to confirm whether this is:

- “expected behavior when misusing the API” (then we should document the contract and keep our fix), or
- a known ESP-IDF bug that is fixed upstream (then we should consider upgrading or backporting the fix).
