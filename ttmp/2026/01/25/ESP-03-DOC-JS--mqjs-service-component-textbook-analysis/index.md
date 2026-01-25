---
Title: mqjs_service component textbook analysis
Ticket: ESP-03-DOC-JS
Status: active
Topics:
    - microquickjs
    - quickjs
    - javascript
    - esp-idf
    - esp32s3
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Ticket collecting a long-form textbook analysis of mqjs_service (MicroQuickJS concurrency wrapper) plus a research diary."
LastUpdated: 2026-01-25T08:52:44.960343152-05:00
WhatFor: "Centralize durable documentation of mqjs_service internals and usage patterns."
WhenToUse: "Use when integrating mqjs_service, debugging JS concurrency/timeouts, or reviewing its design tradeoffs."
---

# mqjs_service component textbook analysis

## Overview

This ticket contains a “textbook-style” deep dive into `components/mqjs_service`, including architecture, pseudocode, diagrams, and a critical assessment of risks/shortcomings. It also includes a detailed diary with intermediate research notes (not just a final summary).

## Key Links

- Textbook analysis: `./design-doc/01-textbook-mqjs-service-deep-dive.md`
- Diary: `./reference/01-diary.md`
- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- microquickjs
- quickjs
- javascript
- esp-idf
- esp32s3

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
