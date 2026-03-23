---
Title: Explicit flash-source A/B experiment plan
Ticket: ESP-46-PAPERS3-WAMR-FLASH-SOURCE-AB
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_module_registry.cpp
      Note: Current embedded-symbol source path
    - Path: 0082-papers3-wamr-allocator-control/main/wasm_module_runner.cpp
      Note: Current load path and copy-before-load mitigation
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T15:05:00-04:00
WhatFor: ""
WhenToUse: ""
---

# Explicit flash-source A/B experiment plan

## Goal

Run tighter source-buffer A/B tests than `EMBED_FILES` alone can provide.

## Planned comparisons

- embedded symbol pointer from `.flash.rodata`
- explicit flash/partition read into RAM
- explicit mmap-backed flash region if appropriate
- current copy-before-load mitigation path

## Purpose

If `ESP-44` does not fully close the root-cause question, this ticket becomes the lower-level experiment track.

