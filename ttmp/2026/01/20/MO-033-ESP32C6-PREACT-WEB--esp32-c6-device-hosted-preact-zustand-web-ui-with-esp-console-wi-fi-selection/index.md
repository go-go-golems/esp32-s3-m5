---
Title: ESP32-C6 device-hosted Preact+Zustand web UI with esp_console Wi-Fi selection
Ticket: MO-033-ESP32C6-PREACT-WEB
Status: complete
Topics:
    - esp32c6
    - esp-idf
    - wifi
    - console
    - http
    - webserver
    - ui
    - assets
    - preact
    - zustand
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0045-xiao-esp32c6-preact-web/README.md
      Note: Implementation tutorial entrypoint (build/flash + usage)
    - Path: 0045-xiao-esp32c6-preact-web/main/app_main.c
      Note: Starts Wi-Fi manager
    - Path: docs/playbook-embedded-preact-zustand-webui.md
      Note: Reference for deterministic Vite bundling and firmware embedding
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T17:08:53.050109479-05:00
WhatFor: Design and implement a tiny ESP32-C6 firmware that uses an interactive esp_console REPL to scan/select/join Wi-Fi networks (credentials persisted in NVS) and serves an embedded Preact+Zustand counter web UI bundle from esp_http_server.
WhenToUse: Use when building a minimal device-hosted web UI demo on ESP32-C6 that needs runtime Wi-Fi selection via esp_console and a firmware-embedded SPA bundle.
---



# ESP32-C6 device-hosted Preact+Zustand web UI with esp_console Wi-Fi selection

## Overview

This ticket is a clean, minimal “device-hosted UI” demo for **ESP32-C6**:

- **esp_console REPL** (prefer USB Serial/JTAG) is used to configure Wi‑Fi at runtime:
  - scan for networks
  - select/join a network
  - optionally persist credentials in NVS
- **esp_http_server** serves a static, embedded **Vite + Preact + Zustand** “counter” bundle from flash.

It is intended to be a template for future “embedded web UI” projects, but sized small enough to be understood in one sitting.

## Key Links

- Analysis: `analysis/01-codebase-analysis-existing-patterns-for-esp-console-wi-fi-selection-embedded-preact-zustand-assets.md`
- Design: `design-doc/01-design-esp32-c6-console-configured-wi-fi-device-hosted-preact-zustand-counter-ui-embedded-assets.md`
- Playbook: `playbook/01-playbook-build-flash-connect-verify-web-ui-esp32-c6.md`

## Status

Current status: **active**

## Topics

- esp32c6
- esp-idf
- wifi
- console
- http
- webserver
- ui
- assets
- preact
- zustand

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
