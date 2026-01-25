---
Title: 'Esper TUI: command palette'
Ticket: ESP-08-TUI-COMMAND-PALETTE
Status: complete
Topics:
    - tui
    - bubbletea
    - lipgloss
    - ux
    - palette
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Palette exec wiring + keybindings/actions
    - Path: esper/pkg/monitor/palette_overlay.go
      Note: Palette rows/separators/filtering
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md
      Note: Canonical current-vs-desired compare doc
    - Path: ttmp/2026/01/25/ESP-08-TUI-COMMAND-PALETTE--esper-tui-command-palette/reference/01-diary.md
      Note: Implementation diary and validation steps
    - Path: ttmp/2026/01/25/ESP-08-TUI-COMMAND-PALETTE--esper-tui-command-palette/scripts/01-tmux-capture-command-palette.sh
      Note: Deterministic palette capture
ExternalSources: []
Summary: Command palette parity with UX spec (grouped separators, full command set, action wiring) with deterministic screenshot verification.
LastUpdated: 2026-01-25T18:49:53.53592481-05:00
WhatFor: Track and deliver command palette parity work for Esper TUI, plus repeatable screenshot evidence.
WhenToUse: Use when modifying the command palette or reviewing parity against the UX wireframe (§2.4).
---




# Esper TUI: command palette

## Overview

Bring the Esper TUI command palette to parity with the UX wireframe (§2.4): grouped separators, complete command list, and command execution wired to real monitor actions (with deterministic capture artifacts for review).

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
- palette

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
