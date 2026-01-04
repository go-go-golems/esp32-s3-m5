---
Title: Zigbee Gateway (M5Stack Unit Gateway H2)
Ticket: 001-ZIGBEE-GATEWAY
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp32
    - esp32h2
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md
      Note: Playbook to build/flash/run the gateway
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/reference/02-rcp-spinel-vs-ncp-znp-style-on-unit-gateway-h2.md
      Note: RCP vs NCP architecture explanation and M5Stack guide comparison
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_gateway.html
      Note: Archived HTML of the M5Stack guide
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_gateway.txt
      Note: Archived text dump of the M5Stack guide
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_ncp.html
      Note: Archived HTML of the M5Stack Zigbee NCP guide
    - Path: esp32-s3-m5/ttmp/2026/01/04/001-ZIGBEE-GATEWAY--zigbee-gateway-m5stack-unit-gateway-h2/sources/m5stack_zigbee_ncp.txt
      Note: Archived text dump of the M5Stack Zigbee NCP guide
ExternalSources:
    - https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway:M5Stack guide (Unit Gateway H2 + CoreS3, ESP Zigbee Gateway)
Summary: Ticket workspace for running the ESP Zigbee Gateway example with Unit Gateway H2 (ESP32-H2 RCP) + CoreS3 (ESP32-S3 gateway).
LastUpdated: 2026-01-04T15:47:58.323452966-05:00
WhatFor: ""
WhenToUse: ""
---



# Zigbee Gateway (M5Stack Unit Gateway H2)

## Overview

Bring up the M5Stack Zigbee gateway reference setup:

- **Unit Gateway H2 (ESP32-H2)** as the 802.15.4 RCP (flashed with `ot_rcp`)
- **CoreS3 (ESP32-S3)** as the gateway MCU (flashed with `esp_zigbee_gateway`)

## Key Links

- Playbook: [ESP Zigbee Gateway (Unit Gateway H2 + CoreS3)](./playbook/01-esp-zigbee-gateway-unit-gateway-h2-cores3.md)
- Archived source: [m5stack_zigbee_gateway.txt](./sources/m5stack_zigbee_gateway.txt)
- External source: https://docs.m5stack.com/en/esp_idf/zigbee/unit_gateway_h2/zigbee_gateway

## Status

Current status: **active**

## Topics

- zigbee
- esp-idf
- esp32
- esp32h2

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbook/ - Command sequences and test procedures
- playbooks/ - Legacy playbook directory (prefer `playbook/`)
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
