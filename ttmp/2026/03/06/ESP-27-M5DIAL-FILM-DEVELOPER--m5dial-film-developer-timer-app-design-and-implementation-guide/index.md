---
Title: M5Dial film developer timer app design and implementation guide
Ticket: ESP-27-M5DIAL-FILM-DEVELOPER
Status: active
Topics:
    - esp32s3
    - m5stack
    - firmware
    - photo-development
    - ui
    - timer
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Ticket workspace for researching and planning a new M5Dial film developer timer app based on the working 0072 timer project and a curated subset of film_dev_times.json."
LastUpdated: 2026-03-06T21:02:48.356376802-05:00
WhatFor: "Use this ticket as the landing page for the proposed 0073 M5Dial film developer timer application."
WhenToUse: "Use when starting implementation, reviewing the design, or locating the related scripts and diary."
---

# M5Dial film developer timer app design and implementation guide

## Overview

This ticket captures the design work for a new M5Dial application that should be created by copying the current `0072-m5dial-timer-demo` and turning it into a film developer timer. The target app should let the user select a film, temperature, push/pull setting, and, where applicable, a developer and dilution, then run a countdown using a curated recipe derived from `film_dev_times.json`.

The key design conclusion is that the app should not browse the raw source dataset directly. Instead, it should use a curated, normalized starter catalog generated from that file so the on-device UI stays fast, predictable, and small enough for the M5Dial form factor.

## Key Documents

- Primary guide:
  - `design-doc/01-m5dial-film-developer-timer-analysis-design-and-implementation-guide.md`
- Chronological diary:
  - `reference/01-investigation-diary.md`
- Task checklist:
  - `tasks.md`
- Change history:
  - `changelog.md`

## Recommended App Target

- Proposed new app path:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0073-m5dial-film-developer-timer`
- Recommended base project to copy:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo`

## Key Inputs

- Working M5Dial timer app:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0072-m5dial-timer-demo`
- Source film dataset:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/film_dev_times.json`
- Ticket-local analysis scripts:
  - `scripts/analyze_film_dev_times.py`
  - `scripts/scope_subset_report.py`

## Current Status

Current status: **research complete, scaffold created**

What is done:

- ticket created
- `0073-m5dial-film-developer-timer` scaffold copied from `0072` and verified to build
- source app architecture mapped
- source dataset inspected
- intern-focused design and implementation guide written
- initial v1 scope narrowed to common B/W developers plus limited explicit color-negative / C-41-like support

What is next:

- scaffold `0073`
- add catalog preprocessing
- implement selector and recipe-driven timer flow

## Structure

- `design-doc/`
  - primary technical design document
- `reference/`
  - diary and supporting implementation context
- `scripts/`
  - saved ticket-local analysis tooling
- `tasks.md`
  - execution checklist for future implementation
- `changelog.md`
  - chronological summary of ticket milestones
