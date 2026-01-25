---
Title: 'Cardputer ADV: DS18B20 temperature over NimBLE + UI + esp_console'
Ticket: 0067-cardputer-adv-ds18b20-ble-temp-ui-console
Status: active
Topics:
    - cardputer
    - ble
    - sensors
    - ui
    - console
    - esp-idf
    - esp32s3
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console--cardputer-adv-ds18b20-temperature-over-nimble-ui-esp-console/analysis/01-how-to-build-cardputer-adv-ds18b20-nimble-notifications-ui-esp-console.md
      Note: Primary analysis document for the ticket.
    - Path: ttmp/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console--cardputer-adv-ds18b20-temperature-over-nimble-ui-esp-console/reference/01-diary.md
      Note: Research diary for how this ticket was produced.
ExternalSources: []
Summary: 'Blueprint ticket for a Cardputer ADV firmware: DS18B20 temperature acquisition + NimBLE GATT notifications + on-device status UI + USB Serial/JTAG esp_console.'
LastUpdated: 2026-01-23T15:44:38.191780992-05:00
WhatFor: To collect the analysis, reference paths, and implementation plan for building a DS18B20-based temperature probe firmware for Cardputer ADV that streams over BLE (NimBLE) and remains debuggable via esp_console.
WhenToUse: Use when starting or reviewing the implementation of a Cardputer ADV temperature probe firmware (DS18B20) with a BLE notify characteristic and an on-device status UI.
---


# Cardputer ADV: DS18B20 temperature over NimBLE + UI + esp_console

## Overview

This ticket is the analysis + reference bundle for building a Cardputer ADV firmware that:

- Reads temperature from a DS18B20 (1‑Wire) probe.
- Exposes it over BLE via a NimBLE GATT server (Read + Notify characteristic with `int16` centi‑°C payload).
- Shows temperature + BLE status on the Cardputer display.
- Provides a USB Serial/JTAG `esp_console` REPL for debugging and manual control.

## Key Links

- Analysis: [01-how-to-build-cardputer-adv-ds18b20-nimble-notifications-ui-esp-console.md](./analysis/01-how-to-build-cardputer-adv-ds18b20-nimble-notifications-ui-esp-console.md)
- Diary: [01-diary.md](./reference/01-diary.md)
- reMarkable upload: `/ai/2026/01/23/0067-cardputer-adv-ds18b20-ble-temp-ui-console/0067 Cardputer ADV DS18B20 NimBLE Temp.pdf`

## Status

Current status: **active**

## Topics

- cardputer
- ble
- sensors
- ui
- console
- esp-idf
- esp32s3
- debugging

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
