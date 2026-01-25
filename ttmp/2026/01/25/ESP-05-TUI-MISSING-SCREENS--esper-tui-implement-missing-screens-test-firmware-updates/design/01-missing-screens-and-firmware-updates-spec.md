---
Title: "ESP-05-TUI-MISSING-SCREENS: spec"
Ticket: ESP-05-TUI-MISSING-SCREENS
Status: active
Topics:
  - tui
  - ux
  - bubbletea
  - lipgloss
  - firmware
  - testing
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
  - esper/pkg/monitor/app_model.go
  - esper/pkg/monitor/monitor_view.go
  - esper/pkg/monitor/overlay_manager.go
  - esper/pkg/monitor/overlay_wrappers.go
  - esper/pkg/decode/coredump.go
  - esper/firmware/esp32s3-test/README.md
Summary: "Implementation spec for missing screens + firmware updates to deterministically trigger and validate them."
LastUpdated: 2026-01-25T15:09:16-05:00
---

# ESP-05 — Missing screens + firmware updates (spec)

This ticket implements the screens that exist in the contractor UX spec but are missing from the current implementation, and updates the `esp32s3-test` firmware (and capture harness workflow) to make these screens easy to trigger and validate.

Primary UX reference (wireframes + keymap):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md`

“What’s missing right now” reference:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md` (see “Screens still missing”)

## Non-negotiable constraints

- Route `tea.KeyMsg` and `tea.WindowSizeMsg` explicitly.
- HOST-only shortcuts must never leak into DEVICE mode input.
- Use Lip Gloss for layout/styling (borders, padding, colors); do not hand-draw UI boxes.

## Deliverable scope (screens)

### A) Monitor / Inspector missing screens (UX spec §1.3)

Wireframe sections to implement:
- “Panic Detail View”
- “Core Dump Capture In Progress”
- “Core Dump Report”

Desired behavior:
- The Inspector list remains the “index” of events.
- Pressing Enter on an event opens a full-screen detail view (or a large modal/overlay) appropriate to the event type:
  - `panic` → Panic Detail View
  - `coredump` → Core Dump Report View
- While a core dump is being buffered, show a “capture in progress” overlay that does not block serial processing.

### B) Confirmation dialogs (UX spec §2.5)

Wireframe sections to implement:
- “Reset Device” confirmation dialog

Desired behavior:
- Safe default is Cancel.
- Confirm triggers a host-side reset action (USB Serial/JTAG) and logs an event/toast if reset fails.

### C) Port picker + device registry screens (UX spec §1.1 + §1.2)

Wireframe sections to implement:
- “Device Manager View” (+ empty state)
- “Assign Nickname Dialog”
- “Edit Existing Nickname”

Desired behavior:
- Device registry entries come from `esper/pkg/devices` registry (`devices.json`).
- Device Manager is accessible from Port Picker and allows:
  - listing entries
  - adding/editing/removing nicknames
  - returning to port picker
- Port Picker should display nicknames for known devices (instead of always “—”).

## Enabling plumbing requirements

### 1) Overlays must not block non-key messages

Problem:
- The app-level overlay system must not swallow serial/tick messages while an overlay is open, otherwise:
  - core dump capture cannot progress
  - progress overlay cannot auto-close

Spec:
- When an overlay is active:
  - `tea.KeyMsg` routes to the overlay first.
  - Non-key msgs still route to the active screen model.

### 2) Deterministic open/close for auto-overlays

Spec:
- Add explicit “close overlay” message(s) handled by `appModel` regardless of overlay state.
- Allow screen models to request:
  - open overlay
  - replace overlay (optional)
  - close overlay

## Firmware updates (esp32s3-test)

Goal: deterministic triggers for these UI states, without manual typing.

Required firmware capabilities:
- Slow/long “core dump marker emission” so the “capture in progress” overlay is visible long enough to observe (multi-second).
- One-shot “emit a suite of events” command so a capture run can reliably produce:
  - logdemo
  - partial line
  - gdb stub packet
  - core dump markers (slow)
  - panic (optional / last)

Notes:
- The firmware console must remain USB Serial/JTAG (`esp_console`) per repo AGENTS guidance.

## Verification / iteration loop

Use the existing capture harness:
- Real hardware:
  - `FIRMWARE_TRIGGERS=logdemo,partial,gdbstub,coredumpfake ./ttmp/2026/01/24/ESP-02-.../scripts/09-tmux-capture-esper-tui.sh`
- Virtual PTY (fallback):
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-.../scripts/09-tmux-capture-esper-tui.sh`

Completion criteria:
- We can capture 120×40 screenshots for each missing wireframe screen and compare visually against the contractor spec.
