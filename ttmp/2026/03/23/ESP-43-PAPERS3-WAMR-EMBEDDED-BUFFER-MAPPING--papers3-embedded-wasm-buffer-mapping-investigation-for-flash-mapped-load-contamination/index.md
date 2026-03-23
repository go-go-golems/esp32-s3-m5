---
Title: PaperS3 embedded Wasm buffer mapping investigation for flash-mapped load contamination
Ticket: ESP-43-PAPERS3-WAMR-EMBEDDED-BUFFER-MAPPING
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
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T12:27:00.822180602-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 embedded Wasm buffer mapping investigation for flash-mapped load contamination

## Overview

This ticket continues the `ESP-42` investigation from the point where the failure boundary became specific enough to justify a new phase.

The current live result is:

- `wasm load-only return-42` from the embedded module bytes still poisons a later persistent PSRAM write on PaperS3
- `wasm load-only-copy-internal return-42` does not
- `wasm load-only-copy-spiram return-42` also does not

That means the next phase is no longer generic WAMR allocator or instantiate debugging. The new focus is the embedded flash-mapped Wasm source path itself: how the module bytes are linked, mapped, and consumed by `wasm_runtime_load(...)`, and why that path interacts badly with later PSRAM writes on PaperS3.

## Key Links

- [design/01-embedded-buffer-mapping-investigation-plan.md](./design/01-embedded-buffer-mapping-investigation-plan.md)
- [reference/01-diary.md](./reference/01-diary.md)
- [tasks.md](./tasks.md)
- [changelog.md](./changelog.md)

## Status

Current status: **active**

Current boundary:

- embedded flash-mapped Wasm source buffer is suspicious
- copied RAM-backed source buffers are currently healthy

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
