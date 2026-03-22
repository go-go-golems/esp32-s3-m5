---
Title: PaperS3 protractor gesture trainer and recognizer
Ticket: ESP-32-PAPERS3-PROTRACTOR
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
RelatedFiles:
    - Path: 0076-papers3-protractor-trainer/CMakeLists.txt
      Note: Donor component reuse for M5PaperS3 display and touch stack
    - Path: 0076-papers3-protractor-trainer/README.md
      Note: Build
    - Path: 0076-papers3-protractor-trainer/main/app_main.cpp
      Note: Minimal firmware entrypoint
    - Path: 0076-papers3-protractor-trainer/main/protractor_math.cpp
      Note: Protractor resample
    - Path: 0076-papers3-protractor-trainer/main/protractor_math.h
      Note: Public Protractor algorithm API
    - Path: 0076-papers3-protractor-trainer/main/trainer_app.cpp
      Note: PaperS3 UI
    - Path: 0076-papers3-protractor-trainer/main/trainer_app.h
      Note: Application state and layout declarations
    - Path: 0076-papers3-protractor-trainer/sdkconfig.defaults
      Note: ESP32-S3 target defaults and USB Serial/JTAG console policy
ExternalSources:
    - local:protractor_gesture_recognizer_demo.html
Summary: ""
LastUpdated: 2026-03-21T20:26:40.633811646-04:00
WhatFor: ""
WhenToUse: ""
---


# PaperS3 protractor gesture trainer and recognizer

## Overview

This ticket captures a new standalone `PaperS3` application, `0076-papers3-protractor-trainer`, that ports the core Protractor gesture recognizer into a device-native training and classification UI. The implementation keeps the algorithmic ideas from the imported browser demo, but adapts the interaction model to PaperS3 constraints: e-paper refresh behavior, GT911 touch input through `M5Unified`, and a touch-only interface with no text keyboard.

Current implementation status:

- firmware project created and builds successfully with `ESP-IDF 5.3.4`
- Protractor preprocessing and recognition implemented in dedicated math files
- PaperS3 UI implemented with a large drawing canvas, eight template slots, action buttons, and recognition bars
- imported HTML source stored in the ticket and used as the algorithm/layout reference
- detailed implementation plan, analysis/design guide, and investigation diary authored for handoff

Primary deliverables:

- `0076-papers3-protractor-trainer/`
- imported source `sources/local/protractor_gesture_recognizer_demo.html`
- intern-oriented guide in `design-doc/01-...`
- detailed execution plan in `design-doc/02-...`
- chronological diary in `reference/01-investigation-diary.md`

## Key Links

- Firmware: `0076-papers3-protractor-trainer`
- Imported source: `sources/local/protractor_gesture_recognizer_demo.html`
- Guide: `design-doc/01-papers3-protractor-gesture-trainer-analysis-design-and-implementation-guide.md`
- Plan: `design-doc/02-papers3-protractor-gesture-trainer-detailed-implementation-plan.md`
- Diary: `reference/01-investigation-diary.md`
- Related Files: see frontmatter `RelatedFiles`
- External Sources: see frontmatter `ExternalSources`

## Status

Current status: **active**

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

## Notes For Reviewers

- The browser demo used free-form named templates with HTML controls. The PaperS3 version intentionally uses fixed slots `A` through `H` because there is no keyboard in scope.
- The algorithm is split out into `protractor_math.*` so it can be tested, reasoned about, or reused separately from the UI shell.
- The app uses `epd_fast` for live stroke drawing and `epd_text` for full-screen chrome redraws to respect the IT8951 update model.
