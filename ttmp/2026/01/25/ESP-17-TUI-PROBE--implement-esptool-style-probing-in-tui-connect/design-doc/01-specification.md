---
Title: Specification
Ticket: ESP-17-TUI-PROBE
Status: active
Topics:
    - tui
    - esptool
    - serial
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/commands/scancmd/scan.go
      Note: CLI help and scan output fields for probe
    - Path: esper/pkg/monitor/messages.go
      Note: connectCmd currently ignores probeEsptool
    - Path: esper/pkg/monitor/port_picker.go
      Note: Probe toggle UI and connectParams wiring
    - Path: esper/pkg/scan/probe_esptool.go
      Note: Current (python) probe implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:42.44706489-05:00
WhatFor: Implement the Port Picker’s “Probe with esptool” toggle end-to-end in the TUI connect path, with explicit UX around resets and with a clear separation between probing UX and probing implementation.
WhenToUse: Use when you want the TUI to identify the connected chip/console reliably at connect-time (and optionally apply smarter defaults), instead of leaving probing as a CLI-only scan feature.
---


# Specification

## Executive Summary

The TUI Port Picker exposes a toggle:
- `esper/pkg/monitor/port_picker.go`: “Probe with esptool (may reset device)”

But the connect path explicitly does not implement probing:
- `esper/pkg/monitor/messages.go`: `connectCmd` comment says probeEsptool is intentionally not implemented yet.

This ticket implements probing in the TUI connect path with “proper UX”:
- explicit confirmation before doing anything that might reset the device
- clear reporting of probe results (chip name/description, MAC, USB mode, etc.)
- no protocol contamination of the monitor stream (probing must complete before monitor session starts, or be isolated)

Important clarification (responding to the user’s note):
- Today, the only “esptool probing” implementation in `esper` is **Python-based**:
  - `esper/pkg/scan/probe_esptool.go` runs `python3` and imports `esptool.cmds.detect_chip`.
- There is **no existing pure-Go port** of that functionality in this repo at the moment.

Therefore this ticket is structured as:
- **TUI UX + integration** (this ticket)
- **Probe engine implementation** (preferably pure Go, in ESP-19 “remove python deps”)

Constraints:
- Performance is not a concern.
- No backwards compatibility requirements.

## Problem Statement

1) **The UI currently lies**
- The Port Picker offers a probe toggle but it does nothing.

2) **Probing is operationally “dangerous”**
- Probing may reset the device, enter ROM download mode, or disrupt running firmware.
- The TUI must not do this silently.

3) **Probing must not corrupt the serial stream**
- The monitor expects to read application logs.
- Probing may send bytes to the port, toggle DTR/RTS, and read ROM responses.
- If done while monitoring is running, it can produce confusing output or break application-level protocols.

4) **Users want reliable identification**
- Scanning heuristics (VID/PID/product strings) are sometimes insufficient.
- A probe provides authoritative identity (chip type, features), similar to ESP-IDF tooling.

## Proposed Solution

### 1) Define what “probe” means in TUI

Probe goals (minimal, Phase 1):
- Determine chip identity (chip name + description).
- Determine whether secure download mode is enabled.
- Fetch:
  - MAC
  - USB mode (if available)
  - crystal frequency
  - feature list (if available)

These fields mirror the existing scan probe output shape:
- `esper/pkg/scan/probe_esptool.go`: `EsptoolProbeResult`.

### 2) Add a probe engine interface (so UX is decoupled from implementation)

Create a small interface in a new package (or inside scan for now):

```go
type ProbeOptions struct {
  Port string
  Baud int

  // Behavior flags analogous to esptool, if we keep them:
  ConnectMode string
  ConnectAttempts int
  After string // hard_reset | no_reset
}

type ProbeResult struct {
  OK bool
  ChipName string
  ChipDescription string
  SecureDownloadMode bool
  Features []string
  CrystalMHz int
  USBMode string
  MAC string
  Log string
  Error string
}

type Prober interface {
  Probe(ctx context.Context, opts ProbeOptions) (*ProbeResult, error)
}
```

Implementation choices:
- Short-term: adapt `scan.ProbeEsptool` into this interface (still python).
- Long-term (preferred): implement a pure-Go prober in ESP-19 and swap the implementation behind the interface.

No backwards compatibility requirement means we can rename “esptool” in UI later; for now, keep label but make it real.

### 3) TUI connect flow changes

Current connect flow:
- Port Picker emits `connectParams{probeEsptool: bool}`.
- `connectCmd` opens the port and returns `connectResultMsg` immediately.
- Monitor starts reading.

New flow (proposed):

**A. If probe is disabled**
- behavior unchanged: open port → connectResultMsg → start monitor.

**B. If probe is enabled**
1) Show a confirmation overlay *before* probing:
   - Title: “Probe device identity?”
   - Body: warn that probing may reset device / enter bootloader
   - Buttons: Cancel / Probe
2) If confirmed:
   - Run probe as a Bubble Tea command (async) so UI doesn’t freeze (depends on ESP-13 style).
   - During probe, show a “Connecting / Probing…” overlay or status line.
3) After probe completes:
   - show result summary in an overlay (or toast + inspector event)
   - then proceed to open monitor session (or, if we already opened the port for probing, reuse it).

Important: avoid “probe while monitoring”.

### 4) Port ownership / reuse strategy (critical for correctness)

We have two workable strategies; choose one explicitly:

#### Strategy 1 (simpler): probe before opening the monitor port
- Probe uses its own open/close cycle.
- After probe completes, we open the port again for monitor.
- Pros: simplest separation, avoids sharing a port handle across phases.
- Cons: doubles open/close churn; may have side-effects if device resets on open.

#### Strategy 2 (preferred long-term): open once, probe using the same port, then hand it to monitor
- `connectCmd` becomes a multi-step connection command:
  - open port
  - probe (reads/writes on that port)
  - return a connected `serialSession` plus probe result
- Pros: single open, easiest to guarantee “no other code reads the port during probe”.
- Cons: requires careful structuring of serial session initialization and ensuring monitor doesn’t start reading until probe is done.

Given “no backwards compatibility” and upcoming pipeline extraction, Strategy 2 aligns better with a clean “connection lifecycle” concept.

### 5) Where probe results live in the UI

Recommended UX:
- On successful probe:
  - add a monitor “event” (inspector) of kind `probe` with details
  - show a toast: “Probe OK: ESP32-S3 …”
- On failure:
  - show a blocking error overlay in port picker with a clear retry path
  - include captured probe logs if available (for debugging)

Also consider storing probe results in session state so they can be displayed in the monitor title/status bar:
- e.g. `Connected: /dev/... 115200 — Chip: ESP32-S3`

### 6) Relationship to “we ported esptool to Go?”

As of the current code:
- scan probe is python-based (`probe_esptool.go`).

This ticket explicitly creates a seam so that:
- TUI can provide probe UX now,
- probe engine can later be replaced by a pure-Go implementation (ESP-19) without rewriting UI again.

## Design Decisions

1) **Explicit confirmation for disruptive operations**
- Probing may reset devices; doing it silently is unacceptable.

2) **Probing must be serialized with monitor start**
- Either probe before monitor starts, or gate monitor start on probe completion.

3) **Separate UX from implementation**
- A `Prober` interface prevents UX changes from being tied to python or a specific probing backend.

## Alternatives Considered

1) Leave probing as a CLI-only scan feature
- Rejected: the TUI advertises it and users want it at connect time.

2) Probe after connecting (while monitor is running)
- Rejected: risks stream contamination and confusing output.

3) Implement probing as “just call esptool.py” in a shell
- Deferred: acceptable as an interim backend, but conflicts with the “remove python dependencies” goal (ESP-19).

## Implementation Plan

1) Add a `Prober` interface and an initial adapter around existing `scan.ProbeEsptool` (if needed).
2) Implement TUI confirmation overlay for probing (reuse `confirm_overlay.go`).
3) Update the connect flow to incorporate probing when enabled.
4) Display probe results (toast + inspector event + optional header).
5) Add tests:
   - “probe enabled requires confirmation”
   - “probe failure returns to port picker with error”
6) Coordinate with ESP-19 to replace the probe backend with a pure-Go implementation.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline:
   - `cd esper && go test ./... -count=1`
2) Implement probing UX gating (confirmation) first; run `go test` and ensure TUI still connects without probing.
3) Implement probe execution in the connect lifecycle (probe enabled path), re-run baseline.
4) Manual smoke (device):
   - with probe disabled: connect and ensure monitor behaves normally.
   - with probe enabled: confirm probe requires explicit confirmation; after probe completes, monitor starts and logs are readable.
5) If probe is still python-backed prior to ESP-19, ensure probe runs with timeouts and does not hang the UI (coordinate with ESP-13/ESP-18 patterns).

Ticket-specific invariants:
- Probing must never happen “silently” (explicit confirmation is required).
- Monitoring must not start reading serial until probe is complete (or probe must be isolated on the same port handle).
- Probe failure must not leave the user in a broken UI state (return to port picker with actionable error).

## Open Questions

1) Should TUI expose advanced probe options (`connect_mode`, `after`, attempts), or keep only a boolean?
2) Should probe results be persisted in the device registry (e.g. known chip type for nickname)?
3) On USB Serial/JTAG, what is the safest reset/probe behavior that avoids leaving device in bootloader?

## References

- TUI toggle and connect plumbing:
  - `esper/pkg/monitor/port_picker.go` (`probeEsptool` UI and `connectParams`)
  - `esper/pkg/monitor/messages.go` (`connectCmd`, probe not implemented)
- Existing (python) probe implementation:
  - `esper/pkg/scan/probe_esptool.go`
- Related tickets:
  - ESP-13 async decode style (for non-blocking probe UI): `ttmp/2026/01/25/ESP-13-ASYNC-DECODE--make-decoding-async-bubble-tea-style/design-doc/01-specification.md`
  - ESP-19 remove python deps (pure Go prober): `ttmp/2026/01/25/ESP-19-REMOVE-PYTHON--remove-external-python-dependencies/design-doc/01-specification.md`
