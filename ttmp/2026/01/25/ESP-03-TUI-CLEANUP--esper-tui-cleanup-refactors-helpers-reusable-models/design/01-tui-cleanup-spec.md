---
Title: "ESP-03-TUI-CLEANUP: cleanup/refactor spec"
Ticket: ESP-03-TUI-CLEANUP
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - refactor
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
  - esper/pkg/monitor/app_model.go
  - esper/pkg/monitor/monitor_view.go
  - esper/pkg/monitor/help_overlay.go
  - esper/pkg/monitor/port_picker.go
  - esper/pkg/monitor/styles.go
Summary: "Spec for refactoring the esper TUI to reduce duplication: shared UI helpers, reusable list models, unified overlay routing, and semantic styling."
LastUpdated: 2026-01-25T18:50:00-05:00
---

# ESP-03-TUI-CLEANUP — cleanup/refactor spec

This is a structural cleanup ticket. UX intent remains the same; the focus is reducing duplicated UI code, unifying rendering patterns, and making future UX iteration faster.

Design references:
- UX iteration loop playbook (process guide):
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/01-ux-iteration-loop.md`
- Architecture notes and refactor ideas:
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

## Goals

- Reduce copy/paste UI code (lists, focus handling, modal chrome, footer/status formatting).
- Keep a unified look across screens (same padding, borders, title conventions, key hint styling).
- Make overlay behavior consistent across all screens and overlays (help/search/filter/palette/confirmations).
- Preserve correctness and safety:
  - explicit `tea.KeyMsg` + `tea.WindowSizeMsg` routing
  - HOST-only shortcuts stay HOST-only
  - Lip Gloss used for borders/layout (no hand-drawn boxes)

## Non-goals

- Changing UX semantics beyond what is needed to enable the refactor.
- Implementing missing wireframe screens (handled by ESP-04-TUI-VISUALS).

## Current architecture (baseline)

- `appModel` handles:
  - screen routing (port picker vs monitor)
  - help overlay (global overlay)
  - window size fan-out
- `monitorModel` handles:
  - viewport/log state + input
  - search/filter/palette overlays (HOST-only)
- `portPickerModel` handles:
  - port list + form fields + connect action

## Proposed refactor components

### 1) Unified overlay stack

Problem:
- Help overlay is app-level, search/filter/palette are monitor-level. Close behavior and rendering patterns differ.

Spec:
- Introduce a small overlay interface, owned by `appModel`, e.g.:
  - `SetSize(size)`
  - `Open()`, `Close()`
  - `Update(tea.Msg) (overlay, tea.Cmd, overlayResult)`
  - `View(styles) string`
- `appModel` holds at most one active overlay at a time (future: a stack if needed).
- Screens can request overlay changes via actions/messages (not by directly mutating overlay state in multiple places).

Acceptance criteria:
- Help/search/filter/palette/confirmation overlays all close consistently (`Esc`), and all resize consistently.
- Overlay routing logic exists in one place (app), not duplicated across screen models.

### 2) Reusable selectable list model

Problem:
- Port list, command palette list, and inspector event list have similar selection/scroll window logic implemented separately.

Spec:
- Create a reusable list core:
  - selection index
  - visible window start/end
  - key handling for up/down/page
  - render callbacks (`RenderRow(i, item, focused)`), plus optional separators.

Acceptance criteria:
- Port picker, palette, and inspector use the same core list behavior and feel identical to navigate.

### 3) Modal chrome + bottom bar helpers

Problem:
- Each modal/overlay hand-builds its title, hint line, error banner, padding.

Spec:
- Add shared render helpers:
  - `RenderModal(title, body, hint, error)`
  - `RenderBottomBar(left, middle, right)` (for wireframe search bar, compact hints, etc.)
  - `RenderFramedScreen(title, main, status, footer)` (optional)

Acceptance criteria:
- Modals share consistent spacing and title style, and adding a new modal requires minimal boilerplate.

### 4) Semantic styles

Problem:
- Styles are mostly “layout primitives”; screens sometimes invent ad-hoc formatting.

Spec:
- Extend `styles` with semantic roles:
  - `KeyHint`, `Divider`, `PrimaryButton`, `SecondaryButton`, `Muted`, `Accent`, etc.

Acceptance criteria:
- Screens and overlays use semantic styles for consistent look.

## Validation approach

- `cd esper && go test ./...`
- Screenshot harness remains unchanged and continues to work:
  - `USE_VIRTUAL_PTY=1 ./ttmp/.../scripts/09-tmux-capture-esper-tui.sh`
- Visual regressions acceptable only if they are immediately re-aligned by the same commit or a follow-up in ESP-04.

