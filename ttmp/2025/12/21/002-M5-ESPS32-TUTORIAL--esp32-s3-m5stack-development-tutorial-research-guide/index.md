---
Title: ESP32-S3 M5Stack Development Tutorial Research Guide
Ticket: 002-M5-ESPS32-TUTORIAL
Status: active
Topics:
    - esp32
    - m5stack
    - freertos
    - esp-idf
    - tutorial
    - grove
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ATOMS3R-CAM-M12-UserDemo/main/service/service_uvc.cpp
      Note: FreeRTOS task pattern example
    - Path: ATOMS3R-CAM-M12-UserDemo/ttmp/2025/12/19/001-FIRMWARE-ARCHITECTURE-BOOK--firmware-architecture-book/design-doc/01-book-brainstorm-firmware-m12-platform.md
      Note: Source of inspiration for tutorial structure debate
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: M5Stack HAL abstraction example
    - Path: M5Cardputer-UserDemo/sdkconfig.old
      Note: Authoritative pre-IDF5 Cardputer config used to derive tutorial defaults
    - Path: echo-base--openai-realtime-embedded-sdk/src/webrtc.cpp
      Note: FreeRTOS task creation with PSRAM example
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/001-ANALYZE-ECHO-BASE--analyze-echo-base-openai-realtime-embedded-sdk/reference/01-research-diary.md
      Note: Example research methodology format
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/analysis/01-gc9107-on-esp-idf-atoms3r-driver-options-integration-approaches-and-findings.md
      Note: GC9107-on-IDF analysis + integration options
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/21/002-M5-ESPS32-TUTORIAL--esp32-s3-m5stack-development-tutorial-research-guide/reference/03-schematic-claude-answers.md
      Note: ATOMS3R display pin/backlight interpretation
    - Path: esp32-s3-m5/0001-cardputer-hello_world/partitions.csv
      Note: Cardputer-style partition layout used by tutorial 0001
    - Path: esp32-s3-m5/0001-cardputer-hello_world/sdkconfig.defaults
      Note: Tutorial defaults (flash/partition/CPU/stack) aligned to Cardputer baseline
    - Path: esp32-s3-m5/0002-freertos-queue-demo/README.md
      Note: FreeRTOS queue tutorial usage
    - Path: esp32-s3-m5/0002-freertos-queue-demo/main/hello_world_main.c
      Note: FreeRTOS queue tutorial source
    - Path: esp32-s3-m5/0003-gpio-isr-queue-demo/README.md
      Note: Tutorial 0003 usage
    - Path: esp32-s3-m5/0003-gpio-isr-queue-demo/main/hello_world_main.c
      Note: Tutorial 0003 source
    - Path: esp32-s3-m5/0004-i2c-rolling-average/README.md
      Note: Tutorial 0004 usage
    - Path: esp32-s3-m5/0004-i2c-rolling-average/main/hello_world_main.c
      Note: Tutorial 0004 source
    - Path: esp32-s3-m5/0005-wifi-event-loop/README.md
      Note: Tutorial 0005 usage
    - Path: esp32-s3-m5/0005-wifi-event-loop/main/hello_world_main.c
      Note: Tutorial 0005 source
    - Path: esp32-s3-m5/0005-wifi-event-loop/sdkconfig
      Note: Scrubbed WiFi credentials (now empty)
    - Path: esp32-s3-m5/0006-wifi-http-client/README.md
      Note: Tutorial 0006 usage
    - Path: esp32-s3-m5/0006-wifi-http-client/main/Kconfig.projbuild
      Note: Tutorial 0006 menuconfig options
    - Path: esp32-s3-m5/0006-wifi-http-client/main/hello_world_main.c
      Note: Tutorial 0006 source (WiFi + HTTP client)
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/README.md
      Note: Tutorial 0007 usage
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/Kconfig.projbuild
      Note: Tutorial 0007 menuconfig
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Tutorial 0007 source
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/README.md
      Note: Tutorial 0008 usage
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/main/hello_world_main.c
      Note: Tutorial 0008 source
    - Path: esp32-s3-m5/0008-atoms3r-display-animation/main/idf_component.yml
      Note: GC9107 managed component manifest
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-21T08:21:52.443554697-05:00
WhatFor: ""
WhenToUse: ""
---














# ESP32-S3 M5Stack Development Tutorial Research Guide

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esp32
- m5stack
- freertos
- esp-idf
- tutorial
- grove

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
