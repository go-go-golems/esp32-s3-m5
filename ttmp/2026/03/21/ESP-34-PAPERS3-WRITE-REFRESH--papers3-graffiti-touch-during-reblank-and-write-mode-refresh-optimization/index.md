---
Title: PaperS3 graffiti touch-during-reblank and write-mode refresh optimization
Ticket: ESP-34-PAPERS3-WRITE-REFRESH
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5gfx
    - m5stack
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.cpp
      Note: Current touch loop
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.h
      Note: Current write-mode state machine and pending segment queues
    - Path: 0077-papers3-alphabet-graffiti/main/protractor_math.cpp
      Note: Current recognition pipeline entrypoints used after stroke completion
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T22:21:46.128546131-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 graffiti touch-during-reblank and write-mode refresh optimization

## Overview

This ticket tracks a focused follow-up on `0077-papers3-alphabet-graffiti`. The immediate concern is that after the recent UI redo, the app appears unresponsive during a screen reblank or full e-paper refresh, which makes touch handling feel blocked. A second concern is efficiency: normal handwriting input in `WRITE` mode should not require a whole-screen redraw when only the canvas and a small text region changed.

The goal of the ticket is to verify whether touch polling is truly blocked or whether the problem is mainly visual latency caused by the EPD busy window, then redesign write-mode rendering around smaller dirty regions instead of a full-screen post-stroke redraw.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

Current investigation status:

- ticket scaffold created
- current `0077` touch/render path identified
- no firmware changes made yet in this ticket

## Topics

- esp32-s3
- esp32s3
- firmware
- m5gfx
- m5stack
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
