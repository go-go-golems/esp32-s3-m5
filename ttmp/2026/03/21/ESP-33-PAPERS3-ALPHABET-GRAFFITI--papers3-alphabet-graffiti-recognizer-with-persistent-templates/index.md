---
Title: PaperS3 alphabet graffiti recognizer with persistent templates
Ticket: ESP-33-PAPERS3-ALPHABET-GRAFFITI
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
    - storage
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/CMakeLists.txt
      Note: Project wiring for the new third PaperS3 app
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.cpp
      Note: Task 1 placeholder UI and mode framing
    - Path: 0077-papers3-alphabet-graffiti/main/protractor_math.cpp
      Note: Gesture math foundation copied from the prior app for reuse
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T21:03:58.840114185-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 alphabet graffiti recognizer with persistent templates

## Overview

This ticket tracks the third PaperS3 handwriting app in the sequence. The goal is a two-mode recognizer:

- `TRAIN` mode for building persistent templates for `A-Z` and `0-9`
- `WRITE` mode for graffiti-style single-stroke writing even before the full alphabet has been trained

The first task establishes the new `0077` project, creates the ticket/diary, and lands a buildable placeholder UI so the rest of the work can proceed in focused commits.

## Key Links

- Project: `0077-papers3-alphabet-graffiti`
- Diary: `reference/01-diary.md`
- Tasks: `tasks.md`
- Changelog: `changelog.md`

## Status

Current status: **active**

## Topics

- esp32-s3
- esp32s3
- firmware
- m5stack
- m5gfx
- ui
- storage

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
