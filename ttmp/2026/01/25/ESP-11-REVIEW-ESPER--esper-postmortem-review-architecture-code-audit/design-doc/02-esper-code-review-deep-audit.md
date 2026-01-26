---
Title: Esper code review (deep audit)
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
    - Path: esper/pkg/decode/coredump.go
      Note: Core dump capture/decode (python exec + buffering)
    - Path: esper/pkg/decode/panic.go
      Note: Backtrace decode (addr2line subprocess)
    - Path: esper/pkg/monitor/ansi_text.go
      Note: ANSI stripping and width/cut helpers
    - Path: esper/pkg/monitor/ansi_wrap.go
      Note: ANSI-aware wrap (duplicate parsing logic)
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Primary complexity hotspot (I/O + pipeline + rendering)
    - Path: esper/pkg/monitor/reset_confirm_overlay.go
      Note: Duplicated confirm overlay logic
    - Path: esper/pkg/tail/tail.go
      Note: Pipeline duplication and tail-mode behaviors
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T19:34:27.201529647-05:00
WhatFor: A deep, section-by-section postmortem code audit of the entire esper app (CLI, backend pipeline, and Bubble Tea TUI), emphasizing duplication, cohesion, blocking behavior, and refactor opportunities.
WhenToUse: Use when deciding what to refactor next, preparing PR review checklists, or onboarding contributors who need to understand where complexity and duplication live.
---


# Esper code review (deep audit)

## Executive Summary

This is an intentionally “food inspector”-style report: picky, highly detailed, and oriented toward maintainability, correctness under stress, and future refactor safety.

**Overall assessment (as of the current code state):**
- The codebase is surprisingly readable for a UI-heavy serial tool: the core concepts are clear, the package boundaries mostly make sense, and the TUI screen router (`appModel`) is a solid foundation.
- The implementation’s **main technical debt** is *duplication and mixed responsibilities*:
  - The serial processing pipeline is implemented twice (tail vs monitor).
  - `monitor_view.go` is a large “god file” with I/O, decoding, buffering, filtering/search, inspector eventing, rendering, and session logging intertwined.
  - There are smaller but repeated patterns (confirm overlays, ANSI parsing, buffer/line handling) that would benefit from consolidation.
- The biggest **behavioral risk** is **blocking work inside the Bubble Tea `Update` loop**:
  - Panic backtrace decode runs `addr2line` processes synchronously when a backtrace line appears.
  - Core dump decode runs `python3`/`esp_coredump` synchronously when a capture completes.
  - Either can freeze the UI for seconds (or longer), and core dump decoding has no explicit timeout because it does not use `exec.CommandContext`.

**Strengths worth preserving:**
- Clear UX model (DEVICE vs HOST mode) and overlay routing that avoids deadlocks.
- Good small helper abstractions where they exist (`selectList`, `search_logic`, `applyHighlightRules`).
- Tests exist for critical UI invariants (view sizing, overlay routing, filter/palette semantics).

**Highest-leverage improvements (in rough priority order):**
1) Extract a **shared pipeline package** so tail and TUI share one implementation.
2) Make decoding **asynchronous in the TUI** (Bubble Tea command → message), with timeouts and cancellation.
3) Split `monitor_view.go` into smaller units (I/O + pipeline, buffering, filter/search, inspector rendering).
4) Remove small duplications (reset confirm overlay vs generic confirm overlay; duplicated ANSI parsing logic).

The companion playbook (`playbook/01-follow-ups-and-refactor-plan.md`) turns these into a staged refactor sequence with validation checkpoints.

## Ticket Map (Findings → Follow-up Tickets)

This audit is intentionally “action-oriented”: every major finding below is mapped to a concrete ticket with a self-contained spec.

- **Shared pipeline extraction (tail vs TUI + tail loop duplication)** → ESP-12:
  - `ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md`
- **Make decoding async (Bubble Tea style + cancellation/timeouts)** → ESP-13:
  - `ttmp/2026/01/25/ESP-13-ASYNC-DECODE--make-decoding-async-bubble-tea-style/design-doc/01-specification.md`
- **Split `monitor_view.go` (“god file”)** → ESP-14:
  - `ttmp/2026/01/25/ESP-14-SPLIT-MONITOR-VIEW--split-monitor-view-go-into-cohesive-units/design-doc/01-specification.md`
- **Small duplication cleanup + helper extraction** → ESP-15:
  - `ttmp/2026/01/25/ESP-15-DEDUPE-EXTRACT-PKG--remove-duplications-and-extract-reusable-helpers-into-pkg/design-doc/01-specification.md`
- **macOS support (scan + nickname resolution + TUI scanning)** → ESP-16:
  - `ttmp/2026/01/25/ESP-16-MACOS--support-macos-port-scan-serial-ux/design-doc/01-specification.md`
- **TUI “probe with esptool” is currently a no-op** → ESP-17:
  - `ttmp/2026/01/25/ESP-17-TUI-PROBE--implement-esptool-style-probing-in-tui-connect/design-doc/01-specification.md`
- **Core dump stability (limits, timeouts, artifact policy, capture vs decode)** → ESP-18:
  - `ttmp/2026/01/25/ESP-18-COREDUMP-STABILIZE--cleanup-and-stabilize-core-dump-capture-decoding/design-doc/01-specification.md`
- **Remove external Python dependencies (probe + core dumps)** → ESP-19:
  - `ttmp/2026/01/25/ESP-19-REMOVE-PYTHON--remove-external-python-dependencies/design-doc/01-specification.md`
- **Fix ANSI correctness (strip/wrap/search/overlay consistency)** → ESP-20:
  - `ttmp/2026/01/25/ESP-20-ANSI-CORRECTNESS--fix-ansi-correctness-across-monitor-strip-wrap-search/design-doc/01-specification.md`
- **Event export (TUI/CLI, NDJSON/JSON, canonical schema)** → ESP-21:
  - `ttmp/2026/01/25/ESP-21-EVENT-EXPORT--event-export-tui-cli/design-doc/01-specification.md`

## Problem Statement

`esper` is now big enough that “it works and is readable” is no longer the only standard. The postmortem problem is maintainability:

- **Duplication**: Core runtime logic is repeated (tail vs TUI pipeline), increasing the chance of behavior drift.
- **Cohesion**: `monitor_view.go` mixes too many concerns, which:
  - increases cognitive load,
  - makes targeted tests harder,
  - and raises the cost of making changes without regressions.
- **Blocking work on the UI thread**: External tool execution (addr2line, esp_coredump) can freeze the TUI.
- **Inconsistent helper semantics**:
  - ANSI stripping/cutting/wrapping logic exists in multiple forms with slightly different escape handling.
  - Some helper functions are defined in unexpected files (e.g. `min/max` in `port_picker.go` but used widely).
- **Feature-shaped inconsistencies**:
  - TUI exposes a “probe with esptool” toggle but the connect path explicitly does not implement probing.

The goal of this audit is to identify *exactly where* these problems occur and propose concrete refactoring/cleanup strategies.

## Proposed Solution

This “solution” section is the audit itself: findings and recommendations by subsystem, with an emphasis on duplication, deprecated/unfinished hooks, and design simplifications.

### Severity model used in this report

- **CRITICAL**: can cause UI freezes, deadlocks, data loss, or severe operational failure.
- **MAJOR**: likely to cause behavior drift, make refactors risky, or create frequent footguns.
- **MINOR**: style, clarity, or small quality issues that accumulate but are not immediately dangerous.

### A) CLI surface (Cobra + Glazed)

**Tracking tickets:** ESP-16 (macOS scan support), ESP-17 (TUI probe integration), ESP-19 (remove python probe backend), ESP-21 (event export CLI surface).

#### `esper/cmd/esper/main.go`

**What it does well**
- Clear CLI wiring and good defaults (`baud=115200`).
- `resolvePort` provides a pragmatic UX:
  - accepts `/dev/...` paths or globs,
  - accepts `tty*` shorthand on Linux,
  - accepts nicknames (device registry lookup).

**Findings**
- **MINOR**: Port resolution behavior is spread across two places:
  - `resolvePort` handles nicknames + tty shorthands.
  - `serialio.Open` handles globs.
  This is fine, but it’s easy to forget the “full resolution” contract when adding new call-sites. A single “resolve everything” helper would reduce repetition and ambiguity.

**Recommendation**
- If the refactor plan introduces a shared pipeline or connection manager, consider:
  - `devices.ResolveNicknameToPort` (nickname)
  - `serialio.Open` (glob expansion)
  - `resolvePort` (tty shorthand)
  as a single cohesive “port resolution” module.

#### `esper/pkg/commands/scancmd/scan.go`

**What it does well**
- Uses Glazed to provide flexible output formatting/fields; good for tooling pipelines.
- Gracefully treats registry load errors as non-fatal for scan output.

**Findings**
- **MINOR**: The long help text says probing is default and warns about reset behavior, which is good, but:
  - scan itself is Linux-only; consider making that explicit in user-facing help text (it is implicit in `ScanLinux` errors).
- **MINOR**: Registry enrichment logic (mapping `Port.Serial` → nickname/name/description) is embedded in command output formatting. That is acceptable, but could be extracted if it grows.

#### `esper/pkg/commands/devicescmd/*`

**What it does well**
- Minimal and clear commands for registry management; aligns with XDG behavior.

**Findings**
- **MINOR**: `devices set` uses flags and required fields; no interactive UX, which is fine. The TUI device manager is the interactive UX.

### B) Backend “plumbing” packages

This section covers everything that is not the TUI: scan/registry/serial open/parse/render/decode/tail.

**Tracking tickets:** ESP-12 (shared pipeline), ESP-16 (macOS scan + nickname resolution), ESP-18 (coredump stability), ESP-19 (remove python), ESP-20 (ANSI correctness), ESP-21 (event export model).

#### `esper/pkg/serialio/serialio.go`

**What it does well**
- Simple, focused responsibility: open a port, expand globs, validate baud/port.

**Findings**
- **MINOR**: Glob expansion chooses the first match (`matches[0]`). That is a common default but can be surprising if the glob matches multiple devices.

**Recommendation**
- Consider:
  - sorting matches for determinism, and/or
  - returning an error if multiple matches are found (or adding an explicit “take first” option).

#### `esper/pkg/parse/linesplitter.go`

**What it does well**
- Clear and correct buffering of tail bytes until newline.

**Findings**
- **MAJOR (perf/GC)**: `Push` allocates:
  - a new combined buffer when tail exists,
  - a new tail copy when trailing partial exists.
  For serial monitors this is probably acceptable, but if logs are high throughput, this becomes a steady allocator.

**Recommendation**
- If performance/GC becomes an issue:
  - reuse buffers (e.g. a `bytes.Buffer`),
  - or store tail in a growable buffer with capacity reuse.
  Only do this once there is evidence it matters; correctness is more important than micro-optimizations in Phase 1.

#### `esper/pkg/render/autocolor.go` and `render/ansi.go`

**What it does well**
- Captures the core of ESP-IDF’s “AUTO_COLOR_REGEX intent” in a cheap prefix check.
- Correctly handles partial-line coloring via `trailingColor` and injects reset when the line ends.

**Findings**
- **MINOR**: The auto-color heuristic is intentionally narrow (I/W/E + space + '('). It will not colorize other ESP-IDF line formats. That’s fine if “Phase 1 parity” is explicitly limited.

#### `esper/pkg/decode/panic.go`

**What it does well**
- Works without ELF/toolchain (falls back to printing PCs).
- Decodes each PC via `addr2line` with readable formatting.

**Findings**
- **CRITICAL (TUI freeze potential)**: `addr2line` is executed synchronously (and repeatedly) for each decoded backtrace line when used in the TUI monitor, because the monitor calls `DecodeBacktraceLine` inline during `Update`.
- **MAJOR**: Decoding invokes `addr2line` once per PC address (not batched). For typical backtraces this can mean many subprocess executions.

**Recommendation**
- Introduce a decoder service with:
  - batching (call addr2line with multiple addresses when possible),
  - caching (per ELF + address),
  - and **context + timeout**.
- In the TUI, run the decode in a `tea.Cmd` and return a message when done; show a “decoding…” event if needed.

#### `esper/pkg/decode/coredump.go`

**What it does well**
- Clear state machine: idle vs reading.
- Saves captured base64 to a temp file for later reference.
- Uses ESP-IDF’s `esp_coredump` Python module for parity.

**Findings**
- **CRITICAL (TUI freeze potential)**: Core dump decoding uses `exec.Command("python3", "-c", ...)` without context or timeout. If the python environment hangs, the TUI can hang indefinitely at the end of a core dump capture.
- **MAJOR (memory growth risk)**: During capture, the decoder appends all buffered lines into `d.buf` without size limits. Large dumps or corrupted streams could balloon memory.
- **MAJOR (artifact sprawl)**: The decoder always writes a temp file but does not have cleanup strategy; temp files may accumulate.
- **MINOR**: When decode fails, `DecodeErr` stores only `fmt.Sprintf("%v", err)` and returns a generic “decode failed” line; the report output (`out`) is stored but not necessarily surfaced in all error messages.

**Recommendation**
- Use `exec.CommandContext` with a reasonable timeout for python decode.
- Add maximum buffer size (and abort capture with an event) to prevent unbounded RAM growth.
- Consider:
  - user-configurable dump artifact directory, and/or
  - cleanup hints (e.g. “saved to /tmp/... (consider deleting when done)”).

#### `esper/pkg/decode/gdbstub.go`

**What it does well**
- Performs checksum validation; avoids false positives.
- Maintains a small tail buffer to find sequences that cross chunk boundaries.

**Findings**
- **MINOR (state update)**: When a match is found, the function returns before updating `d.tail`. This is probably fine, but it’s slightly surprising because:
  - `d.tail` could remain from previous calls even after a match, potentially affecting next scans.

**Recommendation**
- Consider explicitly updating `d.tail` even on match (or resetting it) to make the state machine behavior easier to reason about.

#### `esper/pkg/scan/scan_linux.go`

**What it does well**
- Practical scanning and scoring heuristics:
  - Espressif VID (`303a`) strongly weighted,
  - `/dev/serial/by-id` stability bonus,
  - product/manufacturer string heuristics.
- `readUSBAttrs` walks `/sys` parent chain; good for robustness.

**Findings**
- **MAJOR (portability)**: Linux-only, and the rest of the code sometimes assumes scanning exists (e.g. nickname resolution).

**Recommendation**
- If non-Linux support is not desired: loudly document “Linux only” for scan + nickname resolution.
- If it is desired: hide OS differences behind an interface and implement minimal macOS/Windows support.

#### `esper/pkg/scan/probe_esptool.go`

**What it does well**
- Uses `%q` quoting for python string literals; avoids injection via port path values.
- Redirects stdout/stderr in python to preserve JSON-only output.
- Adds a fallback 20s timeout when the caller context has no deadline.

**Findings**
- **MINOR**: There is duplicated command creation: it constructs `cmd := exec.CommandContext(...)`, then if no deadline it constructs a new context and reconstructs the command. It’s correct but slightly awkward.

### C) Non-TUI runtime: `esper/pkg/tail/tail.go`

**Tracking tickets:** ESP-12 (shared pipeline), ESP-21 (event export from tail).

#### Key strengths
- Clear configuration validation and output routing (stdout/stderr + optional logfile tee).
- Two operation modes:
  - normal tail
  - bidirectional “stdin raw” tail with a centralized write goroutine to avoid concurrent serial writes.

#### Findings
- **MAJOR (duplication)**: `runLoop` and `runLoopWithStdinRaw` duplicate almost the entire pipeline logic (read → gdb → splitter → coredump → panic → autocolor). The only difference is write handling and stdin forwarding.
- **MAJOR (allocations)**: Both loops allocate a new `buf := make([]byte, 4096)` per read iteration.
- **MINOR**: Core dump decode is synchronous; in tail mode that is acceptable, but it does mean tail “stalls” at core dump completion until python decode returns.

#### Recommendations
- Extract a shared pipeline (or at least shared “processChunk” helper) so the only duplicated portion is “how to write/send enter”.
- Reuse read buffers (allocate once and reuse) if throughput becomes an issue.
- Consider making core dump decode optional-by-default in tail mode (it already can be disabled via flags), and ensure the default aligns with user expectations.

### D) Bubble Tea TUI (`esper/pkg/monitor/*`)

This is the highest complexity area and where most “design smell” findings live. The audit here is intentionally detailed and file-specific.

**Tracking tickets:** ESP-13 (async decode), ESP-14 (split monitor_view), ESP-15 (dedupe helpers/overlays), ESP-17 (probe UX), ESP-18 (coredump stability), ESP-20 (ANSI correctness), ESP-21 (event export).

#### D1) App shell and routing: `app_model.go`

**What it does well**
- Clear screen model:
  - Port Picker
  - Monitor
  - Device Manager
- Solid overlay routing rule:
  - overlay captures key events first,
  - non-key events still reach active screen even while overlay open (prevents deadlocks).
- Resize handling is centralized and executed first.

**Findings**
- **MINOR**: Screen models share helpers (e.g. `min/max/padOrTrim`) defined in `port_picker.go`. This is functionally fine (package-level), but it is surprising and increases coupling.

**Recommendation**
- Move shared helpers into a dedicated file (e.g. `ui_helpers.go`) for discoverability.

#### D2) Message and session layer: `messages.go`

**What it does well**
- Centralizes message types and connection management concepts (`serialSession`).
- `serialSession.ResetPulse` documents best-effort reset behavior with RTS/DTR assumptions.

**Findings**
- **MAJOR (feature mismatch)**: `connectParams` includes `probeEsptool`, but `connectCmd` explicitly does not implement probing yet.
- **MINOR**: `connectCmd` contains `_ = ctx` and a comment about future probing, but currently does not use the context; it relies on `serialio.Open` and `SetReadTimeout`.

**Recommendation**
- Either:
  - implement probing with explicit UX confirmation, or
  - remove the toggle until it’s implemented, to avoid “lying UI”.

#### D3) Port selection UX: `port_picker.go`

**What it does well**
- Good interaction model: Tab/Shift-Tab cycles focus, Enter connects, r rescans, q quits.
- Uses registry nicknames to annotate ports in list.

**Findings**
- **MINOR**: The UI includes “Probe with esptool” toggle, but actual connect path ignores it.
- **MINOR**: Helpers `padOrTrim/min/max` live here but are used across the package; this is a discoverability smell.

#### D4) Device manager UX: `device_manager.go` + `device_edit_overlay.go`

**What it does well**
- Registry CRUD is clear and mostly self-contained.
- Online/offline derived from scans is useful and low-risk.
- Edit overlay reuses `updateSingleLineField` helper.

**Findings**
- **MINOR**: `connectSelectedAction` hardcodes baud 115200 in connectParams, relying on the app model to fill in global defaults. It works, but is slightly “action at a distance”.
- **MINOR**: Error propagation uses `flashErrMsg` on `deviceManagerActionNone`, which makes “none” actions carry side-effects; this pattern can become confusing at scale.

#### D5) Overlays: wrappers and duplication

Files involved:
- `overlay_manager.go`
- `overlay_wrappers.go`
- `help_overlay.go`
- `confirm_overlay.go`
- `reset_confirm_overlay.go`
- `palette_overlay.go`
- `filter_overlay.go`
- `inspector_detail_overlay.go`

**What it does well**
- Common overlay interface keeps rendering and routing consistent.
- Wrapper structs (`helpOverlay`, `filterOverlay`, `paletteOverlay`) separate:
  - “overlay interface glue” (Update returns overlayOutcome) from
  - “overlay model logic” (Update returns overlay-specific result).

**Findings**
- **MAJOR (duplication)**: `reset_confirm_overlay.go` duplicates `confirm_overlay.go` almost exactly, rather than using the generic confirm overlay with a custom message and forward msg.
- **MINOR**: Overlay wrapper patterns are repetitive; this is not “wrong”, but suggests a missing mini-framework helper (or an “overlay adapter”).

#### D6) ANSI/search/wrap helpers: `ansi_text.go` and `ansi_wrap.go`

**What it does well**
- Attempts to handle ANSI sequences when cutting/wrapping lines.
- Uses `go-runewidth` for correct display width calculations.

**Findings**
- **MAJOR (inconsistency/duplication)**:
  - `stripANSI` and `hasANSI` use a regex that matches CSI sequences (`\x1b\[[0-9;]*[A-Za-z]`) but does not cover OSC sequences.
  - `ansiCutToWidth` and `ansiCutToWidthWithRemainder` implement similar ANSI scanning logic twice, including OSC handling, but not identically.
  - Search/highlighting decisions depend on `hasANSI` and `stripANSI`; if the log stream contains OSC escapes, search may misbehave.

**Recommendation**
- Consolidate ANSI parsing/cutting into one utility:
  - one state machine that can:
    - strip escapes,
    - compute width,
    - cut with remainder,
    - detect whether a line contains any escapes.
  - or adopt a small external library if one is already a dependency elsewhere in the workspace.

#### D7) The core hotspot: `monitor_view.go`

This file is the heart of the TUI monitor and the primary maintenance risk.

**What it does well**
- Implements the core UX spec:
  - DEVICE vs HOST mode
  - follow behavior, viewport scrolling
  - search UI (decorated matches, navigation)
  - filter overlay integration (levels + regex + highlight rules)
  - inspector with detail overlays
  - core dump capture overlay and abort semantics
- Includes protective buffering:
  - `log` capped at 4000 lines
  - `out` capped at ~1 MiB with 512 KiB retention

**Findings**
- **CRITICAL (UI freeze risk)**: external decode work is performed inline in `Update`:
  - `panic.DecodeBacktraceLine` may run multiple `addr2line` processes synchronously.
  - `coredump.PushLine` triggers `decodeOrRaw` at end marker, which can run python decode synchronously with no context/timeout.
- **MAJOR (mixed responsibilities / god file)**:
  - serial I/O loop behavior
  - pipeline decode
  - buffer management
  - filter/search logic and rendering decoration
  - inspector event management
  - session logging (filesystem)
  - UI rendering layout
  live together, making changes risky.
- **MAJOR (duplication with tail)**:
  - The pipeline logic (gdb detection, core dump capture, panic decode, autocolor, tail finalize) is structurally the same as in `pkg/tail`.
- **MAJOR (state redundancy)**:
  - `m.out` is a big string buffer, but the viewport is derived from `m.log` lines; the duplication invites drift and confusion (buffer trimming only applies to `m.out`).
  - `m.viewport.SetContent(m.out)` in `setSize` suggests older design; actual content is usually set via `refreshViewportContent()` which uses `m.log`.
- **MINOR (allocation)**: `readSerialCmd` allocates a new 4096-byte slice each read and then copies `buf[:n]` into a new slice.
- **MINOR (search decoration vs wrap)**: wrapping is disabled when search is active (`if m.wrap && !m.searchActive`), which is a pragmatic simplification, but worth calling out as a deliberate UX tradeoff.

**Recommendations (structural)**
1) Extract the pipeline into a package-level component, returning structured events:
   - `LogLine` output
   - `GDBStubDetected`
   - `CoreDump{Start,Progress,End,Report}`
   - `BacktraceDecoded`
   - plus “actions” like `SendEnter` requests
2) Make expensive decode work asynchronous:
   - pipeline emits “needs decode” events,
   - a `tea.Cmd` performs decode and sends results back as messages,
   - UI stays responsive and can show “decoding…” status.
3) Split `monitor_view.go` into:
   - `monitor_io.go` (read loop + message wiring)
   - `monitor_pipeline.go` (pure state machine, testable)
   - `monitor_render.go` (View + layout)
   - `monitor_search.go` and `monitor_filter.go` (logic + decoration)
   - `monitor_inspector.go` (event model + view)

### E) Duplication / repetition register (explicit list)

This section lists concrete duplicated patterns and where they live.

**Tracking tickets:** ESP-12 (pipeline duplication), ESP-15 (small dedupe/extractions), ESP-20 (ANSI consolidation).

1) **Tail pipeline duplicated between modes**
- `esper/pkg/tail/tail.go`: `runLoop` vs `runLoopWithStdinRaw`
- Recommendation: unify into shared “processChunk” + distinct input/write handling.

2) **Pipeline duplicated between tail and TUI**
- Tail: `esper/pkg/tail/tail.go`
- TUI: `esper/pkg/monitor/monitor_view.go` (serialChunkMsg handler + tick finalize-tail)
- Recommendation: extract `pkg/pipeline` with shared state + event outputs.

3) **Confirm overlays duplicated**
- Generic: `esper/pkg/monitor/confirm_overlay.go`
- Reset-specific: `esper/pkg/monitor/reset_confirm_overlay.go`
- Recommendation: delete reset overlay and instantiate generic confirm overlay with custom text + forward msg.

4) **ANSI escape handling duplicated**
- `esper/pkg/monitor/ansi_text.go` (strip/has/width/cut)
- `esper/pkg/monitor/ansi_wrap.go` (cut-with-remainder and wrapping)
- Recommendation: unify into a single ANSI utility with consistent escape coverage.

5) **Shared UI helpers in surprising location**
- `min/max/padOrTrim/updateSingleLineField` live in `port_picker.go` but are used across monitor package.
- Recommendation: move to `ui_helpers.go` for discoverability.

### F) “Deprecated/unfinished hooks” register

1) **Probe with esptool in TUI**
- UI exposes a toggle (`port_picker.go`) and plumbing includes `connectParams.probeEsptool` (`messages.go`).
- `connectCmd` explicitly does not implement probing yet.
- Recommendation: implement with explicit confirmation UX, or remove toggle until implemented.
  - **Tracking tickets:** ESP-17 (TUI probe integration), ESP-19 (remove python / Go-native probe backend).

2) **Legacy buffer field `monitorModel.out`**
- Used for “Buf” status and some older viewport calls; viewport content is derived from `log`.
- Recommendation: either:
  - fully switch to a single source of truth (`log`), or
  - clearly document why both exist and keep them in sync.
  - **Tracking tickets:** ESP-14 (split monitor_view; best time to rationalize buffers), ESP-12 (pipeline extraction can simplify “single source of truth”).

## Design Decisions

This section focuses on design decisions implied by the current implementation and where they may be improved.

1) **“UI owns the pipeline”**
- Current: pipeline state lives inside `monitorModel`.
- Tradeoff: simpler wiring vs reduced reuse and harder testing of pipeline in isolation.

2) **External tool parity**
- Current: python `esp_coredump` and toolchain `addr2line`.
- Tradeoff: parity vs blocking behavior and dependency management.

3) **Overlay routing rule**
- Current: key events captured by overlay; non-key events still reach active screen.
- This is a strong decision; it prevents “UI freezes” due to modal overlays blocking serial/tick updates.

4) **Ring buffers**
- Current: output line ring + byte ring.
- This is necessary for a long-running serial monitor, but the implementation should converge on one source of truth to reduce confusion.

## Alternatives Considered

1) **Keep duplication and “copy/paste drift”**
- Reject: behavior drift between tail and TUI will become a long-term maintenance tax.

2) **Rewrite decoding in pure Go**
- Defer: might be worth it long-term, but Phase 1 parity is better served by using ESP-IDF tooling.

3) **Move all decode work into goroutines without message discipline**
- Reject: in Bubble Tea, it’s safer and more testable to use `tea.Cmd` → message patterns, which centralize state transitions through `Update`.

## Implementation Plan

See `playbook/01-follow-ups-and-refactor-plan.md` for a staged plan with validation checkpoints.

At a high level:
1) Build a shared pipeline package (no behavior change; tests + golden output).
2) Refactor tail to use the pipeline (delete duplicated code).
3) Refactor TUI to use the pipeline (delete duplicated code).
4) Make decode asynchronous in TUI (behavior change: UI responsiveness improves; ensure correctness).
5) Clean up overlays and helpers (delete small duplications; improve discoverability).

## Open Questions

1) What is the acceptable behavior when python/toolchain dependencies are missing?
   - Today: decoding falls back to partial output (PC list) or “no -elf provided”.
   - Decide whether to add more explicit notices and/or UX guidance.

2) Should core dump decoding be:
   - enabled by default in TUI,
   - enabled by default in tail,
   - or “opt-in” because of dependency/latency risk?

3) Should scan/nickname resolution be:
   - Linux-only by design, or
   - abstracted for cross-platform?

4) How much should we invest in ANSI correctness?
   - Today: mostly CSI is handled; OSC is partially handled in wrapping but not in regex stripping.

5) Do we want a stable external “event export” interface (e.g. JSON of inspector events)?
   - That might be valuable for automation and bug reports.

## References

- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/design-doc/01-esper-architecture-big-picture.md`
- `ttmp/2026/01/25/ESP-11-REVIEW-ESPER--esper-postmortem-review-architecture-code-audit/playbook/01-follow-ups-and-refactor-plan.md`
- Code entrypoints:
  - `esper/cmd/esper/main.go`
  - `esper/pkg/tail/tail.go`
  - `esper/pkg/monitor/app_model.go`
  - `esper/pkg/monitor/monitor_view.go`
