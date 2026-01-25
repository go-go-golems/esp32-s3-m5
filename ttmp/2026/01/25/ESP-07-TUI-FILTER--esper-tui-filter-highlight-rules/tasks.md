---
Title: "ESP-07-TUI-FILTER: tasks"
Ticket: ESP-07-TUI-FILTER
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - ux
  - filter
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
Summary: "Implement filter overlay parity: D/V levels, include/exclude regex, highlight rules editor, and applying highlight styles to viewport rendering."
LastUpdated: 2026-01-25T16:55:00-05:00
---

# Tasks

## TODO (check off in order)

Spec + scaffolding:
- [ ] Create/update design doc (`design/01-filter-highlight-rules-spec.md`)
- [ ] Add/verify tmux capture script(s) in `scripts/` for filter overlay

Filter parity (§2.3):
- [ ] Extend level toggles to include Debug/Verbose (D/V) and apply to filtering behavior
- [ ] Keep include/exclude regex fields; validate regex and show errors inline

Highlight rules editor:
- [ ] Implement highlight rule list editing (pattern + style dropdown)
- [ ] Support add/remove/reorder rules (minimal: add + remove)
- [ ] Add “Apply / Clear All / Cancel” behavior

Apply highlight rules to viewport:
- [ ] Define a stable style mapping (e.g., `red background`, `yellow underline`, etc.) using Lip Gloss directives
- [ ] Apply highlight rules to rendered lines without breaking ANSI output (best-effort)

Robustness + tests:
- [ ] Unit tests for regex validation and “apply/clear/cancel” semantics
- [ ] Ensure `tea.KeyMsg` + `tea.WindowSizeMsg` routing is explicit and stable

Artifacts + review:
- [ ] Capture 120×40 screenshots and update compare doc (`ESP-02` current-vs-desired)
- [ ] Update changelog and close ticket when complete
