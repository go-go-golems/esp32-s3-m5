---
Title: Port MicroQuickJS REPL to Cardputer
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0015-cardputer-serial-terminal/main/hello_world_main.cpp
      Note: Cardputer USB Serial JTAG reference
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: Current firmware entrypoint (wires console + REPL + evaluators)
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T13:24:50.215173612-05:00
WhatFor: ""
WhenToUse: ""
---


# Port MicroQuickJS REPL to Cardputer

## Overview

Port the MicroQuickJS REPL firmware from a generic ESP32-S3 configuration to the M5Stack Cardputer device. The firmware currently uses UART for console communication and is configured for 2MB flash. Cardputer requires USB Serial JTAG console and uses 8MB flash with different partition layout.

**Key Changes Required:**
- Console interface: UART → USB Serial JTAG
- Flash size: 2MB → 8MB
- Partition table: 1MB app + 960KB storage → 4MB app + 1MB storage
- Memory constraints: Verify fit within Cardputer's 512KB SRAM

**Analysis Documents:**
- [Current Firmware Configuration Analysis](./analysis/01-current-firmware-configuration-analysis.md) - Baseline firmware structure and configuration
- [Cardputer Port Requirements](./analysis/02-cardputer-port-requirements.md) - Required changes and differences

**MicroQuickJS Extension & Architecture Docs:**
- [MicroQuickJS native extensions on ESP32 (playbook + reference manual)](./reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md) - How to expose ESP-IDF functionality to JS (repo-accurate)
- [MicroQuickJS + FreeRTOS: multi-VM / multi-task architecture + communication brainstorm](./design-doc/01-microquickjs-freertos-multi-vm-multi-task-architecture-communication-brainstorm.md) - Design options for running multiple scripts/VMs and letting them communicate

**Process Documentation:**
- [Diary](./reference/01-diary.md) - Step-by-step implementation process

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32s3
- esp-idf
- cardputer
- javascript
- microquickjs
- qemu

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
