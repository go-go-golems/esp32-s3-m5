---
Title: 'Esper tail mode: non-TUI serial pipeline streamer'
Ticket: ESP-02-ESPER-TUI
Status: active
Topics:
    - serial
    - console
    - ui
    - tooling
    - esp-idf
    - debugging
    - usb-serial-jtag
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/README.md
      Note: |-
        Documents  usage
        Documents esper tail usage
    - Path: esper/cmd/esper/main.go
      Note: |-
        Adds the  cobra subcommand and shared port nickname resolution
        Adds the esper tail cobra subcommand and shared port nickname resolution
    - Path: esper/pkg/decode/coredump.go
      Note: Core dump buffering/decoding behavior to reuse
    - Path: esper/pkg/decode/gdbstub.go
      Note: GDB stub detection behavior to reuse
    - Path: esper/pkg/decode/panic.go
      Note: Panic backtrace detection/symbolication behavior to reuse
    - Path: esper/pkg/parse/linesplitter.go
      Note: Line splitting and idle tail flush semantics
    - Path: esper/pkg/render/autocolor.go
      Note: Auto-color and ANSI reset discipline
    - Path: esper/pkg/tail/tail.go
      Note: Tail mode implementation (timeout
ExternalSources: []
Summary: Add `esper tail` (non-TUI) to stream serial output through esper's parsing/decoding/color pipeline, with configurable output flags and a --timeout.
LastUpdated: 2026-01-24T23:48:05.012954178-05:00
WhatFor: Enable quick non-interactive serial log capture (CI, scripts, remote sessions) while reusing esper’s decoding/color pipeline, without running the Bubble Tea TUI.
WhenToUse: Use when you want a plain stdout stream (optionally time-bounded) with ESP-IDF-like coloring and decoding, e.g. for log capture, piping, or quick smoke tests.
---




# Esper tail mode: non-TUI serial pipeline streamer

## Executive Summary

Add a new CLI mode `esper tail` that runs as a normal terminal program (no Bubble Tea UI) and streams the device’s serial output to stdout after passing through esper’s existing pipeline:

- line splitting with idle tail flush,
- ESP-IDF log auto-color with correct ANSI resets,
- panic backtrace detection + optional symbolication,
- core dump prompt/start/end detection + optional auto-enter and decode report,
- GDB stub detection notices.

The mode supports `--timeout` for predictable exit (useful in scripts/CI) and provides multiple flags to control output behavior (color, prefixes, decoding toggles, tee-to-file).

## Problem Statement

The Bubble Tea TUI is not always suitable:

- Some environments want a plain stream for piping to other tools (`grep`, `tee`, log collectors).
- CI/smoke tests need deterministic exit (`--timeout`) and stable output.
- Some users want esper’s “value add” (color + decoding + core dump handling) without committing to the full-screen UI.

We need a non-TUI mode that reuses the same parsing/decoding pipeline so behavior stays consistent with the TUI monitor.

## Proposed Solution

### CLI shape

Add a subcommand:

- `esper tail --port <path> [--baud 115200] [--timeout 5s] [output flags...]`

This avoids changing the default behavior of `esper` (which currently runs the TUI monitor).

### Pipeline behavior (bytes → lines → events → output)

The tail mode processes data in this order (mirrors existing monitor logic):

1. Read serial bytes in chunks (e.g., 4096 bytes).
2. Feed into a line splitter that yields complete `\n`-terminated lines and keeps an internal tail buffer.
3. For each line:
   - Core dump state machine:
     - detect prompt/start/end
     - optionally auto-send Enter on prompt
     - optionally decode core dump to a report (requires `--elf`)
     - optionally mute normal output while core dump capture is in progress
   - Panic backtrace detection:
     - if `Backtrace:` line: emit decoded report lines (requires `--elf` and `--toolchain-prefix`)
     - or optionally emit “PC addresses only”
   - GDB stub detection:
     - detect `$T..#..` and emit a notice line
   - Auto-color ESP-IDF log prefixes I/W/E:
     - insert ANSI color sequences at the start of matching lines
     - ensure resets are correct across partial lines
4. Idle tail flush:
   - if no data arrives for a short interval (e.g. 250ms), finalize the tail and print it with `\n`.

### Flags (“bunch of flags”)

#### Required/common flags

- `--port` (string): serial port path (or glob like `/dev/serial/by-id/*`).
- `--baud` (int, default 115200).
- `--timeout` (duration, default 0): exit after this duration; `0` means run until Ctrl-C.
- `--stdin-raw` (bool): forward stdin to the device as raw bytes (bidirectional, no TUI). Exit on `Ctrl-]` always.

#### Decoding/config flags

- `--elf` (string): ELF path for decoding (optional).
- `--toolchain-prefix` (string): toolchain prefix for `addr2line` (optional).
- `--no-autocolor` (bool): disable monitor-inserted ESP-IDF auto-color.
- `--no-backtrace` (bool): suppress any panic/backtrace decoded output (still prints the original line).
- `--no-gdb` (bool): suppress GDB stub detection notices.
- `--no-coredump` (bool): disable core dump state machine entirely (prints lines normally).
- `--coredump-auto-enter` (bool, default true): when the core dump prompt appears, auto-send Enter (ESP-IDF parity).
- `--coredump-mute` (bool, default true): mute normal output while core dump capture is in progress.
- `--no-coredump-decode` (bool): never attempt to decode core dumps even if `--elf` is provided.

#### Output formatting flags

- `--timestamps` (bool): prefix each output line with `HH:MM:SS`.
- `--prefix-port` (bool): prefix each output line with the resolved port path.
- `--log-file` (string): tee output to a file (in addition to stdout).
- `--log-append` (bool): append to log file instead of truncating.
- `--no-stdout` (bool): write only to `--log-file` (useful for background capture).

Notes:
- Prefixing is applied *before* adding auto-color so the prefix does not get colored by inserted ANSI sequences.
- The YAML in the UX spec is a guideline; tail mode is intentionally not a TUI widget tree.
- Raw stdin mode:
  - When `--stdin-raw` is enabled and stdin is a TTY, esper puts stdin into raw mode and forwards bytes directly to the device.
  - `Ctrl-]` (0x1d) is intercepted and always exits the program (even in raw mode).

## Design Decisions

1. **Subcommand `tail` instead of flag on root**: keeps existing CLI behavior stable and makes usage explicit.
2. **Reuse existing decoder/state machines**: guarantees parity and reduces drift between TUI and non-TUI paths.
3. **Duration-based `--timeout`**: predictable for scripts; separate from “idle timeout” semantics.
4. **Prefix-before-autocolor**: avoids coloring timestamps/prefixes unintentionally.
5. **Raw, byte-preserving stdin forwarding**: supports `esp_console` REPL and interactive device sessions without a full-screen UI.
6. **Ctrl-] as unconditional escape hatch**: mirrors common serial tools and remains reliable in raw mode.

## Alternatives Considered

1. **Replace default monitor with tail when stdout isn’t a TTY**: convenient but surprising; changes behavior implicitly.
2. **Add tail as a flag on the root command**: increases complexity of root behavior and help text; harder to reason about.
3. **Implement as Bubble Tea without alt-screen**: still brings TUI event loop/TTY constraints; not suitable for piping.

## Implementation Plan

1. Add `esper tail` Cobra subcommand (new command file or add to `cmd/esper/main.go`).
2. Implement a `pkg/tail` package that runs the loop and writes to `io.Writer`s.
3. Thread flags into a `tail.Config` struct.
4. Add a small “writer mux” (stdout + optional file).
5. Validate:
   - `go test ./...` in the `esper/` repo
   - run against `esp32s3-test` firmware with `--timeout 5s` and ensure exit.
6. Update ticket tasks and diary; relate code files to this doc.

## Open Questions

1. Should `--timeout` also perform a “final flush” of tail buffer before exit? (Proposed: yes.)
2. Should the core dump prompt auto-enter default be `true` (ESP-IDF parity) or `false` (safer for unattended scripts)?
3. Do we want a `--raw` mode that bypasses all pipeline transforms? (Not requested, but could be useful.)
4. Should we add an alternative `--stdin-line` mode that translates Enter to `\r\n` (more REPL-friendly) in addition to `--stdin-raw`? (Not requested yet.)

## References

- `esper/pkg/decode/*` (core dump, panic, gdb stub)
- `esper/pkg/render/autocolor.go` (ANSI auto-color reset behavior)
- `esper/pkg/parse/linesplitter.go` (tail buffering semantics)
- Ticket decomposition doc: `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/design-doc/02-esper-tui-bubble-tea-model-message-decomposition.md`
