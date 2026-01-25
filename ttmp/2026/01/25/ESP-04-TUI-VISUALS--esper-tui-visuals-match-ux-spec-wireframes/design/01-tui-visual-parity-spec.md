---
Title: "ESP-04-TUI-VISUALS: visual parity spec"
Ticket: ESP-04-TUI-VISUALS
Status: active
Topics:
  - tui
  - ux
  - visuals
  - bubbletea
  - lipgloss
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
  - esper/pkg/monitor/monitor_view.go
  - esper/pkg/monitor/search_overlay.go
  - esper/pkg/monitor/filter_overlay.go
  - esper/pkg/monitor/palette_overlay.go
  - esper/pkg/monitor/help_overlay.go
Summary: "Spec to close the gap between current esper TUI behavior/visuals and the contractor wireframes, using screenshot-based verification."
LastUpdated: 2026-01-25T18:50:00-05:00
---

# ESP-04-TUI-VISUALS — visual parity spec

Design reference (wireframes + keymaps):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md`

Current vs desired comparison:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

Iteration process:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/01-ux-iteration-loop.md`

## Scope notes

- The dedicated “80×24 compact view” wireframe is **out of scope**; a responsive/scaled-down main view is acceptable and preferred.

## Goals (what “done” means)

- The key wireframe screens are implemented and visually close enough for rapid “scan + act” debugging.
- Screens can be verified via deterministic tmux capture at 120×40 (and optionally 80×24 scaled-down).

## Work areas

### A) Search overlay parity (§2.2)

Target behavior:
- Replace centered modal with bottom bar.
- Show live match count and current match index.
- Provide match highlighting / match markers in the viewport similar to “← MATCH i/n”.

### B) Filter overlay parity (§2.3)

Target behavior:
- Add Debug/Verbose levels (D/V) to filtering (best-effort).
- Add highlight rules editor (pattern → style) and apply highlight styles to log lines.

### C) Command palette parity (§2.4)

Target behavior:
- Add separators/grouping.
- Add missing commands (wireframe): wrap toggle, reset device (confirmation), send break, session logging, reconnect.

### D) Inspector parity + missing screens (§1.3)

Target behavior:
- Richer event list presentation (counts, hints).
- Add panic detail view.
- Add core dump capture progress overlay.
- Add core dump report view.

### E) Reset confirmation (§2.5)

Target behavior:
- Add reset confirmation overlay with wireframe layout and safe default (cancel).

### F) Host scrollback UI polish (§1.3 “Scrollback Mode”)

Target behavior:
- Add scroll indicators (right-side bar and/or “▲ N more ▼” line).
- Align HOST footer/help text more closely to the wireframe, while preserving current safe key routing.

## Verification

- Deterministic screenshots:
  - `USE_VIRTUAL_PTY=1 ./ttmp/.../scripts/09-tmux-capture-esper-tui.sh`
- For firmware-driven screens (panic/coredump/gdb/reset), also capture with a real device when possible.

