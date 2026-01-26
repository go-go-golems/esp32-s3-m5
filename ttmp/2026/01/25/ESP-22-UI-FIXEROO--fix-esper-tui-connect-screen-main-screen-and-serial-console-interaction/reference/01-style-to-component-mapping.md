---
Title: Style-to-Component Mapping
Ticket: ESP-22-UI-FIXEROO
Status: active
Topics:
    - esper
    - tui
    - lipgloss
    - styles
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/styles.go
      Note: All style definitions live here
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Monitor screen rendering
    - Path: esper/pkg/monitor/port_picker.go
      Note: Port picker screen rendering
    - Path: esper/pkg/monitor/palette_overlay.go
      Note: Command palette overlay
    - Path: esper/pkg/monitor/filter_overlay.go
      Note: Filter overlay
    - Path: esper/pkg/render/autocolor.go
      Note: ESP-IDF log level coloring (ANSI)
ExternalSources: []
Summary: Complete mapping of visual UI components to their lipgloss styles and source code locations.
LastUpdated: 2026-01-25T22:40:00-05:00
WhatFor: Index for understanding how each UI element is styled and where to make changes.
WhenToUse: Reference when implementing visual fixes or understanding the styling architecture.
---


# Style-to-Component Mapping

This document maps each visual UI component to its:
1. **Style definition** in `styles.go`
2. **Rendering code location** (file + line numbers)
3. **Current appearance** (from screenshots)
4. **Recommended improvements**

---

## Style Definitions (esper/pkg/monitor/styles.go)

### Complete styles struct

```go
type styles struct {
    ScreenFrame lipgloss.Style  // Outer border around entire TUI
    TitleBar    lipgloss.Style  // Title bar at top of screens
    StatusBar   lipgloss.Style  // Status bar (mode indicators)
    Panel       lipgloss.Style  // Port picker panel, inspector panel
    PanelTitle  lipgloss.Style  // Bold titles within panels
    InputBox    lipgloss.Style  // Input field boxes (unused?)
    ErrorBanner lipgloss.Style  // Error display with thick border
    InlineError lipgloss.Style  // Inline error text (red)
    Hint        lipgloss.Style  // Faint helper text
    OverlayDim  lipgloss.Style  // Background dimming for overlays
    OverlayBox  lipgloss.Style  // Overlay modal boxes
    OverlayText lipgloss.Style  // Text inside overlays
    SelectedRow lipgloss.Style  // Selected item in lists
    Row         lipgloss.Style  // Normal row in lists
}
```

### Current style values

| Style | Current Definition | Visual Effect |
|-------|-------------------|---------------|
| `ScreenFrame` | `Border(NormalBorder())` | Plain box border, no color |
| `TitleBar` | `Bold(true)` | Bold text, no color |
| `StatusBar` | `Faint(true)` | Dimmed/gray text |
| `Panel` | `Border(NormalBorder()).Padding(0, 1)` | Box with horizontal padding |
| `PanelTitle` | `Bold(true)` | Bold text, no color |
| `InputBox` | `Border(NormalBorder()).Padding(0, 1)` | Box (appears unused) |
| `ErrorBanner` | `Border(ThickBorder()).Padding(0, 1)` | Thick border for errors |
| `InlineError` | `Foreground(Color("1"))` | **RED** (ANSI color 1) |
| `Hint` | `Faint(true)` | Dimmed/gray text |
| `OverlayDim` | `Faint(true)` | Dimmed background |
| `OverlayBox` | `Border(NormalBorder()).Padding(1, 2)` | Modal box with padding |
| `OverlayText` | (empty style) | No special styling |
| `SelectedRow` | `Bold(true)` | Bold text only |
| `Row` | (empty style) | No styling |

---

## Component-to-Code Mapping

### 1. Screen Frame (Outer Border)

**Visual**: The outermost box border around the entire TUI

**Style**: `ScreenFrame`

**Rendered in**: `esper/pkg/monitor/app_model.go:309-313`
```go
frame := m.styles.ScreenFrame.
    Width(innerSz.W).
    Height(innerSz.H).
    Render(inner)
```

**Issue**: Plain border, no accent color
**Fix**: Add `BorderForeground(dimColor)` or use `RoundedBorder()`

---

### 2. Title Bar

**Visual**: Top line showing "esper — Connected: /path/..."

**Style**: `TitleBar`

**Rendered in**:
- Port Picker: `port_picker.go:241` → `st.TitleBar.Render("esper")`
- Monitor: `monitor_view.go:650` → `st.TitleBar.Render(titleText)`
- Device Manager: `device_manager.go:173` → `st.TitleBar.Render("esper — Device Manager")`

**Issue**: Only bold, no color. Shows full path (ugly truncation)
**Fix**: Add accent color, show device nickname instead of path

---

### 3. Status Bar

**Visual**: Mode indicators line: "Mode: DEVICE | Follow: ON | ..."

**Style**: `StatusBar`

**Rendered in**:
- Monitor: `monitor_view.go:653` → `st.StatusBar.Render(statusText)`
- Port Picker footer: `port_picker.go:263` → `st.StatusBar.Render("↑↓ Navigate...")`
- Device Manager footer: `device_manager.go:182`

**Issue**: Only faint, no color distinction
**Fix**: Could use subtle coloring for different states

---

### 4. Panel (Port Picker / Inspector)

**Visual**: Centered modal-style panels with borders

**Style**: `Panel`

**Rendered in**:
- Port Picker: `port_picker.go:260` → `st.Panel.Width(...).Height(...).Render(...)`
- Inspector panel: `monitor_view.go:1004-1045`
- Device Manager: `device_manager.go:180`

**Issue**: Plain border
**Fix**: Could use rounded border, subtle border color

---

### 5. Panel Title

**Visual**: "Select Serial Port", "Inspector", etc.

**Style**: `PanelTitle`

**Rendered in**:
- Port Picker: `port_picker.go:271` → `st.PanelTitle.Render("Select Serial Port")`
- Inspector: `monitor_view.go:1010` → `st.PanelTitle.Render("Inspector")`
- Filter overlay: `filter_overlay.go:340`
- Command palette: `palette_overlay.go:162`
- Core dump progress: `monitor_view.go:748`

**Issue**: Only bold, no color
**Fix**: Add accent color (e.g., purple/blue)

---

### 6. Hint Text

**Visual**: Faint helper text throughout UI

**Style**: `Hint`

**Rendered in**: 28 locations across codebase
- Key examples:
  - `port_picker.go:333` → "No ports found. Press r to rescan."
  - `monitor_view.go:678` → "(input disabled during capture)"
  - `palette_overlay.go:170` → separator lines
  - `palette_overlay.go:191` → "↑/↓ select  Enter run  Esc close"

**Issue**: Just faint, works well as-is
**Fix**: OK, maybe slightly more visible

---

### 7. Selected Row

**Visual**: Currently selected item in lists (port list, command palette, etc.)

**Style**: `SelectedRow`

**Rendered in**:
- Port picker port list: `port_picker.go:303`
- Command palette: `palette_overlay.go:182`
- Filter overlay: `filter_overlay.go:349`, `369`, `373`, etc.
- Device manager: `device_manager.go:244`
- Inspector event list: `monitor_view.go:1026`

**Issue**: Only bold, no background highlight
**Fix**: Add `Background(subtleHighlight)` for better visibility

---

### 8. Overlay Box

**Visual**: Modal overlays (command palette, filter, help, etc.)

**Style**: `OverlayBox`

**Rendered in**:
- Command palette: `palette_overlay.go:194`
- Filter overlay: `filter_overlay.go:454`
- Help overlay: `help_overlay.go:34`
- Confirm overlays: `confirm_overlay.go:87`, `reset_confirm_overlay.go:93`
- Device edit: `device_edit_overlay.go:165`
- Inspector detail: `inspector_detail_overlay.go:133`
- Core dump progress: `monitor_view.go:764`

**Issue**: Plain border
**Fix**: Rounded border, subtle border color

---

### 9. Error Styling

**Visual**: Error messages (red text, thick border banners)

**Styles**: `ErrorBanner`, `InlineError`

**Rendered in**:
- Port picker error: `port_picker.go:249` (ErrorBanner)
- Filter overlay errors: `filter_overlay.go:391`, `420-423` (InlineError)

**Status**: `InlineError` has red color (good!). ErrorBanner could use red border.

---

## Log Level Colors (NOT in styles.go)

**Visual**: Should be Green/Yellow/Red for I/W/E log prefixes

**Defined in**: `esper/pkg/render/ansi.go`
```go
ANSIReset  = []byte("\x1b[0m")
ANSIRed    = []byte("\x1b[1;31m")    // E (Error)
ANSIGreen  = []byte("\x1b[0;32m")    // I (Info)
ANSIYellow = []byte("\x1b[0;33m")    // W (Warning)
```

**Applied in**: `esper/pkg/render/autocolor.go:50-70`

**Called from**: `monitor_view.go:591` → `m.autoColor.ColorizeLine(line)`

**IMPORTANT DISCOVERY**: The colors ARE being applied as ANSI escape codes! The screenshots appear monochrome because ImageMagick's `convert` strips ANSI codes when rendering text to PNG.

**In a real terminal**: Log lines SHOULD display with colors.

**Verification needed**: Run esper in an actual terminal (not screenshot capture) to confirm colors work.

---

## Search Match Markers

**Visual**: "← MATCH 92/137" markers on matched lines

**Style**: Custom inline styles in `monitor_view.go:1333-1334`
```go
markerStyle := lipgloss.NewStyle().Faint(true)
curMarkerStyle := lipgloss.NewStyle().Bold(true)
highlightStyle := lipgloss.NewStyle().Reverse(true)
```

**Rendered in**: `monitor_view.go:1320-1382` (`decorateSearchLines`)

**Issue**: Markers are faint/bold text, could use color
**Fix**: Add accent color for current match marker

---

## Files Summary

| File | Primary Styles Used | LOC |
|------|-------------------|-----|
| `styles.go` | Definition only | 72 |
| `monitor_view.go` | TitleBar, StatusBar, Hint, Panel, PanelTitle, SelectedRow, Row, OverlayBox | ~1500 |
| `port_picker.go` | TitleBar, Panel, PanelTitle, StatusBar, Hint, ErrorBanner, SelectedRow, Row | ~455 |
| `palette_overlay.go` | PanelTitle, Hint, SelectedRow, OverlayBox | ~200 |
| `filter_overlay.go` | PanelTitle, SelectedRow, Hint, InlineError, OverlayBox | ~460 |
| `device_manager.go` | TitleBar, Panel, PanelTitle, StatusBar, Hint, SelectedRow, Row | ~300 |
| `inspector_detail_overlay.go` | Hint, OverlayBox, PanelTitle, Panel | ~180 |
| `help_overlay.go` | OverlayBox | ~66 |
| `confirm_overlay.go` | OverlayBox, PanelTitle, Hint, Row, SelectedRow | ~90 |
| `device_edit_overlay.go` | OverlayBox, PanelTitle, Hint, SelectedRow, Row | ~170 |

---

## Proposed Enhanced styles.go

```go
func defaultStyles() styles {
    // Color palette
    primary    := lipgloss.Color("#7D56F4")  // Purple accent
    subtle     := lipgloss.Color("#383838")  // Dark gray
    dimBorder  := lipgloss.Color("#444444")  // Border gray
    highlight  := lipgloss.Color("#2d2d3d")  // Selection background
    
    return styles{
        ScreenFrame: lipgloss.NewStyle().
            Border(lipgloss.RoundedBorder()).
            BorderForeground(dimBorder),
            
        TitleBar: lipgloss.NewStyle().
            Bold(true).
            Foreground(primary),
            
        StatusBar: lipgloss.NewStyle().
            Foreground(dimBorder),
            
        Panel: lipgloss.NewStyle().
            Border(lipgloss.RoundedBorder()).
            BorderForeground(dimBorder).
            Padding(0, 1),
            
        PanelTitle: lipgloss.NewStyle().
            Bold(true).
            Foreground(primary),
            
        // ... etc
            
        SelectedRow: lipgloss.NewStyle().
            Bold(true).
            Background(highlight),
            
        // ... etc
    }
}
```

---

## Next Steps

1. **Verify log colors work in real terminal** (not screenshot capture)
2. **Update styles.go** with color palette
3. **Fix port_picker.go layout bugs** (see main analysis)
4. **Consider adding log-level-specific styles** to styles.go for UI-rendered logs (not just ANSI passthrough)
