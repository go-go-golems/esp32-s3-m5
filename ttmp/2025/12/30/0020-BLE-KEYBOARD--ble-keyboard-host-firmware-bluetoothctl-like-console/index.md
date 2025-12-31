---
Title: BLE keyboard host firmware + bluetoothctl-like console
Ticket: 0020-BLE-KEYBOARD
Status: active
Topics:
    - ble
    - keyboard
    - esp32s3
    - console
    - cardputer
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0013-atoms3r-gif-console/main/console_repl.cpp
      Note: Reference esp_console over USB Serial/JTAG (command registration + REPL startup)
    - Path: 0019-cardputer-ble-temp-logger/main/ble_temp_logger_main.c
      Note: Reference BLE bring-up pattern (Bluedroid init + callback wiring)
    - Path: 0020-cardputer-ble-keyboard-host/main/ble_host.c
      Note: Adds decoded logs for GATTC open/disconnect/close; defers connect until scan stop complete (commit 6a96946)
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/playbook/03-using-ble-console-to-scan-connect-pair-and-keylog-a-ble-keyboard.md
      Note: Operator playbook for driving ble> console during pairing/connect attempts
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/reference/01-diary.md
      Note: Step 7 records mapping and sequencing change (commit 6a96946)
    - Path: ttmp/2025/12/30/0020-BLE-KEYBOARD--ble-keyboard-host-firmware-bluetoothctl-like-console/sources/ble-gatt-bug-report-research.md
      Note: Intern research summary mapping 0x85/0x0100 and likely root causes (scan/connect overlap
ExternalSources: []
Summary: |
    Build an ESP32-S3 firmware that acts as a BLE Central (host) to connect to a BLE keyboard (HOGP) and ingest key reports, plus a USB Serial/JTAG `esp_console` REPL that provides bluetoothctl-like commands (scan/connect/pair/unpair/trust/info/bonds) to manage devices from a host terminal.
LastUpdated: 2025-12-30T20:03:33.973096356-05:00
WhatFor: ""
WhenToUse: ""
---


# BLE keyboard host firmware + bluetoothctl-like console

## Overview

This ticket is about building a “BLE keyboard host” firmware for ESP32-S3 (target: Cardputer-class boards) where the ESP32 runs as a **BLE Central** and
connects to a **BLE HID keyboard** (HID over GATT / HOGP), consuming HID input reports and translating them into usable key events.

To make iteration practical, the firmware should expose a **USB Serial/JTAG** `esp_console` REPL that behaves like a tiny `bluetoothctl`: scanning, connecting,
pairing/bond management, and status introspection without reflashing.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Analysis**:
  - `analysis/01-ble-hid-keyboard-host-firmware-architecture-files-and-build-plan.md` — how to build the firmware, key ESP-IDF files/symbols, data flow
  - `analysis/02-usb-esp-console-bluetoothctl-like-ble-scan-pair-trust-cli.md` — how to build the console, command set, wiring to BLE state machine
- **Diary**:
  - `reference/01-diary.md` — ongoing research notes and decisions

## Status

Current status: **active**

## Topics

- ble
- keyboard
- esp32s3
- console
- cardputer

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
