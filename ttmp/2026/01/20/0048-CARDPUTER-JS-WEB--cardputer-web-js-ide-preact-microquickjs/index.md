---
Title: 'Cardputer: Web JS IDE (Preact + microquickjs)'
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: Reference esp_http_server patterns for embedded assets + WS
    - Path: 0017-atoms3r-web-ui/web/vite.config.ts
      Note: Reference deterministic Vite bundling for firmware embedding
    - Path: 0048-cardputer-js-web/main/app_main.cpp
      Note: Firmware entrypoint wiring wifi + http server
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: esp_http_server routes for embedded UI and /api/js/eval
    - Path: 0048-cardputer-js-web/main/js_runner.cpp
      Note: MicroQuickJS init + eval formatting + timeout hook
    - Path: 0048-cardputer-js-web/main/wifi_console.c
      Note: |-
        esp_console Wi-Fi commands (wifi scan/join/status) and connection-status UX (0046 pattern)
        esp_console Wi-Fi commands and status UX (0046 pattern)
    - Path: 0048-cardputer-js-web/main/wifi_mgr.c
      Note: |-
        STA Wi-Fi manager with NVS-backed credentials and status snapshots (0046 pattern)
        STA Wi-Fi manager with NVS credentials (0046 pattern)
    - Path: 0048-cardputer-js-web/partitions.csv
      Note: Defines storage SPIFFS partition required by MQJS stdlib load/autoload helpers
    - Path: 0048-cardputer-js-web/web/vite.config.ts
      Note: Deterministic bundling into main/assets for firmware embedding
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h
      Note: Authoritative JS engine API primitives
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Reference MicroQuickJS eval semantics and formatting
ExternalSources: []
Summary: 'Ticket workspace for designing a Cardputer-hosted Web IDE: embedded Preact/Zustand editor UI, REST endpoint to execute JS via MicroQuickJS, and (Phase 2) WebSocket streaming of encoder telemetry.'
LastUpdated: 2026-01-20T22:54:37.222738591-05:00
WhatFor: Collect designs, playbooks, and prior art for implementing an ESP-IDF device-hosted Web IDE on Cardputer.
WhenToUse: Use when implementing the firmware/UI, reviewing architecture choices, or onboarding someone new to the repo’s device-hosted UI patterns.
---







# Cardputer: Web JS IDE (Preact + microquickjs)

## Overview

This ticket documents how to build a **device-hosted Web IDE** on Cardputer:

- The firmware serves an embedded **Preact + Zustand** web UI that includes a JavaScript code editor.
- The web UI POSTs JavaScript to the device over REST.
- The firmware executes that code via **MicroQuickJS** and returns the formatted result or error.

Phase 2 extends the UI with realtime telemetry:

- read encoder position + click on-device
- stream those values to the browser over WebSocket (`/ws`)

## Key Links

- Phase 1 design (REST eval + embedded UI):
  - `design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md`
- Phase 2 design (encoder telemetry over WS):
  - `design-doc/02-phase-2-design-encoder-position-click-over-websocket.md`
- Prior art map (docs + firmwares + symbol names):
  - `reference/02-prior-art-and-reading-list.md`
- Diary (frequent step-by-step record):
  - `reference/01-diary.md`
- Playbooks:
  - `playbook/01-playbook-phase-1-web-ide-smoke-test-build-connect-run-snippets.md`
  - `playbook/02-playbook-phase-2-websocket-encoder-telemetry-smoke-test.md`

## Status

Current status: **active**

## Topics

- cardputer
- esp-idf
- webserver
- http
- rest
- websocket
- preact
- zustand
- javascript
- quickjs
- microquickjs

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

## Next step (implementation)

Create an ESP-IDF project (suggested: `esp32-s3-m5/0048-cardputer-js-web/`) that follows the Phase 1 design doc, then validate it via the Phase 1 playbook. Once Phase 1 is stable, add the `/ws` channel and encoder driver per the Phase 2 design and validate via the Phase 2 playbook.
