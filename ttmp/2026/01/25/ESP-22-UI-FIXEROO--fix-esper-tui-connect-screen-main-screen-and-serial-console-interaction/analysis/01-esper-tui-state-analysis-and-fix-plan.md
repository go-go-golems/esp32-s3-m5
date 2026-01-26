---
Title: Esper TUI State Analysis and Fix Plan
Ticket: ESP-22-UI-FIXEROO
Status: active
Topics:
    - esper
    - tui
    - bubbletea
    - lipgloss
    - serial
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/styles.go
      Note: Main styles definition - currently too minimal, needs significant expansion
    - Path: esper/pkg/monitor/port_picker.go
      Note: Port picker screen with form layout issues
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Main monitor view with rendering issues
    - Path: esper/pkg/monitor/app_model.go
      Note: Screen router and overlay handling
    - Path: esper/pkg/monitor/help_overlay.go
      Note: Help overlay - minimal styling
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/design-doc/01-esper-tui-ux-design-brief-for-contractor.md
      Note: Original UX design brief with wireframes
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/02-tui-current-vs-desired-compare.md
      Note: Detailed comparison of current vs desired state
ExternalSources: []
Summary: Comprehensive analysis of the esper TUI's current broken state, identification of key issues, and a detailed plan to fix the connect screen, main screen, and serial console interaction.
LastUpdated: 2026-01-25T20:00:00-05:00
WhatFor: Guide the implementation work to fix the esper TUI and make it a pleasant, functional serial monitor experience.
WhenToUse: Reference this document when implementing UI fixes, to understand what's broken and what the target state should be.
---


# Esper TUI State Analysis and Fix Plan

## Executive Summary

The esper TUI has significant UI/UX issues that make it difficult to use effectively. The main problems are:

1. **Visual Ugliness**: Minimal styling with no colors, plain borders, no visual hierarchy
2. **Layout Breaks**: Form fields wrapping incorrectly, creating a broken appearance
3. **Missing Footer Hints**: Port picker lacks visible keyboard navigation hints
4. **Serial Console Interaction**: Need to verify and fix keyboard input handling for REPL interaction

This document analyzes the current state, identifies specific issues, and provides a detailed plan to fix them.

## Diary / Work Log

### Session 1: Initial Analysis (2026-01-25)

**Explored the codebase:**
- Read `esper/cmd/esper/main.go` - CLI entry point that delegates to `monitor.Run()`
- Read `esper/pkg/monitor/monitor.go` - Bubble Tea program setup
- Read `esper/pkg/monitor/app_model.go` - Screen router with Port Picker, Monitor, and Device Manager screens
- Read `esper/pkg/monitor/styles.go` - **KEY ISSUE**: Very minimal styling, almost no colors
- Read `esper/pkg/monitor/port_picker.go` - Port picker with layout problems
- Read `esper/pkg/monitor/monitor_view.go` - ~1500 lines, complex but functional

**Reviewed existing documentation:**
- `ESP-02-ESPER-TUI` has detailed wireframes showing the desired state
- `ESP-11-REVIEW-ESPER` has architecture documentation
- `ESP-02.../various/02-tui-current-vs-desired-compare.md` has side-by-side comparisons

**Examined screenshots:**
- Port picker: Form fields break across lines (ELF path, Toolchain prefix), no footer hints visible
- Monitor device mode: Functional but plain, truncation is ugly
- Monitor host mode: Works but footer is cramped
- Command palette: Actually looks decent with separators

**Key findings:**
1. `styles.go` is the root cause of visual issues - it defines almost no colors
2. `port_picker.go` has hardcoded width assumptions that break on form layout
3. The monitor view truncates titles poorly

**Deep dive into port_picker bugs:**

Bug 1 - Form field wrapping (lines 342, 369):
- `elfField` uses `w-18` for width, but doesn't account for `baudField` on same line
- Math: baudField(18) + "   "(3) + elfField(w-18+7) = w+10 > w
- Fix: Either put fields on separate lines, or calculate available width correctly

Bug 2 - Footer not visible (lines 261-264):
- `sz.PlaceCentered(panel)` creates a block filling ALL `sz.H` lines
- Help line is appended AFTER, making total height `sz.H + 1`
- `appModel.View()` truncates to `innerSz.H`, cutting off the footer
- Fix: Reserve title+footer lines BEFORE centering the panel

**Reviewed the text captures in detail:**
- Line 23-24: ELF field `]` wraps to new line (confirmed bug 1)
- Line 25-26: Toolchain field `]` wraps to new line (same bug)
- Lines 32-40: All blank - no footer visible (confirmed bug 2)

## Problem Statement

The esper TUI was implemented with minimal styling to "get it working first" but was never given a proper visual polish pass. A colleague's work left the UI in a state where:

1. Form fields break awkwardly (see screenshots)
2. No colors are used, making the UI feel unfinished
3. Key hints are missing from critical screens
4. The overall experience is "ugly" rather than "pleasant"

## Current State Analysis

### 1. Port Picker Screen

**Current (broken):**
```
┌──────────────────────────────────────────────────────────────────────────────┐
│esper                                                                         │
│                                                                              │
│        ┌────────────────────────────────────────────────────────────────┐    │
│        │ Select Serial Port                                             │    │
│        │                                                                │    │
│        │ → —          USB JTAG/serial debug unit           USB-JTAG   ★ │    │
│        │                                                                │    │
│        │ Baud: [115200   ▼]   ELF: [                                   │    │
│        │ ]                                            ← BROKEN WRAP     │    │
│        │ Toolchain prefix: [                                           │    │
│        │ ]                                            ← BROKEN WRAP     │    │
│        │                                                                │    │
│        │ [ ] Probe with esptool (may reset device)                      │    │
│        │                                                                │    │
│        │               <Connect>    <Rescan>    <Quit>                  │    │
│        └────────────────────────────────────────────────────────────────┘    │
│                                                                              │
│                                      ← NO FOOTER WITH KEY HINTS!            │
└──────────────────────────────────────────────────────────────────────────────┘
```

**Issues:**
- ELF path field wraps incorrectly, with `]` on a new line
- Toolchain prefix field wraps incorrectly
- No footer with key hints (↑↓ Navigate, Tab, Enter, etc.)
- No colors - everything is white/default
- Title "esper" is plain, no visual distinction

**Desired (from wireframe):**
```
┌─ esper ─────────────────────────────────────────────────────────────── v0.1 ─┐
│                                                                              │
│  ┌─ Select Serial Port ─────────────────────────────────────────────────┐   │
│  │                                                                      │   │
│  │   → atoms3r    M5 AtomS3R              USB-JTAG   ★                 │   │
│  │     xiao-c6    Seeed XIAO ESP32-C6     USB-JTAG   ★                 │   │
│  │                                                                      │   │
│  │   ─────────────────────────────────────────────────────────────────  │   │
│  │   Baud: [115200    ▼]   ELF: [/path/to/firmware.elf           ]     │   │
│  │   Toolchain prefix: [xtensa-esp32s3-elf-                      ]     │   │
│  │                                                                      │   │
│  │   [ ] Probe with esptool (may reset device)                          │   │
│  │                                                                      │   │
│  │                  <Connect>    <Rescan>    <Devices>    <Quit>        │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│ ↑↓ Navigate   Enter Connect   n Nickname   d Devices   r Rescan   ? Help    │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 2. Monitor Device Mode

**Current:**
- Title truncates with `…` in an ugly way (shows full path, not helpful)
- Status bar works but is crowded
- Input field `> [ ... ]` truncates poorly
- No colors for log levels (I/W/E)

**Issues:**
- No colored log output (ESP-IDF logs should be colored: I=green, W=yellow, E=red)
- Title bar shows raw file path instead of device nickname
- Status bar fields are functional but visually plain

### 3. Monitor Host Mode

**Current:**
- Footer shows key hints but is cramped
- No scroll indicators (wireframe shows `▲ N more ▼`)
- Inspector panel is functional but plain

### 4. Styles (Root Cause)

Looking at `esper/pkg/monitor/styles.go`:

```go
func defaultStyles() styles {
    border := lipgloss.NormalBorder()

    return styles{
        ScreenFrame: lipgloss.NewStyle().
            Border(border).
            Padding(0, 0),

        TitleBar: lipgloss.NewStyle().Bold(true),  // NO COLOR!
        StatusBar: lipgloss.NewStyle().Faint(true),

        // ... more minimal styles with no colors
    }
}
```

**Problem**: Almost no colors are defined. The styles are purely structural.

### 5. Serial Console Interaction

Need to verify:
- Typing `help` and Enter sends to device correctly
- REPL navigation works (arrow keys for history, etc.)
- No host keys are accidentally sent to device in DEVICE mode

## Plan of Attack

### Phase 1: Fix Critical Layout Issues (High Priority)

#### 1.1 Fix Port Picker Form Layout

**Problem**: Form fields wrap incorrectly due to width calculation bugs.

**Root Cause Analysis** (line 339-376 in `port_picker.go`):

```go
// Current (broken) - line 342:
elfField := fmt.Sprintf("ELF: [%s]", padOrTrim(m.elfPath, max(10, w-18)))

// Line 369 concatenates them on the same line:
padOrTrim(baudField+"   "+elfField, w),
```

The math is wrong:
- `baudField` = "Baud: [xxxxxxxx ▼]" = ~18 characters
- separator = "   " = 3 characters
- `elfField` uses `w-18` for its value width

Total: 18 + 3 + 7 (for "ELF: []") + (w-18) = w + 10 characters > w

**Result**: The combined line exceeds the panel width, forcing `padOrTrim` to truncate, which creates the ugly wrap.

**Fix** (Option A - recommended, simpler layout):
Put ELF on its own line instead of sharing with Baud:

```go
func (m portPickerModel) renderForm(st styles, w int) []string {
    baudStr := fmt.Sprintf("%d", m.bauds[m.baudIdx])
    baudField := fmt.Sprintf("Baud: [%s ▼]", padOrTrim(baudStr, 8))
    
    // ELF on its own line with more space
    elfFieldW := max(20, w - 10)  // "ELF: []" = 7, some padding
    elfField := fmt.Sprintf("ELF: [%s]", padOrTrim(m.elfPath, elfFieldW))
    
    tcFieldW := max(20, w - 22)  // "Toolchain prefix: []" = 20, some padding
    tcField := fmt.Sprintf("Toolchain prefix: [%s]", padOrTrim(m.toolchainPrefix, tcFieldW))

    // ... styling ...

    return []string{
        padOrTrim(baudField, w),                    // Baud alone
        padOrTrim(elfField, w),                     // ELF alone
        padOrTrim(tcField, w),                      // Toolchain alone
        "",
        padOrTrim(probe, w),
        "",
        lipgloss.Place(w, 1, lipgloss.Center, lipgloss.Center, buttons),
    }
}
```

**Fix** (Option B - keep same layout, fix math):

```go
baudFieldLen := 18  // "Baud: [xxxxxxxx ▼]"
separator := "   "
elfLabelLen := 7    // "ELF: []"
elfAvail := max(10, w - baudFieldLen - len(separator) - elfLabelLen)
elfField := fmt.Sprintf("ELF: [%s]", padOrTrim(m.elfPath, elfAvail))
```

#### 1.2 Fix Footer Visibility in Port Picker

**Problem**: The footer with key hints exists in code but is not visible in the UI.

**Root Cause Analysis** (line 261-266 in `port_picker.go`):

```go
// Line 261: Centers panel in the FULL sz.H height
sections = append(sections, sz.PlaceCentered(panel))

// Line 263-264: Appends help line AFTER the full-height block
help := st.StatusBar.Render("↑↓ Navigate...")
sections = append(sections, padOrTrim(help, sz.W))
```

The bug chain:
1. `sz.PlaceCentered(panel)` calls `lipgloss.Place(sz.W, sz.H, ...)` - creates a block that fills ALL `sz.H` lines
2. Then we append the help line, making total height = `sz.H + 1` lines
3. In `appModel.View()` (line 297), content is truncated to `innerSz.H` lines
4. The help line (the last line) gets cut off!

**Fix**:

```go
func (m portPickerModel) View(st styles, sz size) string {
    // ... error banner logic ...
    
    title := st.TitleBar.Render("esper")
    titleH := 1
    
    help := st.StatusBar.Render("↑↓ Navigate   Tab Next field   Enter Connect   n Nickname   d Device Manager   r Rescan   ? Help   q Quit")
    helpH := 1
    
    // Reserve space for title and help
    availableH := sz.H - titleH - helpH
    
    panelW := min(sz.W, 78)
    panelH := min(availableH, 18)
    
    panelInner := size{W: panelW - st.Panel.GetHorizontalBorderSize(), H: panelH - st.Panel.GetVerticalBorderSize()}
    panel := st.Panel.Width(panelInner.W).Height(panelInner.H).Render(m.renderPanel(st, panelInner))
    
    // Center in available space, NOT full sz
    centeredPanel := lipgloss.Place(sz.W, availableH, lipgloss.Center, lipgloss.Center, panel)
    
    return lipgloss.JoinVertical(lipgloss.Left,
        padOrTrim(title, sz.W),
        centeredPanel,
        padOrTrim(help, sz.W),
    )
}
```

### Phase 2: Add Colors and Visual Polish (Medium Priority)

#### 2.1 Expand styles.go with colors

Add proper color definitions:

```go
func defaultStyles() styles {
    // Define a color palette
    primaryColor := lipgloss.Color("#7D56F4")   // Purple accent
    successColor := lipgloss.Color("#04B575")   // Green
    warningColor := lipgloss.Color("#F4A100")   // Yellow/Orange
    errorColor := lipgloss.Color("#F25D94")     // Red/Pink
    dimColor := lipgloss.Color("#626262")       // Gray
    
    return styles{
        ScreenFrame: lipgloss.NewStyle().
            Border(lipgloss.RoundedBorder()).
            BorderForeground(dimColor),

        TitleBar: lipgloss.NewStyle().
            Bold(true).
            Foreground(primaryColor).
            Background(lipgloss.Color("#1a1a2e")),
            
        StatusBar: lipgloss.NewStyle().
            Foreground(dimColor),
            
        // Log level colors for ESP-IDF output
        LogInfo: lipgloss.NewStyle().Foreground(successColor),
        LogWarn: lipgloss.NewStyle().Foreground(warningColor),
        LogError: lipgloss.NewStyle().Foreground(errorColor),
        
        // ... etc
    }
}
```

#### 2.2 Use rounded borders

Replace `NormalBorder()` with `RoundedBorder()` for a more modern look.

#### 2.3 Add accent colors for interactive elements

- Selected item: highlighted background
- Focused field: border color change
- Buttons: distinct styling

### Phase 3: Fix Monitor View Layout (Medium Priority)

#### 3.1 Improve title bar

Show device nickname (if available) or short port name instead of full path:

```go
func (m monitorModel) renderTitle() string {
    port := filepath.Base(m.cfg.Port)  // Use basename
    if m.session != nil && m.session.nickname != "" {
        port = m.session.nickname
    }
    // ...
}
```

#### 3.2 Add scroll indicators in HOST mode

When not at bottom, show `▲ N more` indicator.

### Phase 4: Verify Serial Console Interaction (High Priority)

#### 4.1 Test keyboard input in tmux

Run esper in tmux, connect to a device, and verify:
- `help` command works
- Arrow keys for history (if supported by device REPL)
- No accidental key leakage to device in HOST mode

#### 4.2 Check textinput widget handling

The `textinput.Model` from bubbles handles input. Verify it's configured correctly:
- Prompt is empty (correct)
- Focus is maintained
- Enter sends the line

### Phase 5: Polish Overlays (Low Priority)

- Command palette: already looks decent
- Help overlay: add colors, better layout
- Filter overlay: functional, could use polish

## Implementation Order

1. **Fix Port Picker layout** - prevents the "broken" first impression
2. **Add Port Picker footer** - provides discoverability
3. **Expand styles.go with colors** - immediate visual improvement
4. **Test serial interaction** - ensure core functionality works
5. **Fix monitor title truncation** - better usability
6. **Add scroll indicators** - improved HOST mode UX
7. **Polish overlays** - final touches

## Testing Checklist

- [ ] Port picker renders correctly at 80x24
- [ ] Port picker renders correctly at 120x40
- [ ] Form fields don't wrap incorrectly
- [ ] Footer key hints are visible
- [ ] Monitor shows colored log output
- [ ] Serial console accepts input and sends to device
- [ ] `help` command works in connected REPL
- [ ] HOST mode scrolling works
- [ ] Overlays render centered and styled

## Open Questions

1. **Color scheme**: Should we use a dark theme (current assumption) or support light themes?
2. **Font considerations**: Do we assume Unicode support for rounded corners?
3. **ESP-IDF log coloring**: The auto-colorer exists but may need styling hooks

## References

- `esper/pkg/monitor/styles.go` - needs expansion
- `esper/pkg/monitor/port_picker.go` - layout fixes needed
- `esper/pkg/monitor/monitor_view.go` - title and scroll fixes
- `ttmp/2026/01/24/ESP-02-ESPER-TUI/.../02-tui-current-vs-desired-compare.md` - detailed wireframes
- Charm libraries: bubbles, lipgloss documentation
