---
Title: "ESP-02-ESPER-TUI: wireframe diffs vs implementation"
Ticket: ESP-02-ESPER-TUI
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - ux
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
  - esper/pkg/monitor/app_model.go
  - esper/pkg/monitor/monitor_view.go
  - esper/pkg/monitor/filter_overlay.go
  - esper/pkg/monitor/search_overlay.go
  - esper/pkg/monitor/palette_overlay.go
Summary: "Notes on what matches the contractor wireframes vs what differs, with screenshot evidence."
LastUpdated: 2026-01-25T10:02:00-05:00
---

# Wireframe diffs vs current implementation

Design reference: `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md`

Screenshot evidence (tmux capture, deterministic virtual PTY):  
`ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/`

## ✅ Implemented (MVP)

- Host-only overlays wired:
  - `/` opens Search overlay
  - `f` opens Filter overlay
  - `Ctrl-T` enters/leaves HOST/DEVICE; `Ctrl-T` then `t` opens Command Palette (HOST-only)
- Key routing: overlays capture `tea.KeyMsg` while active; `tea.WindowSizeMsg` resizes viewport + overlay models.
- Styling: borders/padding/layout done via Lip Gloss styles (no hand-built box drawing in code).
- Screenshot harness: script captures 120×40 + 80×24 states as `.txt` + `.png` via ImageMagick.

## Differences vs wireframes

### Search overlay (wireframe §2.2)

Wireframe intent:
- Bottom “search bar” (not a centered modal), with match count and next/prev hints.
- Visible match markers in the viewport (e.g. “← MATCH 2/5”) and/or highlight of matches as you type.

Current behavior:
- Centered modal box (not bottom bar).
- No match-count display in the overlay UI (match navigation exists: `Enter` jump, `n/N` next/prev).
- No in-viewport match markers or highlight.

Follow-up work:
- Render the search UI as a bottom bar occupying the footer area (and keep the viewport visible above it).
- Compute match count live while typing and show `cur/total`.
- Add match highlight/markers in the viewport rendering.

### Filter overlay (wireframe §2.3)

Wireframe intent:
- Log levels include Debug/Verbose.
- Highlight rules editor (pattern → style dropdown) and “Add Rule”.

Current behavior:
- Only E/W/I level toggles + include/exclude regex.
- No highlight rules section.

Follow-up work:
- Add D/V level flags to the filter config and level matching.
- Add highlight rules list + style selection and apply-highlight step in the render pipeline.

### Command palette (wireframe §2.4)

Wireframe intent:
- Larger command set: wrap toggle, reset device, send break (Ctrl-]), session logging, reconnect, etc.
- Visual grouping with separators.

Current behavior:
- Minimal set: Search, Filter, Toggle Inspector, Disconnect, Clear viewport, Help, Quit.
- Filtering-as-you-type works; arrow key navigation works.

Follow-up work:
- Add the remaining commands + grouping separators and hook them to monitor actions.

## Notes on the screenshot harness

- Script: `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
- For deterministic captures (no hardware contention), run with `USE_VIRTUAL_PTY=1`.
- Captured screens in the directory above correspond to:
  - `01-device` (DEVICE mode)
  - `02-host` (HOST mode)
  - `03-search` (Search overlay)
  - `04-filter` (Filter overlay)
  - `05-palette` (Command palette)
  - `06-inspector` (Inspector panel open)

