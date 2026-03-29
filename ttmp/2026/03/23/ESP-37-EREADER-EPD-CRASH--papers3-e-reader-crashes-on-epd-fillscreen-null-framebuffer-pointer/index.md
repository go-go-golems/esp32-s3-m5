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
Summary: "Resolved on hardware. The PaperS3 EPD init was failing because `_lut_2pixel` was constrained to DMA-capable RAM, and a second PaperS3 default-rotation bug caused boot-time `540x960` clears before app code set rotation."
LastUpdated: 2026-03-23T16:59:25-04:00
WhatFor: "Track the root cause, fixes, and validation for the PaperS3 e-reader EPD crash."
WhenToUse: "Use when reviewing ESP-37, understanding the PaperS3 EPD bring-up fixes, or validating related PaperS3 apps."
---


# PaperS3 e-reader crashes on EPD fillScreen - null framebuffer pointer

## Overview

The null-framebuffer crash is fixed on the attached PaperS3. Two issues were involved:

1. `Panel_EPD::init_intenal()` failed because `_lut_2pixel` requested 43,520 bytes from DMA-capable internal RAM even though it is not a DMA payload.
2. After that was fixed, PaperS3 still had an early boot-time orientation bug because `M5Unified` cleared the display before app code called `setRotation(1)`.

Both fixes were validated on hardware with the e-reader reaching `display_ready=yes`, the `ereader>` prompt, and a successful `opened [0] The Deliverator (page 1/3)` boot sequence.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active, fix validated on hardware**

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
