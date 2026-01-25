---
Title: Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/decode/coredump.go
      Note: Informs UX requirements for capture/mute/report flows
    - Path: esper/pkg/monitor/monitor.go
      Note: Key source inspected to understand current UI constraints
    - Path: esper/pkg/scan/scan_linux.go
      Note: Informs port picker behavior and safety warnings
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/design-doc/01-esper-tui-ux-design-brief-for-contractor.md
      Note: Primary output of this ticket
ExternalSources: []
Summary: Investigation diary for creating the ESP-02-ESPER-TUI contractor UX design brief.
LastUpdated: 2026-01-24T20:45:27.640530039-05:00
WhatFor: Capture the research trail (commands, files, decisions) used to produce the Esper TUI UX design brief.
WhenToUse: Use when reviewing why the brief says what it says, or when continuing UX/TUI work later.
---


# Diary

## Goal

Create ticket `ESP-02-ESPER-TUI`, analyze `esper/`, and write a contractor-ready UX design brief for a full-screen TUI for `esper` (Go ESP-IDF-compatible serial monitor).

## Step 1: Repo orientation and scope finding

I first located the `esper/` subtree in this monorepo and confirmed what `esper` currently does vs what we want to design. The goal of this step was to avoid designing a TUI in a vacuum and instead ground it in the actual behaviors already implemented: auto-coloring, panic/backtrace decoding, core dump capture/decoding, and GDB stub detection.

I also skimmed an existing monitor design doc in a prior ticket to align with previously-articulated “idf.py monitor parity” expectations, because that implies strong UX constraints (safe host commands, robust handling of partial lines, decoding event workflows).

### Prompt Context

**User prompt (verbatim):** "Create a ticket ESP-02-ESPER-TUI and analyze esper/ and create a design brief for a contractor UX designer specializing in TUIs to build a TUI for esper.\n\nStore the design brief in the new ticket.\n\nKeep a detailed diary as you research and investigate and write the brief."

**Assistant interpretation:** Create a new docmgr ticket and produce a UX design brief for a TUI for the `esper` tool, documenting the investigation process as a diary.

**Inferred user intent:** Enable hiring/contracting a specialist TUI UX designer with enough concrete requirements and constraints to deliver implementable wireframes/keymaps, while keeping an audit trail of what was examined and why.

**Commit (code):** N/A

### What I did
- Listed repo root to confirm `esper/` exists and to locate the `ttmp/` docs root.
- Read `esper/README.md` to understand stated goals: Bubble Tea minimal UI + ESP-IDF monitor-compatible behaviors.
- Inspected key source files to understand existing behaviors and what UX must surface:
  - `esper/cmd/esper/main.go` (CLI flags, default behavior runs monitor)
  - `esper/pkg/monitor/monitor.go` (current minimal UI model, input semantics, output buffer behavior)
  - `esper/pkg/commands/scancmd/scan.go` and `esper/pkg/scan/scan_linux.go` (port scanning + scoring + optional `esptool` probe warning)
  - `esper/pkg/decode/*` (panic/backtrace decode; core dump capture/decode; GDB stub detection)
- Reviewed the earlier ticket design doc:
  - `ttmp/2026/01/24/ESP-01-SERIAL-MONITOR--serial-monitor-structured-parsing-multiplexed-capabilities/design-doc/02-go-serial-monitor-idf-py-compatible-design-implementation.md`

### Why
- The UX brief must be consistent with how the monitor works (and is intended to work) so that contractor designs are implementable without re-litigating foundational behaviors.

### What worked
- `esper` is already structured around Bubble Tea and has clear “special events” that naturally map to UI states (core dump capture, panic decode output, GDB stub detection).
- `esper scan` provides a strong foundation for an interactive port picker / connect screen.

### What didn't work
- N/A (no implementation attempts in this step; investigation only).

### What I learned
- The current monitor view is a minimal “output string + input box” and does not yet provide scrollback/search/filter or safe host-command modality; those are key UX gaps to address.
- Core dump behavior explicitly mutes normal output while buffering, and may auto-send Enter when prompted; that must be communicated clearly in the UI.

### What was tricky to build
- N/A (research only).

### What warrants a second pair of eyes
- Ensure the UX brief’s “host commands vs device input” model matches the team’s expectations for parity with `idf.py monitor` and doesn’t conflict with existing firmware console usage patterns.

### What should be done in the future
- Capture a few real-world serial log transcripts (panic, coredump, gdb stub) to give the designer realistic content for wireframes and edge-case rendering.

### Code review instructions
- Start at `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/design-doc/01-esper-tui-ux-design-brief-for-contractor.md`.
- Cross-check constraints by skimming `esper/pkg/monitor/monitor.go` and `esper/pkg/decode/coredump.go`.

### Technical details
- Commands run (representative):
  - `ls`
  - `ls esper`
  - `sed -n '1,200p' esper/README.md`
  - `sed -n '1,260p' esper/pkg/monitor/monitor.go`
  - `sed -n '1,260p' esper/pkg/commands/scancmd/scan.go`
  - `sed -n '1,320p' esper/pkg/decode/coredump.go`

## Step 2: Ticket creation + doc scaffolding (docmgr)

I created the `ESP-02-ESPER-TUI` ticket workspace using `docmgr` and added two documents: a design-doc for the contractor UX brief and a reference doc to serve as the diary. This step ensures the brief and its research trail live in the repo’s standard ticket structure for discoverability and future updates.

I noted that the environment clock was `2026-01-24` at the time of creation, so docmgr placed the ticket under `ttmp/2026/01/24/` (even though “today” in conversation may be considered `2026-01-25` depending on timezone).

### Prompt Context

**User prompt (verbatim):** "Create a ticket ESP-02-ESPER-TUI and analyze esper/ and create a design brief for a contractor UX designer specializing in TUIs to build a TUI for esper.\n\nStore the design brief in the new ticket.\n\nKeep a detailed diary as you research and investigate and write the brief."

**Assistant interpretation:** Use docmgr to create a ticket workspace and store both the design brief and the diary within it.

**Inferred user intent:** Keep UX work auditable and aligned with the project’s documentation workflow.

**Commit (code):** N/A

### What I did
- Created the ticket:
  - `docmgr ticket create-ticket --ticket ESP-02-ESPER-TUI --title "Esper: contractor UX design brief for a TUI serial monitor" --topics serial,console,ui,tooling,esp-idf,debugging,usb-serial-jtag`
- Added the design brief document:
  - `docmgr doc add --ticket ESP-02-ESPER-TUI --doc-type design-doc --title "Esper TUI: UX design brief for contractor"`
- Added the diary document:
  - `docmgr doc add --ticket ESP-02-ESPER-TUI --doc-type reference --title "Diary"`

### Why
- Putting this in `ttmp/` makes it searchable, reviewable, and consistent with other tickets in the repo.

### What worked
- Docmgr created the expected ticket directory structure and frontmatter.

### What didn't work
- N/A.

### What I learned
- Docmgr’s ticket date folder is determined by the environment time (`date -Iseconds`).

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- Verify the chosen ticket topics match the project vocabulary and are appropriate for `esper` work.

### What should be done in the future
- Add a short “how to run `esper` + test firmware” playbook if the designer or reviewers need to reproduce screens with live data.

### Code review instructions
- Check the created paths under `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/`.

### Technical details
- Environment time check:
  - `date -Iseconds`

## Step 3: Write the contractor UX design brief

I translated the technical behaviors in `esper/` into explicit UX requirements and deliverables. The brief focuses on (a) screen inventory and state machine, (b) a safety-critical key model that separates host commands from device input, and (c) core workflows (connect/monitor/search/filter/inspect special events) that make `esper` a “daily driver” for embedded engineers.

I kept it implementation-aware but not code-prescriptive: the intent is that a designer can propose a coherent, keyboard-first UI that an engineer can implement in Bubble Tea without unplanned feature discovery.

### Prompt Context

**User prompt (verbatim):** "Create a ticket ESP-02-ESPER-TUI and analyze esper/ and create a design brief for a contractor UX designer specializing in TUIs to build a TUI for esper.\n\nStore the design brief in the new ticket.\n\nKeep a detailed diary as you research and investigate and write the brief."

**Assistant interpretation:** Produce a contractor-ready UX design brief grounded in the actual `esper` code and intended behaviors.

**Inferred user intent:** Make contracting efficient: a designer can start immediately and deliver artifacts engineers can implement without ambiguity.

**Commit (code):** N/A

### What I did
- Authored the brief in:
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/design-doc/01-esper-tui-ux-design-brief-for-contractor.md`
- Included:
  - deliverables checklist for the contractor,
  - IA (Port Picker / Monitor / Inspector + overlays),
  - key UX behaviors (follow mode, wrap, search, filter, copy/selection),
  - special event UX (panic/backtrace, core dump capture/report, GDB stub detection),
  - non-goals and open questions.

### Why
- These are the highest leverage UX choices for terminal tools and are easy to mis-specify without grounding in the underlying serial-monitor semantics.

### What worked
- The existing `esper` pipeline maps naturally to UI states, especially for “core dump capture in progress” and “panic/backtrace detected”.

### What didn't work
- N/A.

### What I learned
- The most important UX constraint is preventing accidental keystrokes from being sent to the device; this must be treated as a design requirement, not a polish item.

### What was tricky to build
- Choosing defaults (wrap on/off, entry behavior when `--port` missing) without user feedback; I surfaced these as explicit open questions.

### What warrants a second pair of eyes
- Validate the proposed screen inventory is not over-scoped for an initial implementation and that it fits Bubble Tea realities at 80×24.

### What should be done in the future
- After designer delivery, add an engineering-facing implementation plan ticket that maps each UX component to Bubble Tea/Bubbles primitives and identifies risks (ANSI rendering, selection/copy, performance under high log volume).

### Code review instructions
- Review the brief end-to-end and sanity check it against `esper/README.md` and `esper/pkg/monitor/monitor.go`.

### Technical details
- N/A.

## Step 4: Create Full UX Specification with Wireframes

This step produced the comprehensive UX specification document that transforms the high-level design brief into implementable screen layouts, widget specifications, and keyboard mappings. The goal was to provide enough visual and interaction detail that an engineer can build the TUI directly from this document without ambiguity.

I created detailed ASCII wireframes for every screen state (Port Picker, Monitor in device/host modes, Inspector panel, all overlays) at both 80×24 and 120×40 terminal sizes. Each screen is accompanied by YAML pseudocode that describes the widget hierarchy, properties, and keybindings in a structured but non-code format suitable for Bubble Tea implementation reference.

### What I did
- Created new reference doc: `reference/02-esper-tui-full-ux-specification-with-wireframes.md`
- Designed 5 primary screens with ASCII wireframes:
  - Port Picker (normal + error states)
  - Monitor View (device mode, host mode, 80×24 compact)
  - Inspector Panel (side panel, panic detail, core dump capture, core dump report)
- Designed 5 overlay wireframes:
  - Help overlay (keyboard reference grid)
  - Search overlay (inline at bottom)
  - Filter overlay (modal with levels, regex, highlight rules)
  - Command Palette (fuzzy-searchable command list)
  - Confirmation dialogs (reset, clear, quit variants)
- Created YAML widget pseudocode for every screen describing:
  - Widget hierarchy and types
  - Properties and configurable values
  - Keybindings per context
  - State transitions
- Documented complete keyboard mapping across all modes
- Created application state machine diagram
- Specified component behaviors (viewport, input, status bar, notifications, copy/selection)
- Documented error states and edge cases
- Added implementation hints and "must-not-do" list for Bubble Tea

### Why
- The original design brief specifies *what* the TUI should do but not *how it should look*
- Engineers need exact layouts to implement without design iteration
- YAML pseudocode bridges UX design and Bubble Tea model structure

### What worked
- ASCII wireframes at two sizes (80×24 and 120×40) cover the constraint space well
- YAML pseudocode format is readable by both designers and engineers
- State machine diagrams clarify complex mode transitions (device ↔ host ↔ overlays)

### What didn't work
- N/A (documentation only, no implementation)

### What I learned
- The safety-critical separation of "device mode" vs "host mode" is the central UX decision; it must be extremely clear in every wireframe
- Core dump capture is a special blocking state that needs its own modal treatment
- Inspector as a toggle-able side panel works better than a modal for ongoing reference

### What was tricky to build
- Fitting all essential info in 80×24 while keeping it usable
- Balancing vim-style keybindings (j/k/g/G) with more discoverable alternatives (arrows, PgUp/PgDn)
- Designing the filter overlay to be powerful (regex, levels, highlights) without overwhelming

### What warrants a second pair of eyes
- Verify the keybinding scheme doesn't conflict with common terminal emulator shortcuts
- Confirm the YAML pseudocode structure maps cleanly to Bubble Tea Model patterns
- Review the 80×24 compact mode—does it sacrifice too much?

### What should be done in the future
- Create a prototype video/GIF showing the intended interaction flow
- Validate keybindings with actual users to catch muscle-memory conflicts
- Add wireframes for session logging indicators and reconnection flow

### Code review instructions
- Start at `reference/02-esper-tui-full-ux-specification-with-wireframes.md`
- Compare ASCII wireframes against the requirements in `design-doc/01-esper-tui-ux-design-brief-for-contractor.md`
- Verify every open question from the brief is addressed or explicitly deferred

### Technical details
- Document structure:
  - Part 1: Screen Inventory (Port Picker, Monitor, Inspector)
  - Part 2: Overlays (Help, Search, Filter, Command Palette, Confirmations)
  - Part 3: State Machine (app states, monitor substates)
  - Part 4: Keyboard Mapping (global, per-view, per-overlay)
  - Part 5: Component Behaviors (viewport, input, status bar, notifications, copy)
  - Part 6: Error States
  - Part 7: Implementation Hints
  - Part 8: Summary

## Step 5: Integrate Device Registry / Nickname UX

This step incorporated the device registry functionality from ESP-02-DEVICE-REGISTRY into the TUI UX specification. The registry allows users to assign stable nicknames to devices identified by their USB serial number, so they can refer to boards ergonomically (`esper --port atoms3r`) instead of ephemeral paths (`/dev/ttyACM0`).

I reviewed the ESP-02-DEVICE-REGISTRY design doc to understand the data model (JSON in XDG config), CLI surface (`esper devices list/set/remove`), and integration points (scan annotation, nickname resolution). Then I designed screens for:
- Enhanced Port Picker showing nicknames, names, and descriptions
- Assign Nickname dialog (triggered by `n` key on selected port)
- Full Device Manager view for registry CRUD operations

### What I did
- Updated Port Picker wireframe to show nickname/name columns instead of raw paths
- Added "Assign Nickname" dialog wireframe with USB serial display, nickname/name/description fields
- Added "Edit Device" dialog variant with pre-filled values and Remove button
- Created Device Manager view (new section 1.2) with:
  - Device list table showing online/offline status
  - Detail section for selected device
  - Empty state guidance
  - Add Device manual entry dialog
- Updated state machine diagram to include device_manager state
- Updated state definitions YAML with device_manager transitions and new overlays
- Added overlay definitions for assign_nickname, add_device, edit_device
- Added keyboard mappings: Port Picker (`n`, `d`), Device Manager (`e`, `x`, `c`, `a`)
- Updated Model structure with registry and device_manager state
- Updated summary to reflect 7 screens and 8 overlays

### Why
- Device registry is a key ergonomic feature that needs proper TUI integration
- Users should be able to assign nicknames directly from the port picker without CLI commands
- Device Manager provides full visibility and control over the registry

### What worked
- The Port Picker naturally accommodates nickname display in place of raw paths
- `n` key for nickname feels intuitive (similar to vim's "name" mental model)
- Device Manager as a separate view keeps Port Picker focused on quick selection

### What didn't work
- N/A (UX design, no implementation)

### What I learned
- The USB serial (e.g., `D8:85:AC:A4:FB:7C`) needs to be displayed in the Assign Nickname dialog so users understand what identifier is being mapped
- Online/offline status in Device Manager helps users understand which registered devices are currently connected

### What was tricky to build
- Balancing discoverability (`[n: assign]` hint on unregistered ports) vs. visual noise
- Deciding where Add Device manual entry should live (Device Manager only, since it requires knowing the USB serial)

### What warrants a second pair of eyes
- Confirm the keyboard shortcuts (`n`, `d`, `e`, `x`, `c`, `a`) don't conflict with other contexts
- Verify the Device Manager is reachable both from Port Picker and via `esper devices` subcommand
- Review whether the Port Picker should show all registry entries (even offline) or only currently-connected ports

### What should be done in the future
- Add import/export functionality for device registry (share across machines)
- Consider auto-populating Name from chip info when assigning nickname
- Add "learn from scan" ergonomics (batch-assign nicknames to multiple devices)

### Code review instructions
- Review updated screens in `reference/02-esper-tui-full-ux-specification-with-wireframes.md`:
  - Section 1.1 (Port Picker) for nickname integration
  - New Section 1.2 (Device Manager)
- Cross-check against `ESP-02-DEVICE-REGISTRY` design doc for consistency
- Verify state machine and keyboard mappings are complete

### Technical details
- New screens added:
  - Port Picker with registry columns (nickname, name, chip, status)
  - Assign Nickname dialog
  - Edit Device dialog
  - Device Manager view
  - Add Device dialog
- New keybindings:
  - Port Picker: `n` (assign nickname), `d` (device manager)
  - Device Manager: `e`/Enter (edit), `x` (remove), `c` (connect), `a` (add)
- Registry data model:
  - Stored in `~/.config/esper/devices.json`
  - Keyed by `usb_serial`
  - Fields: nickname (required), name, description, preferred_path
