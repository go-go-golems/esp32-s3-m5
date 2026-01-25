---
Title: "ESP-08-TUI-COMMAND-PALETTE: tasks"
Ticket: ESP-08-TUI-COMMAND-PALETTE
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - ux
  - palette
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
Summary: "Bring command palette to parity with UX spec: grouping separators, full command set, and wiring commands to actions."
LastUpdated: 2026-01-25T16:55:00-05:00
---

# Tasks

## TODO (check off in order)

Spec + scaffolding:
- [x] Create/update design doc (`design/01-command-palette-spec.md`)
- [x] Add/verify tmux capture script(s) in `scripts/` for palette state(s)

Palette parity (§2.4):
- [x] Add grouping separators (non-selectable rows) and align list rendering to the wireframe
- [x] Expand command list to match spec (search/filter/inspector/wrap/reset/break/disconnect/clear/logging/reconnect/help/quit)
- [x] Ensure key hints and shortcuts match the wireframe (or document deliberate deviations)

Wire commands to actions:
- [x] Search → open search mode
- [x] Filter → open filter overlay
- [x] Toggle Inspector → show/hide inspector panel
- [x] Toggle wrap → toggle wrapping (if supported)
- [x] Reset device → open reset confirmation overlay and execute on confirm
- [x] Send break → implement host-side “send break” behavior (see design doc note about Ctrl-] conflicts)
- [x] Disconnect / reconnect / clear viewport / session logging toggle

Robustness + tests:
- [x] Unit tests for palette filtering and non-selectable separators
- [x] Ensure `tea.KeyMsg` + `tea.WindowSizeMsg` routing is explicit and stable

Artifacts + review:
- [x] Capture 120×40 screenshots and update compare doc (`ESP-02` current-vs-desired)
- [x] Update changelog and close ticket when complete
