---
Title: ESP32-C3 Stamp Matrix HTTP Firmware
Ticket: ESP-01-STAMP-MATRIX
Status: active
Topics:
    - esp32
    - esp-idf
    - m5stack
    - led-matrix
    - wifi
    - rest
    - console
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: Legacy animation source baseline for 0067 extraction
    - Path: components/wifi_console/wifi_console.c
      Note: Shared Wi-Fi REPL component used by planned 0067 firmware
    - Path: components/wifi_mgr/wifi_mgr.c
      Note: Shared Wi-Fi manager component used by planned 0067 firmware
    - Path: ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/design/01-0067-esp-c3-led-matrix-http-firmware-architecture-and-intern-guide.md
      Note: Primary architecture and onboarding deliverable
    - Path: ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/design/02-reusable-c-matrix-max7219-component-extraction-plan.md
      Note: Second design doc for reusable C++ extraction with built-in parser registration
    - Path: ttmp/2026/02/21/ESP-01-STAMP-MATRIX--esp32-c3-stamp-matrix-http-firmware/reference/01-diary.md
      Note: Frequent implementation diary and rationale trail
ExternalSources: []
Summary: Ticket index for analysis and architecture planning of firmware 0067 (STAMP C3 + MAX7219 + Wi-Fi REST + esp_console).
LastUpdated: 2026-02-21T16:27:00-05:00
WhatFor: Ticket hub for planning firmware 0067 (ESP32-C3 STAMP + 12x MAX7219 + Wi-Fi REST + esp_console).
WhenToUse: Use this index to navigate design, diary, task progress, and changelog for ESP-01-STAMP-MATRIX.
---



# ESP32-C3 Stamp Matrix HTTP Firmware

## Overview

This ticket defines how to create `0067-esp-c3-led-matrix-http`, a new firmware that ports the existing Cardputer MAX7219 bouncing text engine to M5Stack STAMP C3 and adds Wi-Fi REST control with `esp_console` support.

Primary integration inputs are:

- `0036-cardputer-adv-led-matrix-console` for animation and matrix control.
- Shared `components/wifi_mgr` and `components/wifi_console` for runtime Wi-Fi and REPL.
- `0065`/`0066` HTTP patterns for clean JSON APIs and startup sequencing.

## Key Links

- Design doc: `design/01-0067-esp-c3-led-matrix-http-firmware-architecture-and-intern-guide.md`
- Diary: `reference/01-diary.md`
- Tasks: `tasks.md`
- Changelog: `changelog.md`

## Status

Current status: **active**

## Topics

- esp32
- esp-idf
- m5stack
- led-matrix
- wifi
- rest
- console

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
