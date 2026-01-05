---
Title: 'ESP-IDF esp_event demo: Cardputer keyboard + parallel tasks'
Ticket: 0028-ESP-EVENT-DEMO
Status: complete
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - keyboard
    - display
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0028-cardputer-esp-event-demo/README.md
      Note: Entry point for building/running the implemented demo firmware
    - Path: 0028-cardputer-esp-event-demo/main/app_main.cpp
      Note: Main demo implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T00:07:33.541750647-05:00
WhatFor: Ticket hub for designing a Cardputer esp_event demo (keyboard + parallel producers + on-screen event console).
WhenToUse: Use when implementing or reviewing the 0028-ESP-EVENT-DEMO firmware/demo and its supporting docs.
---



# ESP-IDF esp_event demo: Cardputer keyboard + parallel tasks

## Overview

Design/analysis ticket for a demo firmware showcasing ESP-IDF `esp_event`:
- Cardputer keyboard keypresses are posted as events
- a few background tasks post random/periodic events in parallel
- a UI task receives events and renders them on screen (event console)

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- Design doc: `design-doc/01-esp-event-demo-architecture-event-taxonomy.md`
- Diary: `reference/01-diary.md`
- Playbook: `playbook/01-build-flash-demo-walkthrough.md`
- Firmware: `0028-cardputer-esp-event-demo/` (repo root)

## Status

Current status: **active**

## Topics

- esp-idf
- esp32s3
- cardputer
- keyboard
- display
- ui

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
