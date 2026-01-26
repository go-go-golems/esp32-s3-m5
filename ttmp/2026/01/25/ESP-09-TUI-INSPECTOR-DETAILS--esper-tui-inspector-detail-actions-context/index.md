---
Title: 'Esper TUI: inspector detail actions + context'
Ticket: ESP-09-TUI-INSPECTOR-DETAILS
Status: active
Topics:
    - tui
    - bubbletea
    - lipgloss
    - ux
    - inspector
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/inspector_detail_overlay.go
      Note: Detail overlay implementation for required actions
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Jump/copy/save routing in monitor model
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md
      Note: Canonical compare doc
    - Path: ttmp/2026/01/25/ESP-09-TUI-INSPECTOR-DETAILS--esper-tui-inspector-detail-actions-context/reference/01-diary.md
      Note: Implementation diary
    - Path: ttmp/2026/01/25/ESP-09-TUI-INSPECTOR-DETAILS--esper-tui-inspector-detail-actions-context/scripts/01-tmux-capture-inspector-detail-actions.sh
      Note: Capture harness
ExternalSources: []
Summary: 'Inspector detail actions + context: copy/save/jump/tab-next-event with deterministic capture artifacts.'
LastUpdated: 2026-01-25T19:06:55.038347743-05:00
WhatFor: "Track implementation of inspector detail view actions (copy/save/jump/tab-next) and validation artifacts."
WhenToUse: "Use when modifying inspector detail screens or validating against UX spec §1.3."
---



# Esper TUI: inspector detail actions + context

## Overview

Bring the inspector detail overlays (PANIC/COREDUMP) to parity with the UX spec (§1.3) by adding action keys (copy/save/jump/tab-next-event), a Context section, and deterministic capture artifacts for review.

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- tui
- bubbletea
- lipgloss
- ux
- inspector

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
