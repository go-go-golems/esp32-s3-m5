---
Title: 'Esper TUI: Bubble Tea model/message decomposition'
Ticket: ESP-02-ESPER-TUI
Status: active
Topics:
    - serial
    - console
    - ui
    - tooling
    - esp-idf
    - debugging
    - usb-serial-jtag
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/decode/coredump.go
      Note: Core dump capture/mute behavior to model as UI state
    - Path: esper/pkg/decode/gdbstub.go
      Note: GDB stub detection behavior to surface via notifications
    - Path: esper/pkg/decode/panic.go
      Note: Panic/backtrace decode behavior to surface via inspector/toasts
    - Path: esper/pkg/monitor/monitor.go
      Note: Current minimal Bubble Tea monitor; informs existing message types and serial ingest
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md
      Note: Designer's wireframes and interaction spec that this decomposition maps to
ExternalSources: []
Summary: 'Implementation decomposition for the Esper TUI: Bubble Tea models, message routing (KeyMsg/WindowSizeMsg), commands, and Lip Gloss styling approach, grounded in the designer wireframes.'
LastUpdated: 2026-01-24T21:42:15.744163304-05:00
WhatFor: Hand to engineers implementing the Esper TUI in Bubble Tea; maps the UX spec to concrete models/messages/commands and a Lip Gloss styling approach.
WhenToUse: Use when starting implementation or code review of the Esper TUI; serves as the architectural checklist for event routing and layout.
---


# Esper TUI: Bubble Tea model/message decomposition

## Executive Summary

This document decomposes the designer’s “Esper TUI: Full UX Specification with Wireframes” into implementable Bubble Tea concepts:

- a set of Bubble Tea models (root app + view models + overlay models),
- a message catalog (including explicit routing of `tea.KeyMsg` and `tea.WindowSizeMsg`),
- command patterns for serial IO and async actions,
- and a styling/layout approach that uses Lip Gloss primitives (borders, padding, placement) rather than “hand-drawing” borders.

The intent is to give an engineer a clear blueprint for how to structure the TUI code without forcing them to match the YAML widget pseudocode exactly (the YAML in the UX spec is guidance, not binding).

## Problem Statement

We have a full UX spec with ASCII wireframes, overlays, and key mappings. If we jump straight into coding, we risk:

- inconsistent key routing (host vs device; overlay vs base view),
- broken resizing (viewport/input layout drift, truncation),
- manual “ASCII art” borders that don’t match terminal constraints and are hard to maintain,
- and cross-cutting state scattered across screens (e.g., follow mode, filter/search, core dump capture, notifications).

We need an implementation decomposition that cleanly separates:

- app-wide state (connection, device/host mode, active overlay, window size),
- per-view state (port picker selection and forms; monitor viewport and scrollback),
- and rendering concerns (Lip Gloss styles + layout rules).

## Proposed Solution

### 1) Architectural overview

Implement a single Bubble Tea program whose root model composes:

- **App model**: owns global state (window size, active screen, overlay stack, global key bindings, serial session lifecycle).
- **Port Picker model**: used before connection (or on reconnect), with list + form fields + device registry actions.
- **Monitor model**: used during active connection: viewport + status bar + device input line.
- **Overlays** (modal/overlay models): help, command palette / host menu, search UI, filter UI, inspector (right panel or modal), confirmation dialogs, toasts.

The root model is responsible for routing:

- `tea.WindowSizeMsg` (always update layout first; then delegate to active view/overlay),
- `tea.KeyMsg` (apply global shortcuts; then route to overlay if present; else route to active screen).

### 2) Message catalog (core principle)

Bubble Tea already provides:

- `tea.KeyMsg` for keyboard input,
- `tea.WindowSizeMsg` for resize,
- plus lifecycle messages like `tea.Quit` and custom tick messages.

We add *explicit* app-specific messages for async results and streaming IO:

- serial read chunk / error
- port scan results
- connect/disconnect results
- “toast” notifications
- “mode” transitions (device ↔ host)

### 3) Styling/layout principle

Use Lip Gloss styles to render borders, padding, and layout:

- `lipgloss.NewStyle().Border(...)` for boxes/panels (do not manually concatenate box drawing characters),
- `lipgloss.JoinVertical/JoinHorizontal`, `lipgloss.Place`, and `MaxWidth/Width/Height` for layout,
- consistent theme via a shared `styles` struct (colors/typography), used by all submodels.

This keeps UI chrome consistent and makes resizing robust.

## Design Reference (wireframes, verbatim)

The following wireframes are copied verbatim from:
`ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md`

### Port Picker (80×24) — With Device Registry Nicknames (verbatim)

```
┌─ esper ─────────────────────────────────────────────────────────────── v0.1 ─┐
│                                                                              │
│  ┌─ Select Serial Port ─────────────────────────────────────────────────┐   │
│  │                                                                      │   │
│  │   → atoms3r    M5 AtomS3R              USB-JTAG   ★                 │   │
│  │     xiao-c6    Seeed XIAO ESP32-C6     USB-JTAG   ★                 │   │
│  │     cardputer  M5Stack Cardputer       CP2104                        │   │
│  │     —          /dev/ttyUSB1            (unknown)   [n: assign]       │   │
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

### Monitor View — Normal Mode (120×40) (verbatim)

```
┌─ esper ── Connected: /dev/serial/by-id/usb-Espressif_USB_JTAG-if00 ── 115200 ── ELF: ✓ ──┐
│                                                                                          │
│ I (1523) wifi: wifi driver task: 3ffc1e4c, prio:23, stack:6656, core=0                   │
│ I (1527) system_api: Base MAC address is not set                                         │
│ I (1533) system_api: read default base MAC address from EFUSE                            │
│ I (1540) wifi: wifi firmware version: 3.0                                                │
│ I (1545) wifi: config NVS flash: enabled                                                 │
│ W (1550) wifi: Warning: NVS partition low on space                                       │
│ I (1556) wifi_init: rx ba win: 6                                                         │
│ I (1560) wifi_init: tcpip mbox: 32                                                       │
│ I (1565) wifi_init: udp mbox: 6                                                          │
│ I (1570) wifi_init: tcp mbox: 6                                                          │
│ I (1575) wifi_init: tcp tx win: 5744                                                     │
│ I (1580) wifi_init: tcp rx win: 5744                                                     │
│ I (1585) wifi_init: tcp mss: 1436                                                        │
│ I (1590) wifi_init: WiFi IRAM OP enabled                                                 │
│ I (1595) wifi_init: WiFi RX IRAM OP enabled                                              │
│ I (1600) phy_init: phy_version 4670, dec 20 2021                                         │
│ I (1650) wifi: mode : sta (7c:df:a1:b0:c5:d2)                                            │
│ I (1651) wifi: enable tsf                                                                │
│ E (1655) wifi: esp_wifi_connect: station is connecting, return error                     │
│                                                                                          │
│                                                                                          │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ Mode: DEVICE │ Follow: ON │ Capture: — │ Filter: — │ Search: — │ Buf: 128K/1M │ 12:34:56 │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ > [                                                                                    ] │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

### Monitor View — 80×24 Compact Mode (verbatim)

```
┌─ esper ── /dev/ttyUSB0 ── 115200 ──────────────────────┐
│ I (1523) wifi: wifi driver task: 3ffc1e4c, pri...      │
│ I (1527) system_api: Base MAC address is not set       │
│ I (1533) system_api: read default base MAC add...      │
│ I (1540) wifi: wifi firmware version: 3.0              │
│ W (1550) wifi: Warning: NVS partition low on ...       │
│ I (1556) wifi_init: rx ba win: 6                       │
│ I (1560) wifi_init: tcpip mbox: 32                     │
│ I (1565) wifi_init: udp mbox: 6                        │
│ I (1570) wifi_init: tcp mbox: 6                        │
│ I (1575) wifi_init: tcp tx win: 5744                   │
│ I (1580) wifi_init: tcp rx win: 5744                   │
│ I (1585) wifi_init: tcp mss: 1436                      │
│ I (1590) wifi_init: WiFi IRAM OP enabled               │
│ I (1595) wifi_init: WiFi RX IRAM OP enabled            │
│ I (1600) phy_init: phy_version 4670, dec 20 20...      │
├────────────────────────────────────────────────────────┤
│ DEV │ Follow │ ─ │ ─ │ 12:34│ Ctrl-T:menu  ?:help      │
├────────────────────────────────────────────────────────┤
│ > [                                                  ] │
└────────────────────────────────────────────────────────┘
```

## Model Decomposition

### Root model: `appModel`

**Responsibilities**
- Own the global “app state machine”:
  - `screen = PortPicker | Monitor`
  - `overlayStack = []overlayKind` (top-most gets input first)
  - `mode = Device | Host` (as per status bar)
- Own global resources:
  - serial session handle (port, reader goroutine/cmd, writer)
  - shared theme/styles
  - global notifications (toast queue)
- Route Bubble Tea messages, in the correct priority order.

**Key invariants**
- `tea.WindowSizeMsg` is always handled before any `View()` call that depends on width/height.
- When an overlay is active, it captures `tea.KeyMsg` first.
- Device input is only sent to serial when:
  - we are connected AND
  - mode == Device AND
  - no overlay/palette/search dialog is capturing text input.

### View model: `portPickerModel`

**Responsibilities**
- Maintain scan results list, selection, and presentation rows (including device registry nickname columns if present).
- Manage configuration form fields:
  - baud dropdown
  - elf path input
  - toolchain prefix input
  - probe-with-esptool checkbox (and confirmation)
- Produce actions:
  - rescan ports
  - connect (emits `connectRequestedMsg{...}`)
  - open device manager overlay
  - open help overlay

### View model: `monitorModel`

**Responsibilities**
- Display viewport of log output with scrollback buffer.
- Track follow mode, scroll position, wrap toggle, filter/search state (or delegate to overlay models).
- Manage device console input line (text input).
- Interpret special stream events:
  - core dump capture in progress (muted output)
  - panic/backtrace detected and decoded output
  - gdb stub detected
- Emit toasts and “open inspector” actions.

**Notes**
- The existing `esper/pkg/monitor/monitor.go` currently accumulates `m.out` as a bounded string; in the TUI, we should separate:
  - byte stream parsing/decoding (model state)
  - rendered viewport content (string for viewport)
  - and structured events (inspector entries).

### Overlay models (modal/panel)

Overlays should implement a consistent interface:

- `Init() tea.Cmd`
- `Update(msg tea.Msg) (overlayModel, tea.Cmd, overlayResult)`
- `View() string` (rendered via Lip Gloss, placed over a dimmed background)

Suggested overlays:

- `helpOverlayModel` (keymap)
- `hostMenuOverlayModel` (Ctrl-T menu / command palette)
- `searchOverlayModel`
- `filterOverlayModel`
- `confirmOverlayModel` (dangerous actions)
- `inspectorOverlayModel` (or right panel in wide mode)
- `toastModel` (non-blocking, timed dismissal)

## Message Catalog

### Bubble Tea built-ins (must route)

- `tea.KeyMsg`
  - Global shortcuts (`Ctrl-C`, `?`, `Ctrl-T`, etc.)
  - Delegated to overlay or active view depending on state
- `tea.WindowSizeMsg`
  - Update `appModel.viewportW/H`, panel widths, input widths
  - Recompute layout and call child `SetSize(...)` / store sizes in models

### App-defined messages (suggested)

Port picker / connection lifecycle:

- `portsScanRequestedMsg`
- `portsScanResultMsg{ports []scan.Port, err error}`
- `connectRequestedMsg{portPath string, baud int, elfPath string, toolchainPrefix string, probeEsptool bool, ...}`
- `connectResultMsg{session *serialSession, err error}`
- `disconnectRequestedMsg{reason string}`
- `disconnectResultMsg{err error}`

Serial IO:

- `serialChunkMsg{b []byte}` (already exists today)
- `serialErrMsg{err error}` (already exists today)
- `tickMsg{t time.Time}` (already exists today)

Monitor state transitions:

- `followChangedMsg{on bool}`
- `modeChangedMsg{mode Host|Device}`
- `overlayPushedMsg{kind overlayKind}`
- `overlayPoppedMsg{kind overlayKind}`

Notifications:

- `toastMsg{level info|warn|err, text string, ttl time.Duration}`

## Message Routing (pseudocode)

### Root `Update`

```go
func (m appModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
  // 1) Always handle resize first.
  switch msg := msg.(type) {
  case tea.WindowSizeMsg:
    m.winW, m.winH = msg.Width, msg.Height
    m.layout = computeLayout(m.winW, m.winH, m.screen, m.overlayStack)
    m.portPicker.SetSize(m.layout)
    m.monitor.SetSize(m.layout)
    for _, ov := range m.overlays { ov.SetSize(m.layout) }
    return m, nil
  }

  // 2) Global quit.
  if k, ok := msg.(tea.KeyMsg); ok && k.Type == tea.KeyCtrlC {
    return m, tea.Quit
  }

  // 3) Route KeyMsg: overlay captures first, then active screen.
  if k, ok := msg.(tea.KeyMsg); ok {
    // Global overlays toggles.
    if isHelpToggle(k) { m.toggleHelpOverlay(); return m, nil }
    if isHostMenuToggle(k) { m.pushHostMenuOverlay(); return m, nil }

    if m.overlayStack.HasActive() {
      ov := m.overlayStack.Top()
      ov2, cmd, result := ov.Update(k)
      m.overlayStack.ReplaceTop(ov2)
      m.applyOverlayResult(result) // e.g., pop overlay, dispatch action
      return m, cmd
    }

    // No overlay: delegate to current screen.
    switch m.screen {
    case ScreenPortPicker:
      pm, cmd, action := m.portPicker.Update(k)
      m.portPicker = pm
      return m, m.applyAction(action, cmd)
    case ScreenMonitor:
      mm, cmd, action := m.monitor.Update(k, m.mode)
      m.monitor = mm
      return m, m.applyAction(action, cmd)
    }
  }

  // 4) Non-key messages go to the relevant subsystem(s).
  // - serial chunks/errors mostly go to monitor
  // - scan/connect results mostly go to port picker + app state
  switch msg := msg.(type) {
  case serialChunkMsg, serialErrMsg, tickMsg:
    mm, cmd, action := m.monitor.Update(msg, m.mode)
    m.monitor = mm
    return m, m.applyAction(action, cmd)
  case portsScanResultMsg:
    m.portPicker.ApplyScanResult(msg)
    return m, nil
  case connectResultMsg:
    if msg.err != nil { m.portPicker.SetError(msg.err); m.screen = ScreenPortPicker; return m, nil }
    m.session = msg.session
    m.screen = ScreenMonitor
    return m, nil
  default:
    return m, nil
  }
}
```

### Monitor `Update` (key routing and device input)

```go
func (m monitorModel) Update(msg tea.Msg, mode Mode) (monitorModel, tea.Cmd, action Action) {
  switch msg := msg.(type) {
  case tea.KeyMsg:
    // If in Host mode: keys operate on UI (scroll/search/menu), not sent to device.
    if mode == Host {
      return m.updateHostKeys(msg)
    }
    // In Device mode: most printable keys go to input, Enter sends to device.
    return m.updateDeviceKeys(msg)

  case serialChunkMsg:
    // Parse stream -> append to buffer -> update viewport content.
    // Detect special events -> add inspector entries / toast.
    m.ingestSerialBytes(msg.b)
    return m, m.readSerialCmd(), ActionNone

  case tickMsg:
    m.finalizeIdleTailIfNeeded()
    return m, m.tickCmd(), ActionNone

  case tea.WindowSizeMsg:
    // Typically handled by root, but safe to accept if routed here.
    m.setSize(msg.Width, msg.Height)
    return m, nil, ActionNone
  }
}
```

## Styling and Layout (Lip Gloss pseudocode)

### Shared styles

```go
type styles struct {
  TitleBar lipgloss.Style
  StatusBar lipgloss.Style
  Panel lipgloss.Style
  PanelTitle lipgloss.Style
  Input lipgloss.Style
  ErrorBanner lipgloss.Style
  DimOverlay lipgloss.Style
  ToastInfo lipgloss.Style
  ToastWarn lipgloss.Style
  ToastErr lipgloss.Style
}

func defaultStyles(theme Theme) styles {
  return styles{
    TitleBar: lipgloss.NewStyle().Bold(true),
    StatusBar: lipgloss.NewStyle().Faint(true),
    Panel: lipgloss.NewStyle().
      Border(lipgloss.RoundedBorder()).
      BorderForeground(theme.Border).
      Padding(0, 1),
    Input: lipgloss.NewStyle().
      Border(lipgloss.NormalBorder()).
      Padding(0, 1),
    ErrorBanner: lipgloss.NewStyle().
      Border(lipgloss.ThickBorder()).
      BorderForeground(theme.Red).
      Foreground(theme.Red),
    DimOverlay: lipgloss.NewStyle().Faint(true),
  }
}
```

### Layout computation

```go
func computeLayout(w, h int, screen Screen, overlays overlayStack) layout {
  // Avoid hard-coding box-drawing. Use Lip Gloss borders and padding.
  // Reserve:
  // - 1 line for titlebar (or style with fixed height)
  // - 1–2 lines for status bar
  // - 1–3 lines for input
  // The remainder is viewport height.
  return layout{W: w, H: h, ViewportH: h - titleH - statusH - inputH}
}
```

### Rendering a bordered panel (no manual borders)

```go
func (m portPickerModel) View(styles styles, layout layout) string {
  title := styles.TitleBar.Render(renderTitleSegments(...))
  body := styles.Panel.
    Width(layout.W).
    Height(layout.H - 1).
    Render(m.renderPanelContent(styles, layout))
  return lipgloss.JoinVertical(lipgloss.Left, title, body)
}
```

## Design Decisions

1. **Single root router**: all `tea.KeyMsg` and `tea.WindowSizeMsg` routing is centralized to avoid “keys leaked to device” bugs.
2. **Overlay-first input capture**: overlays always take precedence over base view input.
3. **Lip Gloss for chrome**: borders/padding are rendered via Lip Gloss to reduce fragile manual string assembly.
4. **Separate stream ingest from rendering**: serial ingestion updates structured state, then viewport rendering uses that state.

## Alternatives Considered

1. **Each submodel handles `tea.WindowSizeMsg` independently**: tends to duplicate layout logic and causes drift; root should compute layout and push sizes down.
2. **Manually drawing borders/boxes in `View()`**: brittle, hard to keep aligned across sizes, and conflicts with “use Lip Gloss directives” guidance.
3. **Global key handling embedded in each view**: increases chance of inconsistent behavior and makes “host vs device” mode correctness harder.

## Implementation Plan

1. Implement root `appModel` with explicit screen enum, mode enum, and overlay stack.
2. Implement `portPickerModel` (list + form) with scan/connect lifecycle messages.
3. Implement `monitorModel` with viewport + scrollback + follow mode; wire in existing serial stream parsing/detection.
4. Add overlays incrementally: help, host menu/palette, search, filter, confirmations, inspector.
5. Ensure resizing is correct by testing at 80×24 and 120×40 continuously.

## Open Questions

1. Should Host mode be a persistent toggle or a transient menu chord (e.g., `Ctrl-T` then next key)?
2. Should inspector be a right panel in wide mode and a modal in narrow mode, or always modal?
3. What is the minimal viable copy/clipboard model (OSC52 vs “copy to file”)?

## References

- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md`
- `esper/pkg/monitor/monitor.go`

## Design Decisions

<!-- Document key design decisions and rationale -->

## Alternatives Considered

<!-- List alternative approaches that were considered and why they were rejected -->

## Implementation Plan

<!-- Outline the steps to implement this design -->

## Open Questions

<!-- List any unresolved questions or concerns -->

## References

<!-- Link to related documents, RFCs, or external resources -->
