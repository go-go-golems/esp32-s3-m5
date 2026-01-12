---
Title: 'Guide: BLE GATT notify + HID host pairing'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: 'Expanded research and writing guide for BLE GATT notify + HID host pairing.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter draft the BLE GATT + HID host doc.'
WhenToUse: 'Use before writing the BLE recipes document.'
---

# Guide: BLE GATT notify + HID host pairing

## Purpose

This guide enables a ghostwriter to create a single-topic doc with two concise recipes: (1) a GATT notify server that streams temperature, and (2) a BLE HID keyboard host that scans, pairs, and logs key events. The final doc should be practical and command-oriented.

## Assignment Brief

Write a developer guide that is specific and reproducible. The reader should be able to build the two firmwares, connect a BLE client, and see notifications or key events. Keep the scope narrow: GATT notify server and HID host pairing flow only.

## Environment Assumptions

- ESP-IDF 5.4.1
- Cardputer hardware
- BLE-capable host for testing (phone or laptop)

## Source Material to Review

- `esp32-s3-m5/0019-cardputer-ble-temp-logger/README.md`
- `esp32-s3-m5/0020-cardputer-ble-keyboard-host/README.md`
- `esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/ble_client.py`
- `esp32-s3-m5/ttmp/2025/12/30/0019-BLE-TEST--ble-temperature-sensor-logger-on-cardputer/reference/02-diary.md`
- `esp32-s3-m5/ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md`

## Technical Content Checklist

- GATT server layout:
  - Service UUID 0xFFF0
  - Notify characteristic UUID 0xFFF1 (int16 temp in centi-degC)
  - Control characteristic UUID 0xFFF2 (uint16 notify period)
- Host testing with `ble_client.py` or `bleak`
- HID host flow in console:
  - `scan on 30`, `devices`, `connect`, `pair`, `keylog on`
- Expected console prompts and outputs (`ble>`)

## Pseudocode Sketch

Use a brief schema snippet and command flow.

```text
GATT schema:
  service 0xFFF0
    char 0xFFF1 notify (temp_centi_deg)
    char 0xFFF2 write (notify_period_ms)

HID host flow:
  scan on 30
  devices
  connect 0
  pair 0
  keylog on
```

## Pitfalls to Call Out

- Pairing can fail if the keyboard uses a different address type.
- Notifications depend on CCCD enablement by the client.
- Scans may be slow; advise scan duration and filtering usage.

## Suggested Outline

1. Two recipes overview
2. GATT notify server: schema + test steps
3. HID host: scan/pair/connect + key log
4. Troubleshooting checklist

## Commands

```bash
# GATT notify server
cd esp32-s3-m5/0019-cardputer-ble-temp-logger
idf.py set-target esp32s3
idf.py build flash monitor

# HID host
cd ../0020-cardputer-ble-keyboard-host
idf.py build flash monitor
```

## Exit Criteria

- Doc includes the exact UUIDs and payload formats.
- Doc includes the console command flow for HID host.
- Doc includes at least one client test path (bleak or ble_client.py).

## Notes

Keep scope on BLE GATT notify and HID host pairing. Avoid generic BLE theory.
