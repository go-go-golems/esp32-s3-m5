---
Title: Diary
Ticket: ESP-43-PAPERS3-WAMR-EMBEDDED-BUFFER-MAPPING
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T13:46:41.047563537-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Start the next investigation phase from a sharper boundary than `ESP-42` had at the beginning: the remaining PaperS3 fault appears to depend on loading directly from the embedded module bytes, not on generic WAMR load behavior from arbitrary RAM buffers.

## Context

The immediately previous ticket proved:

- embedded `load-only` still poisons later PSRAM writes
- copied internal-RAM `load-only` does not
- copied SPIRAM `load-only` also does not

That is strong enough to justify a new ticket. The open question is now about mapping and buffer provenance, not broad allocator behavior.

## Quick Reference

- New ticket: `ESP-43-PAPERS3-WAMR-EMBEDDED-BUFFER-MAPPING`
- Primary project: `0082-papers3-wamr-allocator-control`
- Primary code targets:
  - `main/wasm_module_registry.cpp`
  - `main/wasm_module_runner.cpp`
  - `main/wasm_command.cpp`

## Usage Examples

### 2026-03-23 12:30 EDT

Created `ESP-43` to split the investigation cleanly. This was a deliberate scoping move rather than a paperwork exercise. `ESP-42` had become too broad: allocator mode, pool backing, instantiate, load, and loader instrumentation were all in scope there. Now that the boundary is narrower, the work should be grouped around the embedded-buffer path itself.

### 2026-03-23 12:31 EDT

Seeded the new ticket with:

- a design note for the embedded-buffer mapping phase
- a fresh task list
- this diary

The immediate aim is to avoid losing the exact transition in thinking. We are no longer asking "why does WAMR poison PSRAM on PaperS3?" in the abstract. We are asking "what is special about the embedded flash-mapped Wasm source path on PaperS3?"

## Related

- [design/01-embedded-buffer-mapping-investigation-plan.md](../design/01-embedded-buffer-mapping-investigation-plan.md)
- [tasks.md](../tasks.md)
