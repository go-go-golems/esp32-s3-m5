---
Title: 'Esper TUI: implement missing screens + test firmware updates'
Ticket: ESP-05-TUI-MISSING-SCREENS
Status: complete
Topics:
    - tui
    - bubbletea
    - lipgloss
    - serial
    - esp-idf
    - firmware
    - testing
DocType: index
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T16:58:37.274415914-05:00
WhatFor: Implement the remaining screens from the ESP-02 contractor UX spec (missing monitor/inspector screens, confirmation dialogs, device manager/nickname dialogs) and update the esp32s3-test firmware so these screens can be triggered deterministically.
WhenToUse: Use when implementing new Bubble Tea screens/overlays and when updating the ESP32-S3 UI test firmware to reliably exercise those UI states.
---


# Esper TUI: implement missing screens + test firmware updates

## Overview

This ticket is the “build the missing screens” workstream. It focuses on the screens that exist in the contractor wireframes but do not exist yet in the implementation, plus firmware + capture harness improvements needed to trigger and validate those screens reliably.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field
- **Primary UX spec (wireframes)**:
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md`
- **Current vs desired compare doc**:
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`
- **UX iteration loop (captures + compare)**:
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/01-ux-iteration-loop.md`

## Status

Current status: **active**

## Spec

- `design/01-missing-screens-and-firmware-updates-spec.md`

## Topics

- tui
- bubbletea
- lipgloss
- serial
- esp-idf
- firmware
- testing

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
