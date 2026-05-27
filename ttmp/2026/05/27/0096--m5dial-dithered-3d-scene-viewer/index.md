---
Title: M5Dial Dithered 3D Scene Viewer
Ticket: "0096"
Status: active
Topics:
    - esp32
    - 3d-rendering
    - dithering
    - m5dial
    - rotary-encoder
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0074-m5dial-web-remote/firmware/main/input_events.h
      Note: Input event types (encoder delta
    - Path: 0074-m5dial-web-remote/firmware/main/m5dial_board.cpp
      Note: Board driver implementation with GC9A01 SPI config and encoder/touch
    - Path: 0074-m5dial-web-remote/firmware/main/m5dial_board.h
      Note: Board driver to reuse for 0096
    - Path: 0074-m5dial-web-remote/firmware/sdkconfig.defaults
      Note: M5Dial sdkconfig template (USB JTAG console
    - Path: 0096-m5dial-dithered-3d/CMakeLists.txt
      Note: Project root CMake; build fix was removing COMPONENT_DIRS so IDF built-ins such as nvs_flash remain discoverable
    - Path: 0096-m5dial-dithered-3d/main/CMakeLists.txt
      Note: Main component source/dependency list
    - Path: 0096-m5dial-dithered-3d/main/app_main.cpp
      Note: Application entry point and USB Serial/JTAG esp_console REPL startup
    - Path: 0096-m5dial-dithered-3d/main/console_commands.cpp
      Note: esp_console command implementations fixed for IDF 5.4.2 argtable/format APIs
    - Path: ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/m5dial.jsx
      Note: |-
        Original React/Three.js simulator that inspired this firmware port
        Original React/Three.js dithered 3D simulator — source of all scenes
    - Path: ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/sources/stackoverflow-resolve-component-esp-idf.md
      Note: Saved remote reference about ESP-IDF custom component resolution using defuddle
ExternalSources: []
Summary: ""
LastUpdated: 2026-05-27T18:41:03.654514618-04:00
WhatFor: ""
WhenToUse: ""
---








# M5Dial Dithered 3D Scene Viewer

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32
- 3d-rendering
- dithering
- m5dial
- rotary-encoder

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
