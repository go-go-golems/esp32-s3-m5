---
Title: Specification
Ticket: ESP-18-COREDUMP-STABILIZE
Status: active
Topics:
    - coredump
    - serial
    - debugging
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/decode/coredump.go
      Note: Core dump capture/decode implementation to refactor
    - Path: esper/pkg/monitor/inspector_detail_overlay.go
      Note: Inspector shows core dump report and provides save/copy actions
    - Path: esper/pkg/monitor/monitor_view.go
      Note: TUI capture UI and inline decode trigger
    - Path: esper/pkg/tail/tail.go
      Note: Tail capture and decode integration
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:42.818464213-05:00
WhatFor: 'Stabilize core dump capture and decoding across TUI and tail: enforce timeouts, bound memory/disk usage, improve artifact handling, and define clear capture/decode boundaries.'
WhenToUse: Use when core dump handling must be reliable under stress (no UI hangs, no unbounded memory growth) and when preparing to replace python-based decoding with a pure Go implementation (ESP-19).
---


# Specification

## Executive Summary

Core dump handling is a central “incident workflow” for esper, but the current implementation has sharp edges:

- **Potential UI freeze / indefinite hang**:
  - `esper/pkg/decode/coredump.go` runs `python3` without context/timeouts.
  - In the TUI, core dump decode happens inline in `monitorModel.Update` while processing serial lines.
- **Unbounded buffering**:
  - during capture, base64 content is appended into memory (`d.buf`) with no size limit.
- **Unclear artifact policy**:
  - raw base64 is saved to a temp file, but retention and discoverability are not well-defined.

This ticket makes core dump handling robust and predictable:
- separate *capture* from *decode* (capture becomes cheap and deterministic)
- bound capture size (RAM and/or disk)
- enforce timeouts and cancellation for decode
- unify behavior and messaging between TUI and tail (within their UX constraints)

Constraints:
- Performance is not a concern; we can favor simplicity and correctness.
- No backwards compatibility requirements; we can change internal APIs and output format.

This ticket coordinates with:
- ESP-13 (async decode) for Bubble Tea orchestration
- ESP-12 (pipeline extraction) so capture events are emitted by a shared component
- ESP-19 (remove python deps) to replace the decode backend

## Problem Statement

Current core dump behavior spans multiple layers and has risks:

1) **Decode happens at capture completion, inline**
- `esper/pkg/decode/coredump.go`: `PushLine` triggers `decodeOrRaw` on end marker.
- In TUI and tail, this happens in the serial processing loop.

2) **No timeouts**
- `exec.Command("python3", ...)` can hang indefinitely.
- On TUI, this can freeze the program at precisely the wrong time.

3) **Unbounded memory growth**
- `d.buf` accumulates base64 lines without caps; a malformed stream can exhaust memory.

4) **Artifact sprawl**
- Temp files accumulate with unclear cleanup guidance.

5) **Mixed UX concerns**
- Tail and TUI have different output surfaces, but should share the same capture/decode semantics.

## Proposed Solution

### 1) Split responsibilities: capture vs decode

Refactor `decode.CoreDumpDecoder` into two layers:

- **Capture state machine** (cheap, deterministic, no external processes)
  - reads lines, tracks prompt/start/end, buffers or streams base64 to an artifact
  - emits a “capture completed” result containing artifact references

- **Decode backend** (expensive, cancelable, timeout-enforced)
  - takes artifact + ELF path and produces a report (or failure)
  - backend is pluggable so we can replace python with Go later (ESP-19)

### 2) Capture output should be an artifact, not an in-memory blob

Instead of appending all base64 to `[]byte` in RAM:
- write captured base64 lines directly to a file as capture progresses

Proposed artifact location:
- `os.UserCacheDir()/esper/coredumps/`

Artifact format:
- raw base64 as text (`.b64`) OR decoded binary (`.bin`) depending on how we implement decoding later.

We keep an in-memory counter for size and a small rolling buffer only for display if needed.

### 3) Enforce hard limits and abort semantics

Add capture limits:
- `MaxCoreDumpBytes` (raw base64 bytes written)
- `MaxCoreDumpLines` (optional)

If limit exceeded:
- abort capture automatically
- emit an event: “Core dump capture aborted (size limit exceeded)”
- reset to idle state

Abort semantics:
- `Abort()` already exists; ensure it:
  - closes any open artifact writer
  - leaves a clear “partial capture saved” message if needed

### 4) Decode API and timeouts

Introduce a decoder interface:

```go
type CoreDumpArtifact struct {
  RawPath string
  RawBytes int
  StartedAt time.Time
  EndedAt time.Time
}

type CoreDumpDecodeResult struct {
  Artifact CoreDumpArtifact
  DecodedOK bool
  Report []byte
  Err error
}

type CoreDumpDecoder interface {
  Decode(ctx context.Context, elfPath string, art CoreDumpArtifact) CoreDumpDecodeResult
}
```

Time bounds:
- decoding must run under `ctx` with an explicit timeout, even if the backend is python.
- if decode times out:
  - return a result with a clear error
  - never block the TUI (ESP-13 ensures async orchestration)

### 5) TUI behavior: “capture blocks input, decode does not”

Current TUI behavior:
- capture mode disables input and shows a progress overlay.
- decode is currently done synchronously at end.

New behavior:
- capture mode remains blocking (it is a streaming acquisition step).
- decode is done in background:
  - show “captured; decoding…” toast and inspector event
  - when decode completes, show “decoded report” inspector event
  - allow user to copy/save raw/report from inspector detail overlay (existing UX)

### 6) Tail behavior

Tail mode has options:
- `CoreDumpAutoEnter`
- `CoreDumpMute`
- `NoCoreDumpDecode`

Proposed behavior after refactor:
- capture always produces an artifact (raw path).
- if decode enabled and `--elf` provided:
  - decode runs synchronously (acceptable) OR optionally asynchronously if we want consistent design (not required).
- mute behavior remains optional but is implemented consistently with capture state.

### 7) Implementation mapping (files/symbols)

Current hotspots:
- `esper/pkg/decode/coredump.go`
  - `CoreDumpDecoder.PushLine`
  - `decodeOrRaw` (python execution)
- `esper/pkg/monitor/monitor_view.go`
  - core dump capture handling in the `serialChunkMsg` path
  - abort via Ctrl-C in capture mode

After refactor:
- `decode/coredump.go` becomes:
  - capture state machine + artifact writer
  - no external process execution
- decode backend lives in:
  - `decode/coredump_python.go` (temporary, with timeouts) and/or
  - `decode/coredump_go.go` (ESP-19 target)

### 8) Tests

Add unit tests for:
- prompt/start/end detection
- artifact writing and byte counts
- abort behavior (manual abort and size-limit abort)
- “end marker produces capture-complete event”
- decode timeout behavior (if python backend still exists, use a stub backend for tests)

## Design Decisions

1) **Artifacts over RAM**
- Prevents memory exhaustion on corrupted streams and makes debugging easier.

2) **Capture and decode are separate**
- Capture is a state machine; decode is optional, expensive, and must be cancelable.

3) **Explicit limits**
- Stability is more important than “always capture everything”.

4) **UI never blocks on decode**
- This is non-negotiable for incident workflows (enforced via ESP-13 pattern).

## Alternatives Considered

1) Keep in-memory buffer but add a max size
- Rejected: still risks large memory spikes and is less robust than streaming to disk.

2) Keep decoding inside `PushLine`
- Rejected: mixes concerns and encourages blocking behavior.

## Implementation Plan

1) Refactor `decode.CoreDumpDecoder` into capture-only state machine with artifact writing.
2) Add capture limits and robust abort/cleanup.
3) Create a decode backend interface; implement a temporary python backend using `exec.CommandContext` + timeout (until ESP-19).
4) Update TUI and tail call sites to:
   - react to “capture completed” events
   - trigger decode via:
     - Bubble Tea cmds (TUI, ESP-13 style)
     - direct calls (tail) or a unified pipeline event handler (ESP-12)
5) Add unit tests.
6) Update ESP-11 deep audit mapping to mark core dump stability as addressed by ESP-18.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline:
   - `cd esper && go test ./... -count=1`
   - `cd esper && go vet ./...`
2) Refactor capture first (capture-only, artifact output), keep decode behavior disabled or stubbed; re-run baseline.
3) Add unit tests for capture markers, abort, and size limits; re-run baseline.
4) Add decode backend (with timeouts) and wire to:
   - tail (synchronous ok)
   - TUI (async via ESP-13 pattern)
5) Manual smoke (device) after each major stage:
   - trigger core dump and verify:
     - capture overlay progress
     - abort works (Ctrl-C)
     - artifact path is produced
     - decode either completes or times out cleanly (no hangs)

Ticket-specific invariants:
- No unbounded memory growth during capture.
- No indefinite hangs during decode (timeout required).
- TUI remains responsive outside capture mode (decode must not block `Update`).

## Open Questions

1) What should the default maximum core dump size be?
2) Should we keep raw base64 forever, or implement cleanup policy (age-based)?
3) Do we want to decode to a binary core file as an intermediate step (for future pure-Go decode)?

## References

- Current core dump implementation:
  - `esper/pkg/decode/coredump.go`
- Current TUI capture behavior:
  - `esper/pkg/monitor/monitor_view.go` (core dump capture + overlay)
- Related tickets:
  - ESP-13 async decode: `ttmp/2026/01/25/ESP-13-ASYNC-DECODE--make-decoding-async-bubble-tea-style/design-doc/01-specification.md`
  - ESP-12 shared pipeline: `ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md`
  - ESP-19 remove python: `ttmp/2026/01/25/ESP-19-REMOVE-PYTHON--remove-external-python-dependencies/design-doc/01-specification.md`
