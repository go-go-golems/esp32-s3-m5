---
Title: Esper postmortem review (architecture + code audit)
Ticket: ESP-11-REVIEW-ESPER
Status: active
Topics:
    - tui
    - backend
    - tooling
    - console
    - serial
DocType: index
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper
      Note: Go module under review
    - Path: esper/pkg/monitor
      Note: Bubble Tea TUI implementation
    - Path: esper/pkg/tail
      Note: Non-TUI tail mode
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T19:34:18.24740462-05:00
WhatFor: 'Ticket hub for the ESP-11 postmortem review of the esper app: architecture map, deep audit report, diary, and follow-up/refactor plan.'
WhenToUse: Start here when reviewing the ESP-11 findings or navigating to the detailed documents.
---


# Esper postmortem review (architecture + code audit)

## Overview

This ticket is a postmortem review of the entire `esper` application (Go CLI + serial monitor), with an intentionally strict audit mindset:
- Explain the big architecture and component interactions.
- Perform a deep code review by subsystem (backend/pipeline, TUI, helpers), emphasizing duplicated/repeated code and design improvements.
- Keep a frequent implementation diary while producing the report.
- Export/upload the resulting documents to reMarkable.

## Key Links

- Architecture overview: `design-doc/01-esper-architecture-big-picture.md`
- Deep audit report: `design-doc/02-esper-code-review-deep-audit.md`
- Diary: `reference/01-diary.md`
- Follow-ups/refactor plan: `playbook/01-follow-ups-and-refactor-plan.md`
- Code under review: `esper/` (Go module)

## Status

Current status: **active**

## Topics

- tui
- backend
- tooling
- console
- serial

## Tasks

See [tasks.md](./tasks.md) for the current task list.

## Changelog

See [changelog.md](./changelog.md) for recent changes and decisions.

## Structure

- design-doc/ - Architecture and design documents
- reference/ - Prompt packs, API contracts, context summaries
- playbooks/ - Command sequences and test procedures
- scripts/ - Temporary code and tooling
- various/ - Working notes and research
- archive/ - Deprecated or reference-only artifacts
