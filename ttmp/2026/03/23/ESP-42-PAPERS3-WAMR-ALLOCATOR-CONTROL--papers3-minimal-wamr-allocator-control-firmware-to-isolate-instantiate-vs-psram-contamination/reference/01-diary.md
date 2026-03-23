---
Title: Diary
Ticket: ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL
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
LastUpdated: 2026-03-23T10:56:06.846153759-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Create and validate a smaller PaperS3 control firmware that preserves the WAMR instantiate and PSRAM-touch reproducer while removing almost all of the demo/display surface from `0079`.

## Context

`0079` already proved that:

- runtime initialization alone is not enough to reproduce the PSRAM fault
- `wasm_runtime_instantiate(...)` is enough to poison later PSRAM writes on PaperS3
- the same general WAMR baseline works on AtomS3R

The open question is whether the remaining fault boundary survives once the PaperS3 application is reduced to a near-minimum harness.

## Quick Reference

- New firmware: `0082-papers3-wamr-allocator-control`
- Ticket: `ESP-42-PAPERS3-WAMR-ALLOCATOR-CONTROL`
- First strict probe matrix:
  - `wasm status`
  - `wasm replay psram-persistent-init`
  - `wasm replay psram-persistent-touch-sync`
  - reboot
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch-sync`

## Usage Examples

### 2026-03-23 11:08 EDT

Started the new slice by creating `ESP-42` and copying `0079` to `0082`. The copied project is intentionally too large at this point; it still contains display files, multiple Wasm demos, and the old demo-oriented command surface. The first job in this ticket is to cut that down before building anything, so the ticket begins with a concrete reduced-scope plan and explicit probe matrix rather than an ad hoc code edit.

### 2026-03-23 11:12 EDT

Reviewed the copied `0082` tree. Confirmed that the current build still drags in `PaperCanvas`, `M5Unified`, display feature flags, and the full embedded module registry. That is exactly the wrong shape for a control harness, so the implementation plan now states that `0082` should be aggressively minimized rather than hidden behind more feature flags.

## Related

- `../design/01-minimal-papers3-allocator-control-implementation-plan.md`
- `../../../../2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/index.md`
