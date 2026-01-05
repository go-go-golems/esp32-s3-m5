---
Title: 'Cardputer: keyboard + esp_console + esp_event + on-screen event log'
Ticket: 0030-CARDPUTER-CONSOLE-EVENTBUS
Status: complete
Topics:
    - cardputer
    - keyboard
    - console
    - esp-idf
    - esp32s3
    - display
    - esp-event
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T09:27:00.106956073-05:00
WhatFor: ""
WhenToUse: ""
---




# Cardputer: keyboard + esp_console + esp_event + on-screen event log

## Overview

Build a small, onboarding-friendly Cardputer firmware that demonstrates how we structure event-driven demos:

- Multiple producers (keyboard, console, background tasks) post typed events into an application `esp_event` loop.
- A single UI task dispatches the loop via `esp_event_loop_run()` and renders received events on screen (M5GFX).

See the onboarding analysis doc for the “where to start” walkthrough:
`ttmp/2026/01/05/0030-CARDPUTER-CONSOLE-EVENTBUS--cardputer-keyboard-esp-console-esp-event-on-screen-event-log/analysis/01-onboarding-how-the-demo-fits-together.md`

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- cardputer
- keyboard
- console
- esp-idf
- esp32s3
- display
- esp-event
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
