---
Title: 'ESP-08-TUI-COMMAND-PALETTE: command palette spec'
Ticket: ESP-08-TUI-COMMAND-PALETTE
Status: active
Topics:
    - tui
    - bubbletea
    - lipgloss
    - ux
    - palette
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/monitor_view.go
      Note: |-
        Palette exec wiring to monitor actions
        Palette execution wiring
    - Path: esper/pkg/monitor/palette_overlay.go
      Note: |-
        Palette overlay model (list/filter/exec) to extend
        Palette list/filter/separators
    - Path: esper/pkg/monitor/styles.go
      Note: Palette styling via Lip Gloss
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md
      Note: UX spec §2.4
ExternalSources: []
Summary: 'Bring command palette to parity with UX spec: full command set, grouping separators, and wiring to monitor/app actions.'
LastUpdated: 2026-01-25T16:55:00-05:00
WhatFor: Serve as implementation guide for the command palette and its Bubble Tea integration.
WhenToUse: Use when extending palette behavior and verifying via tmux screenshots.
---


# ESP-08-TUI-COMMAND-PALETTE — command palette spec

Design reference (source of truth):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md` (§2.4)

Compare doc (verification):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

## Wireframe (verbatim)

### ASCII Wireframe

```
┌─ esper ── Connected ── 115200 ─────────────────────────────────────────────── ELF: ✓ ───┐
│ I (1523) wifi: wifi driver task: 3ffc1e4c, prio:23, stack:6656, core=0                   │
│ I (1527) system_api: Base MAC address is not set                                         │
│                                                                                          │
│         ┌─ Commands ─────────────────────────────────────────────────────────┐           │
│         │ > [                                                              ] │           │
│         │                                                                    │           │
│         │  → Search log output                                    /          │           │
│         │    Filter by level/regex                                f          │           │
│         │    Toggle Inspector                                     i          │           │
│         │    Toggle line wrap                                     w          │           │
│         │    ──────────────────────────────────────────────────────          │           │
│         │    Reset device                                         Ctrl-R     │           │
│         │    Send break                                           Ctrl-]     │           │
│         │    Disconnect                                           Ctrl-D     │           │
│         │    ──────────────────────────────────────────────────────          │           │
│         │    Clear viewport                                       Ctrl-L     │           │
│         │    Toggle session logging                               Ctrl-S     │           │
│         │    Reconnect                                                       │           │
│         │    ──────────────────────────────────────────────────────          │           │
│         │    Help                                                 ?          │           │
│         │    Quit                                                 Ctrl-C     │           │
│         └────────────────────────────────────────────────────────────────────┘           │
│                                                                                          │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

## Commands + shortcuts

Expected palette contents (grouped by separators):
- Search log output (`/`)
- Filter by level/regex (`f`)
- Toggle Inspector (`i`)
- Toggle line wrap (`w`) (if supported; otherwise hide)
---
- Reset device (`Ctrl-R`) (confirmation)
- Send break (`Ctrl-]`) (see note below)
- Disconnect (`Ctrl-D`)
---
- Clear viewport (`Ctrl-L`) (confirmation optional)
- Toggle session logging (`Ctrl-S`)
- Reconnect (only if currently disconnected)
---
- Help (`?`)
- Quit (`Ctrl-C`)

## Open questions / known conflicts

- The spec assigns `Ctrl-]` to “Send break”.
- There is also a separate requirement from earlier work that **Ctrl-] should always exit** (especially in non-TUI tail mode).

Implementation note:
- For the TUI, keep `Ctrl-]` as the global “safe exit” if that’s the project rule, and expose “Send break” via palette-only execution (no direct `Ctrl-]` binding), or assign an alternate key (document the deviation).

## Bubble Tea model decomposition (implementation sketch)

Keep `paletteOverlayModel` but extend it with:
- non-selectable separator rows
- richer filtering behavior (search label + shortcut)

Suggested representation:

```go
type paletteRowKind int
const (
  paletteRowCommand paletteRowKind = iota
  paletteRowSeparator
)

type paletteRow struct {
  kind     paletteRowKind
  label    string
  shortcut string
  cmdKind  paletteCommandKind // only for commands
}
```

Update behavior:
- list navigation skips separators
- enter executes selected command and closes palette

Styling:
- separators rendered as a dim divider line
- shortcuts rendered dim/right-aligned (Lip Gloss width + align)

## Verification

- Deterministic capture:
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-.../scripts/09-tmux-capture-esper-tui.sh`
- Add/extend a ticket-local capture script in `scripts/` to open palette and capture `120x40`.

Acceptance criteria:
- Palette visually matches the wireframe structure (search input, grouped list, separators).
- Commands execute the expected actions without breaking message routing or overlay behavior.
