---
Title: Specification
Ticket: ESP-13-ASYNC-DECODE
Status: active
Topics:
    - tui
    - bubbletea
    - backend
    - serial
    - coredump
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/decode/coredump.go
      Note: python decode without context/timeout
    - Path: esper/pkg/decode/panic.go
      Note: addr2line subprocess decode
    - Path: esper/pkg/monitor/monitor.go
      Note: TUI program context (tea.WithContext)
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Blocking decode occurs inside monitorModel.Update
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:40.403355525-05:00
WhatFor: Make expensive decoding work non-blocking in the Bubble Tea TUI, using idiomatic Bubble Tea command/message patterns, with cancellation and stale-result protection.
WhenToUse: Use when eliminating UI freezes caused by addr2line and core dump decode work currently performed inside `monitorModel.Update`.
---


# Specification

## Executive Summary

The TUI currently performs decoding work synchronously inside Bubble Tea’s `Update` loop:
- backtrace decoding invokes `addr2line` (subprocess) via `decode.PanicDecoder.DecodeBacktraceLine` in `esper/pkg/decode/panic.go`
- core dump decode invokes `python3`/`esp_coredump` via `decode.CoreDumpDecoder.decodeOrRaw` in `esper/pkg/decode/coredump.go`

This can freeze the UI for seconds or indefinitely (core dump decode currently lacks a context/timeout).

This ticket makes decoding **asynchronous and cancelable** using a “proper Bubble Tea style”:
- detection remains on the `Update` path,
- expensive work is executed in `tea.Cmd` functions,
- results are delivered back to `Update` as typed messages,
- a per-session context and monotonic task IDs prevent stale results from mutating state after disconnect/reconnect.

Constraints:
- Performance is not a concern (we can do simple, clear implementations).
- No backwards compatibility is required (we can change UI behavior and internals freely).

This ticket focuses on **TUI orchestration**. It coordinates with:
- ESP-12 (shared pipeline extraction): pipeline should eventually emit “decode needed” events.
- ESP-18 (coredump stabilization): timeouts/buffer caps and artifact policy.
- ESP-19 (remove python deps): decode implementation may change, but async orchestration remains the same.

## Problem Statement

Bubble Tea’s `Update` must remain responsive: it should not run long blocking operations.

Current issues:

1) **Backtrace decode blocks `Update`**
- In `esper/pkg/monitor/monitor_view.go` (`monitorModel.Update`, `serialChunkMsg` path), code calls:
  - `m.panic.DecodeBacktraceLine(line)`
- When configured with ELF + toolchain prefix, this runs multiple `addr2line` subprocesses (one per PC).

2) **Core dump decode blocks `Update` and can hang indefinitely**
- In `esper/pkg/decode/coredump.go`, `decodeOrRaw` runs:
  - `exec.Command("python3", "-c", py)` (no context/timeout).
- In the TUI, this is triggered at core dump end marker while processing serial lines.

3) **Blocking decode harms incident UX**
- Core dumps and panics are “incident moments” where the operator needs an interactive UI (scroll/search/copy/save), not a frozen one.

4) **No structured cancellation**
- Disconnecting or quitting should cancel outstanding decode work; today, decode runs to completion regardless.

## Proposed Solution

### 1) Introduce an async decode subsystem inside `monitorModel`

Add minimal state to `monitorModel` (currently in `esper/pkg/monitor/monitor_view.go`; after ESP-14 split it can move to `monitor_decode.go`):

- a **session generation** integer (`decodeGen`) incremented on each connect/disconnect
- a **monotonic task id** counter (`decodeTaskID`)
- optional “in-flight tasks” bookkeeping if we want UI status (not required)

Conceptual example:

```go
type decodeTaskKind int
const (
  decodeTaskBacktrace decodeTaskKind = iota
  decodeTaskCoreDump
)

type decodeTaskRef struct {
  gen  int // session generation
  id   int // task id
  kind decodeTaskKind
}
```

### 2) Define typed result messages

Add Bubble Tea message types that carry:
- `(gen, id)` to ignore stale results
- success/failure
- decoded payload (or artifact references)

Conceptual examples:

```go
type backtraceDecodeResultMsg struct {
  gen int
  id  int
  rawLine string
  decoded []byte
  err error
}

type coredumpDecodeResultMsg struct {
  gen int
  id  int
  result decode.CoreDumpResult // may change after ESP-18/ESP-19
  report []byte
  err error
}
```

### 3) Spawn decoding via `tea.Cmd` (idiomatic Bubble Tea)

Bubble Tea “proper style”:
- `Update` detects “decode needed” and returns a `tea.Cmd`.
- The `tea.Cmd` performs the blocking work and returns a typed `tea.Msg`.
- The model updates state only when it receives that message in `Update`.

Backtrace decode cmd (conceptual; details may change after pipeline extraction):

```go
func (m *monitorModel) decodeBacktraceCmd(gen, id int, rawLine []byte) tea.Cmd {
  lineCopy := append([]byte{}, rawLine...)
  dec := m.panic // copy decoder config (ElfPath/ToolchainPrefix)
  return func() tea.Msg {
    decoded, ok := dec.DecodeBacktraceLine(lineCopy)
    if !ok {
      return backtraceDecodeResultMsg{gen: gen, id: id}
    }
    raw := strings.TrimSpace(string(bytes.TrimRight(lineCopy, \"\\r\\n\")))
    return backtraceDecodeResultMsg{gen: gen, id: id, rawLine: raw, decoded: decoded}
  }
}
```

### 4) Cancellation and stale-result protection

We need deterministic behavior on disconnect/reconnect:

- `monitor.Run` uses `tea.WithContext(ctx)`; we can derive a per-session context from it.
- On connect: `sessionCtx, cancel := context.WithCancel(m.ctx)`; store `cancel` on the model.
- On disconnect or new connect: call `cancel()` and bump `decodeGen`.

Decode cmds should:
- prefer `exec.CommandContext(sessionCtx, ...)` for subprocesses,
- return a result message even on cancellation (or return nil), but `Update` must ignore stale results using `gen`.

Even if a decode implementation doesn’t respect cancellation (e.g. current python call), the `(gen, id)` guard prevents stale updates.

### 5) UI semantics (no backwards compatibility)

We can improve clarity by making “decode pending” explicit:

- On backtrace detection:
  - append a short notice line: `--- Backtrace detected; decoding in background...`
  - add an inspector event “Backtrace decode pending”
  - on completion, add event “Backtrace decoded”; optionally append decoded frames to viewport (or keep inspector-only).

- On core dump completion:
  - append: `--- Core dump captured; decoding in background...`
  - add inspector event with saved path/size immediately
  - on decode completion, add inspector event with report; keep viewport clean (do not dump full report).

### 6) Timeouts (non-negotiable)

This ticket should enforce “no unbounded hangs”:
- `addr2line`: wrap in `exec.CommandContext` with generous timeout
- core dump decode: must use `CommandContext` with timeout even before ESP-19 removes python

ESP-18/ESP-19 can further refine decode internals, but this ticket ensures UI will not freeze indefinitely.

### 7) Code changes (as-is)

Primary touch points:
- `esper/pkg/monitor/monitor_view.go`:
  - replace direct calls to decode work with cmd spawn + result msg handlers
  - add state fields for generation/task ids
- `esper/pkg/decode/coredump.go`:
  - minimal safety change: use `exec.CommandContext` with timeout (until replaced by ESP-19)
- optionally `esper/pkg/decode/panic.go`:
  - add a context-aware decode path and/or batching (may also be handled under ESP-19)

## Design Decisions

1) **Cmd/Msg discipline**
- No background goroutines mutate state; only `Update` does.

2) **Generation IDs**
- Cheap and reliable stale-result protection.

3) **Session-scoped cancellation**
- Makes disconnect behavior deterministic and avoids resource leaks.

4) **Clarity over parity**
- We will not preserve “exact timing” of when decoded text appears; the UI must remain responsive.

## Alternatives Considered

1) Spawn goroutines directly from `Update` and mutate model state
- Rejected: race-prone and not idiomatic Bubble Tea.

2) Only async core dump decode
- Rejected: addr2line decode can also be slow; solve the pattern once.

3) Do decode in the pipeline (ESP-12)
- Rejected: pipeline should stay cheap and deterministic; expensive work should be explicit and cancelable.

## Implementation Plan

1) Add `decodeGen` and `decodeTaskID` fields to `monitorModel`.
2) Add result message types and handlers for:
   - backtrace decode
   - core dump decode
3) On detection paths, spawn decode cmds and emit “pending decode” notices/events.
4) Add cancellation and generation bumping on disconnect/reconnect.
5) Add timeouts to subprocess calls (addr2line, python if still used).
6) Add/extend tests:
   - “ignore stale decode result after disconnect”
   - “decode result produces inspector event”

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline:
   - `cd esper && go test ./... -count=1`
   - `cd esper && go vet ./...`
2) Implement async decode for **one** path first (suggested: backtrace decode), re-run baseline.
3) Add/extend tests that assert:
   - decode result messages are ignored after disconnect (generation mismatch)
   - decode work is initiated via `tea.Cmd` (not inline)
4) Manual smoke with a device:
   - induce a backtrace and confirm:
     - UI remains responsive during decode (host mode navigation/search still works)
     - decoded results eventually appear (inspector and/or viewport, per the chosen UX)
   - induce a core dump and confirm:
     - capture overlay behaves as before
     - decode happens “in background” and does not freeze the UI
     - timeouts are honored (no indefinite hangs)
5) If behavior changes (e.g. decoded frames no longer appended to viewport), document the change in the ticket changelog/spec; no backwards compat is required, but changes must be explicit.

Ticket-specific invariants to preserve (unless explicitly changed):
- Disconnect/reconnect must not apply stale decode results to a new session.
- Core dump capture abort (Ctrl-C during capture) must remain reliable.
- The serial read loop must continue while overlays are open (existing overlay routing invariant).

## Open Questions

1) Should decoded backtrace text be appended to viewport or inspector-only?
2) Should decode tasks be serialized (single worker) or allowed concurrently?
3) How should decode failures surface (toast vs log line vs inspector)?

## References

- Current blocking decode call sites:
  - `esper/pkg/monitor/monitor_view.go` (`serialChunkMsg` handler)
  - `esper/pkg/decode/panic.go` (`addr2line`)
  - `esper/pkg/decode/coredump.go` (python decode)
- Related tickets:
  - ESP-12 shared pipeline: `ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md`
  - ESP-18 coredump stabilize: `ttmp/2026/01/25/ESP-18-COREDUMP-STABILIZE--cleanup-and-stabilize-core-dump-capture-decoding/design-doc/01-specification.md`
  - ESP-19 remove python: `ttmp/2026/01/25/ESP-19-REMOVE-PYTHON--remove-external-python-dependencies/design-doc/01-specification.md`
