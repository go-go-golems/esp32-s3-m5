---
Title: 'ESP-07-TUI-FILTER: filter + highlight rules spec'
Ticket: ESP-07-TUI-FILTER
Status: active
Topics:
    - tui
    - bubbletea
    - lipgloss
    - ux
    - filter
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/filter_overlay.go
      Note: |-
        Filter overlay UI (currently minimal) to extend
        Filter overlay UI to extend
    - Path: esper/pkg/monitor/monitor_view.go
      Note: |-
        Filter application + viewport rendering pipeline
        Filtering/highlighting render pipeline
    - Path: esper/pkg/monitor/styles.go
      Note: Highlight styles via Lip Gloss
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md
      Note: UX spec §2.3
ExternalSources: []
Summary: Implement filter overlay parity (D/V levels, regex include/exclude) and highlight rules editor, and apply highlight styles to viewport rendering.
LastUpdated: 2026-01-25T16:55:00-05:00
WhatFor: Serve as implementation guide for filtering and highlighting behavior and its Bubble Tea integration.
WhenToUse: Use when implementing/refactoring filter UX and verifying via tmux screenshots.
---


# ESP-07-TUI-FILTER — filter + highlight rules spec

Design reference (source of truth):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md` (§2.3)

Compare doc (verification):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

## Wireframe (verbatim)

### ASCII Wireframe

```
┌─ Filter Log Output ──────────────────────────────────────────────────────────────────────┐
│                                                                                          │
│  Log Levels:                                                                             │
│    [✓] E  Error        [✓] W  Warning      [✓] I  Info                                  │
│    [ ] D  Debug        [ ] V  Verbose                                                    │
│                                                                                          │
│  ────────────────────────────────────────────────────────────────────────────────────    │
│                                                                                          │
│  Include (regex): [wifi|http                                                         ]   │
│  Exclude (regex): [heartbeat                                                         ]   │
│                                                                                          │
│  ────────────────────────────────────────────────────────────────────────────────────    │
│                                                                                          │
│  Highlight Rules:                                                                        │
│    1. [error|fail|crash             ] → [red background      ▼]                         │
│    2. [warn|timeout                 ] → [yellow underline    ▼]                         │
│    3. [                             ] → [                    ▼]   <Add Rule>            │
│                                                                                          │
│  ────────────────────────────────────────────────────────────────────────────────────    │
│                                                                                          │
│                                           <Apply>    <Clear All>    <Cancel>             │
│                                                                                          │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

## Behavior requirements

Levels:
- Support E/W/I/D/V checkboxes.
- Filtering should be applied consistently to both incoming lines and scrollback view (policy choice: filter at render time so buffer remains intact).

Regex:
- Include and exclude are regex patterns (optional).
- Invalid regex must not crash; show an inline error and treat as “no filter applied” until fixed.

Highlight rules:
- User defines a list of `(pattern regex) → (style)` rules.
- Rules apply to visible lines even if the line is not filtered out.
- Styles are applied using Lip Gloss directives (do not hand-inject ANSI).

## Bubble Tea model decomposition (implementation sketch)

Overlay approach:
- Keep filter as a **modal overlay**, but implement it as a multi-section focused form (levels, regex inputs, highlight rule list, buttons).

Proposed state:

```go
type highlightRule struct {
  pattern string
  style   highlightStyle // enum
  // compiled cached regex (nil if invalid)
  rx *regexp.Regexp
}

type filterConfig struct {
  levels   map[level]bool
  include  *regexp.Regexp
  exclude  *regexp.Regexp
  rules    []highlightRule
}
```

Applying in render pipeline (sketch):

```go
renderLine(line string) string {
  // 1) decide visible (level/include/exclude) using stripped text
  // 2) if visible, apply highlight rules (best-effort)
  // 3) return rendered string, preserving ANSI where possible
}
```

## Styling

- Add semantic styles for highlight rule styles (map to Lip Gloss styles):
  - `red background`, `yellow underline`, `green`, `cyan`, `bold`
- Keep visual separators using Lip Gloss borders/dividers.

## Verification

- Deterministic capture:
  - `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-.../scripts/09-tmux-capture-esper-tui.sh`
- Add/extend a ticket-local capture script in `scripts/` to open filter overlay and capture `120x40`.

Acceptance criteria:
- Filter overlay matches wireframe structure (levels + regex + highlight rules + buttons).
- D/V toggles work.
- Highlight rules visibly affect rendered output (best-effort) without corrupting ANSI log output.
