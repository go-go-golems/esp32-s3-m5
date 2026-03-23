---
Title: PaperS3 e-reader crashes on EPD fillScreen - null framebuffer pointer
Ticket: ESP-37-EREADER-EPD-CRASH
Status: active
Topics:
    - papers3
    - esp-idf
    - esp32s3
    - e-paper
    - m5gfx
    - bugfix
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: EPD driver - _buf allocation at line 249
    - Path: 0078-papers3-gnosis-layout/main/app_main.cpp
      Note: Reference - gnosis 0078 works with Run() on core 1 (same pattern)
    - Path: 0080-papers3-ereader/main/app_main.cpp
      Note: Entry point - Init on core 0 then RunLoop on core 1
    - Path: 0080-papers3-ereader/main/ereader_app.cpp
      Note: FullRefresh calls fillScreen which crashes at line 267
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T16:31:07.278505953-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 e-reader crashes on EPD fillScreen - null framebuffer pointer

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- papers3
- esp-idf
- esp32s3
- e-paper
- m5gfx
- bugfix

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
