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
Summary: "Single document to review implementation screenshots with the desired wireframe immediately underneath (plus discrepancy analysis and a missing-screens checklist)."
LastUpdated: 2026-01-25T16:32:44-05:00
---

# Current vs desired (quick compare)

This doc is for fast review. Each section shows:
1) Current implementation screenshot (from tmux capture harness)
2) Desired screenshot (wireframe), verbatim from the contractor UX spec

Current capture set used here:
- `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/`
- Core dump capture overlay (real hardware):
  - `ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/hw_coredump_progress2_20260125-153240/`
- Device registry / port picker nickname flow:
  - `ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/various/screenshots/device_mgr_20260125-162801/`

To regenerate (deterministic):
- `USE_VIRTUAL_PTY=1 ./ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/scripts/09-tmux-capture-esper-tui.sh`
- Device registry captures:
  - `bash ./ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/scripts/07-tmux-capture-device-manager-and-nickname.sh`

## Discrepancies (summary) + plan

This is the “fast feedback” view of what’s most different right now and what I plan to do next.

Top discrepancies:
- Search overlay: currently a centered modal; wireframe expects a bottom bar with match count and in-viewport match indicators/highlight.
- Filter overlay: currently only E/W/I + include/exclude; wireframe expects D/V plus highlight rules editor.
- Command palette: currently minimal commands; wireframe expects more commands + grouping separators.
- HOST scrollback layout: wireframe expects a specific HOST footer + scroll indicators; current implementation differs.

Scope note:
- The “80×24 compact view” wireframe is now considered **out of scope**; a responsive/scaled-down main view is preferred.

Immediate next implementation steps (in order):
1) Refactor search into a bottom “search bar” footer mode (HOST), add live match count, add match highlight/markers in viewport.
2) Extend filter config to include D/V and implement highlight rules (pattern → style) and application in viewport render pipeline.
3) Expand command palette command set + visual grouping; wire the new actions to monitor/app actions.
4) Add captures for missing wireframes (core dump capture, core dump report, reset confirmation, etc.) once screens exist.

## Screens still missing (not yet implemented or not yet captured)

Missing implementations:
- Search highlighting and match annotations in viewport (§2.2).
- Filter highlight rules editor (§2.3).
- Command palette extra commands + separators (§2.4).

Missing captures (screen exists but not included in this compare doc yet):
- Help overlay (§2.1).

## Port Picker (with device registry, 120×40)

Current:

![](ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/various/screenshots/device_mgr_20260125-162801/120x40-01-port-picker.png)

Desired (wireframe, verbatim):

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

Discrepancies + plan:
- The wireframe uses a compact table with a “preferred” ★ column and a per-row `[n: assign]` hint for unregistered devices. Plan: add the missing columns/row-hints and align spacing via a shared table renderer (ESP-04 visual parity).

## Port Picker — Assign Nickname dialog (120×40)

Current:

![](ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/various/screenshots/device_mgr_20260125-162801/120x40-02-assign-nickname.png)

Desired (wireframe, verbatim):

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

Discrepancies + plan:
- Our dialog currently supports the fields + key flow, but button ordering/styling differs. Plan: match the `<Save> <Cancel>` alignment and button styles via shared “modal chrome” helpers (ESP-04 visual parity).

## Device Manager (120×40)

Current:

![](ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/various/screenshots/device_mgr_20260125-162801/120x40-04-device-manager.png)

Desired (wireframe, verbatim):

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

Discrepancies + plan:
- We implement list/add/edit/remove and online/offline, but the wireframe’s table separators and footer key hints are more structured. Plan: move Device Manager onto the same reusable “selectable list” + “footer key hints” rendering used elsewhere (ESP-03/ESP-04 refactor/visual parity).

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

Discrepancies + plan:
- Title bar: wireframe shows a concise connected port; current UI may truncate long paths. Plan: keep truncation but prefer a short display name (device nickname or basename) while preserving full path elsewhere (tooltip/help).
- Status bar: wireframe fields differ slightly. Plan: align labels/ordering and add scrollback indicator when not following.

## Monitor — Scrollback Mode (HOST, 120×40)

Current:

![](ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/20260125-100132/120x40-02-host.png)

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

Discrepancies + plan:
- Footer: current footer differs from the wireframe (“Ctrl-T: HOST COMMANDS …”). Plan: switch HOST footer to match the wireframe (and incorporate search/filter/palette hints in a consistent, compact way).
- Scroll indicators/right-side bar: not implemented. Plan: add a right-side “scrollbar” and/or “▲ N more ▼” indicator when the viewport is not at the bottom/top, using Lip Gloss for layout.

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

Discrepancies + plan:
- Wireframe shows a richer events list (“Events (3)”, event grouping, “Press Enter to view”). Plan: restructure inspector into a reusable list model with richer rows and a detail view mode (panic/coredump/gdb) as separate sub-screens.

## Core Dump Capture In Progress (DEVICE, 120×40)

Current:

![](ttmp/2026/01/24/ESP-02-ESPER-TUI--esper-contractor-ux-design-brief-for-a-tui-serial-monitor/various/screenshots/hw_coredump_progress2_20260125-153240/120x40-01b-device-triggered.png)

Desired (wireframe, verbatim):

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

Discrepancies + plan:
- We estimate percent using a 64 KiB target; wireframe shows an explicit `current/total` byte count. Plan: track buffered bytes and show both; if total is unknown, show “— / —” and keep the bar.
- Our overlay is already centered and dims background, but background content differs (firmware prints a warning about escape sequences). Plan: ignore (firmware-side cosmetic).

## Panic Detail View (Inspector, 120×40)

Current:

![](ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/various/screenshots/inspector_details_20260125-155050/120x40-02-panic-detail.png)

Desired (wireframe, verbatim):

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

Discrepancies + plan:
- We currently show Raw + Decoded, but not the Context box. Plan: parse panic register dump lines into a structured event so we can render the Context section.
- Wireframe shows copy actions; we haven't wired copy yet. Plan: use the existing clipboard dependency (already in `go.mod`) and add `c/C` key handling to copy raw/decoded sections.

## Core Dump Report (Inspector, 120×40)

Current:

![](ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/various/screenshots/inspector_details_20260125-155050/120x40-03b-coredump-report.png)

Desired (wireframe, verbatim):

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

Discrepancies + plan:
- We show status/size/path and a decoded report box, but we don't provide the footer actions yet (`c/s/r/j`). Plan: implement copy/save actions next; saving should write report + raw base64 to predictable locations.
- Our core dump decode currently fails for the fake payload (expected). Plan: either (a) keep UI working for “decode failed” case, and (b) optionally add a “real” coredump emitter path in firmware if we want success cases for screenshots.

## Reset Device confirmation (HOST, 120×40)

Current:

![](ttmp/2026/01/25/ESP-05-TUI-MISSING-SCREENS--esper-tui-implement-missing-screens-test-firmware-updates/various/screenshots/reset_confirm_20260125-155916/120x40-01-reset-confirm.png)

Desired (wireframe, verbatim):

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

Discrepancies + plan:
- Button order differs: we currently show `[ Cancel ] [ Reset ]`; wireframe shows `<Reset> <Cancel>`. Plan: swap order and add a “Confirm:” prefix in title.
- Our overlay message differs slightly. Plan: match the wireframe text more closely.

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

Discrepancies + plan:
- Wireframe uses a bottom bar (not a modal). Plan: re-render search as a bottom bar that replaces the normal DEVICE input/footer area in HOST mode.
- Wireframe shows match count + match markers in viewport. Plan:
  - compute match indices live while typing
  - show `cur/total` and `n/N` hint in the bottom bar
  - add match highlight/markers in the viewport render (without corrupting ANSI color output)

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

Discrepancies + plan:
- Missing D/V level toggles. Plan: extend filter config and level detection to include Debug/Verbose (best-effort; ESP-IDF uses `D (` / `V (` prefixes similarly).
- Missing highlight rules editor. Plan: add a highlight rules list model (pattern + style), and apply highlight styles after filtering (render-time decoration).

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

Discrepancies + plan:
- Missing several commands and separator groupings. Plan:
  - add separators in the command list rendering (visual rows)
  - add commands (wrap toggle, reset device confirm overlay, send break, session logging, reconnect)
  - wire each to explicit monitor/app actions (HOST-only)

## Bubble Tea model/architecture review (how screens are built + reuse opportunities)

This section is for reducing UI code and keeping a unified look/feel across screens by reusing models and shared rendering patterns.

### App shell (`appModel`)

Current structure:
- `esper/pkg/monitor/app_model.go`: owns `screen` routing (port picker vs monitor), global key handling (Ctrl-C, `?`), `tea.WindowSizeMsg` fan-out, and the single active overlay (help/search/filter/palette).

Improvements:
- Overlay unification is now implemented (commit `fcbcd29` in the nested `esper/` repo).
  - Benefit: one overlay stack, consistent close behavior (`Esc`), consistent styling, and reduced duplication of overlay routing.

### Port picker (`portPickerModel`)

Current structure:
- `esper/pkg/monitor/port_picker.go`: selection list + form fields + connect actions.

Improvements:
- Reuse a “selectable list” model: the port list, command palette list, and inspector event list all implement similar selection + scrolling behaviors.
  - Proposed: extract a shared list core (selected index, window/scroll region, renderRow callback).
  - Benefit: consistent selection behavior, less repeated code, fewer UX inconsistencies.

### Monitor (`monitorModel`)

Current structure:
- `esper/pkg/monitor/monitor_view.go`: owns log buffer, viewport, follow, host focus, and host inspector; it now requests overlays via explicit actions/messages rather than owning overlay state.

Improvements:
- Split responsibilities into composable submodels:
  - `logBufferModel` (append/bound memory), `logFilterModel` (level/regex + highlight rules), `logSearchModel` (query + matches), `viewportModel` (scroll/follow).
  - Benefit: easier to match wireframes (e.g., bottom bar search can be implemented as a mode on top of the same reusable buffer/search models).

### Overlays (search/filter/palette/help)

Current structure:
- Search/filter/palette are modal boxes centered via Lip Gloss `Place`.
- All overlays are now hosted by `appModel` via a shared overlay interface (commit `fcbcd29`).

Improvements:
- Create a shared “modal chrome” renderer:
  - title line, body area, hint/footer area, error line area
  - ensures consistent padding, borders, and focus highlighting across all modal overlays.
- Support non-modal overlays (bottom bar):
  - wireframe search is explicitly a bottom bar; implement it as a `footer mode` rather than a modal.

### Styling (`styles`)

Current structure:
- `esper/pkg/monitor/styles.go`: single `styles` struct with styles like `Panel`, `OverlayBox`, `SelectedRow`, etc.

Improvements:
- Expand from “layout primitives” to “semantic styles”:
  - `KeyHint`, `Divider`, `PrimaryButton`, `SecondaryButton`, `Muted`, `Accent`
  - Benefit: consistent UI across port picker, monitor, overlays, and inspector details without re-styling each screen.

### Concrete refactor proposals (to reduce duplication)

- Create `ui/` helpers in the monitor package (or a small internal package) for:
  - `RenderFramedScreen(title, body, status, footer)`
  - `RenderModal(title, body, hint, error)`
  - `RenderBottomBar(prompt, rightStatus, hint)`
- Extract a reusable “selectable list” model used by:
  - port list (port picker)
  - command palette
  - inspector event list
- Make overlays pluggable with a tiny interface:
  - `Open()`, `Close()`, `Update(tea.KeyMsg)`, `View(styles)`, `SetSize(size)`
  - so `appModel` can host *all* overlays uniformly.
