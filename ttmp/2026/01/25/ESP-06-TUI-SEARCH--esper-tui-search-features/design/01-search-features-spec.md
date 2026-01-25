---
Title: 'ESP-06-TUI-SEARCH: search features spec'
Ticket: ESP-06-TUI-SEARCH
Status: active
Topics:
    - tui
    - bubbletea
    - lipgloss
    - ux
    - search
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/monitor_view.go
      Note: |-
        HOST scrollback rendering + key routing
        Search mode rendering + scrollback jump
    - Path: esper/pkg/monitor/ansi_text.go
      Note: |-
        ANSI-safe helpers used by search (strip/query match, visible width, ANSI truncation)
    - Path: esper/pkg/monitor/search_logic.go
      Note: |-
        Pure search logic helpers (matches, wraparound navigation, jump offset)
    - Path: esper/pkg/monitor/styles.go
      Note: Styling via Lip Gloss
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md
      Note: UX spec §2.2
ExternalSources: []
Summary: 'Implement bottom-bar search mode per UX spec: live match count, match navigation, viewport markers/highlighting, and jump-to-match.'
LastUpdated: 2026-01-25T16:55:00-05:00
WhatFor: Serve as implementation guide for search UX and its Bubble Tea model integration.
WhenToUse: Use when implementing/refactoring Esper TUI search and verifying via tmux screenshots.
---


# ESP-06-TUI-SEARCH — search features spec

Design reference (source of truth):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md` (§2.2)

Compare doc (verification):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

## Scope

This ticket implements the contractor “Search Overlay” UX as a **bottom bar** (not a centered modal), including:
- live match count and match navigation
- “jump to match” behavior in scrollback
- match markers and/or match highlighting in the viewport

Out of scope:
- advanced regex search (treat as literal substring unless/until requested)

## Wireframe (verbatim)

### ASCII Wireframe: Active Search

```
┌─ esper ── Connected ── 115200 ─────────────────────────────────────────────── ELF: ✓ ───┐
│ I (1523) wifi: wifi driver task: 3ffc1e4c, prio:23, stack:6656, core=0                   │
│ I (1527) system_api: Base MAC address is not set                                         │
│ I (1533) system_api: read default base MAC address from EFUSE                            │
│ I (1540) wifi: wifi firmware version: 3.0                                                │
│ I (1545) wifi: config NVS flash: enabled                                                 │
│ W (1550) wifi: Warning: NVS partition low on space                                       │
│ I (1556) [wifi_init]: rx ba win: 6                         ← MATCH 2/5                   │
│ I (1560) [wifi_init]: tcpip mbox: 32                       ← MATCH 3/5                   │
│ I (1565) [wifi_init]: udp mbox: 6                          ← MATCH 4/5                   │
│ I (1570) [wifi_init]: tcp mbox: 6                          ← MATCH 5/5                   │
│ I (1575) wifi_init: tcp tx win: 5744                                                     │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ Search: [wifi_init                                       ] │ 5 matches │ n:next N:prev   │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

## UX requirements

Entry/exit:
- `/` opens search mode (HOST scrollback primary; DEVICE optional if it makes sense).
- `Esc` closes search mode and returns to normal footer.

While active:
- typing edits query and updates match set live
- `n` / `Ctrl-n` moves to next match (wraps at end)
- `N` / `Ctrl-p` moves to previous match (wraps at start)
- `Enter` jumps to the current match (scroll viewport so the match is visible; prefer placing match near top-third)

Status display:
- show total matches
- show current match index when applicable (e.g., “2/5”)

Viewport rendering:
- add a right-side match marker like `← MATCH i/n` on matching lines (best-effort; if too narrow, prefer showing only `← MATCH`).
- add match highlighting (best-effort) using Lip Gloss styles.

## Bubble Tea model decomposition (implementation sketch)

The search UX should be implemented as a **mode** on the monitor view, not a modal overlay (so it can be rendered in the footer area and integrated tightly with the viewport).

Proposed state (owned by `monitorModel`):

```go
type searchState struct {
  active       bool
  query        string
  // matches are in terms of "logical line index within buffer"
  matchLineIdx []int
  // currentMatch is an index into matchLineIdx (0-based)
  currentMatch int
}
```

Key routing (sketch):

```go
Update(msg tea.Msg):
  switch msg := msg.(type):
    case tea.WindowSizeMsg:
      setSize(msg)
    case tea.KeyMsg:
      if search.active:
        handleSearchKey(msg)
      else:
        if msg.String() == "/":
          openSearch()
        else:
          handleNormalKey(msg)
    default:
      // serial/tick/ports messages must continue to flow
      handleNonKeyMsg(msg)
```

Rendering (sketch):
- viewport height reduces by 2 rows when `search.active`
- footer renders:
  - left: `Search: [<query>]`
  - middle/right: `<n> matches` and `n:next N:prev`
- when rendering each visible line:
  - compute if that line index is in `matchLineIdx`
  - append a right-aligned marker (`← MATCH i/n`) while keeping total line width ≤ viewport width
  - apply Lip Gloss highlight style if feasible

## Styling

- Use existing `styles` primitives and add semantic helpers if needed:
  - `SearchBar`, `SearchBarPrompt`, `SearchBarField`, `SearchBarHint`, `SearchMatchMarker`
- Do not hand-draw boxes; use Lip Gloss width/align (`lipgloss.Width`, `lipgloss.PlaceHorizontal`, padding) and existing border styles.

## Verification

- Deterministic capture:
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-.../scripts/09-tmux-capture-esper-tui.sh`
- Add/extend a ticket-local capture script in `scripts/` to enter search mode and capture `120x40` screenshots.

Acceptance criteria:
- Search is bottom-bar, not centered modal.
- Match count and `n/N` navigation works deterministically.
- Viewport shows match markers and does not break ANSI log coloring.
