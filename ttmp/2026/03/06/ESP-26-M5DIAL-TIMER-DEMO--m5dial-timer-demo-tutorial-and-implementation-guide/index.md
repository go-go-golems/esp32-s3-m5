---
Title: M5Dial timer demo tutorial and implementation guide
Ticket: ESP-26-M5DIAL-TIMER-DEMO
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - timer
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-06T19:14:42.41905047-05:00
WhatFor: ""
WhenToUse: ""
---

# M5Dial timer demo tutorial and implementation guide

## Overview

This ticket captures a design-quality guide for building a new M5Dial tutorial project inside `esp32-s3-m5`. The intended result is a small, good-looking timer demo that runs on ESP-IDF 5.4.1, uses the M5Dial rotary encoder and center press as primary controls, supports touch where it helps, and stays simple enough for a new intern to implement without reverse-engineering the older `M5Dial-UserDemo` application framework.

The main recommendation is to build a new tutorial as `esp32-s3-m5/0072-m5dial-timer-demo/` rather than trimming the existing `M5Dial-UserDemo` in place. The old demo contains working board knowledge, but it is organized as a larger app launcher with multiple subsystems and historical compatibility patches. The new tutorial should keep only the M5Dial-specific hardware bring-up and combine it with the cleaner tutorial structure already used elsewhere in `esp32-s3-m5`.

## Key Links

- Design doc: `design-doc/01-m5dial-timer-demo-analysis-design-and-implementation-guide.md`
- Diary: `reference/01-investigation-diary.md`
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
- timer
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
