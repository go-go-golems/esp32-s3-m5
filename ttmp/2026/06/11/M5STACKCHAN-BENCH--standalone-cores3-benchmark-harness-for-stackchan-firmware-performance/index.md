---
Title: Standalone CoreS3 Benchmark Harness for StackChan Firmware Performance
Ticket: M5STACKCHAN-BENCH
Status: active
Topics:
    - m5stackchan
    - firmware
    - esp-idf
    - lvgl
    - performance
    - benchmark
    - esp32-s3
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: build/firmware/main/CMakeLists.txt
      Note: Build source collection and possible benchmark entry-point selection location
    - Path: build/firmware/main/apps/app_launcher/app_launcher.cpp
      Note: Production launcher hot path whose choppiness motivates benchmark
    - Path: build/firmware/main/hal/board/stackchan_display.cc
      Note: LVGL port task
    - Path: build/firmware/main/hal/hal.cpp
      Note: HAL initialization
    - Path: build/firmware/main/hal/hal.h
      Note: Public benchmark APIs including LVGL lock guard and RGB methods
    - Path: build/firmware/main/hal/hal_io_expander.cpp
      Note: Direct RGB LED timing path for benchmark
    - Path: build/firmware/main/main.cpp
      Note: Factory entry point and Mooncake loop to replace or bypass for standalone benchmark
    - Path: build/firmware/main/stackchan/addons/neon_light/neon_light.cpp
      Note: NeonLight animation update behavior and 50Hz rate limit
    - Path: build/firmware/main/stackchan/stackchan.h
      Note: Optional StackChan update path cost measurement
    - Path: build/firmware/partitions.csv
      Note: OTA/assets/coredump partition layout for benchmark flashing and assets timing
ExternalSources: []
Summary: ""
LastUpdated: 2026-06-11T19:55:08.875690297-04:00
WhatFor: ""
WhenToUse: ""
---


# Standalone CoreS3 Benchmark Harness for StackChan Firmware Performance

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- m5stackchan
- firmware
- esp-idf
- lvgl
- performance
- benchmark
- esp32-s3

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
