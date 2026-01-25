---
Title: "ESP-05-TUI-MISSING-SCREENS: tasks"
Ticket: ESP-05-TUI-MISSING-SCREENS
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - firmware
  - testing
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
Summary: "Step-by-step task list for implementing all missing screens and the firmware/capture support needed to validate them."
LastUpdated: 2026-01-25T15:17:26-05:00
---

# Tasks

## TODO (check off in order)

Ticket hygiene:
- [x] Write spec doc (`design/01-missing-screens-and-firmware-updates-spec.md`)
- [x] Start diary (`reference/01-diary.md`) and keep it updated per commit
- [x] Add ticket scripts for reproducible capture/firmware steps

Enabling plumbing (must happen before “auto overlays”):
- [x] TUI plumbing: overlays must not block non-key msgs (serial/tick) while open
- [x] TUI plumbing: add `closeOverlayMsg` (and/or `replaceOverlayMsg`) so screens can close auto-overlays deterministically

Missing monitor/inspector screens (wireframe §1.3):
- [ ] Screen: Panic Detail View (open from Inspector event; scrollable; copy/save affordances as per spec)
- [ ] Screen: Core Dump Capture In Progress overlay (auto-show while buffering)
- [ ] Screen: Core Dump Report view (open from event; scrollable; show decode status; raw + report)

Missing confirmation dialogs (wireframe §2.5):
- [ ] Screen: Reset Device confirmation overlay (safe default: Cancel)
- [ ] Host action: implement device reset mechanism (USB Serial/JTAG) + error handling

Missing port picker / device registry screens (wireframes §1.1 + §1.2):
- [ ] Screen: Device Manager view (list entries, empty state)
- [ ] Screen: Assign Nickname dialog (for selected connected device)
- [ ] Screen: Edit Existing Nickname dialog
- [ ] Wire port picker ↔ device manager navigation keys per spec

Test firmware updates (esp32s3-test):
- [x] Firmware: add slow/long core dump emission mode so “capture in progress” overlay is visible for multiple seconds
- [x] Firmware: add one-shot “emit all events” command for deterministic test runs (no manual typing)
- [x] Firmware docs: update `esper/firmware/esp32s3-test/README.md` for new commands

Capture harness + artifacts:
- [ ] Capture harness: add “trigger suite” script in this ticket that runs tmux capture with firmware triggers
- [ ] Capture: produce and commit tmux screenshots for every newly implemented screen at 120×40 (and optional 80×24)
- [ ] Compare doc: update `ttmp/2026/01/24/ESP-02-.../various/02-tui-current-vs-desired-compare.md` with new “current” screenshots for the missing screens
