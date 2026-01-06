---
Title: AtomS3R Web Server - Graphics Upload and WebSocket Terminal
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - webserver
    - websocket
    - graphics
    - atoms3r
    - preact
    - zustand
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../ATOMS3R-CAM-M12-UserDemo/main/service/service_web_server.cpp
      Note: ESPAsyncWebServer implementation example
    - Path: ../../../../../../ATOMS3R-CAM-M12-UserDemo/main/utils/assets/assets.h
      Note: Asset pool interface for memory-mapped flash assets
    - Path: ../../../../../../M5AtomS3-UserDemo/platformio.ini
      Note: Build configuration for Arduino/PlatformIO framework
    - Path: ../../../../../../M5AtomS3-UserDemo/src/main.cpp
      Note: Main application demonstrating display rendering
    - Path: 0013-atoms3r-gif-console/main/console_repl.cpp
      Note: Console REPL implementation using esp_console
    - Path: 0013-atoms3r-gif-console/main/control_plane.cpp
      Note: Button ISR and FreeRTOS queue pattern for event handling
    - Path: 0017-atoms3r-web-ui/main/Kconfig.projbuild
      Note: Menuconfig options for pins + storage + WiFi defaults for the web UI project
    - Path: 0017-atoms3r-web-ui/main/hello_world_main.cpp
      Note: New ESP-IDF-first tutorial project scaffold (display/backlight smoke test baseline)
    - Path: docs/playbook-embedded-preact-zustand-webui.md
      Note: JS-focused playbook for bundling and embedding a Vite (Preact+Zustand) frontend into firmware
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-26T08:53:37.045351299-05:00
WhatFor: ""
WhenToUse: ""
---



# AtomS3R Web Server - Graphics Upload and WebSocket Terminal

## Overview

Build an AtomS3R device-hosted web UI that supports **graphics upload → display render**, plus a **WebSocket terminal** (UART RX/TX) and **button events** streamed to the browser. This ticket is documentation-first: it collects repo-verified patterns and defines an implementable architecture in `esp32-s3-m5` tutorial style (ESP-IDF-first).

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Analysis**: `analysis/01-codebase-analysis-atoms3r-web-server-architecture.md`
- **Final design**: `design-doc/02-final-design-atoms3r-web-ui-graphics-upload-ws-terminal.md`
- **Draft design (superseded)**: `design-doc/01-design-document-web-server-with-graphics-upload-and-websocket-terminal.md`
- **Diary**: `reference/01-diary.md`

## Status

Current status: **active**

## Topics

- webserver
- websocket
- graphics
- atoms3r
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
