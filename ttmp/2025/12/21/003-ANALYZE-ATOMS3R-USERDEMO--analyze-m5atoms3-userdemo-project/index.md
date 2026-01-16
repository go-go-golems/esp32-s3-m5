---
Title: Analyze M5AtomS3 UserDemo Project
Ticket: 003-ANALYZE-ATOMS3R-USERDEMO
Status: complete
Topics:
    - embedded-systems
    - esp32
    - m5stack
    - arduino
    - analysis
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5AtomS3-UserDemo/lib/infrared_tools/include/ir_tools.h
      Note: IR tools library API header
    - Path: M5AtomS3-UserDemo/lib/led_strip/include/led_strip.h
      Note: LED strip library API header
    - Path: M5AtomS3-UserDemo/platformio.ini
      Note: Build configuration and dependency management
    - Path: M5AtomS3-UserDemo/src/img_res.c
      Note: Embedded PNG image resources for function icons
    - Path: M5AtomS3-UserDemo/src/main.cpp
      Note: Main application source file containing all function implementations and main loop
    - Path: echo-base--openai-realtime-embedded-sdk/src/CMakeLists.txt
      Note: echo-base build graph; confirms no M5GFX/M5Unified dependency
    - Path: echo-base--openai-realtime-embedded-sdk/src/Kconfig.projbuild
      Note: echo-base board switch includes M5_ATOMS3R (audio-focused)
    - Path: echo-base--openai-realtime-embedded-sdk/src/idf_component.yml
      Note: echo-base managed components; only es8311
    - Path: echo-base--openai-realtime-embedded-sdk/src/media.cpp
      Note: echo-base ATOMS3R-specific audio (ES8311/I2S/I2C) config
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-15T22:18:34.990391122-05:00
WhatFor: ""
WhenToUse: ""
---




# Analyze M5AtomS3 UserDemo Project

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- embedded-systems
- esp32
- m5stack
- arduino
- analysis

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
