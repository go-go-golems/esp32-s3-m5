---
Title: "ESP-06-TUI-SEARCH: tasks"
Ticket: ESP-06-TUI-SEARCH
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - ux
  - search
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
Summary: "Implement bottom-bar search mode per UX spec, including match count, navigation, and viewport markers/highlighting."
LastUpdated: 2026-01-25T16:55:00-05:00
---

# Tasks

## TODO (check off in order)

Spec + scaffolding:
- [ ] Create/update design doc (`design/01-search-features-spec.md`)
- [ ] Add/verify tmux capture script(s) in `scripts/` for search mode

Bottom-bar search UX (§2.2):
- [ ] Replace modal search overlay with bottom-bar search mode (position bottom, height 2)
- [ ] Show live match count + current match index (`i/n`)
- [ ] Implement navigation: `n`/`N` next/prev, `Enter` jump, `Esc` close

Viewport integration:
- [ ] Add per-line match markers (`← MATCH i/n`) without breaking ANSI rendering
- [ ] Add match highlighting (best-effort) using Lip Gloss styling
- [ ] Ensure behavior works in both HOST scrollback and DEVICE mode (if applicable)

Robustness + tests:
- [ ] Unit test match navigation state machine (no matches, wrap-around, resize)
- [ ] Ensure `tea.KeyMsg` + `tea.WindowSizeMsg` routing is explicit and stable

Artifacts + review:
- [ ] Capture 120×40 screenshots and update compare doc (`ESP-02` current-vs-desired)
- [ ] Update changelog and close ticket when complete
