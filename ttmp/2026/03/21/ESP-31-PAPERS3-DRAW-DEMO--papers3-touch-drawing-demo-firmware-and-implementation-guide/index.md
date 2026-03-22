---
Title: PaperS3 touch drawing demo firmware and implementation guide
Ticket: ESP-31-PAPERS3-DRAW-DEMO
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T20:02:36.419661871-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 touch drawing demo firmware and implementation guide

## Overview

This ticket captures the implementation of a new standalone `PaperS3` firmware example at `0075-papers3-touch-draw-demo/` plus the documentation needed to hand the work to a new intern without requiring them to reverse-engineer the larger vendor demo. The firmware uses the donor `M5PaperS3-UserDemo` project as the starting point for panel bring-up and GT911 touch access, but strips the application down to a single purpose: draw the user’s finger path inside a canvas and provide a touch `CLEAR` button that resets the screen.

The concrete result is:

- a new ESP-IDF 5.3.4 project wired to the donor’s vendored `M5Unified` and `M5GFX` components
- a compiled `idf.py build` result validated with `/home/manuel/esp/esp-idf-5.3.4/export.sh`
- a detailed implementation plan
- a detailed analysis/design/implementation guide for intern onboarding
- an investigation diary with commands, failure notes, and review instructions

## Key Links

- Design doc: `design-doc/02-papers3-touch-draw-demo-analysis-design-and-implementation-guide.md`
- Detailed plan: `design-doc/01-papers3-touch-draw-demo-detailed-implementation-plan.md`
- Diary: `reference/01-investigation-diary.md`
- Firmware project: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0075-papers3-touch-draw-demo`
- Donor project: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo`

## Status

Current status: **active**

Implementation status:

- firmware project created
- ESP-IDF 5.3.4 build completed successfully
- docs authored
- hardware flash/touch smoke test not yet performed in this session

## Topics

- esp32-s3
- esp32s3
- firmware
- m5stack
- m5gfx
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
