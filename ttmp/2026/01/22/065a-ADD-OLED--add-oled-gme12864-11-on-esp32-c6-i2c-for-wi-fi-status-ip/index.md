---
Title: Add OLED (GME12864-11) on ESP32-C6 I2C for Wi-Fi status + IP
Ticket: 065a-ADD-OLED
Status: active
Topics:
    - esp-idf
    - esp32c6
    - xiao
    - i2c
    - oled
    - display
    - wifi
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0065-xiao-esp32c6-gpio-web-server/README.md
      Note: Firmware to integrate OLED status display
    - Path: ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/design-doc/01-oled-integration-wiring-esp-lcd-wi-fi-status-ip.md
      Note: Primary design doc
    - Path: ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/playbook/01-bring-up-checklist-i2c-oled-gme12864-11.md
      Note: Bring-up playbook
    - Path: ttmp/2026/01/22/065a-ADD-OLED--add-oled-gme12864-11-on-esp32-c6-i2c-for-wi-fi-status-ip/reference/01-diary.md
      Note: Implementation diary
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-22T20:48:50.019514823-05:00
WhatFor: ""
WhenToUse: ""
---


# Add OLED (GME12864-11) on ESP32-C6 I2C for Wi-Fi status + IP

## Overview

Add an I2C OLED (GME12864-11 class, 128×64) to the XIAO ESP32C6 firmware `0065-xiao-esp32c6-gpio-web-server` so the device can locally display:
- Wi‑Fi state (IDLE/CONNECTING/CONNECTED)
- SSID (if configured)
- IPv4 address (if DHCP succeeded)

The implementation plan uses ESP-IDF 5.4.1’s built-in `esp_lcd` SSD1306 driver and this repo’s existing `wifi_mgr` status snapshot.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- Design: `design-doc/01-oled-integration-wiring-esp-lcd-wi-fi-status-ip.md`
- Bring-up: `playbook/01-bring-up-checklist-i2c-oled-gme12864-11.md`
- Diary: `reference/01-diary.md`

## Status

Current status: **active**

## Topics

- esp-idf
- esp32c6
- xiao
- i2c
- oled
- display
- wifi
- ui

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
