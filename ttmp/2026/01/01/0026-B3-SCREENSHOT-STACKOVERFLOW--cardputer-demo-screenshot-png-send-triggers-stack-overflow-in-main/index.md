---
Title: 'Cardputer demo: screenshot PNG send triggers stack overflow in main'
Ticket: 0026-B3-SCREENSHOT-STACKOVERFLOW
Status: active
Topics:
    - cardputer
    - esp-idf
    - esp32s3
    - m5gfx
    - screenshot
    - png
    - stack-overflow
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp
      Note: LGFXBase::createPng implementation uses miniz tdefl encoder
    - Path: 0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Keybinding for screenshot trigger (P) and main task context
    - Path: 0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: PNG screenshot creation + USB-Serial/JTAG send path
    - Path: 0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py
      Note: Host-side framing parser for screenshot capture
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T23:05:21.544394626-05:00
WhatFor: ""
WhenToUse: ""
---


# Cardputer demo: screenshot PNG send triggers stack overflow in main

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- cardputer
- esp-idf
- esp32s3
- m5gfx
- screenshot
- png
- stack-overflow
- debugging

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
