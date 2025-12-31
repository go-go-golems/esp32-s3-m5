---
Title: BLE Temperature Sensor Logger on Cardputer
Ticket: 0019-BLE-TEST
Status: active
Topics:
    - ble
    - cardputer
    - sensors
    - esp32s3
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/ble_keyboard_wrap.cpp
      Note: Reference BLE implementation using Bluedroid GATT server
    - Path: M5Cardputer-UserDemo/main/apps/utils/ble_keyboard_wrap/esp_hid_gap.c
      Note: GAP event handling and advertising setup
    - Path: esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c
      Note: SHT3x temperature sensor implementation example with I2C master driver
ExternalSources: []
Summary: Create a BLE-based temperature sensor logger on the M5Stack Cardputer. Comprehensive analysis documents created covering BLE implementation (Bluedroid), temperature sensor options (SHT3x recommended), I2C access patterns, debugging playbook, and step-by-step implementation guide.
LastUpdated: 2025-12-30T11:26:54.312684899-05:00
WhatFor: ""
WhenToUse: ""
---



# BLE Temperature Sensor Logger on Cardputer

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Analysis**:
  - `analysis/03-ble-4-2-vs-ble-5-0-advertising-apis-esp-idf-5-4-1-bluedroid.md` — BLE 4.2 vs 5.0 advertising APIs + how-to guides (ESP-IDF 5.4.1)
- **Playbooks**:
  - `playbook/02-e2e-test-cardputer-ble-temp-logger--tmux.md` — tmux-based end-to-end test (flash+monitor + host BLE client)

## Status

Current status: **active**

## Topics

- ble
- cardputer
- sensors
- esp32s3

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
