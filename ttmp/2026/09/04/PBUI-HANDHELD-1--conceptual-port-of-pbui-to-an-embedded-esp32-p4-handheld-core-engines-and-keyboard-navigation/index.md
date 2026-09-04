---
Title: 'Conceptual port of pbui to an embedded ESP32-P4 handheld: core engines and keyboard navigation'
Ticket: PBUI-HANDHELD-1
Status: active
Topics:
    - pbui
    - embedded
    - architecture
    - design
    - onboarding
    - research
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Firmware-owned PBUI handheld ticket, migrated from pbui. Native C++ redesign and evidence review supersede the initial QuickJS implementation plan; implementation has not started.
LastUpdated: 2026-09-04T12:30:44.255921283-04:00
WhatFor: Landing page for PBUI-HANDHELD-1 - links the imported PBUI/HB prototype sources, the intern design guide and the diary.
WhenToUse: Start here when picking up the handheld port or when looking for the handheld prototype sources.
---

# Conceptual port of pbui to an embedded ESP32-P4 handheld: core engines and keyboard navigation

## Overview

This ticket answers: what of pbui is the *idea* (typed presentations, type-directed actions with four-state availability, typed acceptance, additive help, relations, decks of tiles) and travels to an embedded device, and what has to be built because it was never separable from the pointer.

Inputs:

- `sources/pbui-handheld.jsx` - the PBUI/HB prototype v0.3 (53x32 simulated LCD, keyboard-only, pure reducer).
- `sources/pbui-handheld-manual.md` - the owner's manual; its six tutorials are the acceptance tests of the port.
- `sources/pbui-handheld-project-report.md` - the design rationale and open questions.
- The pbui kernel (`src/presentation`), `workbench-core`, and the ESP32-P4 PicoCalc firmware tree (`/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`: 0099 display+keyboard, 0101 native QuickJS, 0102 visual REPL/PicoOS, `components/`).

Deliverables:

- `design-doc/01-pbui-handheld-port-analysis-design-and-implementation-guide.md` - the intern guide: hardware and firmware evidence, the pbui tour, the prototype tour and its pbui mapping, gap analysis, design with ten decision records, pseudocode for the key flows, a nine-phase plan, test strategy, risks, references.
- `reference/01-investigation-diary.md` - how the guide was derived.

## Current direction and location

This complete ticket was moved from `pbui/ttmp` to the firmware repository `esp32-s3-m5/ttmp` on 2026-09-04. Historical prose paths in the earlier documents refer to their original repository unless stated otherwise; structured file relations have been repaired. The migration manifest is in `sources/ticket-migration-manifest.json`.

The initial QuickJS guide (01) and first native proposal (02) are historical designs under review, not parallel implementation backlogs. The new review guide (03) will establish the implementation baseline. No firmware implementation has started.

- [First native proposal](design-doc/02-native-c-pbui-subset-for-the-esp32-p4-picocalc.md)
- [Fresh review diary](reference/03-native-redesign-review-diary.md)

PBUI source still lives at `/home/manuel/workspaces/2026-09-01/add-plot-editor/pbui`. Firmware components live in this repository.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- pbui
- embedded
- architecture
- design
- onboarding
- research

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
