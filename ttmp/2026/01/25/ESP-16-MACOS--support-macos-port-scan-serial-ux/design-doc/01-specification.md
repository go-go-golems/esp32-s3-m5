---
Title: Specification
Ticket: ESP-16-MACOS
Status: active
Topics:
    - macos
    - serial
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/commands/scancmd/scan.go
      Note: CLI scan command wiring
    - Path: esper/pkg/devices/resolve.go
      Note: Nickname resolution uses ScanLinux today
    - Path: esper/pkg/monitor/messages.go
      Note: TUI scan command uses ScanLinux today
    - Path: esper/pkg/scan/scan_linux.go
      Note: Current Linux-only scan implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:41.991111824-05:00
WhatFor: Add macOS support to esper’s device discovery and nickname resolution by implementing port scanning and stable port selection heuristics on Darwin.
WhenToUse: Use when running esper on macOS and expecting `esper scan`, device nicknames, and TUI port picking to work (instead of being Linux-only).
---


# Specification

## Executive Summary

`esper` currently hardcodes Linux-only scanning and nickname resolution:
- `esper/pkg/scan/scan_linux.go` implements `ScanLinux` and errors on non-Linux.
- `esper/pkg/monitor/messages.go` uses `scan.ScanLinux` via `newScanPortsCmd`.
- `esper/pkg/devices/resolve.go` uses `scan.ScanLinux` to resolve nicknames to a currently connected device.

This ticket adds macOS support by:
- introducing an OS-agnostic scan entrypoint (`scan.Scan`), with OS-specific backends,
- implementing a Darwin scanner that enumerates serial ports and extracts USB metadata,
- maintaining a scoring system similar to Linux to “prefer the right port” (Espressif VID, stable-ish identifiers),
- updating nickname resolution and TUI port scanning to use the OS-agnostic scanner.

Constraints:
- Performance is not a concern.
- No backwards compatibility requirement (we can rename APIs, change field sets, and adjust output).

This ticket does **not** require implementing “esptool probing” on macOS; that is handled by ESP-17/ESP-19. It only needs “enumerate ports + pick the likely ESP console”.

## Problem Statement

On macOS:
- `esper scan` is unusable because `ScanLinux` returns an error.
- device nicknames cannot resolve to current ports because `ResolveNicknameToPort` uses `ScanLinux`.
- the TUI port picker can’t populate ports because it uses `newScanPortsCmd` which calls `ScanLinux`.

This is a structural limitation, not a bug:
- scanning is currently implemented with `/dev/tty*` globs and `/sys` walking.

We need a macOS implementation that:
- enumerates candidate serial ports,
- provides enough metadata (VID/PID, manufacturer/product/serial if available),
- chooses the best port by heuristics (or by stable-ish identity),
- integrates with existing UX (CLI scan output, nickname mapping).

## Proposed Solution

### 1) Replace `ScanLinux` as the primary public API

Introduce a new entrypoint in `esper/pkg/scan`:

```go
func Scan(ctx context.Context, opts Options) ([]Port, error)
```

Implementation:
- `scan_linux.go` becomes the Linux backend (can keep `scanLinux` unexported).
- add `scan_darwin.go` with `scanDarwin(ctx, opts)`.
- `scan.go` dispatches on `runtime.GOOS`.

No backwards compatibility requirement means we can:
- rename `ScanLinux` → `Scan` outright and update all call sites.

### 2) Implement Darwin scanning via Go serial enumerator

Use the go-serial enumerator:
- `go.bug.st/serial/enumerator` (part of the `go.bug.st/serial` ecosystem already used by `esper/pkg/serialio`).

High-level algorithm:
1) Get a list of detailed ports via enumerator.
2) Filter to plausible serial devices (on macOS typically `/dev/tty.*` and `/dev/cu.*`).
3) Populate `scan.Port` fields:
   - `Device`: preferred actual connect path (prefer `/dev/cu.*` when available)
   - `VID`, `PID` (when enumerator provides)
   - `Manufacturer`, `Product`, `Serial` (when available)
4) Score each port using an updated heuristic:
   - Espressif VID `303a` gets high score (same as Linux)
   - strings containing “Espressif” in product/manufacturer get bonus
   - prefer `/dev/cu.*` over `/dev/tty.*` for “connect path” (macOS convention)
5) Return sorted results (score desc, then stable tie-breaker).

### 3) Define “preferred path” semantics on macOS

Linux uses:
- `/dev/serial/by-id/...` as stable path.

macOS does not have `/dev/serial/by-id` by default. We define:
- **PreferredPath** for macOS = `/dev/cu.*` if it corresponds to the same physical device; otherwise the device path itself.

We explicitly accept that this is “stable-ish”, not perfectly stable. Stability is improved by the devices registry:
- users can store `PreferredPath` in registry entries (`devices.json`) when they know the correct port.

### 4) Update call sites to use the OS-agnostic scanner

Replace:
- `scan.ScanLinux(...)`
with:
- `scan.Scan(...)`

Files:
- `esper/pkg/monitor/messages.go` (`newScanPortsCmd`)
- `esper/pkg/commands/scancmd/scan.go` (CLI scan)
- `esper/pkg/devices/resolve.go` (nickname resolution)

### 5) Update `devices.ResolveNicknameToPort` semantics for macOS

Current logic matches the registry’s `USBSerial` to `scan.Port.Serial`.
On macOS, “serial number” availability varies:
- some USB serial devices expose serial number; some do not.

We keep the same matching strategy initially, but improve fallback options:
- If `PreferredPath` exists and is present on disk, use it (already implemented).
- Otherwise, attempt match in this order:
  1) exact serial number match (if enumerator provides serial)
  2) VID/PID + product/manufacturer match (best-effort)
  3) last resort: error with actionable guidance:
     - instruct user to set `preferred_path` explicitly (via TUI Device Manager or `esper devices set --preferred-path`).

No backwards compatibility means we can evolve the registry schema if needed (but that would be a separate ticket; keep this one minimal).

### 6) Testing approach

Since macOS scanning depends on local hardware, unit tests should focus on:
- scoring and preference logic (pure functions)
- path selection logic (`cu.*` preference)
- deterministic sorting

Implementation detail: extract scoring into a shared function that both Linux and macOS scanning backends use.

## Design Decisions

1) **OS-agnostic `scan.Scan` API**
- Makes the rest of the codebase portable; OS differences are localized.

2) **Prefer `/dev/cu.*` on macOS**
- Matches conventional advice: `cu.*` is for “call-up” devices and is typically the correct choice for outgoing connections.

3) **Keep registry matching conservative**
- Prefer explicit `PreferredPath` when available; treat serial-number matching as best-effort.

## Alternatives Considered

1) Use shell commands (`ioreg`, `system_profiler`) and parse output
- Rejected: adds brittle parsing and external dependencies. Go enumerator is cleaner.

2) Require users to always pass explicit `--port` on macOS
- Rejected: undermines the whole “scan + nickname” UX.

## Implementation Plan

1) Introduce `scan.Scan` and move Linux logic behind it.
2) Implement `scan_darwin.go` using `serial/enumerator`.
3) Refactor all call sites (`monitor`, `devices`, `scancmd`) to use `scan.Scan`.
4) Add unit tests for scoring and macOS “cu path preference” behavior.
5) Update docs (`esper/README.md`) to mention macOS support for scan/nicknames.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline (Linux): keep existing behavior stable while adding macOS paths:
   - `cd esper && go test ./... -count=1`
2) Add macOS implementation behind `scan.Scan` without changing Linux semantics; re-run tests.
3) Add unit tests for:
   - scoring determinism
   - preferred path selection (cu.* preference)
4) Manual smoke (on macOS machine):
   - `cd esper && go run ./cmd/esper scan --probe-esptool=false`
   - open TUI and ensure Port Picker populates ports
   - verify nickname resolution via `esper devices resolve --nickname <x>`

Ticket-specific invariants:
- Linux scanning output and scoring should remain stable unless explicitly adjusted.
- Nickname resolution should continue to prefer `PreferredPath` when it exists and is present.

## Open Questions

1) What should the registry key be on macOS if serial number is missing?
   - We may need to extend the registry schema (e.g. store location ID or a composite identity). That would be a follow-up if needed.

2) Do we want to “prefer USB Serial/JTAG” specifically on macOS?
   - Linux heuristics already strongly favor Espressif VID; on macOS we can keep the same.

## References

- Linux scanner (current):
  - `esper/pkg/scan/scan_linux.go`
- Nickname resolution (currently Linux-only):
  - `esper/pkg/devices/resolve.go`
- TUI scan command path:
  - `esper/pkg/monitor/messages.go` (`newScanPortsCmd`)
- ESP-11 review (portability note):
  - `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/design-doc/02-esper-code-review-deep-audit.md`
