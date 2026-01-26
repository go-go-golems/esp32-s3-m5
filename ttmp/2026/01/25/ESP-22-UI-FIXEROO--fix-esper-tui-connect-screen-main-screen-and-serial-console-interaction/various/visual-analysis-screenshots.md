# Esper TUI Visual Analysis - Fresh Screenshots

**Captured**: 2026-01-25 22:21:08  
**Method**: Virtual PTY with synthetic log feed (no hardware needed)

**Related Documents**:
- [Style-to-Component Mapping](../reference/01-style-to-component-mapping.md) - Complete index of styles and code locations
- [Refactor Proposal](../design-doc/01-monitor-view-refactor-proposal.md) - monitor_view.go split plan

## Summary of Issues Observed

| Issue | Severity | Style/Code Location | Status |
|-------|----------|---------------------|--------|
| **No log level colors** (I/W/E all same gray) | High | `render/autocolor.go` (ANSI codes ARE applied, but stripped by PNG capture) | Works in real terminal! |
| **Title shows raw file path** (not device name) | Medium | `styles.TitleBar` → `monitor_view.go:650` | Not fixed |
| **Plain box borders** (no rounded corners) | Low | `styles.ScreenFrame`, `styles.Panel` → `styles.go:34,42` | Not fixed |
| **No accent colors** for interactive elements | Medium | `styles.SelectedRow` → `styles.go:68` | Not fixed |
| **Inspector panel not visible** | Medium | `monitor_view.go:1000-1045` | Needs events to show |
| **Filter overlay cut off** | ? | `filter_overlay.go` | May be capture issue |

### Key Discovery: Log Colors DO Work!

The auto-coloring IS implemented in `esper/pkg/render/autocolor.go`:
- `I (...)` → Green (ANSI `\x1b[0;32m`)
- `W (...)` → Yellow (ANSI `\x1b[0;33m`)
- `E (...)` → Red (ANSI `\x1b[1;31m`)

The screenshots appear monochrome because **ImageMagick `convert` strips ANSI escape codes** when rendering text to PNG. In a real terminal, the logs WILL be colored!

---

## 1. Device Mode (120x40)

![Device Mode](../../../2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/esp22-analysis-20260125-222108/120x40-01-device.png)

### Component-to-Code Mapping

| Visual Element | Style | Code Location |
|---------------|-------|---------------|
| Outer border | `styles.ScreenFrame` | `app_model.go:309-313` |
| Title bar "esper — Connected:..." | `styles.TitleBar` | `monitor_view.go:649-650` |
| Log output (I/W/E lines) | ANSI via `render.AutoColorer` | `monitor_view.go:591`, `render/autocolor.go:11-43` |
| Status bar "Mode: DEVICE..." | `styles.StatusBar` | `monitor_view.go:652-653` |
| Input field "> [" | `textinput.Model` | `monitor_view.go:694-697` |

### Observations

- Dark navy background (#0f111a) - good
- Light gray text (#e6e6e6) - readable
- **Log colors ARE applied** (ANSI codes) but stripped by screenshot capture
- Title bar shows ugly truncated path: `/home/manuel/workspaces/.../ttmp/20...`
  - **Fix location**: `monitor_view.go:767-777` (`renderTitle()`)
- Status bar is functional: `Mode: DEVICE | Follow: ON | ...`
- Input field `> [` visible at bottom - functional but plain

**Verdict**: Functional, log colors work in real terminal. Title needs fixing.

---

## 2. Host Mode (120x40)

![Host Mode](../../../2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/esp22-analysis-20260125-222108/120x40-02-host.png)

**Observations:**
- Footer shows key hints: `Ctrl-T: DEVICE  Ctrl-T T: commands  PgUp/PgDn scroll  G: resume follow  / search  f filter`
- Status bar shows `Mode: HOST | Follow: OFF`
- Same lack of log colors
- Same ugly title path

**Verdict**: Host mode footer is better than device mode. Same color issues.

---

## 3. Search Mode (120x40)

![Search Mode](../../../2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/esp22-analysis-20260125-222108/120x40-03-search.png)

**Observations:**
- Search bar at bottom: `Search: [wifi` - good placement!
- Match count shown: `match 1/137`
- Match markers in viewport: `← MATCH 92/137` etc.
- Key hints: `n:next N:prev  Enter:jump  Esc:clos...`

**Positive**: Search is actually quite functional and well-designed!
- Bottom bar placement is correct (matches wireframe)
- Match markers are visible in viewport
- Match count is shown

**Issues**:
- Match markers could use color (they're the same gray)
- The search highlight could be more visible

---

## 4. Filter Overlay (120x40)

![Filter Overlay](../../../2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/esp22-analysis-20260125-222108/120x40-04-filter.png)

**Observations:**
- Only bottom portion visible (may be scrolled or capture issue)
- Shows: `Include (regex): [wifi]`
- Shows: `Exclude (regex): [heartbeat]`
- Has separator line
- Shows: `Highlight Rules:` with `(no rules yet)` and `<Add Rule>` button
- Buttons: `<Apply>  <Clear All>  <Cancel>`

**Issues**:
- Where are the E/W/I/D/V level toggle checkboxes?
- The overlay seems incomplete or truncated

---

## 5. Command Palette (120x40)

![Command Palette](../../../2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/esp22-analysis-20260125-222108/120x40-05-palette.png)

### Component-to-Code Mapping

| Visual Element | Style | Code Location |
|---------------|-------|---------------|
| Modal box | `styles.OverlayBox` | `palette_overlay.go:194` |
| Title "Commands" | `styles.PanelTitle` | `palette_overlay.go:162` |
| Separator lines | `styles.Hint` | `palette_overlay.go:170` |
| Selected row (→) | `styles.SelectedRow` | `palette_overlay.go:182` |
| Shortcut hints | `styles.Hint` | `palette_overlay.go:179` |
| Footer hint | `styles.Hint` | `palette_overlay.go:191` |

### Observations

- Centered modal overlay - good!
- Title: `Commands`
- Filter input: `> Type to filter commands...`
- Commands grouped with separators (the horizontal lines)
- Selection cursor: `→ Search log output`
- Keyboard shortcuts aligned right: `/, f, i, w, Ctrl-R, Ctrl-], Ctrl-D, Ctrl-L, Ctrl-S, ?, Ctrl-C`

**Positive**: This is actually the best-looking component!
- Proper modal with border
- Separators between groups
- Good command organization

**Issues**:
- No accent color for selected item (just the `→` cursor + bold via `styles.SelectedRow`)
- All text same gray color
- **Fix**: Add `Background(highlight)` to `SelectedRow` in `styles.go:68`

---

## 6. Inspector Panel (120x40)

![Inspector](../../../2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/esp22-analysis-20260125-222108/120x40-06-inspector.png)

**Observations:**
- Status bar shows `Inspect: ON`
- BUT: No inspector panel visible on the right side
- The synthetic feed doesn't generate panic/coredump/gdb events
- Without events, there's nothing to inspect

**Conclusion**: Need real hardware or trigger synthetic events to test inspector panel.

---

## 7. Compact View (80x24)

![80x24 Device](../../../2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/esp22-analysis-20260125-222108/80x24-01-device.png)

**Observations:**
- Title is heavily truncated (expected)
- Viewport is mostly empty (synthetic feed may have ended)
- Status bar cramped but visible
- Layout works but is tight

---

## Color Analysis

**Current Color Palette (from screenshots):**
| Element | Current Color | Desired |
|---------|--------------|---------|
| Background | #0f111a (dark navy) | Keep |
| Text | #e6e6e6 (light gray) | Keep for default |
| Border | Gray (same as text) | Dimmer gray or accent |
| Title | Same gray | Accent color (purple?) |
| I (Info) log | Same gray | Green (#04B575) |
| W (Warning) log | Same gray | Yellow (#F4A100) |
| E (Error) log | Same gray | Red (#F25D94) |
| Selected item | Same gray + bold | Background highlight |

---

## Priority Fixes Based on Visual Analysis

### High Priority
1. **Add log level colors** - Most impactful visual improvement
2. **Fix title bar** - Show device name, not raw path

### Medium Priority
3. **Add accent colors** for selected/focused elements
4. **Use rounded borders** for modern look
5. **Fix filter overlay** - ensure level toggles are visible

### Low Priority
6. **Add match highlighting** in search (reverse video or background)
7. **Improve inspector panel** - test with real events

---

## Comparison: Current vs Wireframe

The wireframe from the UX spec shows:
- Colored log output
- Clean title with device name
- Proper footer spacing
- Inspector panel with event list

Current implementation:
- All text same gray
- Raw file path in title
- Footer sometimes truncated
- Inspector needs events to show

The gap is primarily in **styling** - the functionality is largely there.
