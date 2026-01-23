---
Title: 'Cardputer: simulate ESP32C6 50-LED chain on screen'
Ticket: 0066-cardputer-ledchain-gfx-sim
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - animation
    - esp32c6
    - ui
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/design-doc/01-deep-research-led-pattern-engine-cardputer-gfx-rendering.md
      Note: Deep research analysis for this ticket
    - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/reference/01-diary.md
      Note: Implementation/research diary
    - Path: ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/tasks.md
      Note: Task list
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-23T08:52:33.913269474-05:00
WhatFor: ""
WhenToUse: ""
---


# Cardputer: simulate ESP32C6 50-LED chain on screen

## Overview

Build a Cardputer (ESP32-S3) on-screen simulation of a 50-LED WS281x chain currently driven by an ESP32-C6, using M5GFX. The simulator should be semantically faithful to the existing pattern engine (rainbow/chase/breathing/sparkle/off) and should reuse the repo’s established “sprite/canvas” render-loop idioms.

## Key Links

- `design-doc/01-deep-research-led-pattern-engine-cardputer-gfx-rendering.md` — Deep research (patterns + GFX)
- `reference/01-diary.md` — Diary (frequent research/implementation log)
- `tasks.md` — Implementation checklist
- **Related Files**: See frontmatter `RelatedFiles`

## Status

Current status: **active**

## Topics

- cardputer
- m5gfx
- display
- animation
- esp32c6
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
