---
Title: "ESP-02-ESPER-TUI: current vs desired (quick compare)"
Ticket: ESP-02-ESPER-TUI
Status: active
Topics:
  - tui
  - ux
  - bubbletea
  - lipgloss
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/reference/02-esper-tui-full-ux-specification-with-wireframes.md
  - ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh
Summary: "Single document to review implementation screenshots with the desired wireframe immediately underneath."
LastUpdated: 2026-01-25T10:12:00-05:00
---

# Current vs desired (quick compare)

This doc is for fast review. Each section shows:
1) Current implementation screenshot (from tmux capture harness)
2) Desired screenshot (wireframe), verbatim from the contractor UX spec

Current capture set used here:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/`

To regenerate (deterministic):
- `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`

## Monitor — Normal Mode (DEVICE, 120×40)

Current:

![](ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/120x40-01-device.png)

Desired (wireframe, verbatim):

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

## Inspector — Right Panel (HOST, 120×40)

Current:

![](ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/120x40-06-inspector.png)

Desired (wireframe, verbatim):

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

## Search overlay (HOST, 120×40)

Current:

![](ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/120x40-03-search.png)

Desired (wireframe, verbatim):

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

## Filter overlay (HOST, 120×40)

Current:

![](ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/120x40-04-filter.png)

Desired (wireframe, verbatim):

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

## Command palette (HOST, 120×40)

Current:

![](ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/120x40-05-palette.png)

Desired (wireframe, verbatim):

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

