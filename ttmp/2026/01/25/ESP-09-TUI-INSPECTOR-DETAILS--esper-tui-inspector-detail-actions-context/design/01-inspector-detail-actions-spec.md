---
Title: 'ESP-09-TUI-INSPECTOR-DETAILS: inspector detail actions spec'
Ticket: ESP-09-TUI-INSPECTOR-DETAILS
Status: active
Topics:
    - tui
    - bubbletea
    - lipgloss
    - ux
    - inspector
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/inspector_detail_overlay.go
      Note: |-
        Detail overlays for PANIC/COREDUMP; add actions + context sections
        Implement copy/save/jump/tab actions
    - Path: esper/pkg/monitor/inspector_model.go
      Note: |-
        Event list selection + navigation (Tab next event)
        Event selection + Tab next
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Jump-to-log behavior in HOST scrollback
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md
      Note: UX spec §1.3
ExternalSources: []
Summary: Implement inspector detail screen actions and richer context formatting per UX spec (copy/save/jump/tab-next-event).
LastUpdated: 2026-01-25T16:55:00-05:00
WhatFor: Serve as implementation guide for inspector detail screens.
WhenToUse: Use when implementing/refining PANIC/COREDUMP detail screens and verifying via tmux screenshots.
---


# ESP-09-TUI-INSPECTOR-DETAILS — inspector detail actions spec

Design reference (source of truth):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md` (§1.3 detail views)

Compare doc (verification):
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md`

## Wireframes (verbatim)

### ASCII Wireframe: Panic Detail View

```
┌─ Inspector ─ PANIC ─ 12:34:52 ───────────────────────────────────────────────────────────┐
│                                                                                          │
│  ┌─ Raw Backtrace ──────────────────────────────────────────────────────────────────┐   │
│  │ Backtrace: 0x40082a39:0x3ffb5c70 0x400d1e7c:0x3ffb5c90 0x400d2145:0x3ffb5cb0     │   │
│  │            0x400d2567:0x3ffb5cd0 0x40084d21:0x3ffb5cf0                            │   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
│  ┌─ Decoded Frames ─────────────────────────────────────────────────────────────────┐   │
│  │ #0  0x40082a39 in app_main at /project/main/app_main.c:42                        │   │
│  │ #1  0x400d1e7c in wifi_init at /project/components/wifi/wifi.c:156               │   │
│  │ #2  0x400d2145 in esp_wifi_start at /esp-idf/components/esp_wifi/src/wifi.c:890  │   │
│  │ #3  0x400d2567 in main_task at /esp-idf/components/freertos/app_startup.c:208    │   │
│  │ #4  0x40084d21 in vPortTaskWrapper at /esp-idf/components/freertos/port.c:131    │   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
│  ┌─ Context ────────────────────────────────────────────────────────────────────────┐   │
│  │ Error: LoadProhibited                                                            │   │
│  │ Core: 0                                                                          │   │
│  │ PC: 0x40082a3c   A0: 0x800d1e7f   A1: 0x3ffb5c70                                │   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ c Copy raw   C Copy decoded   j Jump to log   Tab Next event   Esc Close                 │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

### ASCII Wireframe: Core Dump Report

```
┌─ Inspector ─ COREDUMP ─ 12:30:15 ────────────────────────────────────────────────────────┐
│                                                                                          │
│  Status: ✓ Captured and decoded successfully                                            │
│  Size: 65,536 bytes                                                                      │
│  Saved to: /tmp/esper-coredump-20260125-123015.bin                                       │
│                                                                                          │
│  ┌─ Decoded Report (truncated) ─────────────────────────────────────────────────────┐   │
│  │ espcoredump.py v1.2                                                              │   │
│  │ Crash: LoadProhibited                                                            │   │
│  │ Task: main (0x3ffb5c00)                                                          │   │
│  │                                                                                  │   │
│  │ Backtrace:                                                                       │   │
│  │   #0 app_main (/project/main/app_main.c:42)                                      │   │
│  │   #1 wifi_init (/project/components/wifi/wifi.c:156)                             │   │
│  │   ...                                                                            │   │
│  │                                                                                  │   │
│  │ Thread 1 (main):                                                                 │   │
│  │   Stack: 0x3ffb5c00 - 0x3ffb6000 (4096 bytes)                                    │   │
│  │   Registers: PC=0x40082a3c SP=0x3ffb5c70 ...                                     │   │
│  └──────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ c Copy report   s Save full report   r Copy raw base64   j Jump to log   Esc Close       │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

## Behavior requirements

Common:
- `Esc` closes detail view.
- PageUp/PageDown (or existing scroll keys) scrolls the detail viewport.
- Actions should show a toast on success/failure (e.g., “copied”, “saved to …”).

Panic detail:
- `c`: copy raw backtrace section
- `C`: copy decoded frames section (if present)
- `j`: jump to log (best-effort: focus HOST scrollback and scroll to an anchor line)
- `Tab`: advance to next inspector event and re-render the detail view
- Context section: extract error/core/registers if available, otherwise render “(no context parsed)”.

Core dump report:
- `c`: copy decoded report snippet shown
- `s`: save full report text to file (include raw report if longer)
- `r`: copy raw core dump base64 (or raw captured content) if available
- `j`: jump to log (best-effort)

## Bubble Tea model decomposition (implementation sketch)

Current approach uses detail overlays opened from the inspector event list. Extend them with:
- action key handling (copy/save/jump/tab)
- a small “document model” representing sections (raw/decoded/context/report) so copy/save is deterministic

Suggested section representation:

```go
type detailSection struct {
  title string
  body  string
  copyKey string // "c", "C", etc.
}
```

Jump-to-log (best-effort strategy):
- store an “anchor substring” when opening detail (e.g., first backtrace line, or “Core dump capture started”)
- when `j` pressed:
  - switch to HOST scrollback mode
  - search within buffer for the anchor substring
  - scroll viewport so that line is visible

## Styling

- Keep using Lip Gloss boxes for each section.
- Footer key hints must match the wireframe ordering/wording.

## Verification

- Capture screenshots:
  - `ttmp/2026/01/25/ESP-05-.../scripts/05-tmux-capture-inspector-detail-screens.sh` (firmware-triggered)
  - extend with a new ticket-local script in `scripts/` if needed

Acceptance criteria:
- Key actions work and are surfaced in the footer.
- Copy/save/jump provide user feedback and do not crash.
- Layout matches wireframes closely enough for “scan + act”.
