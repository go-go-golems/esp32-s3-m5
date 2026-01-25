---
Title: Reusable keyboard library for Cardputer + Cardputer Adv
Ticket: G11-CARDPUTER-ADV-KEYBOARD-LIBRARY
Status: complete
Topics:
    - cardputer
    - keyboard
    - esp-idf
    - esp32s3
    - console
    - usb-serial-jtag
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T17:56:50.334714998-05:00
WhatFor: ""
WhenToUse: ""
---


# Reusable keyboard library for Cardputer + Cardputer Adv

## Overview

Goal: produce a reusable, board-agnostic keyboard library design for both **Cardputer** (GPIO matrix scan) and **Cardputer-ADV** (TCA8418 over I2C), with a clear event delivery story for different application styles (queues, `esp_event`, LVGL, scripting runtimes).

This ticket is documentation-only: it extracts contracts and patterns from existing code (`components/cardputer_kb` and tutorial `0066`) and proposes a layered architecture that can be implemented incrementally.

## Key Links

- Design analysis: [Reusable Keyboard Library: Design Analysis](./design-doc/01-reusable-keyboard-library-design-analysis.md)
- Diary: [Diary](./reference/01-diary.md)

## Status

Current status: **active**

## Topics

- cardputer
- keyboard
- esp-idf
- esp32s3
- console
- usb-serial-jtag
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
