---
Title: PaperS3 Interactive E-Reader using Gnosis Layout Engine
Ticket: ESP-36-PAPERS3-EREADER
Status: active
Topics:
    - papers3
    - display
    - esp-idf
    - esp32s3
    - e-paper
    - layout-engine
    - e-reader
    - touch
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/main/glyph_store.cpp
      Note: Reference SPIFFS mount and file I/O pattern
    - Path: 0078-papers3-gnosis-layout/main/gnosis_types.h
      Note: Node struct
    - Path: 0078-papers3-gnosis-layout/main/screens.cpp
      Note: BuildReader preset - starting point for reader UI
    - Path: 0078-papers3-gnosis-layout/main/widget_renderer.cpp
      Note: DrawTextBlock renderer to extend with ext_text
    - Path: 0080-papers3-ereader
      Note: Target firmware project directory
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-22T11:04:00.555027811-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 Interactive E-Reader using Gnosis Layout Engine

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- papers3
- display
- esp-idf
- esp32s3
- e-paper
- layout-engine
- e-reader
- touch

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
