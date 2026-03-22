---
Title: PaperS3 WAMR AssemblyScript console demo with precompiled scripts
Ticket: ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT
Status: active
Topics:
    - papers3
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - console
    - usb-serial-jtag
    - storage
    - wasm
    - assemblyscript
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/main/app_main.cpp
      Note: Current PaperS3 app entrypoint pattern to preserve in the new project
    - Path: 0077-papers3-alphabet-graffiti/sdkconfig.defaults
      Note: ESP-IDF 5.3.4 and USB Serial/JTAG baseline for PaperS3 work
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Local esp_console REPL bootstrap example with USB Serial/JTAG
    - Path: 0067-esp-c3-led-matrix-http/main/app_main.c
      Note: Prior art for registering extra script-oriented console commands
    - Path: 0067-esp-c3-led-matrix-http/main/js_console.c
      Note: Prior art for a runtime-specific esp_console command surface
    - Path: /home/manuel/esp/esp-idf-5.3.4/components/console/esp_console.h
      Note: Official ESP-IDF 5.3.4 console API used by the proposed design
ExternalSources:
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/README.md
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md
    - https://github.com/AssemblyScript/website/blob/main/src/compiler.md
    - https://github.com/AssemblyScript/website/blob/main/src/runtime.md
    - https://docs.espressif.com/projects/esp-idf/en/stable/esp32c5/api-guides/usb-serial-jtag-console.html
Summary: Ticket for planning a new `0079` PaperS3 firmware that embeds precompiled AssemblyScript-generated WebAssembly modules and runs them through a USB Serial/JTAG `esp_console` workflow on ESP-IDF 5.3.4.
LastUpdated: 2026-03-22T10:22:19.578547781-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 WAMR AssemblyScript console demo with precompiled scripts

## Overview

This ticket plans the next PaperS3 firmware after the current `0076` and `0077` workstreams. The target is a new project, proposed as `0079-papers3-wamr-assemblyscript-console`, where a set of small AssemblyScript programs are compiled on the host into `.wasm` artifacts and then executed on the PaperS3 through WAMR.

The guiding product idea is intentionally simple:

- compile a curated set of demo programs ahead of time
- bundle them into the firmware image
- expose them through `esp_console`
- keep the console on USB Serial/JTAG, not UART
- use the PaperS3 display as the visible output surface for those guest programs

This ticket is documentation-first. It does not implement `0079` yet. Instead, it captures the architecture, file plan, ABI decisions, build pipeline, demo inventory, and implementation phases in enough detail that a new intern can start the firmware confidently on ESP-IDF `5.3.4`.

## Key Links

- Project target: planned `0079-papers3-wamr-assemblyscript-console`
- Primary guide: `design-doc/01-papers3-wamr-assemblyscript-analysis-design-and-implementation-guide.md`
- Diary: `reference/01-diary.md`
- Tasks: `tasks.md`
- Changelog: `changelog.md`
- Related files: see frontmatter `RelatedFiles`
- External sources: see frontmatter `ExternalSources`

## Status

Current status: **active**

## Topics

- papers3
- esp32-s3
- esp32s3
- firmware
- m5stack
- m5gfx
- console
- usb-serial-jtag
- storage
- wasm
- assemblyscript

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
