---
Title: Tab5 WiFi/HTTP Benchmark Firmware - ESP32-P4 + ESP-Hosted Stack Throughput Analysis
Ticket: "0094"
Status: complete
Topics:
    - esp32
    - wifi
    - benchmark
    - esp-hosted
    - http
    - performance
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0093-tab5-ui-screen-viewer/main/CMakeLists.txt
      Note: Fork base CMake - component dependencies
    - Path: 0093-tab5-ui-screen-viewer/main/http_server.c
      Note: Fork base - HTTP server with upload handler and timing patterns
    - Path: 0093-tab5-ui-screen-viewer/main/wifi_app.c
      Note: Fork base - WiFi APSTA with NVS persistence
    - Path: 0093-tab5-ui-screen-viewer/sdkconfig.defaults
      Note: ESP-Hosted SDIO config
    - Path: 0094-tab5-wifi-bench/main/bench_server.c
      Note: Benchmark HTTP server with 7 endpoints and per-segment timing
ExternalSources: []
Summary: ""
LastUpdated: 2026-05-27T17:21:52.421873398-04:00
WhatFor: ""
WhenToUse: ""
---







# Tab5 WiFi/HTTP Benchmark Firmware - ESP32-P4 + ESP-Hosted Stack Throughput Analysis

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32
- wifi
- benchmark
- esp-hosted
- http
- performance

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
