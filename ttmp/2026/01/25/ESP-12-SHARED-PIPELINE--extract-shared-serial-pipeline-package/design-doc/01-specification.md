---
Title: Specification
Ticket: ESP-12-SHARED-PIPELINE
Status: active
Topics:
    - pipeline
    - backend
    - serial
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/decode/coredump.go
      Note: Core dump capture/decode primitive
    - Path: esper/pkg/decode/gdbstub.go
      Note: GDB stub detector
    - Path: esper/pkg/decode/panic.go
      Note: Backtrace decode primitive
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Current TUI pipeline implementation (duplicated)
    - Path: esper/pkg/parse/linesplitter.go
      Note: Line framing primitive used by both
    - Path: esper/pkg/render/autocolor.go
      Note: ESP-IDF auto-color insertion
    - Path: esper/pkg/tail/tail.go
      Note: Current non-TUI pipeline implementation (duplicated)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:39.965910625-05:00
WhatFor: Define and implement a shared, reusable serial-processing pipeline package so esper’s TUI monitor and non-TUI tail mode share one canonical implementation.
WhenToUse: Use when removing duplicated serial decode logic between `esper/pkg/tail` and `esper/pkg/monitor`, or when adding new decoding/event features that must behave identically in both modes.
---


# Specification

## Executive Summary

Extract a new Go package (proposed name: `esper/pkg/pipeline`) that encapsulates esper’s core serial processing logic:

- byte chunk ingestion
- newline framing (tail buffering + idle flush)
- ESP-IDF-ish auto-color insertion
- GDB stub detection
- core dump capture state machine
- panic/backtrace detection (and optional decode trigger)
- emission of:
  - rendered log lines (for display/printing),
  - “notices” (for stderr / inspector events),
  - optional “device write requests” (e.g. “send Enter to start core dump”),
  - structured events (for TUI inspector and future event export).

This eliminates duplicated pipeline logic currently spread across:
- `esper/pkg/tail/tail.go` (`runLoop` and `runLoopWithStdinRaw`)
- `esper/pkg/monitor/monitor_view.go` (the `serialChunkMsg` and `tickMsg` paths)

Constraints:
- **Performance is not a concern** (favor clarity and correctness).
- **No backwards compatibility requirements** (we can refactor internal APIs and behavior freely).

Primary deliverable: a self-contained pipeline API with tests, plus refactors of TUI + tail to use it.

## Problem Statement

`esper` has one conceptual “serial pipeline” but multiple implementations, which creates drift risk and makes changes expensive:

1) **Duplication between runtime modes**
- Tail mode implements a pipeline in `esper/pkg/tail/tail.go`.
- TUI mode implements a similar pipeline inside `esper/pkg/monitor/monitor_view.go`.

2) **Duplication within tail mode**
- `runLoop` and `runLoopWithStdinRaw` are near-copies, differing mostly in how they serialize writes to the port and forward stdin.

3) **Hard-to-test orchestration**
- The “interesting” logic is currently glued inside UI update loops, making it harder to unit-test pipeline invariants without dragging UI state along.

4) **Future features need a single choke point**
- Upcoming tickets (async decode, event export, remove python deps, ANSI correctness, macOS support) all benefit from one canonical pipeline implementation.

This ticket creates that shared choke point.

## Proposed Solution

### 1) Create new package: `esper/pkg/pipeline`

This package becomes the canonical home for “serial bytes → framed lines → detection/capture → emitted outputs/events”.

Proposed structure:

- `pipeline/pipeline.go`
  - `type Pipeline struct { ... }`
  - `type Config struct { ... }`
  - `type Output struct { ... }`
  - main methods: `PushChunk`, `FlushIdle`, `Reset`
- `pipeline/types.go`
  - event types, output types
- `pipeline/tests/*` (or plain `_test.go`)

The pipeline will *use* existing low-level components (initially) rather than reimplementing them:
- framing: `esper/pkg/parse.LineSplitter`
- auto-color: `esper/pkg/render.AutoColorer`
- gdb stub: `esper/pkg/decode.GDBStubDetector`
- coredump capture: `esper/pkg/decode.CoreDumpDecoder`
- panic decode/detect: `esper/pkg/decode.PanicDecoder` (but see separation note below)

### 2) Define a clean boundary: “pipeline orchestration” vs “I/O surfaces”

The pipeline should not:
- open serial ports
- write to stdout/stderr
- manage TUI viewport state
- forward stdin

Those remain in:
- `esper/pkg/serialio` (open)
- `esper/pkg/tail` (stdout/stderr/stin raw forwarding)
- `esper/pkg/monitor` (Bubble Tea UI, inspector, overlays)

The pipeline *can* emit “write requests” (bytes to send to device), but the *consumer* owns write serialization and actual writes.

### 3) Pipeline API (proposed)

#### 3.1 Config

```go
type Config struct {
    // Feature toggles (should map to existing tail flags / monitor behaviors).
    EnableAutoColor bool
    EnableGDBDetect bool
    EnableCoreDump  bool
    EnableBacktrace bool

    // For parity with current behavior.
    CoreDumpAutoEnter bool // request send "\n" when prompt detected
    CoreDumpMute      bool // suppress normal output while capturing

    // Decode config (may be unused until ESP-19 removes python).
    ElfPath         string
    ToolchainPrefix string
}
```

Notes:
- We deliberately use positive flags (`EnableX`) to avoid confusion; callers can map from existing negative flags (`NoX`) as needed.
- **No backwards compatibility**: callers can be updated accordingly.

#### 3.2 Outputs

Pipeline output is a list of items (ordered), representing “what happened because of this chunk/flush”.

```go
type OutputKind int
const (
    OutputLine OutputKind = iota      // bytes to display/print as log output
    OutputNotice                      // human-facing notice (stderr / toast)
    OutputDeviceWrite                 // request: bytes to send to device
    OutputEvent                        // structured event (inspector/export)
)

type Output struct {
    Kind OutputKind
    Line   []byte      // for OutputLine
    Notice string      // for OutputNotice
    Write  []byte      // for OutputDeviceWrite
    Event  Event       // for OutputEvent
}
```

#### 3.3 Events (structured)

The pipeline should emit events that are independent of any UI:

```go
type EventKind string

const (
    EventGDBStubDetected EventKind = "gdb_stub"
    EventCoreDumpPrompt  EventKind = "coredump_prompt"
    EventCoreDumpStart   EventKind = "coredump_start"
    EventCoreDumpEnd     EventKind = "coredump_end"
    EventBacktraceFound  EventKind = "backtrace_found"
    EventBacktraceDecoded EventKind = "backtrace_decoded" // optional
)

type Event struct {
    At   time.Time
    Kind EventKind
    Title string
    Body  string

    // Optional machine-usable fields for future (event export).
    Fields map[string]any
}
```

The exact contents can be evolved, since there is no backwards-compat requirement.

#### 3.4 Methods

```go
func New(cfg Config) *Pipeline

// PushChunk ingests raw serial bytes and returns ordered outputs.
func (p *Pipeline) PushChunk(chunk []byte) []Output

// FlushIdle forces the pipeline to flush a partial line tail (if any) by appending '\n'
// semantics. This replaces the duplicated “FinalizeTail after 250ms” logic.
func (p *Pipeline) FlushIdle(now time.Time) []Output

// Reset clears pipeline state (e.g. on disconnect).
func (p *Pipeline) Reset()
```

Important behavioral notes (parity with current code):
- The pipeline must support partial lines and “idle flush”:
  - Tail mode: see `esper/pkg/tail/tail.go` (250ms finalize tail)
  - TUI: see `tickMsg` path in `esper/pkg/monitor/monitor_view.go`
- The pipeline must support “core dump capture mutes normal output” when configured.
- The pipeline must maintain “auto-color trailing reset” correctness:
  - currently done in `render.AutoColorer` with `trailingColor` tracking.

### 4) Refactor call sites to use pipeline

#### 4.1 Tail mode

Refactor `esper/pkg/tail/tail.go`:
- Replace duplicated per-loop pipeline instantiations with one `pipeline.Pipeline`.
- Unify `runLoop` and `runLoopWithStdinRaw` around:
  - one serial read loop,
  - one pipeline feed point,
  - separate stdin forwarding if enabled,
  - one “device writes” serialization channel (already present in stdin-raw loop; make it universal).

Tail still owns:
- timestamps/port prefixing: `writePrefixedBytes`
- output sinks: stdout/stderr/logfile tee
- exit policy: context timeout, Ctrl-] behavior (stdin raw)

#### 4.2 TUI mode

Refactor `esper/pkg/monitor/monitor_view.go`:
- Replace the inline pipeline logic in `serialChunkMsg` and `tickMsg` with calls to pipeline methods.
- Map pipeline outputs to:
  - viewport `append` calls (for OutputLine and some OutputNotice if desired)
  - `monitorEvent` creation (for OutputEvent)
  - device writes: `m.session.port.Write` (or, ideally, a single write channel in monitor too)

Note: The “async decode” work is addressed by ESP-13, but pipeline extraction should still be designed to support async decode by emitting structured events (and/or decode requests) rather than doing blocking decode inside `PushChunk`.

### 5) Tests

Add pipeline unit tests that cover:

- newline framing across chunk boundaries
- idle flush behavior
- gdb stub detection across chunk boundaries
- core dump start/end capture and mute behavior
- backtrace detection trigger
- auto-color trailing reset behavior across partial lines

We can reuse existing tests as fixtures, and add new tests that feed byte sequences directly to `pipeline.Pipeline`.

## Design Decisions

1) **A single ordered output stream (`[]Output`)**
- Rationale: both TUI and tail need to process outputs in order and may choose to render/ignore some.

2) **Pipeline emits “device write requests” but does not write**
- Rationale: write serialization differs between runtime modes (tail raw stdin uses a channel; TUI currently writes directly).

3) **Pipeline emits structured events independent of UI**
- Rationale: the TUI inspector and the planned “event export” feature (ESP-21) can share the same event model.

4) **No performance tuning**
- Rationale: per requirements, clarity beats micro-optimizations. We keep the door open for later buffer reuse if needed.

## Alternatives Considered

1) **Keep duplication and “just be careful”**
- Rejected: behavior drift is inevitable and review cost grows superlinearly.

2) **Move everything into the monitor package**
- Rejected: tail mode needs the same behavior; coupling it to the TUI package is an architecture smell.

3) **Rewrite all low-level utilities at once**
- Rejected: unnecessary risk. Start by extracting orchestration and reusing existing pieces; later tickets can replace internals (e.g. python removal, ANSI correctness).

## Implementation Plan

1) Create `esper/pkg/pipeline` with the API described above.
2) Write unit tests for the pipeline (feed byte sequences, assert emitted outputs/events).
3) Refactor `esper/pkg/tail/tail.go` to use the pipeline:
   - unify loops
   - preserve CLI flags semantics (mapping to pipeline config)
4) Refactor `esper/pkg/monitor/monitor_view.go` to use the pipeline:
   - preserve UI semantics (follow, capture overlay, inspector)
5) Run `go test ./...` and keep UI tests passing.
6) Update the ESP-11 deep audit mapping to mark pipeline duplication as “addressed by ESP-12”.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline (before any edits):
   - `cd esper && go test ./... -count=1`
   - `cd esper && go vet ./...`
2) Make a small pipeline change (prefer one detector/feature at a time), then immediately re-run baseline commands.
3) Run targeted pipeline unit tests (new tests added in this ticket) to ensure byte-for-byte behavior stays stable.
4) Manual smoke on a real device (optional but strongly recommended after each major stage):
   - `cd esper && go run ./cmd/esper tail --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' --timeout 10s`
   - `cd esper && go run ./cmd/esper --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*'`
5) If anything changes unexpectedly:
   - treat it as a regression (fix pipeline logic or update tests/spec if the change is intentionally desired; there is no backwards compat requirement, but changes must be explicit).

Ticket-specific invariants to preserve (unless explicitly changed and documented):
- Auto-color behavior for I/W/E lines, including “partial line trailing reset” correctness.
- GDB stub detection notice semantics (detected across chunk boundaries).
- Core dump prompt/start/end detection semantics and mute behavior.
- Idle tail flush behavior (~250ms finalize tail semantics).
- TUI overlay routing invariants remain true (serial/tick msgs keep flowing even with overlays open).

## Open Questions

1) **Where should core dump “artifact saving” live?**
- Today: `decode.CoreDumpDecoder` writes temp file internally.
- Pipeline could:
  - keep that behavior, or
  - emit “raw base64 captured” and let a higher layer decide where to write.
- This decision should be coordinated with ESP-18 (coredump stabilize) and ESP-19 (remove python).

2) **Should backtrace decode occur inside pipeline?**
- For async decode (ESP-13), it is better if pipeline emits “backtrace found” and decode is performed asynchronously.

3) **Should pipeline expose “raw lines” vs “rendered lines”?**
- We propose emitting rendered lines (including auto-color) because both tail and monitor want that.
- Event export (ESP-21) might also want raw lines; we can emit both or store raw in event fields.

## References

- ESP-11 review (problem framing and duplication register):
  - `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/design-doc/02-esper-code-review-deep-audit.md`
- Current duplicated pipeline implementations:
  - `esper/pkg/tail/tail.go`
  - `esper/pkg/monitor/monitor_view.go`
- Low-level components intended to be orchestrated by the pipeline:
  - `esper/pkg/parse/linesplitter.go` (`LineSplitter`)
  - `esper/pkg/render/autocolor.go` (`AutoColorer`)
  - `esper/pkg/decode/gdbstub.go` (`GDBStubDetector`)
  - `esper/pkg/decode/coredump.go` (`CoreDumpDecoder`)
  - `esper/pkg/decode/panic.go` (`PanicDecoder`)
