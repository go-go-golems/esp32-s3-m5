---
Title: 'ESP-10-TUI-DOCS: docs + review workflow spec'
Ticket: ESP-10-TUI-DOCS
Status: active
Topics:
    - docs
    - tui
    - ux
    - testing
    - remarkable
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/01-ux-iteration-loop.md
      Note: |-
        UX iteration loop playbook (may need updates)
        Iteration process
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md
      Note: |-
        Primary review doc (current screenshot vs desired wireframe)
        Main review doc
ExternalSources: []
Summary: 'Define the documentation and review workflow for iterating on Esper TUI UX: capture scripts, compare docs, and reMarkable export.'
LastUpdated: 2026-01-25T16:55:00-05:00
WhatFor: Make UX iteration reproducible and reviewable with minimal friction.
WhenToUse: Use when updating screenshots/compare docs or sharing progress via PDF/reMarkable.
---


# ESP-10-TUI-DOCS — docs + review workflow spec

## Goals

- One place to review “current vs desired” for every screen: the compare doc.
- Reproducible capture: tmux scripts produce deterministic screenshots (120×40 primary).
- Reproducible sharing: render compare doc(s) to PDF and upload to reMarkable.

## Canonical review document

Primary compare doc (keep current):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

Source of truth wireframes:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md`

## Capture principles

- All scripts that generate artifacts must live in the relevant ticket’s `scripts/`.
- Captures must not pollute screenshot directories with config/state/telemetry:
  - use `mktemp -d` for `XDG_CONFIG_HOME` (and `XDG_CACHE_HOME`, `XDG_STATE_HOME`)
  - set `GOTELEMETRY=off` for `go run`/`go test` where it helps
- Capture both `.txt` (pane text) and `.png` (rendered) when possible.

## Hardware capture principles

- Prefer USB Serial/JTAG for ESP32-S3 console interactions (UART may be repurposed).
- When using the UI test firmware, document the exact port used (`/dev/serial/by-id/...`).

## PDF + reMarkable export

The workflow should be:
1) Render selected markdown docs to PDF (repeatable script)
2) Upload PDF(s) to a dated folder on reMarkable (repeatable script)

Acceptance criteria:
- A single command (or two) produces an updated PDF and uploads it.
- The PDF layout preserves code blocks (wireframes) and screenshots legibly.
