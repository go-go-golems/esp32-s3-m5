---
Title: "ESP-02-ESPER-TUI: UX iteration loop playbook"
Ticket: ESP-02-ESPER-TUI
Status: active
Topics:
  - tui
  - ux
  - bubbletea
  - lipgloss
  - testing
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/01-tui-wireframe-diffs.md
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md
Summary: "How we iterate on the esper TUI to match the contractor wireframes, with deterministic screenshot captures for fast review."
LastUpdated: 2026-01-25T14:31:18-05:00
---

# UX iteration loop (playbook)

Goal: converge implementation visuals/behavior to the contractor wireframes quickly and safely (no device input contamination), with a repeatable “capture → compare → adjust” loop.

## Ground rules (non-negotiable)

- Route `tea.KeyMsg` and `tea.WindowSizeMsg` explicitly (screen + overlay routing).
- Keep HOST-only keybindings from interfering with DEVICE mode (avoid sending UI keystrokes to device).
- Use Lip Gloss for layout + styling (borders/padding/colors), not hand-drawn box chars.
- Every significant UX change gets:
  - a screenshot capture run (80×24 and 120×40)
  - a short diff note update
  - a commit (in the correct repo)

## Repos + where to commit

- Code changes: commit in `esper/` (nested Git repo).
- Docs/scripts/screenshots: commit in outer repo (`esp32-s3-m5`).
- Never stage `esper/` from the outer repo.

## Loop steps

### 1) Pick the next UX target (small slice)

Example slices:
- Search overlay bottom bar + match count + highlight.
- Filter overlay: add Debug/Verbose and highlight rules list.
- Command palette: add missing commands + group separators.

Write down:
- Which wireframe section we’re matching (e.g. “Search Overlay §2.2”).
- Acceptance criteria (what will look/behave identically).

### 2) Implement the slice (Lip Gloss + Bubble Tea)

While coding:
- Keep new UI state in explicit structs (`overlay model`, `filter config`, etc.).
- Ensure all new keys are routed in HOST-only unless explicitly safe for DEVICE.
- Keep styling in `styles` (Lip Gloss styles), not ad-hoc string drawing.

### 3) Validate build quickly

From `esper/`:
- `gofmt -w ./...`
- `go test ./...`

### 4) Generate deterministic screenshots (primary review artifact)

From repo root:
- Deterministic (no hardware):  
  `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
- With real device:  
  `./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`

Recommended pre-flight (real device):
- Confirm the device node exists:
  - `ls -la /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*`
- If port is busy or prior runs were interrupted, kill stale tmux sessions and orphaned `go run` processes:
  - `tmux ls | rg '^esper_tui_cap_' || true`
  - `for s in $(tmux ls -F '#S' 2>/dev/null | rg '^esper_tui_cap_' || true); do tmux kill-session -t \"$s\"; done`
  - `pgrep -fal \"go run ./cmd/esper\" || true`
  - `pkill -f \"go run ./cmd/esper\" || true`

This produces:
- `.txt` pane captures (authoritative for exact chars)
- `.png` renders (easy visual review / reMarkable-friendly)

### 5) Update the diff notes (fast feedback surface)

Edit:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/01-tui-wireframe-diffs.md`

Record:
- “What matches now”
- “What still differs”
- “What I think the next change should be”

### 6) Commit at the right granularity

- Code in `esper/`: one commit per UX slice.
- Screenshots/docs: one commit per capture run + docs update.

### 7) Ask for review using the compare doc

Point reviewers at:
- the screenshot compare doc (current vs desired wireframe)
- the screenshot directory for raw captures
- the diff notes for context

## Quick troubleshooting

- If screenshots are missing borders/title: ensure tmux capture uses `-pN` and that the UI sizing is clamped to the terminal.
- If port is “busy”: kill orphaned `esper` processes, and ensure the tmux harness exits the program (`Ctrl-C`) before killing the session.
