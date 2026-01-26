---
Title: Follow-ups and refactor plan
Ticket: ESP-11-REVIEW-ESPER
Status: active
Topics:
    - tui
    - backend
    - tooling
    - console
    - serial
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/decode/coredump.go
      Note: Needs timeouts and buffer caps
    - Path: esper/pkg/decode/panic.go
      Note: Needs batching/caching and async handling
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Target of staged refactors (split and async decode)
    - Path: esper/pkg/tail/tail.go
      Note: Target of duplication removal (shared pipeline)
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T19:34:28.788402214-05:00
WhatFor: A staged refactor plan that turns the ESP-11 audit findings into concrete, testable work items with validation commands.
WhenToUse: Use when implementing refactors or reviewing PRs derived from the ESP-11 audit.
---


# Follow-ups and refactor plan

## Purpose

Turn the ESP-11 “food inspector” audit into a practical, staged refactor sequence that:
- reduces duplication (tail vs TUI, duplicated overlays/utilities),
- improves TUI responsiveness (async decode + timeouts),
- keeps behavior stable via frequent validation checkpoints.

## Environment Assumptions

- Go toolchain available (`go`), able to build and test `esper/`.
- Linux environment for `esper scan` and nickname resolution (current code is Linux-first).
- Optional (for decoding features):
  - Toolchain `addr2line` for the target (e.g. `xtensa-esp32s3-elf-addr2line`).
  - Python environment that provides:
    - `esptool` (for scan probe)
    - `esp_coredump` (for core dump decode)
- Hardware assumptions (for real-device manual tests):
  - Prefer USB Serial/JTAG console on ESP32-S3 boards to avoid UART pin contention with peripherals/protocol links.
  - Keep UART console disabled unless explicitly planned and pinned to known-safe pins.

## Commands

```bash
# 0) Baseline validation (before any refactor)
cd esper
go test ./... -count=1
go vet ./...

# Optional: run scan and confirm ports are found (Linux only)
go run ./cmd/esper scan --probe-esptool=false

# Optional: smoke the monitor and tail (requires a real device)
# go run ./cmd/esper --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*'
# go run ./cmd/esper tail --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' --timeout 10s
```

### Stage 1: Create a shared pipeline package (no behavior change)

Goal: One implementation of “serial bytes → lines → detectors/decoders → output + events”.

Suggested approach:
- Add `esper/pkg/pipeline/` (or similar) with:
  - stateful components: line splitter, gdb detector, coredump capture state, panic backtrace detection
  - a pure “push bytes” API that returns:
    - emitted log lines (as `[]byte` or `[]string`),
    - emitted “notice events” (structured),
    - requested “device writes” (e.g. “send enter”)
  - an explicit “idle flush” method to finalize tail partial line.

Validation checkpoint:
```bash
cd esper
go test ./... -count=1
```

### Stage 2: Refactor `tail` to use the shared pipeline

Goal: Delete duplicated logic between `runLoop` and `runLoopWithStdinRaw`, and converge on:
- shared pipeline core
- separate “I/O surfaces”:
  - output writers
  - optional stdin forwarding
  - safe serialization of device writes (auto-enter + stdin writes)

Validation checkpoint:
```bash
cd esper
go test ./... -count=1
go vet ./...
```

Manual smoke (device):
```bash
cd esper
go run ./cmd/esper tail --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' --timeout 10s
go run ./cmd/esper tail --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' --stdin-raw
```

### Stage 3: Refactor the TUI monitor to use the shared pipeline

Goal: Remove pipeline duplication in `monitor_view.go` while preserving UI semantics.

Validation checkpoint:
```bash
cd esper
go test ./... -count=1
```

### Stage 4: Make decoding asynchronous in the TUI (behavior improvement)

Goal: prevent UI freezes by moving external tool execution off the `Update` thread.

Rules of thumb:
- Use `tea.Cmd` to kick off decode work and return a message with results.
- Use `exec.CommandContext` with timeouts for:
  - `addr2line`
  - python `esp_coredump`
- Consider cancellation semantics on disconnect/quit:
  - decode tasks should respect the Bubble Tea context (or a per-session context).

Validation checkpoint:
```bash
cd esper
go test ./... -count=1
```

Manual smoke (device):
- Induce a backtrace and confirm:
  - logs keep flowing,
  - UI remains responsive,
  - inspector eventually shows decoded frames.
- Induce a core dump and confirm:
  - capture overlay shows progress,
  - UI remains responsive to abort (Ctrl-C),
  - report appears when ready, with a clear timeout error if decode exceeds limit.

### Stage 5: Delete small duplications and improve discoverability

Targets:
- Replace `reset_confirm_overlay.go` with instances of `confirm_overlay.go`.
- Consolidate ANSI utilities (`ansi_text.go`, `ansi_wrap.go`) into a single implementation with consistent escape handling.
- Move shared helpers (`min/max/padOrTrim/updateSingleLineField`) into a dedicated `ui_helpers.go`.
- Decide whether to:
  - implement “probe with esptool” in the TUI connect path, or
  - remove the toggle until implemented.

## Exit Criteria

- `cd esper && go test ./... -count=1` passes.
- Tail mode and TUI mode behave equivalently for:
  - I/W/E auto-color,
  - GDB stub notices,
  - core dump capture semantics (including auto-enter),
  - panic/backtrace decode (when toolchain present).
- TUI remains responsive during decode; no multi-second freezes or indefinite hangs during core dump completion.
- The duplicated code paths listed in the ESP-11 audit are removed or centralized.

## Notes

- The refactor plan is intentionally staged: do not do “pipeline extraction + async decode + UI split” in one PR if you want reviewable diffs.
- If you keep core dump decode enabled by default, enforce timeouts and buffer size caps; otherwise the monitor can hang or consume unbounded memory.
