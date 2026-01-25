---
Title: 'Go Serial Monitor (idf.py-compatible): Design + Implementation'
Ticket: ESP-01-SERIAL-MONITOR
Status: active
Topics:
    - serial
    - console
    - tooling
    - esp-idf
    - usb-serial-jtag
    - debugging
    - display
    - esp32s3
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/coredump.py
      Note: Core dump buffering/decoding behavior
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/gdbhelper.py
      Note: GDB stub detection + launch
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/output_helpers.py
      Note: ANSI constants + auto-color regex
    - Path: ../../../../../../../../../../.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/serial_handler.py
      Note: Coloring + line splitting + decoding triggers
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/tools/idf_monitor.py
      Note: Wrapper used by idf.py monitor
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T18:50:23.726040974-05:00
WhatFor: ""
WhenToUse: ""
---


# Go Serial Monitor (idf.py-compatible): Design + Implementation

## Executive Summary

We will build a Go reimplementation of an ESP-IDF serial monitor that is compatible with `idf.py monitor` in the behaviors that matter day-to-day:

- log coloring and terminal output semantics,
- detection/decoding workflows for panic backtraces, core dumps, and GDB stub events,
- resilience under chunked reads, invalid UTF-8, and reconnects,
- and maintaining a usable bidirectional serial console (keystrokes to device; host commands via a menu modality).

Phase 1 focuses on correctness and parity, not UI polish: a “pure terminal” monitor using Bubble Tea (and optionally Bubbles) as a minimal event-loop and rendering harness. Phase 2 reimplements the decoding pieces in Go (panic/core dump/gdb recognition + decoding), replacing any subprocess-backed helpers while keeping the monitor’s architecture and surface behavior stable.

## Problem Statement

### We need a Go-native monitor without losing `idf.py monitor` ergonomics

ESP-IDF’s Python monitor (`idf.py monitor` → `esp_idf_monitor`) already solves many hard problems (line buffering, color resets across partial lines, core dump buffering/decoding, GDB stub detection, reconnect behavior). Our broader serial-stream work (structured frames like screenshots, richer automation, embedding into other Go tools) is pulling us toward a Go implementation.

The constraint is compatibility: users have strong muscle memory and expectations from `idf.py monitor`. The goal is not to copy every flag immediately, but to match the important behavioral contract.

### “Compatibility” is a behavioral spec, not a feature checklist

When users say “idf.py compatible,” they typically mean:

1. **Colors**: ESP-IDF logs are colored reliably and do not leave the terminal “stuck” in a colored mode.
2. **Decoding hooks**:
   - panic sequences are recognized and backtraces are made actionable,
   - core dumps are recognized, buffered transactionally, and decoded to a report (or at least preserved),
   - GDB stub events are detected and can trigger a debugging workflow.
3. **Robustness**: the monitor doesn’t wedge on bad bytes, missing newlines, or reconnect churn.

This doc defines the compatibility requirements and an implementation plan in Go.

## Proposed Solution

### 1) Treat the monitor as a stream processor (bytes → records → sinks)

The monitor’s input is a byte stream. The core job is to convert that stream into typed records:

- `TextLine`: a complete line (or a finalized incomplete line),
- `TextFragment`: a partial line segment intended for immediate display,
- `ModeEvent`: “we are in core dump capture”, “gdb stub detected”, etc.

Each record is routed to one or more sinks (terminal output, log file, decoder modules). This separation is what allows us to add structured payloads (e.g., screenshots) later without turning the monitor into regex soup.

### 2) Architecture: event loop + explicit state machines

We implement three concurrent producers and a single deterministic consumer:

1. **Serial reader**: reads chunks from serial and emits `SerialChunk([]byte)` messages.
2. **Keyboard reader**: reads terminal input in raw mode and emits `Key` messages.
3. **Timer ticks**: periodic ticks for “finalize last line if idle”.
4. **Main loop**: consumes messages and updates parsing state, then renders/output side effects.

Bubble Tea is used mainly to provide a well-behaved message loop and a minimal renderer. We intentionally avoid committing early to a complex TUI; correctness comes first.

### 3) IDF-compatible behavior is implemented as modules

We isolate the compatibility logic into small modules with stable interfaces:

- `LineSplitter`: chunked bytes → `TextLine` + `TextFragment`, with an explicit tail buffer.
- `AutoColorer`: annotate records with “should color by level” and ensure reset discipline.
- `PanicDecoder` (transactional): detect, ingest, and produce a report.
- `CoreDumpDecoder` (transactional): detect start/end, buffer payload, decode to report.
- `GDBStubDetector`: detect `$T..#..` with checksum and emit an action event.

Phase 1 may implement decoding by invoking ESP-IDF tooling (subprocess) while preserving the same detection and buffering semantics. Phase 2 replaces the subprocess-backed decoders with Go-native implementations.

### 4) “Pure terminal” UI, minimal Bubble Tea / Bubbles

For Phase 1:

- render output as a streaming log to the terminal,
- forward keystrokes to the device,
- provide a small modal “menu key” (like `idf.py monitor`) to invoke host commands without contaminating device input.

If we use Bubbles, it should be minimal (e.g., a viewport with scrollback and a help overlay). We explicitly defer “proper TUI” features (panes, search, filtering UI).

### 5) Implementation Design (concrete)

This section turns the design into a plan you can implement without guessing.

#### 5.1 Data model

Think in two layers: *events* and *records*.

- Events are what the runtime delivers:
  - `SerialChunk{Bytes []byte, At time.Time}`
  - `Key{Bytes []byte}` (raw bytes/escape sequences)
  - `Tick{At time.Time}` (used to finalize idle tails)
  - `ConnState{Up bool, Err error}`

- Records are what the parser produces:
  - `TextFragment{Bytes []byte, HasNewline bool}`
  - `TextLine{Bytes []byte, Finalized bool}`
  - `DecodedReport{Kind: panic|coredump, Text []byte}`
  - `Action{Kind: launch_gdb|reset|toggle_logging|...}`

The key invariant: **serial parsing never directly prints**; it produces records. Printing happens in one place (renderer), which is where we guarantee ANSI reset discipline.

#### 5.2 Line splitting algorithm (IDF-compatible)

Maintain:

- `tail []byte` (incomplete last line),
- `lastDataAt time.Time` (used for idle finalization),
- `trailingColor bool` (presentation state; see below).

On each `SerialChunk(b)`:

1. Append `b` to a buffer (conceptually `tail + b`).
2. Split into lines with keepends semantics:
   - iterate bytes; each `\n` terminates a line (include it).
3. The suffix after the last newline becomes the new `tail`.
4. Emit `TextLine` records for complete lines; do not decode as UTF-8 yet.
5. Update `lastDataAt`.

On each `Tick`:

- if `tail != nil` and `now - lastDataAt > finalizeInterval`, emit `TextLine{Bytes: tail, Finalized: true}` and clear `tail`.

This mirrors the behavioral intent of `esp_idf_monitor`’s “incomplete tail + timer flush” approach.

#### 5.3 Coloring algorithm (IDF-compatible semantics, Go implementation)

There are two color streams:

1. **Device-emitted ANSI**: pass through unchanged.
2. **Monitor-inserted ANSI**: must always be balanced with a reset.

Operational rule: only insert colors when `--disable-auto-color` is false *and* the line matches the ESP-IDF log prefix heuristic.

Implementation approach:

- Define a byte-level matcher for the ESP-IDF log prefix, similar in spirit to `AUTO_COLOR_REGEX` in ESP-IDF monitor.
- If matched, determine level (`I/W/E`) and pick ANSI color sequence bytes.

Printing discipline:

- For a full line (newline present): print `color + lineStripped + reset + newline`.
- For a partial line (no newline): print `color + bytes` and set `trailingColor=true`.
- If `trailingColor` is true and the next emitted fragment includes a newline *but is not colored*, print `fragmentStripped + reset + newline` and clear `trailingColor`.

This is the non-obvious bit: **reset belongs to the moment the line ends**, not to the moment color starts.

#### 5.4 Decoding workflow state machines

For parity, treat panic and core dump as explicit modes that can temporarily mute normal output.

**Core dump mode**

State:

- `coreDumpState = idle|reading|done`
- `coreDumpBuf []byte`
- `outputMuted bool`

Transitions:

- detect `CORE DUMP START` → enter `reading`, mute output, clear buffer
- in `reading`: append each line (normalized, with CR stripped)
- detect `CORE DUMP END` → enter `done`, run decode, then return to `idle` only after printing the final end marker line

**Panic mode**

Similar structure, but detection is based on panic markers and stack dump phases.

Phase 1 decoding option: call the canonical toolchain as a subprocess (so the monitor remains “idf.py compatible” in output meaning even before we rewrite internals).

Phase 2 decoding option: parse and symbolicate in Go.

#### 5.5 GDB stub detection

GDB stop reasons can occur mid-stream and can be split across chunks. Keep a short `gdbTail` buffer (e.g., last ~6 bytes, following the ESP-IDF monitor approach) and scan `gdbTail + line` for the `$T..#..` pattern:

- if checksum validates, emit `Action{Kind: launch_gdb}` (or a structured event).
- launching GDB is a host-side action; it must temporarily stop serial reading or at least stop printing, then resume cleanly.

#### 5.6 Input/command model (menu key compatible)

Replicate the usability contract:

- Most keys go to the device verbatim.
- A “menu key” chord enters host command mode (default `Ctrl-T`), then the next key chooses an action (reset, toggle logging, etc.).

This is how we “offer serial capabilities” without injecting accidental control bytes into device-side REPLs or protocols.

#### 5.7 Go package layout (suggested)

Keep modules small and testable:

- `cmd/esm/main.go` (CLI wiring; “esp serial monitor”)
- `monitor/` (Bubble Tea model + message loop)
- `serialio/` (port open/reconnect; USB Serial/JTAG friendly defaults)
- `parse/` (line splitter + mode recognizers; pure functions where possible)
- `render/` (ANSI-safe printer; color discipline)
- `decode/` (panic/core dump decoders; Phase 1 subprocess wrappers, Phase 2 Go-native)

#### 5.8 Test plan (parser-first)

The test strategy is to treat the parser as a deterministic function over byte streams:

- Feed synthetic serial streams that interleave:
  - partial lines,
  - colored/uncolored logs,
  - invalid UTF-8,
  - core dump start/end blocks,
  - gdb stub sequences split across boundaries.
- Assert:
  - record boundaries are correct,
  - auto-color resets occur exactly at line ends,
  - transactional decoders buffer and resume as expected,
  - resynchronization never wedges (the parser always makes progress).

## Design Decisions

### D1: Compatibility-first, UI-later

We optimize for parity with the `idf.py monitor` experience rather than building a custom UI early. A correct but plain terminal tool beats a pretty tool that mishandles partial lines, color resets, or core dump capture.

### D2: Separate parsing from presentation

Color and formatting are presentation concerns. Parsing should produce records plus metadata; rendering decides how to print (and how to reset ANSI) without feeding presentation artifacts back into parsing decisions.

### D3: Default console transport is USB Serial/JTAG for ESP32-S3 projects

UART pins are often used for peripherals/protocol links. Defaulting to USB Serial/JTAG reduces the chance of corrupting non-console UART traffic.

### D4: Transactional decoders (panic/core dump) are explicit modes

Panic and core dump are multi-line payloads that should be buffered and processed with clear boundaries. Mode-based parsing (with resynchronization rules) is more robust than ad-hoc line matching.

## Alternatives Considered

### A1: Wrap or extend `esp_idf_monitor` instead of reimplementing

Pros:
- immediate parity,
- reuse a mature implementation.

Cons:
- core remains Python, which complicates deeper Go-based integrations and distribution.

We may still use ESP-IDF tooling via subprocess for decoding in Phase 1, but the monitor’s control plane and parsing pipeline should be Go-native.

### A2: Full TUI immediately

Pros:
- better UX early.

Cons:
- UI complexity tends to hide subtle stream bugs (incomplete-line timing, ANSI reset boundaries, resync behavior).

We defer a full TUI until stream semantics and decoders are stable.

## Implementation Plan

### Phase 1: Minimal Go monitor with IDF-compatible semantics

**1) Core I/O**

- Open port (support glob patterns like `/dev/serial/by-id/*`).
- Read serial in chunks (do not assume lines).
- Put terminal into raw mode; forward input to serial.
- Handle disconnect/reconnect; expose reset-on-connect as an explicit option.

**2) Line splitting + idle finalization**

- Maintain a tail buffer for the last incomplete line.
- Split with “keepends” behavior.
- If idle for a short interval, finalize the tail into a printable record.

**3) Colors**

- Implement auto-coloring heuristics compatible with ESP-IDF logs (I/W/E).
- Reset discipline across partial lines (never leave the terminal colored due to monitor-inserted codes).
- Provide `--disable-auto-color` to preserve device-emitted ANSI.

**4) Decoding hooks (subprocess parity allowed)**

- **Panic**: detect panic/stack-dump phases; buffer; invoke an ESP-IDF decoder subprocess; print report; resume.
- **Core dump**: detect start/end; buffer; decode to report via subprocess; resume.
- **GDB stub**: detect `$T..#..` with checksum; optionally launch GDB.

**5) CLI surface**

Start minimal, but align flag names and defaults with user expectations:

- `--port`, `--baud`
- `--elf` (repeatable), `--toolchain-prefix`
- `--timestamps`, `--timestamp-format`
- `--print-filter`
- `--disable-auto-color`
- `--no-reset` / `--reset-on-connect`

### Phase 2: Reimplement decoders in Go

**Panic decode**

- Parse ESP-IDF panic formats and extract backtrace addresses.
- Symbolicate addresses using either:
  - subprocess `addr2line` (pragmatic), or
  - DWARF parsing in Go (more ambitious, fewer external dependencies).

**Core dump decode**

- Implement base64 core dump extraction and decode to a usable report.
- Decide whether to embed an existing library or implement the minimum needed for parity.

**GDB stub workflow**

- Keep checksum-valid detection.
- Expand to structured handoff events (optional) for integration with automation tools.

## Open Questions

1. What is the minimum flag subset required to be “compatible enough” for our workflows?
2. Should host auto-color be on by default, or should we treat device-emitted ANSI as authoritative by default?
3. For Phase 2 symbolication, do we require pure-Go DWARF decoding, or is `addr2line` as a subprocess acceptable?
4. How do we want to surface “transactional modes” (panic/core dump) in the minimal terminal UI (status line vs log annotations)?

## References

- ESP-IDF monitor wrapper (this environment): `/home/manuel/esp/esp-idf-5.4.1/tools/idf_monitor.py`
- ESP-IDF monitor implementation (this environment): `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/idf_monitor.py`
- Serial handler (splitting, coloring, decode hooks): `/home/manuel/.espressif/python_env/idf5.4_py3.11_env/lib/python3.11/site-packages/esp_idf_monitor/base/serial_handler.py`
