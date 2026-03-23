---
Title: PaperS3 WAMR flash-mapped load root-cause investigation
Ticket: ESP-44-PAPERS3-WAMR-FLASH-LOAD-ROOT-CAUSE
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources:
    - https://github.com/bytecodealliance/wasm-micro-runtime/releases
    - https://github.com/bytecodealliance/wasm-micro-runtime/pull/4591
Summary: ""
LastUpdated: 2026-03-23T22:05:00-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 WAMR flash-mapped load root-cause investigation

## Overview

This ticket explains the root cause of the embedded-Wasm load crash that originally looked like a broad PaperS3 PSRAM/display/WAMR problem. The current state is stronger than “workaround found”: the investigation now isolates WAMR's in-place const-string reuse on flash-mapped embedded Wasm buffers as the critical mechanism, with a long-form postmortem, saved proof patches, and an added public/upstream research appendix documenting what current WAMR sources and open pull requests do and do not say about the same bug family.

## Key Links

- [Postmortem Report](./design/02-wamr-flash-mapped-embedded-load-postmortem-report.md)
- [Investigation Plan](./design/01-flash-load-root-cause-plan.md)
- [Diary](./reference/01-diary.md)
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
