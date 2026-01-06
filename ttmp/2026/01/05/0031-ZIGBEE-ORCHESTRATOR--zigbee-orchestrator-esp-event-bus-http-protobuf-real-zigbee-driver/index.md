---
Title: 'Zigbee orchestrator: esp_event bus + HTTP/protobuf + real Zigbee driver'
Ticket: 0031-ZIGBEE-ORCHESTRATOR
Status: active
Topics:
    - zigbee
    - esp-idf
    - esp-event
    - http
    - websocket
    - protobuf
    - nanopb
    - architecture
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: thirdparty/esp-zigbee-sdk/README.md
      Note: Vendored H2 NCP firmware subset + build instructions
    - Path: thirdparty/esp-zigbee-sdk/components/esp-zigbee-ncp/src/esp_ncp_zb.c
      Note: Vendored NCP request handlers + signal logs we patch
    - Path: thirdparty/esp-zigbee-sdk/examples/esp_zigbee_ncp/README.md
      Note: Upstream example readme for the NCP project
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/10-tmux-dual-monitor.sh
      Note: tmux helper to monitor host+H2 concurrently (uses --no-reset)
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/11-capture-serial.py
      Note: Capture raw serial logs to file (no TTY required; avoids monitor side effects)
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/22-host-flash.sh
      Note: Flash helper for Cardputer host firmware.
    - Path: ttmp/2026/01/05/0031-ZIGBEE-ORCHESTRATOR--zigbee-orchestrator-esp-event-bus-http-protobuf-real-zigbee-driver/scripts/30-experiment0-security.sh
      Note: Automated security-first debug flow (TCLK/LKE/permit-join).
ExternalSources:
    - local:Zigbee debug logs.md
Summary: ""
LastUpdated: 2026-01-06T03:17:28.000000000-05:00
WhatFor: ""
WhenToUse: ""
---




# Zigbee orchestrator: esp_event bus + HTTP/protobuf + real Zigbee driver

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- zigbee
- esp-idf
- esp-event
- http
- websocket
- protobuf
- nanopb
- architecture

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
