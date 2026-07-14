---
Title: PaperS3 E-Reader Native Primitives and Future JavaScript API
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - ereader
    - esp-idf
    - esp32s3
    - m5gfx
    - microquickjs
    - architecture
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/design-doc/01-papers3-e-reader-primitives-analysis-design-and-implementation-guide.md
      Note: Primary architecture and phased implementation guide
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/reference/01-investigation-diary.md
      Note: Chronological evidence decisions failures and delivery record
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/00-research-log.md
      Note: Entry point for reproducible research scripts and snapshots
ExternalSources: []
Summary: Native-first PaperS3 e-reader plan that qualifies the EPD stack, builds reader primitives in phases, ships a native vertical slice, and only then exposes the proven substrate through MicroQuickJS.
LastUpdated: 2026-07-14T16:30:00-04:00
WhatFor: Track the design, evidence, phases, and eventual implementation of reusable PaperS3 reader primitives and the future s3paper JavaScript layer.
WhenToUse: Start here before implementing, reviewing, or resuming ESP-50 work.
---


# PaperS3 E-Reader Native Primitives and Future JavaScript API

## Overview

This ticket defines a native-first route to the fluent `s3paper` JavaScript API. It starts by qualifying the PaperS3 EPD/toolchain stack, then implements safe display, refresh, input, text, storage, pagination, persistence, layout, and power primitives. A useful native reader is the mandatory milestone before a bounded MicroQuickJS feasibility spike and JavaScript bindings.

The plan is based on the imported API/studio prototype, local PaperS3 firmwares `0075`–`0082`, the M5PaperS3 factory demo, prior display crash investigations, and current M5GFX/MicroQuickJS sources.

## Key Links

- [Primary analysis, design, and implementation guide](./design-doc/01-papers3-e-reader-primitives-analysis-design-and-implementation-guide.md)
- [Investigation diary](./reference/01-investigation-diary.md)
- [Implementation phases](./tasks.md)
- [Imported and web source inventory](./sources/README.md)
- [Reproducible research trace](./scripts/00-research-log.md)
- [Current research snapshots](./scripts/output/)

## Status

Current status: **active — research/design package complete; implementation phases not started**

## Topics

- papers3
- eink
- ereader
- esp-idf
- esp32s3
- m5gfx
- microquickjs
- architecture

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- `design-doc/` — primary architecture and phased intern guide
- `reference/` — chronological investigation diary
- `sources/local/` — imported user-authored API and studio prototype
- `sources/web/` — Defuddle captures of primary/external research
- `scripts/` — reproducible inventory, fetch, upstream-query, and anchor scripts
- `scripts/output/` — 2026-07-14 evidence snapshots

## Central decision

Build and validate a complete native reader before integrating MicroQuickJS. JavaScript will later describe trees, callbacks, and policy through a versioned primitive ABI; it will not own M5GFX transactions directly.
