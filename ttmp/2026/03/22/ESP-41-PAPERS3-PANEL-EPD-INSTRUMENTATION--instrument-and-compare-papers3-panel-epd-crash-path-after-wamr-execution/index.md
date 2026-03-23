---
Title: Instrument and compare PaperS3 Panel_EPD crash path after WAMR execution
Ticket: ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - display
    - m5gfx
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Driver-focused ticket for instrumenting the PaperS3 M5GFX EPD backend and comparing it to newer upstream changes after the WAMR-side search space was narrowed.
LastUpdated: 2026-03-22T22:42:45.948604035-04:00
WhatFor: Keep the remaining PaperS3-specific display-path debugging separate from the broader WAMR migration history.
WhenToUse: Read this when continuing the PaperS3 EPD investigation or reviewing why Panel_EPD became the primary suspect.
---

# Instrument and compare PaperS3 Panel_EPD crash path after WAMR execution

## Overview

This ticket isolates the remaining PaperS3-specific failure after the WAMR migration and headless-control work. The current best model is that generic WAMR execution is no longer the main problem; the surviving issue is in the PaperS3 EPD display path, especially `M5GFX` `Panel_EPD`.

The immediate plan is to instrument the direct framebuffer write path and the display queue boundary in `Panel_EPD.cpp`, then compare the local driver against newer upstream PaperS3 changes.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- papers3
- wasm
- firmware
- esp-idf
- debugging
- display
- m5gfx

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
