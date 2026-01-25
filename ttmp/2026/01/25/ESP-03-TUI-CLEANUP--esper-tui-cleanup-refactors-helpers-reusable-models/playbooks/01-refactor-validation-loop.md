---
Title: "ESP-03-TUI-CLEANUP: refactor validation loop"
Ticket: ESP-03-TUI-CLEANUP
Status: active
Topics:
  - tui
  - bubbletea
  - lipgloss
  - refactor
  - testing
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/01-ux-iteration-loop.md
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh
Summary: "How we validate UI-preserving refactors: tests + real-device (or virtual PTY) tmux captures."
LastUpdated: 2026-01-25T14:31:18-05:00
---

# Refactor validation loop (playbook)

This ticket is about refactoring without changing UX intent. The canonical iteration loop is the ESP-02 playbook:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/playbooks/01-ux-iteration-loop.md`

## Invariants (must not regress during refactors)

- Explicit routing of `tea.KeyMsg` and `tea.WindowSizeMsg`.
- HOST-only shortcuts never leak into DEVICE input.
- Use Lip Gloss for styling/layout (no manual box drawing).

## Validation steps (every refactor slice)

1) Tests (nested repo):
- `cd esper && go test ./... -count=1`

2) Screenshot regression captures:
- Real device:
  - `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
- Virtual PTY fallback (no hardware):
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`

3) Commit order:
- `esper/` repo: commit the refactor slice.
- Outer repo: commit captures + diary updates.
