---
Title: Specification
Ticket: ESP-14-SPLIT-MONITOR-VIEW
Status: active
Topics:
    - tui
    - bubbletea
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Monolithic file to split
    - Path: esper/pkg/monitor/port_picker.go
      Note: Contains shared helpers that should move
    - Path: esper/pkg/monitor/view_test.go
      Note: Existing sizing invariants that must remain true
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:40.88493145-05:00
WhatFor: Split `esper/pkg/monitor/monitor_view.go` into smaller cohesive files to reduce cognitive load, clarify responsibilities, and make future refactors (pipeline extraction, async decode, ANSI fixes, event export) safer.
WhenToUse: Use when `monitor_view.go` has become a maintenance hotspot and changes are hard to review due to mixed concerns in one large file.
---


# Specification

## Executive Summary

`esper/pkg/monitor/monitor_view.go` (~1500 LOC) currently mixes too many concerns:
- serial I/O command creation
- pipeline orchestration (line splitting, GDB detection, core dump capture, backtrace decode, autocolor)
- buffer/ring management
- filter/search logic and viewport decoration
- inspector event model + rendering
- session logging (filesystem)
- UI layout/rendering

This ticket splits that file into cohesive units while keeping behavior stable (even though no backwards compatibility is required, we still prefer “no surprise regressions”).

Primary outcomes:
- smaller, reviewable files aligned to responsibilities
- clearer seams for subsequent tickets:
  - ESP-12 (shared pipeline extraction)
  - ESP-13 (async decode)
  - ESP-20 (ANSI correctness)
  - ESP-21 (event export)

## Problem Statement

The current monolith increases risk and slows development:

1) **Mixed responsibilities**
- Changes to one feature (e.g., search) can accidentally affect unrelated logic (e.g., core dump capture).

2) **Hard to reason about correctness**
- Critical invariants (follow mode, capture mode, overlay routing, viewport sizing) are intertwined with low-level serial processing.

3) **Hard to test and refactor**
- Extracting a shared pipeline or making decoding async becomes a multi-concern edit in one file, creating huge diffs.

This ticket reduces the surface area of each change by giving each concern a home.

## Proposed Solution

### 1) Target file layout (within `esper/pkg/monitor/`)

Proposed decomposition (names are suggestions):

- `monitor_model.go`
  - `type monitorModel` struct definition
  - `newMonitorModel`, `setSize` (layout calculations)

- `monitor_update.go`
  - `func (m monitorModel) Update(...) ...`
  - message routing and state transitions
  - after ESP-12, this should mostly interpret pipeline outputs

- `monitor_view.go` (new, smaller)
  - `View`, `renderTitle`, `renderStatus`, `renderCoreDumpProgressBox`

- `monitor_serial.go`
  - `readSerialCmd`, `tickCmd`
  - `resetDeviceCmd`, `sendBreakCmd`

- `monitor_buffers.go`
  - `append`, `appendCoreDumpLogEvents`, ring buffer bounds
  - `splitKeepNewline`

- `monitor_filter.go`
  - `filteredLines`, `filterSummary`

- `monitor_search.go`
  - search state + rendering decoration (`decorateSearchLines`, `renderSearchBar`, etc.)

- `monitor_inspector.go`
  - `monitorEvent`, `addEvent`, `renderInspectorPanel`
  - glue for inspector detail overlay message types

- `monitor_sessionlog.go`
  - `toggleSessionLogging`, `closeSessionLogging`, `writeSessionLog`

This layout intentionally mirrors the conceptual responsibilities already visible in the code.

### 2) Shared helpers: create `ui_helpers.go` (within monitor package)

Move package-wide helpers out of surprising locations:
- currently `padOrTrim`, `min`, `max`, `updateSingleLineField` live in `port_picker.go`
- `splitLinesN` lives inside `monitor_view.go`

`ui_helpers.go` should contain:
- `padOrTrim`
- `min`, `max`, `clamp`
- `splitLinesN`
- `updateSingleLineField`
- any small join helpers (e.g. `stringsJoinVertical`) if still needed

### 3) Testing / safety

Existing tests already protect key invariants:
- sizing invariants: `esper/pkg/monitor/view_test.go`
- overlay routing: `esper/pkg/monitor/app_model_overlay_test.go`
- filter and palette semantics: `esper/pkg/monitor/filter_overlay_test.go`, `esper/pkg/monitor/palette_overlay_test.go`

This split should keep tests passing. If the split reveals missing test coverage for critical behavior, add targeted unit tests (but keep scope tight: this ticket is a refactor).

### 4) No backwards compatibility requirements

We do not need to keep backwards compatibility, but for this ticket we still prefer a “pure refactor” mindset because it is an enabler for later behavior-changing work.

## Design Decisions

1) **Split by responsibility**
- Each file should have a single conceptual reason to change.

2) **No new public API**
- Keep functions unexported; do not accidentally create cross-package contracts.

3) **Defer package extraction**
- Package boundaries are handled by other tickets; this is file hygiene inside `pkg/monitor`.

## Alternatives Considered

1) Leave the monolith and proceed directly with pipeline/async work
- Rejected: every future change becomes a huge diff and raises regression risk.

2) Rewrite the TUI architecture wholesale
- Rejected: the existing screen/overlay routing architecture is solid; this is a surgical decomposition.

## Implementation Plan

1) Add `ui_helpers.go` first and move shared helpers (minimal diff).
2) Split rendering (`View` + render helpers) into a smaller `monitor_view.go`.
3) Split search and filter logic into dedicated files.
4) Split inspector logic.
5) Split session logging functions.
6) Split serial cmd creation functions.
7) Keep `go test ./...` green throughout.
8) Update ESP-11 deep audit to reference ESP-14 as the “monitor_view split” ticket.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket (pure refactor discipline):
1) Baseline:
   - `cd esper && go test ./... -count=1`
   - `cd esper && go vet ./...`
2) Move code in small, mechanically verifiable chunks (one responsibility at a time), re-run baseline after each move.
3) Pay special attention to the existing “guardrail” tests:
   - `cd esper && go test ./pkg/monitor -count=1`
4) Manual smoke (optional):
   - open TUI and verify basic flows: Port Picker → Connect → Monitor → Host mode → Inspector → overlays.

Ticket-specific invariants (should not change in this ticket):
- View sizing invariants remain true (see `esper/pkg/monitor/view_test.go`).
- Overlay routing invariant remains true (see `esper/pkg/monitor/app_model_overlay_test.go`).
- Keybindings and mode semantics remain unchanged (unless explicitly documented).

## Open Questions

1) Keep the filename `monitor_view.go` for the main View function, or rename to `monitor_render.go` for clarity?
2) Should we keep all files flat in `pkg/monitor`, or create subfolders (not recommended unless it becomes too large again)?

## References

- Primary hotspot:
  - `esper/pkg/monitor/monitor_view.go`
- Follow-up tickets enabled by this split:
  - ESP-12: `ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md`
  - ESP-13: `ttmp/2026/01/25/ESP-13-ASYNC-DECODE--make-decoding-async-bubble-tea-style/design-doc/01-specification.md`
  - ESP-20: `ttmp/2026/01/25/ESP-20-ANSI-CORRECTNESS--fix-ansi-correctness-across-monitor-strip-wrap-search/design-doc/01-specification.md`
  - ESP-21: `ttmp/2026/01/25/ESP-21-EVENT-EXPORT--event-export-tui-cli/design-doc/01-specification.md`
