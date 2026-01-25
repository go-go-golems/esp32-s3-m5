---
Title: "ESP-09-TUI-INSPECTOR-DETAILS: tasks"
Ticket: ESP-09-TUI-INSPECTOR-DETAILS
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - ux
  - inspector
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
Summary: "Implement inspector detail screen actions and context sections per UX spec: copy/save/jump/tab-next-event and richer formatting."
LastUpdated: 2026-01-25T16:55:00-05:00
---

# Tasks

## TODO (check off in order)

Spec + scaffolding:
- [ ] Create/update design doc (`design/01-inspector-detail-actions-spec.md`)
- [ ] Add/verify tmux capture script(s) in `scripts/` for detail screens

Panic detail actions:
- [ ] Implement `c` (copy raw), `C` (copy decoded), `j` (jump to log) and footer hints
- [ ] Implement `Tab` next event and update view accordingly
- [ ] Add “Context” section (parse registers/error/core from panic text if available)

Core dump report actions:
- [ ] Implement `c` (copy report), `s` (save full report), `r` (copy raw base64), `j` (jump to log)
- [ ] Ensure saved files are named deterministically (timestamp) and paths are shown in the UI

Robustness + tests:
- [ ] Unit tests for save path creation and copy action error handling
- [ ] Ensure `tea.KeyMsg` + `tea.WindowSizeMsg` routing is explicit and stable

Artifacts + review:
- [ ] Capture 120×40 screenshots and update compare doc (`ESP-02` current-vs-desired)
- [ ] Update changelog and close ticket when complete
