---
Title: Esper architecture (big picture)
Ticket: ESP-11-REVIEW-ESPER
Status: active
Topics:
    - tui
    - backend
    - tooling
    - console
    - serial
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/README.md
      Note: High-level goals and CLI usage
    - Path: esper/cmd/esper/main.go
      Note: CLI wiring and port/nickname resolution
    - Path: esper/pkg/monitor/app_model.go
      Note: TUI screen router + overlay routing
    - Path: esper/pkg/monitor/monitor_view.go
      Note: TUI monitor core (pipeline + rendering)
    - Path: esper/pkg/tail/tail.go
      Note: Non-TUI tail pipeline
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T19:34:25.971854603-05:00
WhatFor: A big-picture architecture map of the esper CLI + serial monitor (TUI and non-TUI), including component responsibilities and message/data flow.
WhenToUse: Use when onboarding, planning refactors, or reviewing how esper components interact (CLI, scanning/registry, serial I/O, decoding pipeline, and the Bubble Tea TUI).
---


# Esper architecture (big picture)

## Executive Summary

`esper` is a Go-based, ESP-IDF-compatible serial monitor and utilities suite.

At a high level, it consists of:
- A Cobra CLI entrypoint (`esper/cmd/esper/main.go`) that exposes:
  - the TUI monitor (default command),
  - a non-TUI “tail” mode (`esper tail`) that streams processed output to stdout,
  - a Linux serial port scanner (`esper scan`) with optional esptool probing,
  - a per-user device registry (`esper devices ...`) to map USB serial strings → human nicknames.
- A shared “serial → line splitter → decoder/color pipeline” that exists today in two mostly-parallel implementations:
  - `pkg/tail` (non-TUI) and
  - `pkg/monitor` (Bubble Tea TUI).
- A Bubble Tea application (`pkg/monitor`) built as a “screen router” (`appModel`) + three screens:
  - Port Picker (select a port + basic config)
  - Monitor (serial output + host/device modes + search/filter/inspector)
  - Device Manager (CRUD for the devices registry, with online/offline signal)
  … plus modal overlays for help, command palette, filter editor, confirm dialogs, and inspector detail.

The primary architectural “shape” of the app is event-driven:
- CLI wires configuration and starts either (a) TUI program or (b) tail loop.
- Serial bytes are read in chunks, split into newline-terminated lines, and passed through decoding stages:
  - ESP-IDF auto-coloring (I/W/E prefix)
  - GDB stub detection
  - Core dump capture + optional decode
  - Panic backtrace decode (addr2line)
- The TUI additionally turns some decoder observations into structured “events” surfaced via an Inspector panel and detail overlays.

This document describes the system as it exists (“as-is architecture”), and identifies the main boundaries where refactoring could simplify reuse and reduce duplication (see also the deep audit and follow-up plan).

## Problem Statement

`esper` aims to replicate key behaviors of ESP-IDF’s monitor tooling while providing a terminal-native, minimal TUI that works well with ESP32-S3 projects, especially when the console is on USB Serial/JTAG (UART pins are commonly repurposed for peripherals and protocol traffic).

The software needs to:
- Make it easy to pick the right serial port (prefer stable `/dev/serial/by-id/...` paths and Espressif VID/PID signals).
- Stream serial output with ESP-IDF-ish conveniences:
  - auto-color for I/W/E logs,
  - detection/handling of panic backtraces, core dumps, and GDB stub.
- Provide both:
  - a high-bandwidth “device mode” for sending lines to the device and “following” output, and
  - a “host mode” for scrolling/searching/filtering/analyzing output without disrupting follow behavior.
- Persist minimal per-user state (device nicknames and optional preferred paths).

The postmortem/review angle for ESP-11 is: the app works, but the implementation has grown organically across the ESP-02..ESP-09 arc, and now needs an architectural map plus an audit for duplication, cohesion, and maintainability.

## Proposed Solution

This section describes the architecture “as built”, broken down by major components and their interactions.

### Top-level directory structure (within the `esper` Go module)

- `esper/cmd/esper/`
  - CLI entrypoint (Cobra root command) and subcommands wiring.
- `esper/pkg/commands/`
  - `scancmd`: Glazed-based scan command, bridged into Cobra.
  - `devicescmd`: Devices registry commands (`devices list`, `devices set`, etc.).
- `esper/pkg/scan/`
  - Linux port scanning + scoring + optional esptool probing.
- `esper/pkg/devices/`
  - Per-user registry: config path, JSON load/save, nickname resolution.
- `esper/pkg/serialio/`
  - Serial port open helper + glob handling.
- `esper/pkg/parse/`
  - Low-level line splitting (bytes → newline-terminated frames).
- `esper/pkg/render/`
  - Auto-coloring utilities and ANSI constants.
- `esper/pkg/decode/`
  - Decoders/detectors:
    - panic backtrace decode via `addr2line`
    - core dump capture + decode via Python `esp_coredump`
    - GDB stub detection
- `esper/pkg/tail/`
  - Non-TUI streaming loop with the decoding pipeline applied to serial output.
- `esper/pkg/monitor/`
  - The Bubble Tea TUI application and all screens/overlays.

### High-level runtime modes

`esper` has two fundamentally different runtime “modes”, selected by CLI subcommand:

1) **TUI monitor mode (default command)**:
- entry: `cmd/esper/main.go` → `monitor.Run(ctx, cfg)`
- purpose: interactive UI (port picking, device management, scrolling/searching/filtering, inspector)
- framework: Charm Bubble Tea + Bubbles + Lip Gloss

2) **Non-TUI tail mode** (`esper tail`):
- entry: `cmd/esper/main.go` → `tail.Run(ctx, cfg, stdout, stderr)`
- purpose: streaming output to stdout/stderr; optionally bidirectional raw stdin forwarding
- framework: plain Go loops, no Bubble Tea

Both modes share the same *conceptual* pipeline, but currently implement it in two separate places.

### Key dataflows (ASCII diagrams)

#### CLI → TUI dataflow

```
cmd/esper/main.go
  └─ monitor.Run(ctx, monitor.Config)
        └─ Bubble Tea Program (tea.NewProgram)
              └─ appModel (screen router + overlay router)
                    ├─ Port Picker screen (scan ports, choose baud/elf/toolchain)
                    ├─ Device Manager screen (registry CRUD + online status)
                    └─ Monitor screen
                         ├─ readSerialCmd(): read chunk bytes from serial.Port
                         ├─ lineSplitter.Push(chunk): []line
                         ├─ decoders:
                         │    - gdb.Push(chunk) → event + notice
                         │    - coredump.PushLine(line) → capture state + events (+ auto-enter)
                         │    - panic.DecodeBacktraceLine(line) → decoded frames + event
                         │    - autoColor.ColorizeLine(line) → rendered line
                         ├─ append() → log ring buffer + viewport refresh
                         └─ host tools:
                              - filter overlay produces filterCfg
                              - search bar decorates lines
                              - inspector surfaces structured events
```

#### CLI → tail dataflow

```
cmd/esper/main.go
  └─ tail.Run(ctx, tail.Config, stdout, stderr)
        ├─ serialio.Open() → serial.Port
        └─ runLoop:
             - port.Read(chunk)
             - gdb.Push(chunk) → notices to stderr
             - lineSplitter.Push(chunk) → lines
             - for each line:
                 * coredump.PushLine(line) → events (+ optional auto-enter)
                 * panic.DecodeBacktraceLine(line) → decoded bytes
                 * autoColor.ColorizeLine(line)
                 * writePrefixedBytes(...)
```

### The “serial decoding pipeline” as a conceptual component

Even though it is not expressed as a first-class package yet, both monitor and tail implement the same sequence:

1) **Chunking**: serial bytes read in blocks (4096 bytes).
2) **Line framing**: `parse.LineSplitter` yields newline-terminated “lines”, buffering a tail for partial lines.
3) **GDB stub detection**: `decode.GDBStubDetector` scans raw chunk bytes for `$T..#..` messages with checksum.
4) **Core dump capture**: `decode.CoreDumpDecoder` watches lines for:
   - prompt: “Press Enter to print core dump…”
   - start marker: “CORE DUMP START”
   - end marker: “CORE DUMP END”
   and buffers base64 data between markers, optionally decoding to a report when complete.
5) **Panic backtrace decode**: `decode.PanicDecoder` watches for “Backtrace:” lines and optionally runs `addr2line`.
6) **Auto-color**: `render.AutoColorer` injects ANSI color prefix and reset for ESP-IDF-ish `I/W/E (...)` lines, including partial-line “trailing color” reset logic.
7) **Output routing**:
   - Tail mode: bytes to stdout and notices to stderr.
   - TUI mode: appended into a log buffer and rendered via viewport; structured sub-events recorded into the inspector.

### Bubble Tea app structure (screens + overlays)

The TUI is not a single monolithic model; it is a router with sub-models.

#### Screens (mutually exclusive)

1) **Port Picker** (`port_picker.go`)
- Runs a scan command (`newScanPortsCmd`) and renders a simple list.
- Provides fields: baud, ELF path, toolchain prefix, and a “probe with esptool” toggle.
- Emits actions (`portPickerActionConnect`, `...Rescan`, etc.) for the `appModel` to interpret.

2) **Monitor** (`monitor_view.go`)
- Owns the serial decoding pipeline (splitter, decoders, autocolor).
- Maintains the output buffers:
  - `log []string` (line-ring)
  - plus additional UI state (viewport, follow flag, input field, search/filter state, session logging, inspector state).
- Has two user-facing modes:
  - DEVICE: line-input to device, follow output
  - HOST: scrolling/search/filter/inspector, command palette, session log toggles

3) **Device Manager** (`device_manager.go`)
- Loads/saves registry via `pkg/devices`.
- Displays a “table” of entries with online/offline derived from the latest scan results.
- Supports add/edit/remove via overlays, and connect to selected device.

#### Overlays (modal)

Overlays implement a common interface and are “painted over” the screen:
- Help overlay (`help_overlay.go`)
- Filter overlay (`filter_overlay.go`)
- Palette overlay (`palette_overlay.go`)
- Confirm overlay (`confirm_overlay.go`) and reset confirm overlay (`reset_confirm_overlay.go`)
- Device edit overlay (`device_edit_overlay.go`)
- Inspector detail overlay (`inspector_detail_overlay.go`)

The overlay layer uses a fairly strict routing rule:
- Key messages go to the overlay first (and may also forward a message back to the underlying screen).
- Non-key messages (serial reads, ticks) still go to the underlying screen even if an overlay is visible, to avoid deadlock and allow “auto overlays” (e.g. core dump capture state) to continue.

### Firmware test harness (context)

The repository also includes a test firmware under `esper/firmware/esp32s3-test/` to generate representative output that exercises the monitor pipeline (logs, panic patterns, core dump prompts, etc.).

Given the project-level constraint that UART pins are often repurposed, the expected baseline for ESP32-S3 consoles in this environment is USB Serial/JTAG (see `firmware/.../sdkconfig.defaults` for `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`).

## Design Decisions

This section captures notable architectural choices visible in the codebase.

1) **Bubble Tea with explicit screen routing**
- `appModel` is intentionally a router with explicit `screen` state, rather than a single giant model.
- This is a good fit for:
  - port selection UX,
  - device manager UX,
  - monitor UX,
  - and overlays that must “sit on top” of any screen.

2) **Decoder logic lives “in the UI” (for now)**
- The TUI monitor currently performs decoding work directly inside its `Update` handler.
- This keeps the pipeline close to where UI events are emitted, but couples “decode” and “render” tightly.

3) **External tool parity over pure-Go decoding**
- Core dump decode uses Python `esp_coredump` (parity with ESP-IDF behaviors).
- Panic decode uses toolchain `addr2line` (direct mapping to ELF symbols).
- This is pragmatic for correctness and parity, but introduces latency, blocking, and dependency management considerations.

4) **Per-user registry stored under XDG config**
- The device registry intentionally uses XDG conventions (`$XDG_CONFIG_HOME` or `~/.config/esper/devices.json`) and atomic writes.
- This keeps the tool stateless at the repository level and makes it portable across projects.

5) **Linux-first serial scanning**
- Port scanning is implemented for Linux only (`scan_linux.go`) and is used in both CLI and TUI.
- The focus is on stable `/dev/serial/by-id/` paths and Espressif signals (VID `303a`, manufacturer/product heuristics), which aligns with the USB Serial/JTAG preference.

## Alternatives Considered

The codebase implicitly rejects (or defers) a few alternatives:

1) **Use ESP-IDF’s Python monitor directly**
- Would reduce implementation burden but makes terminal UX customization and “host-mode analysis tools” harder to evolve.

2) **Use a fully curses-style UI**
- Bubble Tea + Lip Gloss provides a functional/declarative UI approach that is simpler to reason about, easier to test, and aligns with the existing Go ecosystem for this repo.

3) **Implement core dump and backtrace decode fully in Go**
- Would reduce external dependencies and allow better control over timeouts and concurrency.
- But correctness parity and the required parsing logic are non-trivial; using ESP-IDF’s established tooling is a pragmatic Phase 1 choice.

4) **Cross-platform device scanning from day one**
- The scanning logic leans heavily on `/sys` and `/dev` conventions.
- Cross-platform is possible, but has not been made a design constraint yet.

## Implementation Plan

For ESP-11, the “implementation plan” is documentation-first: produce a deep audit and a follow-up/refactor plan grounded in the as-is architecture.

The concrete follow-up work (if accepted) is documented in:
- `playbook/01-follow-ups-and-refactor-plan.md`

At a high level, the likely next steps after this review are:
1) Extract a shared “pipeline” package so `tail` and `monitor` do not duplicate logic.
2) Move expensive/slow decode work off the Bubble Tea `Update` thread (async + messages).
3) Reduce `monitor_view.go` size by splitting responsibilities (I/O loop, filtering/search, inspector).
4) Rationalize overlay patterns (remove duplicated confirm overlays, unify helper utilities).

## Open Questions

1) **What is the intended OS support envelope?**
- Today: scan is Linux-only; device nickname resolution assumes Linux scanning.
- Decide whether to:
  - formally constrain to Linux for now, or
  - introduce “best-effort” alternatives (e.g. go-serial enumerations on macOS/Windows).

2) **Should “probe with esptool” be available in the TUI connect path?**
- The Port Picker exposes the toggle, but `connectCmd` explicitly does not implement it yet.
- UX implications: probing can reset the device; likely requires a confirm overlay and/or explicit warnings.

3) **What is the intended policy for core dump artifacts?**
- Core dumps currently get saved to temp (in `decode`) and reports can be saved to cache (in `monitor`).
- Decide on:
  - retention and cleanup,
  - discoverability,
  - and user control over destinations.

4) **How important is “no UI freezes” during decode?**
- As-is, panic/core dump decoding may block the UI; if that is unacceptable, async decode becomes a priority.

5) **Do we want a stable “event model” shared between tail and monitor?**
- Today, only the TUI emits structured events (inspector).
- A shared event model could:
  - simplify testing,
  - enable JSON output modes,
  - and make tail richer without duplicating logic.

## References

- `esper/README.md` (high-level intent and usage)
- `esper/cmd/esper/main.go` (CLI wiring)
- `esper/pkg/monitor/app_model.go` (screen routing)
- `esper/pkg/monitor/monitor_view.go` (monitor model, decoding pipeline, view rendering)
- `esper/pkg/tail/tail.go` (non-TUI pipeline)
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/design-doc/02-esper-code-review-deep-audit.md` (deep audit)
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md` (actionable follow-ups)
