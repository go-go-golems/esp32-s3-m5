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
- [x] Create/update design doc (`design/01-filter-highlight-rules-spec.md`)
- [x] Add/verify tmux capture script(s) in `scripts/` for filter overlay

Filter parity (§2.3):
- [x] Extend level toggles to include Debug/Verbose (D/V) and apply to filtering behavior
- [x] Keep include/exclude regex fields; validate regex and show errors inline

Highlight rules editor:
- [x] Implement highlight rule list editing (pattern + style dropdown)
- [x] Support add/remove/reorder rules (minimal: add + remove)
- [x] Add “Apply / Clear All / Cancel” behavior

Apply highlight rules to viewport:
- [x] Define a stable style mapping (e.g., `red background`, `yellow underline`, etc.) using Lip Gloss directives
- [x] Apply highlight rules to rendered lines without breaking ANSI output (best-effort)

Robustness + tests:
- [x] Unit tests for regex validation and “apply/clear/cancel” semantics
- [x] Ensure `tea.KeyMsg` + `tea.WindowSizeMsg` routing is explicit and stable

Artifacts + review:
- [x] Capture 120×40 screenshots and update compare doc (`ESP-02` current-vs-desired)
- [x] Update changelog and close ticket when complete
