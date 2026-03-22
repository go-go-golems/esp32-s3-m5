---
Title: Use AtomS3R to cross-check WAMR runtime and display-path behavior
Ticket: ESP-40-PAPERS3-ATOMS3R-WAMR-CROSSCHECK
Status: active
Topics:
    - papers3
    - atoms3r
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "AtomS3R control-board experiment for the WAMR investigation. The new 0081 probe project builds and runs on real hardware. After applying the same Espressif WAMR platform fixes already needed in 0079, AtomS3R succeeds on return-42, log-only, replay hello-frame, and full wasm hello-frame. Current conclusion: the remaining live failure is not explained by generic ESP32-S3 WAMR bring-up alone."
LastUpdated: 2026-03-22T16:19:09.23142559-04:00
WhatFor: ""
WhenToUse: ""
---

# Use AtomS3R to cross-check WAMR runtime and display-path behavior

## Overview

This ticket uses AtomS3R as a control board for the PaperS3 WAMR debugging effort. The purpose is to separate "WAMR on ESP32-S3 is fundamentally unstable" from "the remaining bug is tied to PaperS3's display path and driver stack."

The work produced a new project, `0081-atoms3r-wamr-probe-console`, built from known-good AtomS3R display bring-up plus the current minimal WAMR console/runtime surface. The decisive result is that AtomS3R reproduces the stock Espressif `os_mmap()` instantiate failure until the same two local WAMR platform fixes are applied, and then all baseline probes succeed. That sharply narrows the live problem on PaperS3.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- papers3
- atoms3r
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
