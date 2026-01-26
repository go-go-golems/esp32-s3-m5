---
Title: Fix Esper TUI - Connect Screen, Main Screen, and Serial Console Interaction
Ticket: ESP-22-UI-FIXEROO
Status: active
Topics:
    - esper
    - tui
    - bubbletea
    - lipgloss
    - serial
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/render/ansi.go
      Note: ANSI color constants (Green/Yellow/Red)
    - Path: esper/pkg/render/autocolor.go
      Note: ESP-IDF log level coloring - ANSI codes applied here (colors work in real terminal)
    - Path: pkg/monitor/app_model.go
      Note: Screen router and overlay handling
    - Path: pkg/monitor/monitor_view.go
      Note: Main monitor view rendering
    - Path: pkg/monitor/port_picker.go
      Note: Port picker with form layout issues
    - Path: pkg/monitor/styles.go
      Note: Root cause - minimal styling with no colors
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/design-doc/01-esper-tui-ux-design-brief-for-contractor.md
      Note: Original UX design brief with wireframes
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T22:10:02.667465679-05:00
WhatFor: ""
WhenToUse: ""
---



# Fix Esper TUI - Connect Screen, Main Screen, and Serial Console Interaction

## Overview

<!-- Provide a brief overview of the ticket, its goals, and current status -->

## Key Links

- **Related Files**: See frontmatter RelatedFiles field
- **External Sources**: See frontmatter ExternalSources field

## Status

Current status: **active**

## Topics

- esper
- tui
- bubbletea
- lipgloss
- serial

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
