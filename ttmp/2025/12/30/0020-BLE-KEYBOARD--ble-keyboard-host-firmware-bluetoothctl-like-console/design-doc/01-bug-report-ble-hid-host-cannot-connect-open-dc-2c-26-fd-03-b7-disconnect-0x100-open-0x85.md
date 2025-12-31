---
Title: 'Bug report: BLE HID host cannot connect/open DC:2C:26:FD:03:B7 (disconnect 0x100, open 0x85)'
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
RelatedFiles: []
ExternalSources: []
Summary: '0020 firmware boots and scans, but direct connect to target keyboard fails: GATTC disconnect reason 0x100 and BLE_HIDH open status 0x85 (ESP_GATT_ERROR).'
LastUpdated: 2025-12-30T20:44:58.607576523-05:00
WhatFor: ""
WhenToUse: ""
---

# Bug report: BLE HID host cannot connect/open DC:2C:26:FD:03:B7 (disconnect 0x100, open 0x85)

## Executive Summary

We have an ESP32-S3 firmware (`esp32-s3-m5/0020-cardputer-ble-keyboard-host/`) intended to act as a BLE Central HID Host (HOGP) and print keyboard input over serial. The firmware now boots reliably and provides an `esp_console` REPL over USB Serial/JTAG.

However, connecting to the target keyboard address `DC:2C:26:FD:03:B7` fails:

- The GATTC layer reports a disconnect with `reason=0x100`.
- The HID host open path reports `BLE_HIDH: OPEN failed: 0x85` where `0x85` is `ESP_GATT_ERROR` (generic error).
- The target address is not observed during scanning (`scan on …` + `devices`).

This document captures current behavior, expected behavior, reproduction steps, and hypotheses so we can converge on a fix efficiently.

## Problem Statement

**Expected**: With the keyboard in pairing mode, the firmware sees it in `devices`, can `connect` and `pair`, and then prints typed key events over serial when `keylog on` is enabled.

**Actual**:

1) The keyboard address `DC:2C:26:FD:03:B7` does not appear in `devices` after scanning.
2) Direct connect attempts fail with:
   - `gattc disconnect: reason=0x100`
   - `BLE_HIDH: OPEN failed: 0x85`
   - `hid_host: esp_hidh_dev_open failed`

So we cannot pair/bond, and we never receive `ESP_HIDH_INPUT_EVENT` reports.

## Proposed Solution

This is not yet a “solution” but a working plan to converge quickly:

1) Determine whether the address `DC:2C:26:FD:03:B7` is actually the keyboard’s currently-advertised address (vs an identity address / previously seen public address / sticker address). If the keyboard uses address randomization (RPA), connecting to the old address will fail and scanning will show a different address.
2) Improve scan output so we can confidently identify the keyboard:
   - parse the full advertising payload (adv + scan response) where applicable
   - add a “HID keyboard hint” (appearance/service uuid 0x1812) when found
3) If the keyboard is visible but connect fails:
   - capture the exact sequence of GAP and GATTC events around connect/open
   - confirm correct `addr_type` for the target (pub vs random) and carry it from scan results
4) If the keyboard is not visible at all:
   - validate the keyboard is in connectable advertising mode (pairing mode) and not already bonded to another host
   - attempt longer scans and/or different scan parameters

The immediate next engineering step is to enhance diagnostics and identification, not to rewrite the stack.

## Design Decisions

- Use ESP-IDF `esp_hidh` host component rather than hand-implementing HOGP over raw GATTC.
  - Rationale: reduces GATT discovery/subscription complexity; focus on UX and stability.
- Keep scan registry small and print a stable table to support manual workflows and debugging.

## Alternatives Considered

- Implement raw GATTC connection + manual HOGP discovery (HID service, report characteristic, CCCD writes).
  - Not chosen initially because it is significantly more code and easier to get wrong.
- Switch to NimBLE host stack.
  - Not chosen initially because we want to reduce moving parts while debugging; the repo already has Bluedroid-based BLE work.

## Implementation Plan

### Reproduction steps

1) Flash + monitor:
   - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
   - `cd esp32-s3-m5/0020-cardputer-ble-keyboard-host`
   - `idf.py -p /dev/ttyACM0 flash monitor`
2) At the `ble>` prompt:
   - `scan on 15`
   - `devices`
   - attempt:
     - `connect DC:2C:26:FD:03:B7 pub`
     - `connect DC:2C:26:FD:03:B7 rand`

### Current observed output (key lines)

- Scan completes and yields many results, but not `DC:2C:26:FD:03:B7`.
- Connect attempt:
  - `gattc disconnect: reason=0x100`
  - `BLE_HIDH: OPEN failed: 0x85`

### Triage checklist

- Verify keyboard is in pairing/connectable advertising mode.
- Confirm whether keyboard rotates addresses (RPA) and whether `DC:2C:26:FD:03:B7` is stable.
- If scan shows a likely candidate, connect by scan index so addr_type is carried.
- If connect fails even with a just-seen scan entry, record:
  - GAP security events (`SEC_REQ`, `NC_REQ`, `PASSKEY_*`)
  - GATTC `CONNECT`/`DISCONNECT` events
  - HID host open status (`ESP_HIDH_OPEN_EVENT` status, and `BLE_HIDH: OPEN failed` code)

## Open Questions

- What is `reason=0x100` for the GATTC disconnect in Bluedroid on ESP-IDF 5.4.1? Is it “connection timeout”, “failed to establish”, or a mapped HCI status?
- Is `DC:2C:26:FD:03:B7` an identity address while advertisements use a resolvable private address?
- Are we missing scan response parsing such that we fail to identify the keyboard (even if it is advertising)?

## References

- Firmware project: `esp32-s3-m5/0020-cardputer-ble-keyboard-host/`
- Diary timeline: `reference/01-diary.md`
