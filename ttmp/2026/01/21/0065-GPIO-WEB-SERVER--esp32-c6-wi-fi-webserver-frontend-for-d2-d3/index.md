---
Title: ESP32-C6 Wi-Fi webserver + frontend for D2/D3
Ticket: 0065-GPIO-WEB-SERVER
Status: active
Topics:
    - esp-idf
    - esp32c6
    - wifi
    - webserver
    - http
    - gpio
    - console
    - usb-serial-jtag
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0065-xiao-esp32c6-gpio-web-server
      Note: Firmware project (ESP32-C6)
    - Path: ttmp/2026/01/21/0065-GPIO-WEB-SERVER--esp32-c6-wi-fi-webserver-frontend-for-d2-d3/design-doc/01-analysis-esp32-c6-gpio-web-server-d2-d3-toggle.md
      Note: Primary design/analysis
    - Path: ttmp/2026/01/21/0065-GPIO-WEB-SERVER--esp32-c6-wi-fi-webserver-frontend-for-d2-d3/reference/01-xiao-esp32c6-pin-mapping.md
      Note: Board pin mapping used by firmware
ExternalSources: []
Summary: 'ESP32-C6 firmware: Wi-Fi STA via esp_console (USB Serial/JTAG) + embedded web UI to toggle active-low D2/D3.'
LastUpdated: 2026-01-21T22:36:25.203057479-05:00
WhatFor: ""
WhenToUse: ""
---



# ESP32-C6 Wi-Fi webserver + frontend for D2/D3

## Overview

Deliverable: `0065-xiao-esp32c6-gpio-web-server/` (ESP-IDF) for ESP32‑C6.

- Wi‑Fi STA credentials configured at runtime via `esp_console` over USB Serial/JTAG (`wifi scan`, `wifi join`, …).
- After DHCP, device serves `http://<sta-ip>/` with a tiny UI and JSON endpoints to:
  - read GPIO state (`GET /api/gpio`)
  - set D2/D3 (`POST /api/gpio` with `{d2, d3}`)
  - toggle D2/D3 (`POST /api/gpio/d2/toggle`, `POST /api/gpio/d3/toggle`)
- Electrical model: active-low outputs by default (logical “on” drives the pin low).

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- `0065-xiao-esp32c6-gpio-web-server/README.md` (build/flash/use)
- `design-doc/01-analysis-esp32-c6-gpio-web-server-d2-d3-toggle.md` (design rationale)

## Status

Current status: **active**

## Topics

- esp-idf
- esp32c6
- wifi
- webserver
- http
- gpio
- console
- usb-serial-jtag

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
