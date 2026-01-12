---
Title: Analyze NCP H2 Gateway Firmware
Ticket: 0032-ANALYZE-NCP-FIRMWARE
Status: active
Topics:
    - zigbee
    - ncp
    - esp32
    - firmware
    - protocol
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/include/esp_zb_ncp.h
      Note: Public API header for NCP component
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c
      Note: UART bus transport layer implementation
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_frame.c
      Note: Protocol frame encoding/decoding with SLIP
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_main.c
      Note: NCP main task and event loop
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Zigbee command handler and stack integration
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/slip.c
      Note: SLIP encoding/decoding implementation
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/main/esp_zigbee_ncp.c
      Note: Main entry point for NCP firmware
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-06T02:07:34.493514544-05:00
WhatFor: ""
WhenToUse: ""
---


# Analyze NCP H2 Gateway Firmware

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- zigbee
- ncp
- esp32
- firmware
- protocol

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
