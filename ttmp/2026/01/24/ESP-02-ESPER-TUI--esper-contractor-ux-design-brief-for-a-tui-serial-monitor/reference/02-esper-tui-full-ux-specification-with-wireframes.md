---
Title: 'Esper TUI: Full UX Specification with Wireframes'
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esper/pkg/monitor/monitor.go
      Note: Current minimal implementation to be replaced
    - Path: ttmp/2026/01/24/ESP-02-DEVICE-REGISTRY--device-registry-nicknames-for-serial-consoles/design-doc/01-device-registry-nickname-resolution.md
      Note: Device registry design informing Port Picker and Device Manager UX
    - Path: ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/design-doc/01-esper-tui-ux-design-brief-for-contractor.md
      Note: Source requirements and problem statement
ExternalSources: []
Summary: Complete UX specification with ASCII wireframes and YAML widget pseudocode for implementing the Esper TUI in Bubble Tea.
LastUpdated: 2026-01-25T00:00:00-05:00
WhatFor: Hand to an engineer implementing the TUI in Bubble Tea; provides exact screen layouts, widget structure, and keyboard mappings.
WhenToUse: Use when building or reviewing the Esper TUI implementation; serves as the visual contract for all screens and interactions.
---



# Esper TUI: Full UX Specification with Wireframes

## Goal

Provide a complete, implementable UX specification for the Esper TUI serial monitor with:
- ASCII wireframes for every screen and overlay state
- YAML pseudocode describing Bubble Tea widget structure
- Complete keyboard mapping with state-aware behavior
- Visual state machine showing screen transitions

## Document Conventions

**ASCII Wireframe Symbols:**
```
┌─┐ └─┘ │ ├ ┤ ┬ ┴ ┼  Box drawing (UI chrome)
[...]                  Interactive input field
<...>                  Button/action target
(...)                  Status indicator
*...*                  Highlighted/selected item
→                      Focus indicator
```

**YAML Pseudocode:**
- Describes widget hierarchy and properties
- NOT executable code — implementation reference only
- `bindings:` sections show key → action mappings

---

# Part 1: Screen Inventory

## 1.1 Port Picker View

The entry screen when no `--port` is provided or connection fails.

### ASCII Wireframe (80×24) — With Device Registry Nicknames

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

**Port List Columns (with registry):**
- **Nickname**: Device nickname from registry (or `—` if unregistered)
- **Name**: Human-readable device name (or raw path if unregistered)
- **Chip Type**: Detected chip family (USB-JTAG, CP210x, etc.)
- **Preferred**: ★ indicates Espressif-preferred device
- **Action Hint**: `[n: assign]` shown for unregistered devices

### Port Picker: Error State

```
┌─ esper ─────────────────────────────────────────────────────────────── v0.1 ─┐
│                                                                              │
│  ┌─ ERROR ──────────────────────────────────────────────────────────────┐   │
│  │  Connection failed: /dev/ttyUSB0: permission denied                  │   │
│  │  Try: sudo usermod -aG dialout $USER                                 │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  ┌─ Select Serial Port ─────────────────────────────────────────────────┐   │
│  │                                                                      │   │
│  │   → atoms3r    M5 AtomS3R              USB-JTAG   ★                 │   │
│  │     —          /dev/ttyUSB0            (error)                       │   │
│  │                                                                      │   │
│  ... (rest of form) ...
```

### Port Picker: Assign Nickname Dialog

When pressing `n` on an unregistered port, or selecting "Assign nickname" action:

```
┌─ esper ─────────────────────────────────────────────────────────────── v0.1 ─┐
│                                                                              │
│  ┌─ Select Serial Port ─────────────────────────────────────────────────┐   │
│  │                                                                      │   │
│  │   → —          /dev/ttyUSB1            USB-JTAG   ★                 │   │
│  │                                                                      │   │
│  │        ┌─ Assign Nickname ────────────────────────────────────┐      │   │
│  │        │                                                      │      │   │
│  │        │  USB Serial: D8:85:AC:A4:FB:7C                       │      │   │
│  │        │  Port: /dev/serial/by-id/usb-Espressif_USB_JTAG...   │      │   │
│  │        │                                                      │      │   │
│  │        │  Nickname:    [atoms3r                          ]    │      │   │
│  │        │  Name:        [M5 AtomS3R                       ]    │      │   │
│  │        │  Description: [bench device; display experiments]    │      │   │
│  │        │                                                      │      │   │
│  │        │              <Save>    <Cancel>                      │      │   │
│  │        └──────────────────────────────────────────────────────┘      │   │
│  │                                                                      │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│ Tab Next field   Enter Save   Esc Cancel                                     │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Port Picker: Edit Existing Nickname

When pressing `n` on a registered port, show pre-filled form:

```
┌─ esper ─────────────────────────────────────────────────────────────── v0.1 ─┐
│                                                                              │
│  ┌─ Select Serial Port ─────────────────────────────────────────────────┐   │
│  │                                                                      │   │
│  │   → atoms3r    M5 AtomS3R              USB-JTAG   ★                 │   │
│  │                                                                      │   │
│  │        ┌─ Edit Device: atoms3r ───────────────────────────────┐      │   │
│  │        │                                                      │      │   │
│  │        │  USB Serial: D8:85:AC:A4:FB:7C                       │      │   │
│  │        │  Port: /dev/serial/by-id/usb-Espressif_USB_JTAG...   │      │   │
│  │        │                                                      │      │   │
│  │        │  Nickname:    [atoms3r                          ]    │      │   │
│  │        │  Name:        [M5 AtomS3R                       ]    │      │   │
│  │        │  Description: [bench device; display experiments]    │      │   │
│  │        │                                                      │      │   │
│  │        │       <Save>    <Remove Device>    <Cancel>          │      │   │
│  │        └──────────────────────────────────────────────────────┘      │   │
│  │                                                                      │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│ Tab Next field   Enter Save   x Remove device   Esc Cancel                   │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Widget Structure (YAML Pseudocode)

```yaml
PortPickerView:
  type: container
  layout: vertical
  children:
    - TitleBar:
        type: statusline
        content: "esper"
        right: "v{version}"

    - ErrorBanner:
        type: box
        style: error
        visible: "{{.HasError}}"
        children:
          - Text: "{{.ErrorMessage}}"
          - Text: "{{.ErrorHint}}"

    - PortListPanel:
        type: box
        title: "Select Serial Port"
        children:
          - PortList:
              type: list
              items: "{{.AvailablePorts}}"
              selected: "{{.SelectedIndex}}"
              item_template:
                columns:
                  - path: width=55
                  - chip_type: width=12
                  - preferred_star: width=3

          - Divider: { type: line }

          - OptionsForm:
              type: form
              fields:
                - name: baud
                  type: dropdown
                  options: [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]
                  default: 115200
                - name: elf_path
                  type: textinput
                  placeholder: "/path/to/firmware.elf"
                - name: toolchain_prefix
                  type: textinput
                  placeholder: "xtensa-esp32s3-elf-"
                - name: probe_esptool
                  type: checkbox
                  label: "Probe with esptool (may reset device)"

          - ButtonRow:
              type: horizontal
              justify: right
              children:
                - Button: { label: "Connect", action: connect, style: primary }
                - Button: { label: "Rescan", action: rescan }
                - Button: { label: "Quit", action: quit }

    - HelpLine:
        type: statusline
        content: "↑↓ Navigate   Enter Connect   Tab Fields   r Rescan   ? Help   q Quit"

  bindings:
    global:
      "up", "k":     { action: list.prev }
      "down", "j":   { action: list.next }
      "enter":       { action: connect }
      "tab":         { action: focus.next }
      "shift+tab":   { action: focus.prev }
      "n":           { action: overlay.assign_nickname }
      "d":           { action: view.device_manager }
      "r":           { action: rescan }
      "?":           { action: overlay.help }
      "q", "ctrl+c": { action: quit }
```

### Assign Nickname Dialog (YAML Pseudocode)

```yaml
AssignNicknameDialog:
  type: modal
  title: "{{if .IsEdit}}Edit Device: {{.Nickname}}{{else}}Assign Nickname{{end}}"
  width: 60
  height: 14
  children:
    - InfoSection:
        type: container
        children:
          - Text: { label: "USB Serial:", value: "{{.UsbSerial}}", style: dim }
          - Text: { label: "Port:", value: "{{.PortPath}}", style: dim, truncate: 50 }

    - Divider: { type: line }

    - FormSection:
        type: form
        fields:
          - name: nickname
            type: textinput
            label: "Nickname:"
            value: "{{.Nickname}}"
            placeholder: "mydevice"
            validation: "^[a-z0-9_-]+$"
            required: true
          - name: name
            type: textinput
            label: "Name:"
            value: "{{.Name}}"
            placeholder: "M5 AtomS3R"
          - name: description
            type: textinput
            label: "Description:"
            value: "{{.Description}}"
            placeholder: "bench device; used for testing"

    - ButtonRow:
        type: horizontal
        justify: center
        children:
          - Button: { label: "Save", action: save, style: primary }
          - Button: { label: "Remove Device", action: remove, style: danger, visible: "{{.IsEdit}}" }
          - Button: { label: "Cancel", action: cancel }

  bindings:
    "tab":         { action: focus.next }
    "shift+tab":   { action: focus.prev }
    "enter":       { action: save }
    "x":           { action: remove, when: "{{.IsEdit}}", confirm: true }
    "escape":      { action: cancel }

  validation:
    nickname_unique:
      message: "Nickname already in use by another device"
      check: "{{not (registry.NicknameExists .Nickname .UsbSerial)}}"
```

---

## 1.2 Device Manager View

Full-screen view for managing the device registry. Accessed via `d` key from Port Picker or `esper devices` command.

### ASCII Wireframe (80×24)

```
┌─ esper ─ Device Manager ─────────────────────────────────────────────── v0.1 ─┐
│                                                                               │
│  ┌─ Registered Devices ──────────────────────────────────────────────────┐   │
│  │                                                                       │   │
│  │  NICKNAME     NAME                    USB SERIAL           STATUS     │   │
│  │  ─────────────────────────────────────────────────────────────────    │   │
│  │  → atoms3r    M5 AtomS3R              D8:85:AC:A4:FB:7C    ● online   │   │
│  │    xiao-c6    Seeed XIAO ESP32-C6     F4:12:FA:CC:21:5E    ● online   │   │
│  │    cardputer  M5Stack Cardputer       A1:B2:C3:D4:E5:F6    ○ offline  │   │
│  │    bench-s3   ESP32-S3 DevKit         98:CD:AC:12:34:56    ○ offline  │   │
│  │                                                                       │   │
│  │  ─────────────────────────────────────────────────────────────────    │   │
│  │                                                                       │   │
│  │  Selected: atoms3r                                                    │   │
│  │  Description: bench device; used for display experiments              │   │
│  │  Preferred path: /dev/serial/by-id/usb-Espressif_USB_JTAG_seria...    │   │
│  │                                                                       │   │
│  └───────────────────────────────────────────────────────────────────────┘   │
│                                                                               │
├───────────────────────────────────────────────────────────────────────────────┤
│ ↑↓ Navigate   e Edit   x Remove   c Connect   a Add   Esc Back   ? Help      │
└───────────────────────────────────────────────────────────────────────────────┘
```

### Device Manager: Empty State

```
┌─ esper ─ Device Manager ─────────────────────────────────────────────── v0.1 ─┐
│                                                                               │
│  ┌─ Registered Devices ──────────────────────────────────────────────────┐   │
│  │                                                                       │   │
│  │                                                                       │   │
│  │                                                                       │   │
│  │                    No devices registered yet.                         │   │
│  │                                                                       │   │
│  │           Press 'a' to add a device, or go back to the               │   │
│  │           port picker and press 'n' on a connected device.           │   │
│  │                                                                       │   │
│  │                                                                       │   │
│  │                                                                       │   │
│  └───────────────────────────────────────────────────────────────────────┘   │
│                                                                               │
├───────────────────────────────────────────────────────────────────────────────┤
│ a Add device   Esc Back to Port Picker   ? Help                               │
└───────────────────────────────────────────────────────────────────────────────┘
```

### Device Manager: Add Device (Manual Entry)

For adding a device that is not currently connected:

```
┌─ esper ─ Device Manager ─────────────────────────────────────────────── v0.1 ─┐
│                                                                               │
│  ┌─ Registered Devices ──────────────────────────────────────────────────┐   │
│  │                                                                       │   │
│  │  → atoms3r    M5 AtomS3R              D8:85:AC:A4:FB:7C    ● online   │   │
│  │                                                                       │   │
│  │        ┌─ Add Device ─────────────────────────────────────────┐       │   │
│  │        │                                                      │       │   │
│  │        │  USB Serial: [                                  ]    │       │   │
│  │        │              (from 'esper scan' output)              │       │   │
│  │        │                                                      │       │   │
│  │        │  Nickname:    [                                 ]    │       │   │
│  │        │  Name:        [                                 ]    │       │   │
│  │        │  Description: [                                 ]    │       │   │
│  │        │                                                      │       │   │
│  │        │                    <Save>    <Cancel>                │       │   │
│  │        └──────────────────────────────────────────────────────┘       │   │
│  │                                                                       │   │
│  └───────────────────────────────────────────────────────────────────────┘   │
│                                                                               │
├───────────────────────────────────────────────────────────────────────────────┤
│ Tab Next field   Enter Save   Esc Cancel                                      │
└───────────────────────────────────────────────────────────────────────────────┘
```

### Widget Structure (YAML Pseudocode)

```yaml
DeviceManagerView:
  type: container
  layout: vertical
  children:
    - TitleBar:
        type: statusline
        content: "esper — Device Manager"
        right: "v{version}"

    - DeviceListPanel:
        type: box
        title: "Registered Devices"
        children:
          - DeviceTable:
              type: table
              columns:
                - name: nickname
                  label: "NICKNAME"
                  width: 12
                - name: name
                  label: "NAME"
                  width: 24
                - name: usb_serial
                  label: "USB SERIAL"
                  width: 20
                - name: status
                  label: "STATUS"
                  width: 10
              items: "{{.Devices}}"
              selected: "{{.SelectedIndex}}"
              empty_message: "No devices registered yet.\n\nPress 'a' to add a device..."

          - Divider: { type: line, visible: "{{.HasSelection}}" }

          - DetailSection:
              type: container
              visible: "{{.HasSelection}}"
              children:
                - Text: { content: "Selected: {{.Selected.Nickname}}", style: bold }
                - Text: { content: "Description: {{.Selected.Description}}", style: dim }
                - Text: { content: "Preferred path: {{.Selected.PreferredPath}}", style: dim, truncate: 60 }

    - HelpLine:
        type: statusline
        content: "↑↓ Navigate   e Edit   x Remove   c Connect   a Add   Esc Back   ? Help"

  bindings:
    navigation:
      "up", "k":     { action: list.prev }
      "down", "j":   { action: list.next }
      "home":        { action: list.first }
      "end":         { action: list.last }

    actions:
      "e", "enter":  { action: overlay.edit_device }
      "x":           { action: remove_device, confirm: true }
      "c":           { action: connect_to_device, when: "{{.Selected.IsOnline}}" }
      "a":           { action: overlay.add_device }

    navigation_out:
      "escape":      { action: view.port_picker }
      "?":           { action: overlay.help }
      "q":           { action: quit }

  states:
    list:
      description: "Browsing device list"
    add_dialog:
      description: "Adding new device"
    edit_dialog:
      description: "Editing existing device"

AddDeviceDialog:
  type: modal
  title: "Add Device"
  width: 60
  height: 12
  children:
    - FormSection:
        type: form
        fields:
          - name: usb_serial
            type: textinput
            label: "USB Serial:"
            placeholder: "D8:85:AC:A4:FB:7C"
            hint: "(from 'esper scan' output)"
            validation: "^[A-Fa-f0-9:]+$"
            required: true
          - name: nickname
            type: textinput
            label: "Nickname:"
            placeholder: "mydevice"
            validation: "^[a-z0-9_-]+$"
            required: true
          - name: name
            type: textinput
            label: "Name:"
            placeholder: "M5 AtomS3R"
          - name: description
            type: textinput
            label: "Description:"
            placeholder: "bench device"

    - ButtonRow:
        type: horizontal
        justify: center
        children:
          - Button: { label: "Save", action: save, style: primary }
          - Button: { label: "Cancel", action: cancel }

  bindings:
    "tab":         { action: focus.next }
    "shift+tab":   { action: focus.prev }
    "enter":       { action: save }
    "escape":      { action: cancel }
```

---

## 1.3 Monitor View (Main)

The primary screen during active serial connection.

### ASCII Wireframe: Normal Mode (120×40)

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

### ASCII Wireframe: Scrollback Mode (Follow OFF)

```
┌─ esper ── Connected: /dev/serial/by-id/usb-Espressif_USB_JTAG-if00 ── 115200 ── ELF: ✓ ──┐
│                                                                                          │
│ I (1523) wifi: wifi driver task: 3ffc1e4c, prio:23, stack:6656, core=0                   │
│ I (1527) system_api: Base MAC address is not set                                         │
│ I (1533) system_api: read default base MAC address from EFUSE                            │
│ I (1540) wifi: wifi firmware version: 3.0                                                │
│ I (1545) wifi: config NVS flash: enabled                                                 │
│ W (1550) wifi: Warning: NVS partition low on space                                       │
│ I (1556) wifi_init: rx ba win: 6                                                         │ █
│ I (1560) wifi_init: tcpip mbox: 32                                                       │ █
│ I (1565) wifi_init: udp mbox: 6                                                          │ ░
│ I (1570) wifi_init: tcp mbox: 6                                                          │ ░
│ I (1575) wifi_init: tcp tx win: 5744                                                     │ ░
│ I (1580) wifi_init: tcp rx win: 5744                                                     │ ░
│ I (1585) wifi_init: tcp mss: 1436                                                        │ ░
│                                                                           ▲ 847 more ▼   │
│                                                                                          │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ Mode: HOST  │ Follow: OFF [G=resume] │ Capture: — │ Filter: — │ Search: — │ Buf: 512K/1M │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ Ctrl-T: HOST COMMANDS   Type to search scrollback, Enter to send to device              │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

### ASCII Wireframe: 80×24 Compact Mode

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

### Widget Structure (YAML Pseudocode)

```yaml
MonitorView:
  type: container
  layout: vertical
  children:
    - TitleBar:
        type: statusline
        segments:
          - "esper"
          - "Connected: {{.Port}}"
          - "{{.Baud}}"
          - "ELF: {{if .HasELF}}✓{{else}}—{{end}}"

    - LogViewport:
        type: viewport
        content: "{{.LogBuffer}}"
        scrollable: true
        ansi: true  # preserve ANSI colors
        wrap: "{{.WrapEnabled}}"
        follow: "{{.FollowMode}}"
        properties:
          scroll_indicator: true
          lines_ahead_hint: true
          max_buffer_size: "{{.MaxBufferBytes}}"

    - StatusBar:
        type: statusline
        segments:
          - label: "Mode"
            value: "{{if .HostMode}}HOST{{else}}DEVICE{{end}}"
            style: "{{if .HostMode}}highlight{{else}}normal{{end}}"
          - label: "Follow"
            value: "{{if .FollowMode}}ON{{else}}OFF [G=resume]{{end}}"
          - label: "Capture"
            value: "{{.CaptureStatus}}"
          - label: "Filter"
            value: "{{.FilterStatus}}"
          - label: "Search"
            value: "{{.SearchStatus}}"
          - label: "Buf"
            value: "{{.BufferUsed}}/{{.BufferMax}}"
          - value: "{{.Timestamp}}"
            align: right

    - InputLine:
        type: textinput
        prompt: "> "
        placeholder: ""
        submit_action: send_to_device
        mode_indicator: "{{.InputMode}}"

  bindings:
    # Navigation (always available)
    "up", "k":       { action: scroll.up, when: host_mode }
    "down", "j":     { action: scroll.down, when: host_mode }
    "pageup":        { action: scroll.pageup }
    "pagedown":      { action: scroll.pagedown }
    "home", "g g":   { action: scroll.top }
    "end", "G":      { action: scroll.bottom_and_follow }
    "left", "h":     { action: scroll.left, when: "!wrap" }
    "right", "l":    { action: scroll.right, when: "!wrap" }

    # Host command mode toggle
    "ctrl+t":        { action: toggle.host_mode }
    "escape":        { action: exit.host_mode, to: device_mode }

    # Global shortcuts (host mode only)
    "/":             { action: overlay.search, when: host_mode }
    "f":             { action: overlay.filter, when: host_mode }
    "i":             { action: overlay.inspector, when: host_mode }
    "?":             { action: overlay.help }
    "ctrl+l":        { action: clear.viewport, confirm: true }
    "ctrl+s":        { action: toggle.session_log }
    "w":             { action: toggle.wrap, when: host_mode }

    # Device actions (host mode only)
    "ctrl+r":        { action: device.reset, confirm: true }
    "ctrl+]":        { action: device.send_break }

    # Disconnect / quit
    "ctrl+d":        { action: disconnect }
    "ctrl+c":        { action: quit, confirm: true }

  states:
    device_mode:
      description: "Keystrokes go to device console (except ctrl+t)"
      input_focus: InputLine
      passthrough: true  # all printable chars → device

    host_mode:
      description: "Keystrokes interpreted as TUI commands"
      input_focus: none
      passthrough: false
```

---

## 1.3 Inspector Panel

Overlay or side panel showing special events (panic/backtrace, core dump, GDB stub).

### ASCII Wireframe: Inspector as Right Panel (120×40)

```
┌─ esper ── Connected ── 115200 ────────────────────────────────┬─ Inspector ─────────────┐
│                                                               │                         │
│ I (1523) wifi: wifi driver task: 3ffc1e4c                     │ Events (3)              │
│ I (1527) system_api: Base MAC address is not set              │                         │
│ I (1533) system_api: read default MAC from EFUSE              │ → PANIC 12:34:52        │
│ ...                                                           │     Backtrace decoded   │
│ ...                                                           │     Press Enter to view │
│ Guru Meditation Error: Core  0 panic'ed (LoadProhibited)      │                         │
│ Core  0 register dump:                                        │   COREDUMP 12:30:15     │
│ PC      : 0x40082a3c  PS      : 0x00060c30                    │     Captured 64KB       │
│ A0      : 0x800d1e7f  A1      : 0x3ffb5c70                    │                         │
│ Backtrace: 0x40082a39:0x3ffb5c70 0x400d1e7c:0x3ffb5c90        │   GDB_STUB 12:28:00     │
│                                                               │     Stop reason: SIGTRAP│
│ ELF file SHA256:  a1b2c3d4e5f6                                │                         │
│                                                               │                         │
├───────────────────────────────────────────────────────────────┴─────────────────────────┤
│ Mode: HOST │ Follow: OFF │ Inspector: ON │                             │ Ctrl-T:menu    │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│ > [                                                                                   ] │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

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

### ASCII Wireframe: Core Dump Capture In Progress

```
┌─ esper ── Connected ── 115200 ─────────────────────────────────────────────── ELF: ✓ ───┐
│                                                                                          │
│                                                                                          │
│                                                                                          │
│              ┌─ CORE DUMP CAPTURE IN PROGRESS ─────────────────────────┐                │
│              │                                                         │                │
│              │   Receiving core dump data...                           │                │
│              │                                                         │                │
│              │   ████████████████████░░░░░░░░░░  67%                   │                │
│              │   43,520 / 65,536 bytes                                 │                │
│              │                                                         │                │
│              │   Normal output is MUTED during capture.                │                │
│              │   Core dump will auto-decode when complete.             │                │
│              │                                                         │                │
│              └─────────────────────────────────────────────────────────┘                │
│                                                                                          │
│                                                                                          │
│                                                                                          │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ Mode: CAPTURE │ Follow: — │ Capture: 67% │ Press Ctrl-C to abort                         │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│ (input disabled during capture)                                                          │
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

### Widget Structure (YAML Pseudocode)

```yaml
InspectorPanel:
  type: container
  layout: vertical
  position: overlay | sidepanel
  width: "30%" | "100%"  # sidepanel vs overlay
  children:
    - TitleBar:
        type: statusline
        content: "Inspector"
        right: "{{.EventType}} — {{.Timestamp}}"

    - EventList:
        type: list
        visible: "{{.ShowEventList}}"
        items: "{{.Events}}"
        item_template:
          type: event_summary
          fields:
            - icon: "{{.EventIcon}}"
            - type: "{{.EventType}}"
            - timestamp: "{{.Timestamp}}"
            - summary: "{{.OneLiner}}"

    - EventDetail:
        type: container
        visible: "{{.ShowEventDetail}}"
        children:
          - PanicDetail:
              visible: "{{eq .EventType 'PANIC'}}"
              children:
                - RawBacktrace:
                    type: box
                    title: "Raw Backtrace"
                    content: "{{.RawBacktrace}}"
                    copyable: true
                - DecodedFrames:
                    type: box
                    title: "Decoded Frames"
                    content: "{{.DecodedFrames}}"
                    copyable: true
                - ContextInfo:
                    type: box
                    title: "Context"
                    content: "{{.RegisterDump}}"

          - CoredumpDetail:
              visible: "{{eq .EventType 'COREDUMP'}}"
              children:
                - StatusLine:
                    type: text
                    content: "Status: {{.Status}}"
                - SizeLine:
                    type: text
                    content: "Size: {{.Size}} bytes"
                - SavedPath:
                    type: text
                    content: "Saved to: {{.SavePath}}"
                - Report:
                    type: box
                    title: "Decoded Report (truncated)"
                    content: "{{.DecodedReport}}"
                    scrollable: true
                    copyable: true

          - GdbStubDetail:
              visible: "{{eq .EventType 'GDB_STUB'}}"
              children:
                - StopReason:
                    type: text
                    content: "Stop reason: {{.StopReason}}"
                - GdbCommand:
                    type: box
                    title: "Connect with GDB"
                    content: "{{.GdbConnectCommand}}"
                    copyable: true

    - HelpLine:
        type: statusline
        content: "{{.ContextualHelp}}"

  bindings:
    "up", "k":       { action: list.prev }
    "down", "j":     { action: list.next }
    "enter":         { action: event.detail }
    "c":             { action: copy.raw }
    "C":             { action: copy.decoded }
    "s":             { action: save.report }
    "r":             { action: copy.raw_base64 }
    "tab":           { action: event.next }
    "shift+tab":     { action: event.prev }
    "escape", "q":   { action: close.inspector }

  states:
    event_list:
      description: "Browsing event list"
    event_detail:
      description: "Viewing single event detail"
```

---

# Part 2: Overlays

## 2.1 Help Overlay

### ASCII Wireframe

```
┌─ Help ─ Esper TUI Keyboard Reference ────────────────────────────────────────────────────┐
│                                                                                          │
│  ┌─ Mode Control ───────────────────┐  ┌─ Navigation ───────────────────────────────┐   │
│  │ Ctrl-T    Toggle HOST/DEVICE mode│  │ ↑/k ↓/j   Scroll up/down (host mode)      │   │
│  │ Escape    Return to DEVICE mode  │  │ PgUp/PgDn Page up/down                     │   │
│  │                                  │  │ Home/gg   Scroll to top                    │   │
│  │ In DEVICE mode:                  │  │ End/G     Scroll to bottom + follow        │   │
│  │   All keys → device console      │  │ ←/h →/l   Horizontal scroll (no wrap)     │   │
│  │   Ctrl-T → HOST mode             │  └─────────────────────────────────────────────┘   │
│  └──────────────────────────────────┘                                                    │
│                                                                                          │
│  ┌─ Search & Filter ────────────────┐  ┌─ Inspector ────────────────────────────────┐   │
│  │ /         Open search            │  │ i         Toggle Inspector panel          │   │
│  │ n/N       Next/prev match        │  │ Enter     View event detail               │   │
│  │ f         Open filter panel      │  │ c/C       Copy raw/decoded                │   │
│  │ Escape    Clear search/filter    │  │ s         Save report                     │   │
│  └──────────────────────────────────┘  └─────────────────────────────────────────────┘   │
│                                                                                          │
│  ┌─ Device Control ─────────────────┐  ┌─ View Options ─────────────────────────────┐   │
│  │ Ctrl-R    Reset device (confirm) │  │ w         Toggle line wrap                │   │
│  │ Ctrl-]    Send break             │  │ Ctrl-L    Clear viewport (confirm)        │   │
│  │ Ctrl-D    Disconnect             │  │ Ctrl-S    Toggle session logging          │   │
│  └──────────────────────────────────┘  └─────────────────────────────────────────────┘   │
│                                                                                          │
├──────────────────────────────────────────────────────────────────────────────────────────┤
│                                    Press Escape or ? to close                            │
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

### Widget Structure (YAML Pseudocode)

```yaml
HelpOverlay:
  type: modal
  title: "Help — Esper TUI Keyboard Reference"
  width: 90
  height: 24
  children:
    - Grid:
        type: grid
        columns: 2
        gap: 2
        children:
          - Box:
              title: "Mode Control"
              rows:
                - ["Ctrl-T", "Toggle HOST/DEVICE mode"]
                - ["Escape", "Return to DEVICE mode"]
                - ["", ""]
                - ["In DEVICE mode:", ""]
                - ["  All keys", "→ device console"]
                - ["  Ctrl-T", "→ HOST mode"]

          - Box:
              title: "Navigation"
              rows:
                - ["↑/k ↓/j", "Scroll up/down (host mode)"]
                - ["PgUp/PgDn", "Page up/down"]
                - ["Home/gg", "Scroll to top"]
                - ["End/G", "Scroll to bottom + follow"]
                - ["←/h →/l", "Horizontal scroll (no wrap)"]

          - Box:
              title: "Search & Filter"
              rows:
                - ["/", "Open search"]
                - ["n/N", "Next/prev match"]
                - ["f", "Open filter panel"]
                - ["Escape", "Clear search/filter"]

          - Box:
              title: "Inspector"
              rows:
                - ["i", "Toggle Inspector panel"]
                - ["Enter", "View event detail"]
                - ["c/C", "Copy raw/decoded"]
                - ["s", "Save report"]

          - Box:
              title: "Device Control"
              rows:
                - ["Ctrl-R", "Reset device (confirm)"]
                - ["Ctrl-]", "Send break"]
                - ["Ctrl-D", "Disconnect"]

          - Box:
              title: "View Options"
              rows:
                - ["w", "Toggle line wrap"]
                - ["Ctrl-L", "Clear viewport (confirm)"]
                - ["Ctrl-S", "Toggle session logging"]

    - HelpLine:
        type: statusline
        content: "Press Escape or ? to close"

  bindings:
    "escape", "?", "q": { action: close }
```

---

## 2.2 Search Overlay

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

### Widget Structure (YAML Pseudocode)

```yaml
SearchOverlay:
  type: overlay
  position: bottom
  height: 2
  children:
    - SearchInput:
        type: textinput
        prompt: "Search: "
        live_filter: true  # updates matches as you type
        submit_action: jump_to_match

    - StatusSegment:
        type: text
        content: "{{.MatchCount}} matches"

    - HintSegment:
        type: text
        content: "n:next N:prev"

  bindings:
    "enter":         { action: jump_to_current_match }
    "n", "ctrl+n":   { action: next_match }
    "N", "ctrl+p":   { action: prev_match }
    "escape":        { action: close_search }
    "ctrl+c":        { action: close_search }

  effects:
    on_input_change:
      - action: highlight_matches_in_viewport
      - action: update_match_count
```

---

## 2.3 Filter Overlay

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

### Widget Structure (YAML Pseudocode)

```yaml
FilterOverlay:
  type: modal
  title: "Filter Log Output"
  width: 80
  height: 20
  children:
    - LevelSection:
        type: container
        children:
          - Label: { text: "Log Levels:" }
          - CheckboxRow:
              type: horizontal
              children:
                - Checkbox: { id: level_e, label: "E  Error", default: true }
                - Checkbox: { id: level_w, label: "W  Warning", default: true }
                - Checkbox: { id: level_i, label: "I  Info", default: true }
          - CheckboxRow:
              type: horizontal
              children:
                - Checkbox: { id: level_d, label: "D  Debug", default: false }
                - Checkbox: { id: level_v, label: "V  Verbose", default: false }

    - Divider: { type: line }

    - RegexSection:
        type: container
        children:
          - TextInput:
              id: include_regex
              label: "Include (regex):"
              placeholder: "wifi|http"
          - TextInput:
              id: exclude_regex
              label: "Exclude (regex):"
              placeholder: "heartbeat"

    - Divider: { type: line }

    - HighlightSection:
        type: container
        children:
          - Label: { text: "Highlight Rules:" }
          - HighlightRuleList:
              type: list
              items: "{{.HighlightRules}}"
              item_template:
                type: horizontal
                children:
                  - TextInput: { value: "{{.Pattern}}" }
                  - Dropdown: { options: ["red background", "yellow underline", "green", "cyan", "bold"], value: "{{.Style}}" }
          - Button: { label: "Add Rule", action: add_highlight_rule }

    - Divider: { type: line }

    - ButtonRow:
        type: horizontal
        justify: right
        children:
          - Button: { label: "Apply", action: apply_filter, style: primary }
          - Button: { label: "Clear All", action: clear_filter }
          - Button: { label: "Cancel", action: cancel }

  bindings:
    "tab":           { action: focus.next }
    "shift+tab":     { action: focus.prev }
    "enter":         { action: apply_filter }
    "escape":        { action: cancel }
```

---

## 2.4 Command Palette

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

### Widget Structure (YAML Pseudocode)

```yaml
CommandPalette:
  type: modal
  title: "Commands"
  width: 70
  height: 20
  children:
    - SearchInput:
        type: textinput
        prompt: "> "
        placeholder: "Type to filter commands..."
        live_filter: true

    - CommandList:
        type: list
        items: "{{.FilteredCommands}}"
        item_template:
          type: horizontal
          children:
            - Text: { value: "{{.Label}}", width: 50 }
            - Text: { value: "{{.Shortcut}}", align: right, style: dim }
        divider_after:
          - "Toggle Inspector"
          - "Disconnect"
          - "Toggle session logging"

  commands:
    - { label: "Search log output", action: overlay.search, shortcut: "/" }
    - { label: "Filter by level/regex", action: overlay.filter, shortcut: "f" }
    - { label: "Toggle Inspector", action: overlay.inspector, shortcut: "i" }
    - { label: "Toggle line wrap", action: toggle.wrap, shortcut: "w" }
    - { divider: true }
    - { label: "Reset device", action: device.reset, shortcut: "Ctrl-R", confirm: true }
    - { label: "Send break", action: device.send_break, shortcut: "Ctrl-]" }
    - { label: "Disconnect", action: disconnect, shortcut: "Ctrl-D" }
    - { divider: true }
    - { label: "Clear viewport", action: clear.viewport, shortcut: "Ctrl-L", confirm: true }
    - { label: "Toggle session logging", action: toggle.session_log, shortcut: "Ctrl-S" }
    - { label: "Reconnect", action: reconnect }
    - { divider: true }
    - { label: "Help", action: overlay.help, shortcut: "?" }
    - { label: "Quit", action: quit, shortcut: "Ctrl-C" }

  bindings:
    "up", "ctrl+p":   { action: list.prev }
    "down", "ctrl+n": { action: list.next }
    "enter":          { action: execute_selected }
    "escape":         { action: close }
```

---

## 2.5 Confirmation Dialogs

### ASCII Wireframe: Reset Device

```
┌─ esper ── Connected ── 115200 ─────────────────────────────────────────────── ELF: ✓ ───┐
│ I (1523) wifi: wifi driver task: 3ffc1e4c, prio:23, stack:6656, core=0                   │
│ I (1527) system_api: Base MAC address is not set                                         │
│                                                                                          │
│                    ┌─ Confirm: Reset Device ──────────────────────────┐                  │
│                    │                                                  │                  │
│                    │  This will toggle RTS/DTR to reset the device.   │                  │
│                    │  Any running program will restart.               │                  │
│                    │                                                  │                  │
│                    │                   <Reset>    <Cancel>            │                  │
│                    └──────────────────────────────────────────────────┘                  │
│                                                                                          │
│ I (1533) system_api: read default base MAC address from EFUSE                            │
│ I (1540) wifi: wifi firmware version: 3.0                                                │
```

### Widget Structure (YAML Pseudocode)

```yaml
ConfirmationDialog:
  type: modal
  title: "Confirm: {{.Title}}"
  width: 50
  height: 8
  style: warning
  children:
    - Message:
        type: text
        content: "{{.Message}}"
        wrap: true

    - ButtonRow:
        type: horizontal
        justify: center
        children:
          - Button: { label: "{{.ConfirmLabel}}", action: confirm, style: danger }
          - Button: { label: "Cancel", action: cancel }

  bindings:
    "enter":   { action: confirm }
    "escape":  { action: cancel }
    "y":       { action: confirm }
    "n":       { action: cancel }

  variants:
    reset_device:
      title: "Reset Device"
      message: "This will toggle RTS/DTR to reset the device.\nAny running program will restart."
      confirm_label: "Reset"

    clear_viewport:
      title: "Clear Viewport"
      message: "This will clear all visible logs.\nScrollback history will be lost."
      confirm_label: "Clear"

    quit_with_capture:
      title: "Quit During Capture"
      message: "A core dump capture is in progress.\nQuitting now will discard the capture."
      confirm_label: "Quit Anyway"
```

---

# Part 3: State Machine

## 3.1 Application State Diagram

```
                                    ┌─────────────────────┐
                                    │      STARTUP        │
                                    │  (parse args, init) │
                                    └──────────┬──────────┘
                                               │
                          ┌────────────────────┼────────────────────┐
                          │                    │                    │
                          ▼                    ▼                    ▼
                  ┌───────────────┐   ┌────────────────┐   ┌────────────────┐
                  │  PORT_PICKER  │◄─►│ DEVICE_MANAGER │   │    MONITOR     │
                  │               │ d │                │   │   (connected)  │
                  └───────┬───────┘   └───────┬────────┘   └───────┬────────┘
                          │                   │                    │
                          │ connect success   │ connect/esc        │ disconnect
                          └───────────────────┼────────────────────┘
                                              │
                                              ▼
                                      ┌────────────────┐
                                      │    MONITOR     │
                                      │   (connected)  │
                                      └────────────────┘
```

**Device Manager Integration:**
- `d` key from Port Picker → Device Manager
- `Esc` from Device Manager → Port Picker
- `c` (connect) from Device Manager on online device → Monitor
- Device Manager can also be reached via `esper devices` subcommand

## 3.2 Monitor View State Machine

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                                    MONITOR VIEW                                          │
│                                                                                         │
│   ┌─────────────┐      Ctrl-T      ┌─────────────┐                                      │
│   │ DEVICE_MODE │◄────────────────►│  HOST_MODE  │                                      │
│   │ (typing→dev)│      Ctrl-T      │ (vim-style) │                                      │
│   └─────────────┘       Esc        └──────┬──────┘                                      │
│                                           │                                              │
│                                           │ overlay keys                                 │
│                          ┌────────────────┼────────────────┐                            │
│                          ▼                ▼                ▼                            │
│                    ┌──────────┐    ┌───────────┐    ┌────────────┐                      │
│                    │  SEARCH  │    │  FILTER   │    │ CMD_PALETTE│                      │
│                    │ (/ key)  │    │  (f key)  │    │ (Ctrl-T T) │                      │
│                    └────┬─────┘    └─────┬─────┘    └──────┬─────┘                      │
│                         │ Esc            │ Esc             │ Esc                        │
│                         └────────────────┴─────────────────┘                            │
│                                           │                                              │
│                                           ▼                                              │
│                                    ┌─────────────┐                                       │
│                                    │  HOST_MODE  │                                       │
│                                    └─────────────┘                                       │
│                                                                                         │
│   Special States (can occur in any mode):                                               │
│                                                                                         │
│   ┌──────────────────┐      capture complete      ┌────────────────┐                    │
│   │ CAPTURE_COREDUMP │───────────────────────────►│ INSPECTOR_OPEN │                    │
│   │  (output muted)  │                            │ (shows report) │                    │
│   └──────────────────┘                            └────────────────┘                    │
│           ▲                                                                             │
│           │ coredump detected                                                           │
│           │                                                                             │
│   ┌───────┴──────┐                                                                      │
│   │  ANY STATE   │                                                                      │
│   └──────────────┘                                                                      │
│                                                                                         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## 3.3 State Definitions (YAML)

```yaml
states:
  startup:
    description: "Application initialization"
    transitions:
      - to: port_picker
        when: "!args.port || args.port_open_failed"
      - to: device_manager
        when: "args.subcommand == 'devices'"
      - to: monitor.device_mode
        when: "args.port && port_open_success"

  port_picker:
    description: "Select serial port and configure connection"
    overlay_allowed: [help, assign_nickname]
    transitions:
      - to: monitor.device_mode
        when: connect_success
      - to: device_manager
        when: "key_d"
      - to: quit
        when: user_quit

  device_manager:
    description: "Manage device registry (nicknames)"
    overlay_allowed: [help, add_device, edit_device, confirmation]
    transitions:
      - to: port_picker
        when: escape
      - to: monitor.device_mode
        when: connect_to_online_device
      - to: quit
        when: user_quit

  monitor:
    description: "Main serial monitor view"
    substates:
      device_mode:
        description: "Keystrokes sent to device console"
        input_passthrough: true
        only_intercept: [ctrl+t, ctrl+c, ctrl+d]
        transitions:
          - to: monitor.host_mode
            when: ctrl+t

      host_mode:
        description: "Keystrokes interpreted as TUI commands"
        input_passthrough: false
        transitions:
          - to: monitor.device_mode
            when: escape
          - to: monitor.device_mode
            when: ctrl+t

      capture_coredump:
        description: "Core dump capture in progress"
        mute_output: true
        disable_input: true
        show_progress: true
        transitions:
          - to: monitor.host_mode
            when: capture_complete
            then: open_inspector
          - to: monitor.host_mode
            when: capture_abort
            then: show_error

    overlay_allowed: [help, search, filter, command_palette, inspector, confirmation]
    transitions:
      - to: port_picker
        when: disconnect
      - to: port_picker
        when: serial_error
      - to: quit
        when: user_quit

  quit:
    description: "Application exit"
    pre_action: confirm_if_capture_in_progress

overlays:
  help:
    modal: true
    close_on: [escape, "?", q]

  search:
    modal: false  # inline at bottom
    close_on: [escape]
    persist_results: true

  filter:
    modal: true
    close_on: [escape]
    apply_on: [enter]

  command_palette:
    modal: true
    close_on: [escape]
    execute_on: [enter]

  inspector:
    modal: false  # side panel
    close_on: [escape, q, i]
    toggle_key: i

  confirmation:
    modal: true
    blocking: true
    close_on: [escape, n]
    confirm_on: [enter, y]

  assign_nickname:
    modal: true
    close_on: [escape]
    save_on: [enter]
    context: "port_picker"
    description: "Assign or edit nickname for selected port"

  add_device:
    modal: true
    close_on: [escape]
    save_on: [enter]
    context: "device_manager"
    description: "Add new device to registry (manual entry)"

  edit_device:
    modal: true
    close_on: [escape]
    save_on: [enter]
    context: "device_manager"
    description: "Edit existing device in registry"
```

---

# Part 4: Keyboard Mapping

## 4.1 Global Keys (Always Active)

```yaml
global_bindings:
  "ctrl+c":
    action: quit
    confirm: true
    description: "Quit esper (with confirmation if capture in progress)"

  "?":
    action: overlay.help
    description: "Show help overlay"
    when: "mode != 'device_mode' || overlay_active"
```

## 4.2 Port Picker Keys

```yaml
port_picker_bindings:
  navigation:
    "up", "k":     { action: list.prev }
    "down", "j":   { action: list.next }
    "home":        { action: list.first }
    "end":         { action: list.last }

  form:
    "tab":         { action: focus.next }
    "shift+tab":   { action: focus.prev }

  device_registry:
    "n":           { action: overlay.assign_nickname, description: "Assign/edit nickname for selected port" }
    "d":           { action: view.device_manager, description: "Open Device Manager" }

  actions:
    "enter":       { action: connect }
    "r":           { action: rescan }
    "q":           { action: quit }
```

## 4.2.1 Device Manager Keys

```yaml
device_manager_bindings:
  navigation:
    "up", "k":     { action: list.prev }
    "down", "j":   { action: list.next }
    "home":        { action: list.first }
    "end":         { action: list.last }

  device_actions:
    "e", "enter":  { action: overlay.edit_device, description: "Edit selected device" }
    "x":           { action: remove_device, confirm: true, description: "Remove selected device" }
    "c":           { action: connect, when: "selected.is_online", description: "Connect to device" }
    "a":           { action: overlay.add_device, description: "Add new device manually" }

  navigation_out:
    "escape":      { action: view.port_picker }
    "?":           { action: overlay.help }
    "q":           { action: quit }
```

## 4.2.2 Assign Nickname Dialog Keys

```yaml
assign_nickname_bindings:
  form:
    "tab":         { action: focus.next }
    "shift+tab":   { action: focus.prev }

  actions:
    "enter":       { action: save }
    "x":           { action: remove, when: is_edit, confirm: true, description: "Remove device from registry" }
    "escape":      { action: cancel }
```

## 4.3 Monitor Keys: Device Mode

```yaml
device_mode_bindings:
  # In device mode, almost ALL keys go to device
  passthrough: true

  # Only these keys are intercepted by the TUI:
  intercept:
    "ctrl+t":      { action: toggle.host_mode }
    "ctrl+c":      { action: quit, confirm: true }
    "ctrl+d":      { action: disconnect }
    "pageup":      { action: scroll.pageup }
    "pagedown":    { action: scroll.pagedown }
```

## 4.4 Monitor Keys: Host Mode

```yaml
host_mode_bindings:
  mode_control:
    "ctrl+t":      { action: toggle.device_mode }
    "escape":      { action: exit_to.device_mode }

  navigation:
    "up", "k":     { action: scroll.up }
    "down", "j":   { action: scroll.down }
    "pageup":      { action: scroll.pageup }
    "pagedown":    { action: scroll.pagedown }
    "home", "gg":  { action: scroll.top }
    "end", "G":    { action: scroll.bottom_and_follow }
    "left", "h":   { action: scroll.left, when: "!wrap" }
    "right", "l":  { action: scroll.right, when: "!wrap" }

  overlays:
    "/":           { action: overlay.search }
    "f":           { action: overlay.filter }
    "i":           { action: overlay.inspector }
    "ctrl+t t":    { action: overlay.command_palette }

  view_options:
    "w":           { action: toggle.wrap }
    "ctrl+l":      { action: clear.viewport, confirm: true }
    "ctrl+s":      { action: toggle.session_log }

  device_control:
    "ctrl+r":      { action: device.reset, confirm: true }
    "ctrl+]":      { action: device.send_break }

  session:
    "ctrl+d":      { action: disconnect }
```

## 4.5 Search Overlay Keys

```yaml
search_bindings:
  "any_char":    { action: append_to_query }
  "backspace":   { action: delete_char }
  "enter":       { action: jump_to_match }
  "n", "ctrl+n": { action: next_match }
  "N", "ctrl+p": { action: prev_match }
  "escape":      { action: close_search }
```

## 4.6 Inspector Keys

```yaml
inspector_bindings:
  navigation:
    "up", "k":       { action: list.prev }
    "down", "j":     { action: list.next }
    "enter":         { action: event.detail }
    "backspace":     { action: event.list }
    "tab":           { action: event.next }
    "shift+tab":     { action: event.prev }

  actions:
    "c":             { action: copy.raw }
    "C":             { action: copy.decoded }
    "s":             { action: save.report }
    "r":             { action: copy.raw_base64, when: "event == 'coredump'" }
    "j":             { action: jump_to_log }

  close:
    "escape", "q", "i": { action: close.inspector }
```

---

# Part 5: Component Behavior Specifications

## 5.1 Log Viewport

```yaml
LogViewport:
  description: "Scrollable ANSI-colored log output area"

  properties:
    buffer_size:
      default: "1MB"
      configurable: true
      behavior: "oldest lines evicted when limit reached"

    follow_mode:
      default: true
      auto_disable: "when user scrolls up"
      indicator: "FOLLOW: ON|OFF in status bar"
      resume: "press G or End"

    wrap:
      default: false
      toggle: "w key in host mode"
      horizontal_scroll: "←/→ when wrap off"

    ansi_handling:
      preserve_colors: true
      reset_on_newline: "if line contains incomplete SGR sequence"
      partial_line_flush: "after 100ms idle, flush partial line with dim styling"

  scroll_behavior:
    up_arrow: "1 line"
    down_arrow: "1 line"
    page_up: "viewport_height - 2 lines"
    page_down: "viewport_height - 2 lines"
    home: "scroll to first line"
    end: "scroll to last line + enable follow"

  line_rendering:
    max_line_length: 4096
    truncation: "... (line truncated)" suffix
    timestamp_prefix:
      optional: true
      format: "HH:MM:SS.mmm"
      style: dim
```

## 5.2 Input Line

```yaml
InputLine:
  description: "Text input for sending data to device console"

  properties:
    prompt: "> "
    history:
      enabled: true
      max_entries: 100
      persist: false  # in-memory only per session
      navigation: "↑/↓ in device mode"

    submit:
      key: "enter"
      send_suffix: "\r\n"  # configurable: \n, \r, \r\n
      clear_on_submit: true

    mode_indicator:
      device_mode: "> " (green)
      host_mode: "HOST> " (yellow, input disabled)
      capture_mode: "(capture in progress)" (red, disabled)

  editing:
    cursor_movement: "←/→, Home/End"
    delete: "Backspace, Del"
    clear_line: "Ctrl-U"
    no_multiline: true
```

## 5.3 Status Bar

```yaml
StatusBar:
  description: "Single-line status display between viewport and input"

  segments:
    - name: mode
      values: ["DEVICE", "HOST"]
      style:
        DEVICE: "green bg"
        HOST: "yellow bg"

    - name: follow
      values: ["ON", "OFF [G=resume]"]
      style:
        ON: normal
        OFF: "dim, with hint"

    - name: capture
      values: ["—", "CAPTURE {percent}%"]
      style:
        capturing: "red bg, bold"

    - name: filter
      values: ["—", "FILTER: {summary}"]
      style:
        active: cyan

    - name: search
      values: ["—", "SEARCH: {query} ({n}/{total})"]
      style:
        active: cyan

    - name: buffer
      format: "{used}/{max}"
      warn_at: "90%"
      style:
        normal: dim
        warning: yellow

    - name: timestamp
      format: "HH:MM:SS"
      align: right
      live_update: true

  layout:
    separator: " │ "
    overflow: "truncate middle segments, preserve mode and timestamp"
    min_width: 80
```

## 5.4 Notifications / Toasts

```yaml
Notifications:
  description: "Non-modal temporary messages"

  types:
    info:
      duration: 3s
      position: "top-right corner of viewport"
      style: "blue border"

    success:
      duration: 2s
      style: "green border"

    warning:
      duration: 5s
      style: "yellow border"
      dismissable: true

    error:
      duration: 10s
      style: "red border"
      dismissable: true
      action_hint: "press Enter for details"

  events:
    panic_detected:
      type: warning
      message: "Backtrace detected — press 'i' to view in Inspector"

    coredump_complete:
      type: info
      message: "Core dump captured (64KB) — press 'i' to view report"

    gdb_stub_detected:
      type: warning
      message: "GDB stub active — see Inspector for connection command"

    session_log_started:
      type: success
      message: "Session logging to {path}"

    connection_lost:
      type: error
      message: "Serial connection lost — press Enter to reconnect"
```

## 5.5 Copy / Selection Model

```yaml
CopySelection:
  description: "Keyboard-driven text selection and clipboard"

  modes:
    # Mode 1: Quick copy shortcuts (no visual selection)
    quick_copy:
      "yy":          { action: copy_current_line }
      "yv":          { action: copy_visible_viewport }
      "yb":          { action: copy_last_n_lines, prompt: "Lines to copy: " }

    # Mode 2: Visual selection (vim-like)
    visual_mode:
      enter: "v"
      movement: "j/k/gg/G"
      copy: "y"
      exit: "escape"
      indicator: "VISUAL" in status bar

  clipboard:
    # Try OSC52 first (works in most modern terminals)
    primary: osc52
    fallback: pbcopy | xclip | wl-copy  # detected at startup
    notification: "Copied {n} lines to clipboard"

  inspector_copy:
    "c": "copy raw content to clipboard"
    "C": "copy decoded content to clipboard"
    "s": "save to file (prompt for path)"
```

---

# Part 6: Error States and Edge Cases

## 6.1 Connection Errors

```yaml
connection_errors:
  port_not_found:
    display: "Error banner in Port Picker"
    message: "Port not found: {path}"
    hint: "Run 'esper scan' or check cable connection"

  permission_denied:
    display: "Error banner in Port Picker"
    message: "Permission denied: {path}"
    hint: "Try: sudo usermod -aG dialout $USER"

  port_busy:
    display: "Error banner in Port Picker"
    message: "Port busy: {path}"
    hint: "Another process may have the port open"

  connection_lost:
    display: "Notification + status bar indicator"
    message: "Serial connection lost"
    action: "Press Enter to reconnect"
    auto_reconnect: false  # user must confirm
```

## 6.2 Decode Errors

```yaml
decode_errors:
  elf_not_found:
    display: "Warning notification"
    message: "ELF file not found — backtrace decode unavailable"
    severity: warning
    continue: true  # show raw backtrace anyway

  addr2line_failed:
    display: "Warning in Inspector"
    message: "addr2line failed: {error}"
    fallback: "Show raw addresses only"

  coredump_decode_failed:
    display: "Error in Inspector"
    message: "Core dump decode failed: {error}"
    preserve: "Raw base64 still available for copy/save"

  python_not_found:
    display: "Warning notification"
    message: "Python not found — core dump decode unavailable"
    at: "first core dump capture attempt"
```

## 6.3 Buffer Overflow

```yaml
buffer_overflow:
  warning_threshold: 90%
  warning_display: "Yellow buffer indicator in status bar"

  at_limit:
    behavior: "Evict oldest lines (FIFO)"
    notification: "Buffer full — oldest logs being discarded"
    notification_frequency: "once per 10% eviction"
```

---

# Part 7: Implementation Hints for Bubble Tea

## 7.1 Model Structure Suggestion

```yaml
# NOT CODE — conceptual structure for Bubble Tea Model

EsperModel:
  # View state
  current_view: "port_picker | device_manager | monitor"
  mode: "device | host | capture"

  # Overlay stack (can have multiple)
  overlays: []  # help, search, filter, inspector, confirmation, assign_nickname, add_device, edit_device

  # Device registry (loaded from ~/.config/esper/devices.json)
  registry:
    version: 1
    devices: []  # array of DeviceEntry

  # Port picker state
  port_picker:
    ports: []  # merged with registry data (nickname, name, description)
    selected_index: 0
    form_focus: "port_list | baud | elf | toolchain | probe"
    error: null

  # Device manager state
  device_manager:
    selected_index: 0
    devices: []  # from registry, with online/offline status from scan
    edit_form:
      usb_serial: ""
      nickname: ""
      name: ""
      description: ""
      is_new: false

  # Monitor state
  monitor:
    # Connection
    port: null
    baud: 115200
    connected: false

    # Viewport
    log_buffer: RingBuffer
    viewport_offset: 0
    follow_mode: true
    wrap_enabled: false
    horizontal_offset: 0

    # Input
    input_value: ""
    input_history: []
    history_index: -1

    # Search
    search_active: false
    search_query: ""
    search_matches: []
    search_index: 0

    # Filter
    filter_active: false
    filter_config: FilterConfig

    # Events
    events: []  # panic, coredump, gdbstub
    inspector_open: false
    inspector_selected: 0

    # Capture
    capture_in_progress: false
    capture_progress: 0
    capture_buffer: []
```

## 7.2 Key Considerations

```yaml
implementation_notes:
  ansi_handling:
    - "Use a library that preserves ANSI sequences (not strips them)"
    - "Track 'dangling' SGR state at end of partial lines"
    - "Reset color state at line boundaries to prevent 'color leakage'"

  partial_lines:
    - "Serial output often arrives mid-line"
    - "Buffer incomplete lines; flush after timeout (100ms)"
    - "Visual indicator for 'line in progress' (e.g., blinking cursor)"

  high_throughput:
    - "Batch incoming bytes; don't re-render on every byte"
    - "Use ring buffer for scrollback; O(1) eviction"
    - "Consider 60fps render cap with coalesced updates"

  viewport_sizing:
    - "Reserve 3 lines: title bar, status bar, input line"
    - "Viewport height = terminal_height - 3"
    - "Handle resize gracefully (recalculate viewport, preserve scroll position)"

  focus_management:
    - "Only one 'focused' widget at a time"
    - "Device mode: InputLine focused"
    - "Host mode: no widget focused (viewport receives keys)"
    - "Overlay: overlay receives keys"
```

## 7.3 Must-Not-Do List

```yaml
must_not_do:
  - "NEVER send host-mode keys to device (safety-critical)"
  - "NEVER auto-launch external processes (GDB, esptool probe) without confirmation"
  - "NEVER block the main loop during serial I/O (use goroutines + channels)"
  - "NEVER assume terminal supports mouse (keyboard-only required)"
  - "NEVER hard-code colors that assume dark theme (use ANSI base colors)"
  - "NEVER truncate core dump capture mid-stream (buffer fully, then process)"
  - "NEVER lose the raw capture data even if decode fails"
```

---

# Part 8: Summary

This document provides a complete UX specification for implementing the Esper TUI:

1. **7 primary screens/views** with ASCII wireframes at 80×24 and 120×40:
   - Port Picker (with device registry integration)
   - Device Manager (registry CRUD)
   - Monitor View (device mode, host mode, compact mode)
   - Inspector Panel
2. **8 overlays** with wireframes and widget specs:
   - Help, Search, Filter, Command Palette, Confirmation
   - Assign Nickname, Add Device, Edit Device
3. **YAML widget pseudocode** describing component hierarchy and properties
4. **Complete keyboard mapping** with state-aware behavior (including device registry keys)
5. **State machine** showing all valid transitions (including Device Manager)
6. **Component behavior specs** for scrolling, input, notifications, copy/selection
7. **Error states and edge cases**
8. **Implementation hints** for Bubble Tea (including registry model structure)

**Device Registry Integration:**
- Port Picker shows nicknames, names, and descriptions from `~/.config/esper/devices.json`
- `n` key assigns/edits nickname for selected port
- `d` key opens Device Manager for full CRUD operations
- `esper --port <nickname>` resolves nickname to port path

The designer/implementer should use this as the visual and interaction contract. Any deviations should be documented and discussed.

## Related Documents

- [01-esper-tui-ux-design-brief-for-contractor.md](../design-doc/01-esper-tui-ux-design-brief-for-contractor.md) — Original requirements brief
- [01-diary.md](./01-diary.md) — Research and investigation diary
- [ESP-02-DEVICE-REGISTRY design doc](../../ESP-02-DEVICE-REGISTRY--device-registry-nicknames-for-serial-consoles/design-doc/01-device-registry-nickname-resolution.md) — Device registry design
