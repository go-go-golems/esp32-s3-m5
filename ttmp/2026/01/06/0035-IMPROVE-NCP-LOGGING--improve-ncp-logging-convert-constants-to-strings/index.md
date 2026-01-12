---
Title: 'Improve NCP Logging: Convert Constants to Strings'
Ticket: 0035-IMPROVE-NCP-LOGGING
Status: active
Topics:
    - logging
    - debugging
    - ncp
    - codegen
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/include/esp_zb_ncp.h
      Note: Status and state enum definitions
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_bus.c
      Note: UART bus implementation with logging
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Main NCP Zigbee implementation with command handlers and logging
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/priv/esp_ncp_zb.h
      Note: Command ID constant definitions
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/aps/esp_zigbee_aps.h
      Note: APS addressing modes enum definitions
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_common.h
      Note: ZCL status codes
    - Path: esp32-s3-m5/thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_common.h
      Note: ZDO signal types and status codes enum definitions
    - Path: esp32-s3-m5/ttmp/2026/01/06/0035-IMPROVE-NCP-LOGGING--improve-ncp-logging-convert-constants-to-strings/analysis/01-ncp-logging-constant-conversion-analysis.md
      Note: Analysis document with approach evaluation and recommendations
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-06T09:28:35.551092073-05:00
WhatFor: ""
WhenToUse: ""
---




# Improve NCP Logging: Convert Constants to Strings

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- logging
- debugging
- ncp
- codegen

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
