---
Title: Specification
Ticket: ESP-15-DEDUPE-EXTRACT-PKG
Status: active
Topics:
    - backend
    - tui
    - tooling
    - pipeline
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/confirm_overlay.go
      Note: Generic confirmation overlay
    - Path: esper/pkg/monitor/port_picker.go
      Note: Shared helpers currently live here
    - Path: esper/pkg/monitor/reset_confirm_overlay.go
      Note: Duplicated confirmation overlay to delete
    - Path: esper/pkg/monitor/select_list.go
      Note: Reusable selection helper candidate
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:41.372992167-05:00
WhatFor: Remove duplicated code paths and consolidate reusable helpers into dedicated packages (or dedicated helper files), reducing drift risk and improving discoverability.
WhenToUse: Use when repeated patterns across `pkg/monitor`, `pkg/tail`, and helpers make changes fragile or create unnecessary code volume.
---


# Specification

## Executive Summary

This ticket is a “hygiene + consolidation” sweep that targets duplication and misplaced helpers that are *not* the primary focus of other major refactor tickets.

It is intentionally scoped as:
- **small-to-medium refactors** that reduce duplicated logic and improve code organization,
- **without** re-deriving major pipeline architecture (ESP-12) or deep ANSI correctness work (ESP-20).

Examples of concrete duplicates to eliminate:
- duplicated confirm overlays (`reset_confirm_overlay.go` vs `confirm_overlay.go`)
- package-wide helper functions living in surprising files (e.g. `padOrTrim` in `port_picker.go`)
- repeated ANSI parsing logic patterns across multiple helper files (to be coordinated with ESP-20)
- repeated “list selection windowing” patterns (formalize `selectList` as a general helper)

Constraints:
- Performance is not a concern.
- No backwards compatibility requirements (we can change internal APIs and behaviors freely).

## Problem Statement

Duplication and poor helper placement cause:

1) **Behavior drift**
- The same conceptual behavior implemented twice will diverge over time.

2) **Discoverability debt**
- Shared helpers living in unrelated files make it harder to find and review logic.

3) **Review friction**
- Minor fixes become multi-file hunts, and PRs accumulate “noise” from repeated code.

In the ESP-11 audit, several such duplications were identified as “easy wins” with high leverage.

## Proposed Solution

### 1) Deduplicate confirm overlays

Current state:
- `esper/pkg/monitor/confirm_overlay.go` (generic confirm overlay with configurable message and forwarded msg)
- `esper/pkg/monitor/reset_confirm_overlay.go` (reset-specific overlay with essentially the same UI logic)

Plan:
- Delete `reset_confirm_overlay.go`.
- Replace its uses with `newConfirmOverlay(confirmOverlayConfig{... forward: resetDeviceMsg{}})`:
  - call sites:
    - `monitor_view.go` (Host mode `ctrl+r` and palette command `cmdResetDevice`)

Acceptance criteria:
- Reset confirm UX remains functional (or improves).
- Tests still pass.

### 2) Consolidate package-wide UI helpers into a single home

Current state:
- `padOrTrim`, `min`, `max`, `updateSingleLineField` live in `esper/pkg/monitor/port_picker.go` but are used throughout `pkg/monitor`.

Plan (coordinated with ESP-14 split):
- Create `esper/pkg/monitor/ui_helpers.go` and move:
  - `padOrTrim`
  - `min`, `max`, `clamp`
  - `updateSingleLineField`
  - `splitLinesN` (currently in `monitor_view.go`)

Acceptance criteria:
- No behavior change intended.
- Helpers are easy to discover and consistently used.

### 3) Consolidate “selection list” helpers into a reusable utility

Current state:
- `selectList` exists as a small reusable core in `esper/pkg/monitor/select_list.go`.

Plan:
- Keep `selectList` concept, but decide whether it belongs:
  - inside `pkg/monitor` (if only TUI uses it), or
  - in a new reusable package (e.g. `esper/pkg/ui` or `esper/pkg/termui`) if we want to reuse it in other future TUIs/CLIs.

Since this repo already has multiple UI-like tools, a small `pkg/ui` might be justified.

### 4) Consolidate overlay adapters (optional)

Current state:
- overlay wrappers in `esper/pkg/monitor/overlay_wrappers.go` follow a consistent pattern:
  - wrapper implements `overlayModel`
  - wrapper delegates to an inner “model” that returns a result enum

Plan:
- Optional: create a small generic adapter/helper to reduce boilerplate for overlays that:
  - accept only `tea.KeyMsg`
  - return a “result kind” plus maybe a forwarded msg

If this adds complexity, skip it; this is not a performance-driven ticket.

### 5) Remove duplicated tail pipeline loops (coordination with ESP-12)

Current state:
- `esper/pkg/tail/tail.go` has two near-duplicate loops:
  - `runLoop`
  - `runLoopWithStdinRaw`

Preferred plan:
- Let ESP-12 handle the main refactor (shared pipeline + unified tail loop).

This ticket’s role:
- If ESP-12 lands first, ensure any remaining duplication in tail mode is removed.
- If ESP-12 is delayed, we can still unify tail loops using a local helper (but that risks redoing work).

### 6) “Extract things better kept in pkg”

The phrase “better kept in the pkg” means: code that is currently embedded in UI code but should be reusable and testable.

Candidates:
- `writePrefixedBytes` (currently in `tail.go`) could move to a small `pkg/format` or `pkg/output` if event export (ESP-21) needs it.
- “timestamped path allocation” helpers (`makeTimestampedPath` in `pkg/monitor/save_paths.go`) could be moved to a generic `pkg/paths` helper if reused.

This ticket should be opportunistic: extract only when:
- the helper is clearly reusable,
- extraction does not entangle unrelated dependencies.

## Design Decisions

1) **Prefer deletion over abstraction**
- Remove duplicate files (e.g. reset confirm) rather than creating a new “framework” for confirmations.

2) **Single “home” for shared helpers**
- A helper used across many files should not live in a feature-specific file.

3) **Coordinate with larger refactors**
- Avoid stepping on ESP-12 (pipeline) and ESP-20 (ANSI correctness) by keeping scope clear and referencing dependencies explicitly.

## Alternatives Considered

1) Do nothing and rely on discipline
- Rejected: discipline decays; the codebase grows.

2) Make a huge “refactor everything” ticket
- Rejected: unreviewable and will stall. This ticket is intentionally bounded.

## Implementation Plan

1) Remove `reset_confirm_overlay.go` and replace with `confirm_overlay.go` call sites.
2) Create `ui_helpers.go` and move shared helpers.
3) Decide whether `selectList` stays in `pkg/monitor` or moves to a new reusable package.
4) Identify 1–2 additional “obvious” helper extractions and execute if they are low-risk.
5) Run `go test ./...` and keep existing UI tests green.
6) Update ESP-11 deep audit mapping to mark small duplications as “addressed by ESP-15” (and annotate overlaps with ESP-12/ESP-20).

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline:
   - `cd esper && go test ./... -count=1`
   - `cd esper && go vet ./...`
2) Deduplicate one “small area” at a time (e.g. reset confirm overlay), re-run baseline.
3) Ensure UI invariants remain true:
   - `cd esper && go test ./pkg/monitor -count=1`
4) Manual smoke (optional but recommended):
   - in HOST mode, trigger Reset confirmation (Ctrl-R and palette) and confirm the UX still works.

Ticket-specific invariants (should not change unless explicitly documented):
- Keybindings remain stable.
- Overlay visuals and close semantics remain stable (Esc, Enter, etc.).
- Device manager and port picker editing behavior remains stable (field editing, tab order).

## Open Questions

1) Do we want a general-purpose `pkg/ui` package, or should UI helpers remain private to `pkg/monitor`?
2) Should confirm overlays support richer formatting (multiline, warning emphasis), or keep minimal?

## References

- Duplicated overlays:
  - `esper/pkg/monitor/confirm_overlay.go`
  - `esper/pkg/monitor/reset_confirm_overlay.go`
- Shared helpers currently in surprising locations:
  - `esper/pkg/monitor/port_picker.go` (contains `padOrTrim`, `min/max`, `updateSingleLineField`)
- ESP-11 review (duplication register):
  - `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/design-doc/02-esper-code-review-deep-audit.md`
- Coordination tickets:
  - ESP-12 shared pipeline: `ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md`
  - ESP-20 ANSI correctness: `ttmp/2026/01/25/ESP-20-ANSI-CORRECTNESS--fix-ansi-correctness-across-monitor-strip-wrap-search/design-doc/01-specification.md`
