---
Title: Specification
Ticket: ESP-20-ANSI-CORRECTNESS
Status: active
Topics:
    - ansi
    - tui
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/ansi_text.go
      Note: Current ANSI stripping and width/cut helpers (regex + scanner)
    - Path: esper/pkg/monitor/ansi_wrap.go
      Note: Current ANSI wrapping helper (duplicate scanner)
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Search
    - Path: esper/pkg/monitor/overlay_manager.go
      Note: Overlay placement uses stripANSI
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:43.782436597-05:00
WhatFor: 'Make ANSI handling correct and consistent across esper’s TUI: stripping, width calculations, cutting, wrapping, searching, and overlay composition.'
WhenToUse: Use when log streams contain ANSI escapes (CSI, OSC, etc.) and features like search, wrap, and overlay placement must behave correctly and predictably.
---


# Specification

## Executive Summary

ANSI handling in `esper/pkg/monitor` is currently “best-effort” and inconsistent:

- `esper/pkg/monitor/ansi_text.go`:
  - `stripANSI` uses a regex that matches only a subset of CSI sequences
  - `hasANSI` depends on that regex
  - `ansiCutToWidth` implements a manual scanner that supports CSI and OSC
- `esper/pkg/monitor/ansi_wrap.go`:
  - `ansiCutToWidthWithRemainder` re-implements a similar scanner (slightly differently)
  - `wrapViewportLines` uses these utilities and appends resets in certain cases

Search and filtering depend on `stripANSI` and `hasANSI`:
- `containsQuery` strips ANSI using the regex-based approach
- search highlighting avoids highlighting if `hasANSI` returns true

This ticket replaces the ad-hoc mix with one canonical ANSI utility implementation and updates all TUI features to use it consistently.

Constraints:
- Performance is not a concern.
- No backwards compatibility requirements; we can change internal helpers and output formatting where needed.

## Problem Statement

Current problems:

1) **Inconsistent escape coverage**
- Regex stripping (`ansi_text.go`) does not handle OSC sequences, but cutting/wrapping scanners do (partially).
- This can break:
  - searching (query matching against partially stripped text)
  - “does this line contain ANSI?” checks
  - width calculations

2) **Duplicated scanners**
- Two manual scanners exist (`ansiCutToWidth` and `ansiCutToWidthWithRemainder`) with similar but not identical logic.

3) **Hidden coupling to UX features**
- Search highlighting intentionally avoids highlighting when ANSI is present; but “ANSI present” is defined inconsistently.
- Overlay rendering uses `stripANSI` to decide whether overlay lines are “non-empty” (`overlay_manager.go`).

4) **Hard-to-test behavior**
- Without a single canonical ANSI parser, tests are difficult and bugs are subtle.

## Proposed Solution

### 1) Create a canonical ANSI utility package

Create a new package in the esper module:
- `esper/pkg/ansiutil` (name can be bikeshedded; avoid colliding with stdlib names)

It provides a single state-machine scanner over strings (or bytes) that recognizes at least:
- CSI sequences: `ESC [` … final byte (0x40..0x7E)
- OSC sequences: `ESC ]` … terminated by `BEL` or `ESC \`
- Optionally:
  - other ESC sequences (treat conservatively; strip them as best-effort)

Core API:

```go
func Strip(s string) string
func HasEscapes(s string) bool
func VisibleWidth(s string) int

// Cut returns a prefix of s whose visible width <= w, preserving escapes in the output.
func Cut(s string, w int) string

// CutRemainder returns (head, tail) such that VisibleWidth(head) <= w and head+tail == s (modulo boundaries),
// preserving escape sequences and not splitting them.
func CutRemainder(s string, w int) (head, tail string)
```

Optional higher-level helpers (can be built on the primitives):

```go
// WrapLines wraps each line to width w using CutRemainder, preserving original trailing newline behavior.
func WrapLines(lines []string, w int) []string
```

### 2) Replace monitor-local ANSI helpers with calls into the package

Remove or greatly simplify:
- `esper/pkg/monitor/ansi_text.go`
- `esper/pkg/monitor/ansi_wrap.go`

Update call sites:
- `monitor/monitor_view.go`:
  - search substring highlight logic uses `Strip` and `VisibleWidth` consistently
  - `renderSearchBar` uses `VisibleWidth`
  - wrap logic uses `WrapLines`
- `monitor/overlay_manager.go`:
  - uses `Strip` for “non-empty overlay line” detection

### 3) Define a clear policy for ANSI + search highlighting

Current policy:
- do not attempt substring highlighting inside lines containing ANSI.

We can keep that policy (it’s safer) but make it correct:
- `HasEscapes(line)` must reliably detect any escape sequences we preserve.

Alternatively (since no backwards compat), we can adopt a new policy:
- allow highlighting even with ANSI by highlighting on the stripped view and mapping offsets back.
- This is more complex; given “performance not issue” but complexity *is* an issue, we should keep the safe policy unless there is a strong need.

### 4) Tests

Add unit tests in the new package that cover:
- CSI-only inputs
- OSC hyperlinks and OSC terminated by BEL and by ST (`ESC \`)
- mixed text + escapes, including nested/adjacent sequences
- VisibleWidth correctness with wide runes (emoji / CJK)
- Cut and CutRemainder correctness:
  - never split escapes
  - never exceed visible width

Add integration tests in `pkg/monitor` for:
- wrapping output lines with ANSI (ensuring resets don’t leak)
- search matching correctness when ANSI is present (query matches stripped content)
- overlay placement logic with ANSI in overlay lines

## Design Decisions

1) **One canonical scanner**
- Eliminates duplication and inconsistent definitions.

2) **Conservative escape support**
- Support CSI and OSC robustly; treat other escapes as “strip best-effort”.

3) **Keep highlighting policy simple**
- Avoid complex index mapping unless needed; correctness over fancy UX.

## Alternatives Considered

1) Keep regex stripping and patch it for OSC
- Rejected: regex-based stripping is brittle and doesn’t naturally model OSC terminators.

2) Use an external ANSI parsing library
- Deferred: possible, but we already have custom needs (width/cut/wrap). A small in-house scanner is acceptable given scope.

## Implementation Plan

1) Add `esper/pkg/ansiutil` with scanner + tests.
2) Switch `pkg/monitor` call sites to use `ansiutil`.
3) Delete or simplify `ansi_text.go` and `ansi_wrap.go`.
4) Ensure `go test ./...` passes.
5) Update ESP-11 deep audit mapping to mark ANSI correctness as addressed by ESP-20.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline:
   - `cd esper && go test ./... -count=1`
2) Add ANSI utility package with tests first (no call site changes yet); re-run baseline.
3) Switch one TUI feature at a time to use the canonical utility:
   - strip/width usage in search
   - wrap logic
   - overlay placement
   Re-run baseline after each switch.
4) Add integration tests with ANSI-containing lines to catch regressions.

Ticket-specific invariants:
- Search should match against the visible text (escapes stripped) deterministically.
- Wrapping must not split escape sequences or cause style “leakage”.
- Overlay placement must remain correct even when overlay content includes ANSI.

## Open Questions

1) Do we need to support DCS sequences or other less common escapes?
2) Should we enforce “always end wrapped segments with reset” or rely on upstream logs to reset?

## References

- Current ANSI helpers:
  - `esper/pkg/monitor/ansi_text.go`
  - `esper/pkg/monitor/ansi_wrap.go`
- Call sites:
  - `esper/pkg/monitor/monitor_view.go` (search/render/wrap)
  - `esper/pkg/monitor/overlay_manager.go` (overlay placement using stripANSI)
- ESP-11 review (ANSI duplication/consistency note):
  - `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/design-doc/02-esper-code-review-deep-audit.md`
