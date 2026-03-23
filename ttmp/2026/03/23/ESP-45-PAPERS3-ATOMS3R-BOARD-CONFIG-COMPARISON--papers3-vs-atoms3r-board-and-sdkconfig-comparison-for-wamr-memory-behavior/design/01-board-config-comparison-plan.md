---
Title: PaperS3 vs AtomS3R board/config comparison plan
Ticket: ESP-45-PAPERS3-ATOMS3R-BOARD-CONFIG-COMPARISON
Status: active
Topics:
    - papers3
    - atoms3r
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults
      Note: PaperS3-side defaults and memory-related settings
    - Path: 0081-atoms3r-wamr-probe-console/sdkconfig.defaults
      Note: AtomS3R control-board defaults
    - Path: 0082-papers3-wamr-allocator-control/sdkconfig.defaults
      Note: Minimal PaperS3 harness settings
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-23T15:05:00-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 vs AtomS3R board/config comparison plan

## Goal

Document which differences between PaperS3 and AtomS3R are actually relevant to the surviving flash-mapped load issue.

## Scope

- sdkconfig and generated config comparison
- flash/PSRAM mode, speed, topology, and memory-region differences
- official board docs, schematics, and datasheets where available
- board-level interpretation, not just raw diff dumps

## Expected value

This ticket should answer whether the remaining fault is best explained by:

- PaperS3-specific flash/PSRAM topology
- a config difference that affects external-memory behavior
- or a software path that only happens to be easier to trigger on PaperS3

