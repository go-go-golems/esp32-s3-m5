---
Title: Matrix JavaScript Runtime API (mquickjs)
Ticket: ESP-02-JS-MATRIX-API
Status: active
Topics:
    - esp32
    - esp-idf
    - mquickjs
    - javascript
    - led-matrix
    - rest
    - rtos
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0067-esp-c3-led-matrix-http/main/http_server.c
      Note: REST endpoints for JS eval/reset/reset-hard/status/mem
    - Path: 0067-esp-c3-led-matrix-http/main/js_console.c
      Note: esp_console js parser and examples
    - Path: 0067-esp-c3-led-matrix-http/main/matrix_engine.c
      Note: Script framebuffer APIs and script mode support
    - Path: 0067-esp-c3-led-matrix-http/main/mqjs/esp32_stdlib_runtime.c
      Note: Native bindings for matrix/timing commands
    - Path: 0067-esp-c3-led-matrix-http/main/mqjs/js_runtime_bridge.cpp
      Note: JS runtime bridge with soft/hard reset and eval status
    - Path: 0067-esp-c3-led-matrix-http/main/mqjs/mqjs_timers.cpp
      Note: JS timer scheduler for setTimeout/clearTimeout
    - Path: components/mqjs_service/mqjs_service.cpp
      Note: Arena allocation fallback and diagnostics for ESP32-C3
    - Path: ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_console_js_smoke.py
      Note: Tracked console command smoke test
    - Path: ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_js_smoke.sh
      Note: Tracked JS HTTP smoke test
    - Path: ttmp/2026/02/21/ESP-02-JS-MATRIX-API--matrix-javascript-runtime-api-mquickjs/scripts/0067_http_matrix_smoke.sh
      Note: Tracked matrix HTTP regression smoke test
ExternalSources: []
Summary: ""
LastUpdated: 2026-02-21T16:47:11.98727583-05:00
WhatFor: ""
WhenToUse: ""
---


# Matrix JavaScript Runtime API (mquickjs)

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32
- esp-idf
- mquickjs
- javascript
- led-matrix
- rest
- rtos

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
