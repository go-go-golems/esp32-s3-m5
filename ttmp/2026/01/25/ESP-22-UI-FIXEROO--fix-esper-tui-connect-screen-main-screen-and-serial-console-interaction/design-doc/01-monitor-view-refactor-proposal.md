---
Title: Monitor View Refactor Proposal
Ticket: ESP-22-UI-FIXEROO
Status: active
Topics:
    - esper
    - tui
    - bubbletea
    - refactor
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/monitor_view.go
      Note: Monolithic file to split (~1500 LOC)
    - Path: esper/pkg/monitor/styles.go
      Note: Style definitions (needs expansion)
    - Path: ttmp/2026/01/25/ESP-14-SPLIT-MONITOR-VIEW--split-monitor-view-go-into-cohesive-units/design-doc/01-specification.md
      Note: Detailed split specification
    - Path: ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md
      Note: Shared pipeline extraction spec
ExternalSources: []
Summary: Proposal to refactor monitor_view.go by splitting it into cohesive units, with a focus on making visual styling fixes easier to implement and maintain.
LastUpdated: 2026-01-25T22:50:00-05:00
WhatFor: Guide the refactoring of monitor_view.go to enable cleaner visual fixes.
WhenToUse: When planning how to restructure the monitor code before or alongside visual fixes.
---


# Monitor View Refactor Proposal

## Executive Summary

`esper/pkg/monitor/monitor_view.go` is a ~1500 LOC monolith mixing:
- Serial I/O and pipeline orchestration
- Buffer management
- Search/filter logic
- Inspector event model
- Session logging
- UI layout and rendering

This document proposes a refactoring approach that:
1. Aligns with ESP-14-SPLIT-MONITOR-VIEW (file decomposition)
2. Prepares for ESP-12-SHARED-PIPELINE (pipeline extraction)
3. Makes visual/styling fixes easier to implement and test

## Current State Analysis

### File Structure (monitor package)

```
esper/pkg/monitor/
├── app_model.go              # Screen router (297 LOC)
├── monitor_view.go           # THE MONOLITH (1482 LOC) ← Problem
├── port_picker.go            # Port picker screen (455 LOC)
├── device_manager.go         # Device manager screen (300 LOC)
├── styles.go                 # Style definitions (72 LOC) ← Needs expansion
├── overlay_*.go              # Various overlays (6 files, ~600 LOC total)
├── *_test.go                 # Tests (4 files)
└── helpers                   # select_list.go, messages.go, etc.
```

### Responsibilities Mixed in monitor_view.go

| Responsibility | Lines (approx) | Should Be |
|---------------|----------------|-----------|
| **Type definitions** (`monitorModel`, etc.) | 1-105 | `monitor_model.go` |
| **Update logic** (message handling) | 160-642 | `monitor_update.go` |
| **View rendering** (View, renderTitle, etc.) | 644-765 | `monitor_view.go` (smaller) |
| **Serial commands** (readSerialCmd, etc.) | 947-968, 1416-1431 | `monitor_serial.go` |
| **Buffer management** (append, ring bounds) | 883-932 | `monitor_buffers.go` |
| **Search logic** (search*, decorate*) | 1069-1414 | `monitor_search.go` |
| **Filter logic** (filteredLines, filterSummary) | 823-873, 1095-1156 | `monitor_filter.go` |
| **Inspector** (addEvent, renderInspectorPanel) | 970-1046 | `monitor_inspector.go` |
| **Session logging** (toggle*, write*) | 1434-1481 | `monitor_sessionlog.go` |

---

## Proposed Decomposition

### Phase 1: Create ui_helpers.go (Low Risk)

Move shared helpers currently scattered in `port_picker.go`:

```go
// esper/pkg/monitor/ui_helpers.go
package monitor

import (
    "strings"
    "github.com/charmbracelet/lipgloss"
)

// From port_picker.go
func padOrTrim(s string, w int) string { ... }
func min(a, b int) int { ... }
func max(a, b int) int { ... }

// From monitor_view.go
func splitLinesN(s string, n int) []string { ... }
func stringsJoinVertical(lines []string) string { ... }
```

### Phase 2: Split Rendering (Medium Risk)

Create focused `monitor_view.go` for View only:

```go
// esper/pkg/monitor/monitor_view.go (new, ~200 LOC)
package monitor

// View renders the monitor screen
func (m monitorModel) View(st styles, sz size, curMode mode) string { ... }

// renderTitle returns the title bar content
func (m monitorModel) renderTitle() string { ... }

// renderStatus returns the status bar content
func (m monitorModel) renderStatus(curMode mode) string { ... }

// renderCoreDumpProgressBox renders the capture overlay
func (m monitorModel) renderCoreDumpProgressBox(st styles, sz size) string { ... }
```

### Phase 3: Split Search Logic

```go
// esper/pkg/monitor/monitor_search.go (~300 LOC)
package monitor

func (m *monitorModel) openSearch() { ... }
func (m *monitorModel) closeSearch() { ... }
func (m *monitorModel) searchApplyJump() { ... }
func (m *monitorModel) searchComputeMatches() { ... }
func (m monitorModel) renderSearchBar(w int) string { ... }
func (m monitorModel) decorateSearchLines(lines []string, width int) []string { ... }
func highlightPlainSubstring(s, query string, st lipgloss.Style) string { ... }
```

### Phase 4: Split Filter Logic

```go
// esper/pkg/monitor/monitor_filter.go (~100 LOC)
package monitor

func (m monitorModel) filteredLines() []string { ... }
func (m monitorModel) filterSummary() string { ... }
func (m monitorModel) searchSummary() string { ... }
```

### Phase 5: Split Inspector Logic

```go
// esper/pkg/monitor/monitor_inspector.go (~150 LOC)
package monitor

type monitorEvent struct { ... }

func (m *monitorModel) addEvent(kind, title, body string) { ... }
func (m monitorModel) renderInspectorPanel(st styles, sz size) string { ... }
```

### Phase 6: Split Serial Commands

```go
// esper/pkg/monitor/monitor_serial.go (~100 LOC)
package monitor

func (m monitorModel) readSerialCmd() tea.Cmd { ... }
func (m monitorModel) tickCmd() tea.Cmd { ... }
func (m monitorModel) resetDeviceCmd() tea.Cmd { ... }
func (m monitorModel) sendBreakCmd() tea.Cmd { ... }
```

### Phase 7: Split Session Logging

```go
// esper/pkg/monitor/monitor_sessionlog.go (~80 LOC)
package monitor

func (m *monitorModel) toggleSessionLogging() error { ... }
func (m *monitorModel) closeSessionLogging() { ... }
func (m *monitorModel) writeSessionLog(b []byte) { ... }
```

### Phase 8: Model Definition

```go
// esper/pkg/monitor/monitor_model.go (~150 LOC)
package monitor

type monitorModel struct { ... }

func newMonitorModel(cfg Config, session *serialSession) monitorModel { ... }
func (m *monitorModel) setSize(sz size) { ... }
func (m *monitorModel) append(b []byte) { ... }
func (m *monitorModel) appendCoreDumpLogEvents(events [][]byte) { ... }
func (m *monitorModel) refreshViewportContent() { ... }
func (m *monitorModel) setToast(text string, d time.Duration) { ... }
```

### Final: Update Logic

```go
// esper/pkg/monitor/monitor_update.go (~400 LOC)
package monitor

func (m monitorModel) Update(msg tea.Msg, curMode mode) (monitorModel, tea.Cmd, monitorAction) { ... }

// Internal helpers for update
func (m *monitorModel) execPalette(kind paletteCommandKind) (tea.Cmd, monitorAction) { ... }
```

---

## Resulting File Layout

```
esper/pkg/monitor/
├── app_model.go              # Screen router (unchanged)
├── monitor_model.go          # Type + constructor + size      (~150 LOC)
├── monitor_update.go         # Update logic                   (~400 LOC)
├── monitor_view.go           # View rendering only            (~200 LOC)
├── monitor_serial.go         # Serial commands                (~100 LOC)
├── monitor_buffers.go        # Buffer/append logic            (~80 LOC)
├── monitor_filter.go         # Filter logic                   (~100 LOC)
├── monitor_search.go         # Search logic                   (~300 LOC)
├── monitor_inspector.go      # Inspector events + render      (~150 LOC)
├── monitor_sessionlog.go     # Session logging                (~80 LOC)
├── port_picker.go            # Port picker (unchanged)
├── device_manager.go         # Device manager (unchanged)
├── styles.go                 # Style definitions
├── ui_helpers.go             # Shared helpers                 (~50 LOC)
├── overlay_*.go              # Overlays (unchanged)
└── *_test.go                 # Tests
```

---

## Relationship to ESP-14 and ESP-12

### ESP-14-SPLIT-MONITOR-VIEW

This proposal **implements** ESP-14. The file layout matches the ESP-14 specification with minor naming adjustments:
- ESP-14's `monitor_buffers.go` → same
- ESP-14's `monitor_serial.go` → same
- etc.

### ESP-12-SHARED-PIPELINE

This split **prepares for** ESP-12. After the split:
- Serial pipeline logic is isolated in `monitor_update.go` (the `serialChunkMsg` handler)
- Pipeline components (LineSplitter, AutoColorer, GDBStubDetector, CoreDumpDecoder, PanicDecoder) are clearly called from one place
- ESP-12 can then extract these into `pkg/pipeline` with minimal churn

### Current pipeline code path (to be extracted in ESP-12)

```go
// In monitor_update.go, serialChunkMsg handler:
case serialChunkMsg:
    m.lastDataAt = time.Now()
    
    // GDB stub detection
    if g := m.gdb.Push(t.b); g != nil { ... }
    
    // Line framing
    lines := m.lineSplitter.Push(t.b)
    
    for _, line := range lines {
        // Core dump detection
        events, sendEnter := m.coredump.PushLine(line)
        
        // Backtrace detection
        if decoded, ok := m.panic.DecodeBacktraceLine(line); ok { ... }
        
        // Auto-color and append
        m.append(m.autoColor.ColorizeLine(line))
    }
```

ESP-12 will extract this into:
```go
outputs := m.pipeline.PushChunk(t.b)
for _, out := range outputs {
    switch out.Kind {
    case pipeline.OutputLine:
        m.append(out.Line)
    case pipeline.OutputEvent:
        m.addEvent(out.Event.Kind, out.Event.Title, out.Event.Body)
    // etc.
    }
}
```

---

## Visual Fixes Integration

### Where to make style changes

After the split:

| Fix | File to Edit |
|-----|--------------|
| Log level colors | `monitor_model.go` (append) or wait for ESP-12 pipeline |
| Title bar styling | `monitor_view.go` (renderTitle) |
| Status bar styling | `monitor_view.go` (renderStatus) |
| Inspector panel styling | `monitor_inspector.go` (renderInspectorPanel) |
| Search markers styling | `monitor_search.go` (decorateSearchLines) |
| Filter overlay styling | `filter_overlay.go` (unchanged) |
| Command palette styling | `palette_overlay.go` (unchanged) |
| All style definitions | `styles.go` |

### Recommended order

1. **Do visual fixes FIRST** (on the monolith) - they're smaller changes
2. **Then split files** - pure refactor, easier to verify
3. **Then extract pipeline** (ESP-12) - benefits from clean file structure

OR

1. **Split ui_helpers.go first** (trivial, reduces duplication)
2. **Do visual fixes** (still in monolith, but helpers are shared)
3. **Complete split** (after visual fixes are working)

---

## Testing Strategy

### Existing tests to preserve

- `view_test.go` - sizing invariants
- `app_model_overlay_test.go` - overlay routing
- `filter_overlay_test.go` - filter semantics
- `palette_overlay_test.go` - palette semantics

### Regression check

After each split step:
```bash
cd esper && go test ./pkg/monitor -count=1
cd esper && go vet ./...
```

### Manual smoke test

```bash
# Virtual PTY (no hardware)
USE_VIRTUAL_PTY=1 ./ttmp/.../scripts/09-tmux-capture-esper-tui.sh

# Real hardware (if available)
cd esper && go run ./cmd/esper --port '/dev/serial/by-id/usb-Espressif*'
```

---

## Open Questions

1. **Split first or fix first?**
   - Recommendation: Fix visual bugs first (smaller scope), then split

2. **Should log colors use lipgloss styles or ANSI passthrough?**
   - Current: ANSI passthrough via `render/autocolor.go`
   - Works in terminal, but screenshots don't show colors
   - Could add lipgloss-based styling for UI consistency

3. **Should styles be in a separate package?**
   - No, keep in `monitor` package for now
   - Could extract to `pkg/tui/styles` later if reused

---

## Implementation Checklist

- [ ] Create `ui_helpers.go` and move shared helpers
- [ ] Run tests to verify no regressions
- [ ] Split `monitor_view.go` (View functions only)
- [ ] Run tests
- [ ] Split `monitor_search.go`
- [ ] Run tests
- [ ] Split `monitor_filter.go`
- [ ] Run tests
- [ ] Split `monitor_inspector.go`
- [ ] Run tests
- [ ] Split `monitor_serial.go`
- [ ] Run tests
- [ ] Split `monitor_sessionlog.go`
- [ ] Run tests
- [ ] Split `monitor_model.go` (types + constructor)
- [ ] Run tests
- [ ] Rename remaining file to `monitor_update.go`
- [ ] Final test run + manual smoke test

---

## References

- ESP-14-SPLIT-MONITOR-VIEW specification:
  `ttmp/2026/01/25/ESP-14-SPLIT-MONITOR-VIEW--split-monitor-view-go-into-cohesive-units/design-doc/01-specification.md`
  
- ESP-12-SHARED-PIPELINE specification:
  `ttmp/2026/01/25/ESP-12-SHARED-PIPELINE--extract-shared-serial-pipeline-package/design-doc/01-specification.md`
  
- Style-to-component mapping:
  `ttmp/2026/01/25/ESP-22-UI-FIXEROO--fix-esper-tui-connect-screen-main-screen-and-serial-console-interaction/reference/01-style-to-component-mapping.md`
