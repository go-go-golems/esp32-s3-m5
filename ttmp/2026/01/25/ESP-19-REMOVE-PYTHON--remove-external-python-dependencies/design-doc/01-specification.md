---
Title: Specification
Ticket: ESP-19-REMOVE-PYTHON
Status: active
Topics:
    - tooling
    - backend
    - coredump
    - esptool
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/decode/coredump.go
      Note: Python dependency for core dump decoding
    - Path: esper/pkg/scan/probe_esptool.go
      Note: Python dependency for probing
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T21:09:43.196772525-05:00
WhatFor: Eliminate runtime dependencies on Python for esper’s probing and core dump decoding by replacing them with Go-native implementations (and/or toolchain-based non-Python fallbacks).
WhenToUse: Use when deploying esper in environments without Python/ESP-IDF venvs, or when aiming for a simpler, self-contained toolchain.
---


# Specification

## Executive Summary

`esper` currently relies on Python for two major features:

1) **Chip probing (“esptool probe”)**
- `esper/pkg/scan/probe_esptool.go` runs `python3 -c ...` and imports `esptool.cmds.detect_chip`.

2) **Core dump decoding**
- `esper/pkg/decode/coredump.go` runs `python3 -c ...` and imports `esp_coredump`.

These dependencies:
- complicate installation (requires ESP-IDF python environment or equivalent),
- introduce runtime failure modes (python import errors, hanging subprocesses),
- block UI responsiveness unless carefully managed (ESP-13 / ESP-18).

This ticket removes those python dependencies. Because we have **no backwards compatibility constraints**, we are free to:
- change CLI flags and output formats,
- change how probing and core dump decode results are presented,
- drop or degrade features temporarily if needed, as long as the end-state is clean and self-contained.

Preferred end-state:
- a pure-Go probing implementation used by both CLI scan and TUI connect probing (ESP-17).
- a non-Python core dump analysis pipeline, ideally pure Go; as an intermediate, a toolchain/GDB-based report is acceptable.

## Problem Statement

Python dependencies are currently a “hidden platform requirement”:

- Users need a working `python3` and correct imports for:
  - `esptool`
  - `esp_coredump`
- These usually live inside an ESP-IDF venv, which may not be present on all machines.
- The current core dump decode subprocess is not context-bound and can hang indefinitely.

Even when python is available, the architecture is brittle:
- probing and decoding happen via dynamic python snippets embedded in Go strings,
- error reporting and logs are not structured,
- behavior differs across machines depending on python environment.

## Proposed Solution

This ticket is split into two deliverables: probing and core dumps.

### A) Replace Python-based probing with Go-native probing

#### Current state
- `esper/pkg/scan/probe_esptool.go` uses python `esptool` to run `detect_chip`.
- CLI scan can probe (`--probe-esptool=true` default).
- TUI has a probe toggle but does not implement it yet (ESP-17 adds UX/integration).

#### Desired behavior
- Provide authoritative chip identity and key metadata without Python.

#### Proposed design
Introduce a new package (name options):
- `esper/pkg/probe` (generic)
- `esper/pkg/esptool` (if we intentionally model esptool semantics)
- `esper/pkg/rom` (if we implement ROM protocol directly)

Minimum viable prober:
- connect to device (potentially entering ROM bootloader via DTR/RTS manipulation)
- perform a “sync / chip detect” handshake
- fetch:
  - chip id / chip name (enough to distinguish ESP32/ESP32-S3/ESP32-C6, etc.)
  - MAC (if feasible)
  - optionally flash size/crystal (nice-to-have)

Implementation strategy:
- We implement the minimum subset of Espressif ROM serial protocol needed for “detect chip”, similar to esptool:
  - SLIP framing
  - command send/recv
  - checksum validation

Notes:
- Performance is not a concern, so a straightforward implementation is fine.
- No backwards compat means the API can be designed cleanly rather than matching esptool exactly.

Integration points:
- CLI: `esper/pkg/scan` should call the new Go prober instead of `python3`.
- TUI: ESP-17 should call the same prober (with proper confirmation UX).

#### Acceptance criteria (probing)
- `esper scan` works without python installed.
- `--probe-esptool` (or renamed `--probe`) returns structured identity fields.
- Probing errors are clear and do not leave the device in a broken state (best-effort reset after probing).

### B) Replace Python-based core dump decoding

#### Current state
- `esper/pkg/decode/coredump.go`:
  - captures base64 between markers
  - writes raw base64 to temp file
  - if ELF provided, runs python `esp_coredump` to generate a report

#### Desired behavior
- Capture must always work reliably (ESP-18 addresses stability and artifact policy).
- Decoding/report generation must not require python.

#### Proposed design (two-stage, with a clean seam)

**Stage 1 (must-have): capture artifact**
- Implemented/defined in ESP-18:
  - core dump capture writes base64 to an artifact path under cache dir
  - yields `CoreDumpArtifact` describing the capture

**Stage 2 (decode/report): non-Python backend**

We propose a pluggable decode interface (coordinated with ESP-18):

```go
type CoreDumpDecoder interface {
  Decode(ctx context.Context, elfPath string, art CoreDumpArtifact) CoreDumpDecodeResult
}
```

Backend options (choose one explicitly for the final state):

1) **Pure Go core dump parser/decoder**
- Pros: fully self-contained, no external tooling.
- Cons: large scope; requires implementing Espressif core dump formats, target architecture nuances, and report formatting.

2) **Toolchain/GDB-based report generation (non-Python)**
- Pros: leverages existing C toolchains (xtensa/riscv gdb) rather than python; likely simpler than full Go parser.
- Cons: still depends on external binaries; output format depends on gdb scripts; may be less “pretty” than esp_coredump.

Given “no backwards compat” and “performance not issue”, option (2) is an acceptable intermediate or even a permanent solution if it meets user needs.

#### Minimal non-Python report (acceptable baseline)
- Decode base64 → core binary file (or keep base64 but provide a decode step).
- Generate a report with:
  - basic metadata (size, saved paths)
  - a backtrace-like summary (using gdb in batch mode if possible)
  - clear instructions if full decode is not possible (missing toolchain)

#### Acceptance criteria (core dumps)
- Capturing core dump never requires python.
- Decoding/report generation never requires python.
- TUI never blocks indefinitely on decode (ESP-13 ensures async orchestration; ESP-18 enforces timeouts).

### C) Remove python usage from the codebase

After replacements:
- delete `esper/pkg/scan/probe_esptool.go` (or keep behind build tag if needed; but goal is removal)
- delete python invocation code from `esper/pkg/decode/coredump.go`
- update CLI help text to remove references to python tooling

### D) Update docs and UX

Update:
- `esper/README.md` to remove python prerequisites
- command help text in `scancmd/scan.go` to reflect the new probing implementation

Also update ESP-11 deep audit mapping to mark python dependency removal as addressed by ESP-19.

## Design Decisions

1) **Prefer Go-native implementations**
- Reduces installation friction and runtime variability.

2) **Expose clean interfaces**
- Allows multiple backends (especially for core dumps) while keeping call sites stable.

3) **No attempt to preserve output parity**
- No backwards compat requirement means we can redesign output formats for clarity and stability.

## Alternatives Considered

1) Keep python but vendor a venv
- Rejected: still python, still brittle, still heavy.

2) Shell out to `esptool.py` as a binary
- Rejected: still python and introduces dependency on external scripts.

## Implementation Plan

1) Implement Go-native prober package (minimal ROM protocol subset).
2) Replace `scan.ProbeEsptool` call sites to use the Go prober.
3) Coordinate with ESP-17 to use the same prober in the TUI connect path.
4) Implement non-Python core dump report backend:
   - either pure Go parser or gdb/toolchain-based report generation
5) Remove python invocations from `esper/pkg/decode/coredump.go`.
6) Remove python file(s) and update docs/help text.
7) Ensure tests pass; add unit tests for probe result parsing and core dump artifact/report plumbing.

## Regression / Feedback Loop (Use the ESP-11 Playbook)

Use the feedback loop described in:
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`

Recommended loop for this ticket:
1) Baseline (with python still present): `cd esper && go test ./... -count=1`
2) Replace one python dependency at a time:
   - first probing (`probe_esptool.go`) → Go prober
   - then core dump decode → non-python backend
   Re-run baseline after each replacement.
3) Add unit tests for:
   - probe handshake parsing (using recorded fixtures / simulated device responses)
   - core dump artifact → report pipeline (using stub backends if needed)
4) Manual smoke (device):
   - `esper scan` should probe without python installed
   - TUI probe (ESP-17) should also work without python
   - core dump decode should produce a report without python

Ticket-specific invariants:
- The tool must run without python installed at runtime (no hidden imports or subprocess calls).
- Probing and core dump decode failures must be explicit and actionable (not silent “OK=false” without context).

## Open Questions

1) What is the minimum acceptable core dump “decode report” content if we choose the gdb/toolchain approach?
2) Which chip families do we need to support first (ESP32-S3 vs ESP32-C6, etc.)?
3) How do we guarantee probing leaves the device in a usable state (reset behavior) across USB Serial/JTAG vs USB-UART bridges?

## References

- Python-based probing (to be removed):
  - `esper/pkg/scan/probe_esptool.go`
- Python-based core dump decode (to be removed/refactored):
  - `esper/pkg/decode/coredump.go`
- Related tickets:
  - ESP-17 TUI probing UX: `ttmp/2026/01/25/ESP-17-TUI-PROBE--implement-esptool-style-probing-in-tui-connect/design-doc/01-specification.md`
  - ESP-18 core dump stabilization: `ttmp/2026/01/25/ESP-18-COREDUMP-STABILIZE--cleanup-and-stabilize-core-dump-capture-decoding/design-doc/01-specification.md`
