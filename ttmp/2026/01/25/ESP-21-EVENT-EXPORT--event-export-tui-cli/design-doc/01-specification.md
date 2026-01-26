---
Title: Specification
Ticket: ESP-21-EVENT-EXPORT
Status: active
Topics:
    - events
    - tui
    - tooling
    - backend
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/monitor_view.go
      Note: TUI event model and inspector
    - Path: esper/pkg/tail/tail.go
      Note: Tail mode pipeline and potential event emission points
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:44.336696321-05:00
WhatFor: Define a structured event model for esper and implement export of events from both TUI and CLI surfaces (NDJSON/JSON), enabling automation, bug reports, and downstream tooling.
WhenToUse: Use when you want to capture machine-readable incidents (GDB stub, panic/backtrace, core dump capture, user actions) instead of relying on raw log text parsing.
---


# Specification

## Executive Summary

`esper` already creates structured “events” internally in the TUI:
- `esper/pkg/monitor/monitor_view.go`: `events []monitorEvent` populated by `addEvent(...)`
- the Inspector panel renders these events and allows copying/saving certain details

However:
- events are not exported in a machine-readable way,
- the CLI tail mode does not emit structured events,
- any automation must parse human-oriented text output.

This ticket introduces:
- a canonical structured event schema (shared across modes),
- an exporter that can write NDJSON/JSON to a file or stdout,
- integration points in:
  - TUI (export current event buffer and/or stream events while running),
  - CLI tail mode (stream events as NDJSON).

Constraints:
- Performance is not a concern.
- No backwards compatibility requirements; we can change event schema and CLI flags freely.

This ticket strongly benefits from ESP-12 (shared pipeline) so that:
- events are emitted once and reused everywhere,
- export does not depend on TUI-specific logic.

## Problem Statement

1) **Operators and automation need structure**
- “panic detected” and “core dump captured” are semantically meaningful events.
- Parsing these from log text is brittle (ANSI, partial lines, different formats).

2) **TUI events are trapped in-memory**
- Inspector events are useful but cannot be exported or replayed easily.

3) **No stable artifact for bug reports**
- “Send me the logs” is not enough; we want:
  - event stream
  - core dump artifact paths
  - decoded backtrace details
  - probe results

4) **Upcoming refactors will be easier with a canonical event model**
- pipeline extraction, async decode, python removal all benefit from a stable “event contract” in code.

## Proposed Solution

### 1) Define a canonical event type (package-level)

Create `esper/pkg/events` (or `esper/pkg/pipeline` owns the event type if we prefer fewer packages).

Proposed schema (Go type):

```go
type Event struct {
  At time.Time         `json:"at"`
  Kind string          `json:"kind"`      // e.g. "panic", "coredump", "gdb_stub", "probe", "reset", ...
  Title string         `json:"title"`     // short summary
  Body string          `json:"body,omitempty"` // human readable details

  // Machine-usable structured fields.
  Fields map[string]any `json:"fields,omitempty"`

  // Optional: ties events to a session/device.
  SessionID string `json:"session_id,omitempty"`
  Port string      `json:"port,omitempty"`
}
```

No backwards compat means we can evolve this freely; still, we define it explicitly so all features use the same object.

### 2) Event kinds and required fields (initial list)

Event kinds (initial; extend over time):
- `gdb_stub_detected`
  - Fields: `payload` (base64 or hex), `checksum_ok` (bool), etc.
- `backtrace_detected`
  - Fields: `raw_line`
- `backtrace_decoded`
  - Fields: `raw_line`, `decoded_frames` (string/[]string)
- `coredump_prompt_detected`
  - Fields: `auto_enter` (bool)
- `coredump_capture_started`
  - Fields: `started_at`
- `coredump_capture_progress`
  - Fields: `buffered_bytes`
- `coredump_capture_finished`
  - Fields: `raw_path`, `raw_bytes`
- `coredump_decoded`
  - Fields: `report_path` (optional), `report` (string), `decode_ok` (bool)
- `device_reset_sent` / `device_reset_failed`
- `send_break_sent` / `send_break_failed`
- `probe_ok` / `probe_failed` (ESP-17)

The “required” fields per kind should be documented in this spec, and the code should enforce them in constructors.

### 3) Export formats

We support two formats:

1) **NDJSON streaming** (recommended)
- One JSON object per line.
- Suitable for tail mode streaming and long-running sessions.

2) **JSON array snapshot**
- Dump current event buffer as a single JSON array.
- Suitable for exporting from TUI at a point in time.

### 4) Export destinations

Provide:
- stdout/stderr (explicit flags)
- a file path (append mode for NDJSON)
- optionally “auto path under cache dir” (like session logging)

### 5) Integrate with pipeline / monitor / tail

#### Pipeline (preferred)
- ESP-12 pipeline should emit `events.Event` values for semantic occurrences.
- Both tail and TUI consume the pipeline output stream and:
  - render human-facing output
  - forward events to exporters

#### TUI integration
Two modes:

1) **Snapshot export**
- Add a command palette action: “Export events”
- Choose destination:
  - default path under cache dir
  - show a toast with saved path

2) **Streaming export**
- CLI flag: `esper --events-file <path>` (or similar) when running TUI
- While running, every event appended to inspector is also written as NDJSON

Implementation notes:
- Reuse existing session logging patterns (`toggleSessionLogging`) but for structured events.

#### Tail integration
- Add `esper tail --events` flag(s):
  - `--events-ndjson` (default true when `--events-file` set)
  - `--events-file <path>` or `--events-stdout`
- Tail should emit events for:
  - gdb detection
  - core dump capture start/end
  - backtrace detected/decoded
  - probe results if probing is used in tail mode (optional)

### 6) Testing

Unit tests should:
- feed synthetic serial sequences into the pipeline (ESP-12) and assert emitted events.
- validate JSON encoding/decoding of events.
- validate exporter writes valid NDJSON lines and handles errors gracefully.

Integration tests (lightweight) can assert:
- monitor event buffer includes expected events after processing certain input sequences (similar to existing tests that feed `serialChunkMsg`).

## Design Decisions

1) **NDJSON is the primary streaming format**
- Simple and robust for long-running sessions.

2) **Canonical event type shared across modes**
- Prevents TUI-only events and allows CLI to be “first-class” for automation.

3) **No stability guarantees**
- No backwards compat requirement, but still document the schema so internal consumers can evolve with it.

## Alternatives Considered

1) Only export raw logs and ask users to parse them
- Rejected: brittle and defeats the value of an inspector/event model.

2) Export Bubble Tea internal messages
- Rejected: too UI-specific and not a stable abstraction.

## Implementation Plan

1) Add `pkg/events` with `Event` type and helpers/constructors per event kind.
2) Add an `Exporter` abstraction (writer + NDJSON encoder) with tests.
3) Integrate into pipeline outputs (ESP-12) or directly into current call sites if pipeline is not ready.
4) Add CLI flags for `tail` event export and implement.
5) Add TUI flags and/or command palette action for event export.
6) Update ESP-11 deep audit mapping to mark “event export” as addressed by ESP-21.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline:
   - `cd esper && go test ./... -count=1`
2) Add event schema + exporter with unit tests first (no runtime integration yet); re-run baseline.
3) Integrate export in one surface at a time:
   - tail mode streaming NDJSON
   - then TUI snapshot export (or streaming export)
   Re-run baseline after each integration.
4) Manual smoke (optional):
   - run tail with `--events` and validate NDJSON output is well-formed
   - run TUI and export events; verify file content

Ticket-specific invariants:
- Event export must not affect raw log output rendering (unless explicitly designed to).
- NDJSON output must be valid JSON per line and stable under ANSI-rich logs.
- Exported events should include enough context to be useful (timestamp, kind, and key artifact paths).

## Open Questions

1) Should events include a monotonic sequence number for ordering across threads?
2) Should events include raw log offsets or anchors to support “jump to log” in exports?
3) How do we handle large reports (core dump report) in events:
   - inline in `Body`, or
   - saved to a file with path in fields?

## References

- Existing TUI event model:
  - `esper/pkg/monitor/monitor_view.go` (`monitorEvent`, `addEvent`, inspector rendering)
- ESP-12 pipeline extraction (preferred event emission point):
  - `ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md`
