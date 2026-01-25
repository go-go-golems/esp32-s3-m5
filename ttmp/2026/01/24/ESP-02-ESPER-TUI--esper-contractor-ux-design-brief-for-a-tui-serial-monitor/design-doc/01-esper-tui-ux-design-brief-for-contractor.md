---
Title: 'Esper TUI: UX design brief for contractor'
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
      Note: Defines esper goals and current CLI usage
    - Path: esper/pkg/commands/scancmd/scan.go
      Note: Scan command semantics and flags (probe-esptool warning)
    - Path: esper/pkg/decode/coredump.go
      Note: Core dump capture/mute behavior and decode paths that must be surfaced in UX
    - Path: esper/pkg/monitor/monitor.go
      Note: Current minimal Bubble Tea monitor UI and input/output behavior
    - Path: esper/pkg/scan/scan_linux.go
      Note: Port scanning + scoring used for port picker defaults
    - Path: ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md
      Note: Prior analysis of idf.py monitor parity and host-command model expectations
ExternalSources: []
Summary: A contractor-ready UX design brief for building a Bubble Tea TUI for `esper` (ESP-IDF-compatible serial monitor).
LastUpdated: 2026-01-24T20:45:27.127045842-05:00
WhatFor: Hand to a contractor TUI/UX designer to create wireframes, keymaps, and screen-state specs for an `esper` full-screen terminal UI.
WhenToUse: Use when commissioning UX design work for `esper`'s TUI; serves as the UX requirements and acceptance criteria for design deliverables.
---


# Esper TUI: UX design brief for contractor

## Executive Summary

We need a keyboard-first, full-screen terminal UI (TUI) for `esper`, a Go-based ESP-IDF-compatible serial monitor. The current UI is intentionally minimal (a streaming output area and a text input) and is missing the ergonomics engineers expect from `idf.py monitor`: safe “host commands” vs “device input”, robust scrollback, search/filter, and clear handling of special events (panic/backtrace, core dumps, GDB stub detection).

This brief specifies the desired user experience, information architecture, key flows, and edge cases. The primary deliverable from the contractor is a set of TUI wireframes + interaction specs (keybindings, states, component behaviors) that an engineer can implement using Bubble Tea (+ Bubbles/Lip Gloss).

## Audience / Deliverables (what we want from you)

You are a contractor UX designer specializing in TUIs. Please deliver:

1. **Screen inventory + state machine**
   - List of screens/views and all modal/overlay states.
   - Entry/exit conditions, error states, and empty states.
2. **Keybinding spec**
   - A complete keymap (global + per-view), including conflict resolution and discoverability (help overlay).
   - Explicit separation of “keys sent to device” vs “host-only keys”.
3. **Wireframes**
   - Primary flows at common terminal sizes (minimum 80×24; ideal 120×40).
   - Narrow-mode adaptations (e.g., 80×24) without losing core functionality.
4. **Component behavior spec**
   - Scrolling model, selection/copy model, search UX, filtering UX, notifications/toasts, and status indicators.
   - How partial lines and ANSI colors should appear (no “stuck colors”; no broken resets).
5. **Handoff notes**
   - Implementation hints for Bubble Tea (no code required) and any “must not do” pitfalls.

Preferred format: a single PDF or Figma file + a short written spec (Markdown) suitable for engineers.

## Problem Statement

`esper` is a Go serial monitor built for ESP-IDF parity: it colorizes ESP-IDF log lines, detects and reacts to panic backtraces, buffers core dumps, and detects GDB stub sequences. Today, `esper`’s UI is “Phase 1 minimal”: it can show output and send typed lines to the device, but it lacks essential TUI affordances:

- no scrollback viewport, search, or filtering in a durable UX,
- no safe, discoverable host-command model (to avoid contaminating device input),
- no clear, actionable presentation of special decoding events (panic/backtrace decode output, core dump capture/report, GDB stub detection),
- no interactive port selection flow (beyond passing `--port`), despite `esper scan` existing.

Engineers need an ergonomic monitor that can be lived in for hours: rapidly diagnose faults, keep context, and avoid input mistakes while staying keyboard-only.

## Proposed Solution

### Product context (what `esper` is today)

`esper` is a Go program with:

- `esper scan` to list likely ESP32 serial ports (Linux), optionally probing with `esptool` (may reset the device).
- `esper` (no subcommand) to run the serial monitor.
- monitor pipeline behaviors:
  - auto-color for ESP-IDF log prefixes (I/W/E),
  - opportunistic panic “Backtrace:” decoding (optionally via `addr2line`),
  - core dump capture (mute output during capture) + optional decode via `python3 -c import esp_coredump...`,
  - GDB stub stop-reason detection.

The TUI should expose these behaviors as *clear UI states* with minimal cognitive load.

### Information architecture (IA)

**Primary views**

1. **Port Picker / Connect**
   - Select port (show “preferred” stable `/dev/serial/by-id/...` when available).
   - Configure baud, ELF path, toolchain prefix, and (optional) “probe with esptool” toggle.
   - Connect/disconnect, show connection errors.
2. **Monitor (Main)**
   - Streaming log viewport with scrollback.
   - Bottom input line for device console (send `\r\n` on Enter).
   - Status bar for connection + mode indicators.
3. **Inspector (Side panel or overlay; togglable)**
   - Special event summaries and drill-down:
     - Panic/backtrace: decoded frames list; jump-to timestamp; copy.
     - Core dump: capture progress + decoded report view; save/copy.
     - GDB stub: notification with “launch GDB” guidance (host action).

**Global overlays**

- **Help / Keymap** overlay (always available)
- **Command palette** (host-only commands, safe from device input)
- **Search** overlay (scrollback search + next/prev)
- **Filter** overlay (levels, regex, include/exclude, highlight rules)
- **Confirmations** for dangerous actions (device reset, esptool probe, clearing logs)

### Primary flow: start → connect → monitor

**Entry behavior**

- If `--port` is provided and opens successfully: go straight to Monitor view.
- If `--port` is missing or fails to open: start in Port Picker view with a helpful error banner.

**Port Picker expectations**

- Default selection should favor ports that look like Espressif devices (VID 303a, by-id contains “Espressif”, USB Serial/JTAG), matching `esper scan` scoring.
- Clearly label when “esptool probe” may reset the device; require confirmation.

### Monitor view wireframe (baseline)

```
┌ esper ── Connected: /dev/serial/by-id/...  115200 ── ELF: ✓  AutoColor: ✓ ─┐
│                                                                            │
│  [scrollback viewport: ANSI-colored serial output; monospaced; no wrap*]   │
│                                                                            │
│                                                                            │
├────────────────────────────────────────────────────────────────────────────┤
│ Mode: NORMAL  Capture: —  GDB: —   Filter: —   Search: —   Buf: 512KiB/1MiB│
├────────────────────────────────────────────────────────────────────────────┤
│ > [device input line....................................................] │
└────────────────────────────────────────────────────────────────────────────┘
* wrap toggle available; default discussed below
```

### Key UX behaviors (must specify)

**Scrolling / follow mode**

- “Follow” mode auto-scrolls to the newest output.
- When user scrolls up, auto-follow pauses and a clear indicator appears (e.g., `FOLLOW: OFF`).
- One key returns to bottom and re-enables follow (e.g., `End` or `g g` / `G` depending on final keymap).

**Wrap vs horizontal scroll**

Serial logs often include long lines (JSON, stack traces). The TUI must support:

- Wrap toggle (off by default is acceptable if horizontal scroll exists).
- Horizontal scrolling if wrap is off.
- A clear per-line rendering that preserves ANSI color resets.

**Search**

- Search should work across scrollback (not just visible viewport).
- Provide “next/prev match” and highlight matches.

**Filtering**

- Quick log-level toggles: show/hide `I/W/E` (and optionally `D/V` later).
- Regex include/exclude filter.
- Highlight rules (regex → color/underline) should be possible as a later enhancement; design should not block it.

**Copy / selection**

Engineers need copy/paste for backtraces and core dump reports.
- Specify a keyboard-only selection model (line selection, block selection, or “copy last N lines”).
- Provide a “copy current line” and “copy visible viewport” shortcut.

**Host commands vs device input (safety-critical)**

We must prevent accidental keystrokes (meant for the TUI) from being sent to the device.

- Adopt a *host-command modality*: either a **menu key** chord (like `Ctrl-T`) or a **command palette** that “captures” input.
- Any keybinding that is not explicitly “device input” should be host-only.
- The UI must always communicate “where your keystrokes are going” (device vs host) via an obvious indicator.

### Special event UX (panic, core dump, GDB)

**Panic/backtrace**

- Detect lines containing `Backtrace:` and show a non-intrusive notification (“Backtrace detected; press … to open inspector”).
- In inspector, show:
  - raw backtrace line(s),
  - decoded stack frames (if ELF + toolchain prefix provided),
  - copy buttons/shortcuts for raw/decoded.

**Core dump**

`esper` mutes normal output during core dump capture and may auto-send Enter to begin the dump.

UX requirements:

- When core dump capture begins: show a persistent “CAPTURE IN PROGRESS” banner and clearly indicate output is being muted/buffered.
- After capture ends: offer to open the decoded report view; allow saving report + raw base64.
- If decode fails: show failure reason and still preserve raw capture for copy/save.

**GDB stub detection**

- On detection: show a notification with recommended next actions.
- Do not automatically launch external processes without confirmation.
- Provide a “copy GDB stop packet” and “open help” action.

### Secondary flows (should be designed, can be phased)

- **Reconnect**: on serial error/disconnect, show banner + retry action.
- **Session logging**: save output to file; toggle “log to file” in status bar.
- **Clear scrollback**: confirm and preserve “last event” info in inspector.
- **Settings**: toggles for auto-color, wrap, timestamps, showing control chars, buffer size.

### Accessibility and terminal realities

- Support both light/dark terminal themes by avoiding hard-coded low-contrast colors; rely on ANSI sparingly and consistently.
- Ensure all essential info has *non-color cues* (labels/icons/text).
- No mouse required; no assumptions about focus-follow-mouse.

### Non-goals (for initial design)

- A fully visual “graph” UI or rich widgets beyond terminal constraints.
- Multi-device multiplexing (tabs for multiple ports) in v1; however, avoid designs that make it impossible later.
- Full parity with every `idf.py monitor` feature immediately; focus on daily-driver ergonomics and safe input handling.

## Design Decisions

1. **Two-step entry (Port Picker → Monitor)**: reduces friction when `--port` is unknown and leverages `esper scan`.
2. **Single primary Monitor view with overlays**: minimizes complexity while still enabling search/filter/help/command palette.
3. **Explicit host-command modality**: safety-critical; prevents accidental device input contamination.
4. **Inspector for special events**: keeps main log clean while making panic/core dump/GDB events actionable.

## Alternatives Considered

1. **Keep the minimal UI (status quo)**: insufficient for long sessions; lacks scroll/search/filter and safe host commands.
2. **Split into separate commands only (no interactive port selection)**: forces context switching; wastes time and increases error rates.
3. **Multiple always-visible panes/tabs for everything**: can overwhelm users; risks unusability on 80×24 terminals.

## Implementation Plan

### Designer work plan (suggested)

1. **Discovery (0.5–1 day)**
   - Read `esper/README.md` and confirm the special events (`panic`, `coredump`, `gdbstub`) and current minimal monitor behavior.
   - Review `idf.py monitor` ergonomics to understand “menu key” expectations.
2. **IA + keymap (1–2 days)**
   - Propose screen inventory, state machine, and keymap (global + per view).
   - Validate against terminal constraints (80×24).
3. **Wireframes (2–4 days)**
   - Port Picker, Monitor baseline, Inspector, overlays (help/search/filter/palette), error states.
4. **Interaction spec + handoff (1–2 days)**
   - Component behaviors (scrolling/follow, selection/copy, notifications, confirmations).
   - “Must not do” pitfalls (e.g., sending host keys to device).

### Engineering notes (for context only; no code requested from designer)

Implementation target is Go + Bubble Tea/Bubbles/Lip Gloss. The TUI must preserve ANSI correctness and handle partial-line output (tail flush on idle) and special-event modes (core dump muting).

## Open Questions

1. Should the TUI be the default behavior of `esper`, or behind a flag (e.g., `esper monitor` / `--tui`)? (Current default runs monitor.)
2. What is the preferred “menu key” chord for host commands (keep `Ctrl-T` parity, or use `/` for palette, etc.)?
3. Should wrap be on by default, or off with horizontal scroll? (Consider logs vs backtraces.)
4. How should copy/selection work given Bubble Tea limitations and terminal variance (OSC52 vs system clipboard)?
5. What is the desired policy for auto-actions (auto-send Enter for core dump prompt is current behavior; should UI make this explicit/confirmable)?
6. What is the desired default for scrollback buffer size and persistence (in-memory only vs optional file-backed)?

## References

- `esper/README.md`
- `esper/pkg/monitor/monitor.go`
- `esper/pkg/commands/scancmd/scan.go` and `esper/pkg/scan/scan_linux.go`
- `ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md` (behavioral parity notes for ESP-IDF monitor)
