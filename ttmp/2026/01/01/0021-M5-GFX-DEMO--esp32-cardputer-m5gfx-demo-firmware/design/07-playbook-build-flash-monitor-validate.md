---
Title: 'Playbook: build, flash, monitor, validate (Cardputer demo-suite)'
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - esp32s3
    - esp-idf
    - firmware
    - debugging
    - tmux
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/build.sh
      Note: Primary build/flash helper + tmux workflow entry point
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/README.md
      Note: Project-level build/flash notes and expected behavior
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0019-cardputer-ble-temp-logger/tools/run_fw_flash_monitor.sh
      Note: Prior art for robust port picking + calling ESP-IDF tooling from tmux
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0021-atoms3-memo-website/build.sh
      Note: Prior art for a tmux-based firmware feedback loop
ExternalSources: []
Summary: How to build/flash/run the demo-suite on a real Cardputer, with a tight tmux-based feedback loop and validation checklist.
LastUpdated: 2026-01-02T00:45:00Z
WhatFor: ""
WhenToUse: ""
---

# Playbook: build, flash, monitor, validate (Cardputer demo-suite)

This doc describes the workflow I’m using to **iterate quickly** on the demo-suite firmware (project `0022-cardputer-m5gfx-demo-suite`) on a real Cardputer, with minimal “terminal yak shaving”.

## Goals

- One command to build + flash + start serial monitor.
- Use a **stable serial port path** (by-id), not “whatever `/dev/ttyACM0` is today”.
- Keep a feedback loop that works well in **tmux** (since monitor requires a real TTY).
- Have an explicit **validation checklist** that’s easy to repeat after changes.

## Constraints / gotchas (that shape the approach)

### 1) `idf.py monitor` needs a real TTY

`idf.py monitor` fails in non-interactive contexts with:

> Monitor requires standard input to be attached to TTY

So monitoring must be done in an actual terminal (including tmux panes).

### 2) Serial port locking conflicts

`idf.py monitor` holds an exclusive lock on the serial device. If monitor is running, `idf.py flash` may fail with:

> the port is busy or doesn't exist … Could not exclusively lock port … Resource temporarily unavailable

This is why the tmux workflow runs **`flash monitor` as a single command in one pane**, rather than trying to run `flash` and `monitor` simultaneously in separate panes.

### 3) tmux/direnv/PATH quirks

Some older scripts in this repo explicitly call:

- `${IDF_PYTHON_ENV_PATH}/bin/python`
- `${IDF_PATH}/tools/idf.py`

instead of relying on `idf.py` being on PATH, because tmux panes + login shells + pyenv/direnv can produce confusing python/module mismatches.

`0022-cardputer-m5gfx-demo-suite/build.sh` follows that “explicit python + explicit idf.py path” pattern.

## Recommended workflow (the one I chose)

### One-shot (simple, no tmux)

If you just want to build and flash (no monitor):

```bash
cd esp32-s3-m5/0022-cardputer-m5gfx-demo-suite
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash
```

For build + flash + monitor (best default in a normal terminal):

```bash
cd esp32-s3-m5/0022-cardputer-m5gfx-demo-suite
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash monitor
```

### tmux feedback loop (recommended)

This starts a tmux session with:

- left pane: `flash monitor` (single process; no port lock conflicts)
- right pane: an interactive shell in the project directory

```bash
cd esp32-s3-m5/0022-cardputer-m5gfx-demo-suite
./build.sh tmux-flash-monitor
```

Notes:

- `build.sh` auto-picks a port via `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*` first, then falls back to `/dev/ttyACM*`.
- Override port with `PORT=/dev/ttyACM1 ./build.sh tmux-flash-monitor`.

## Validation checklist (on-device)

After flashing, validate these in order:

### Boot + display bring-up

In serial output:

- Look for `cardputer_m5gfx_demo_suite: boot; free_heap=...`
- Look for `M5GFX: [Autodetect] board_M5Cardputer`
- Look for `display ready: width=240 height=135`

On screen:

- A menu list renders (no flicker), with one highlighted row.

### Input sanity

Press keys on the Cardputer keyboard:

- `Fn+;` / `Fn+.` moves selection up/down (Cardputer “arrow” chords)
- `Fn+,` / `Fn+/` moves selection left/right (page up/down in list view)
- `W` / `S` also works as a fallback (raw key events)
- `Enter` prints `open: idx=<n>` to serial
- `Fn+1` (Esc/back) returns to the menu
- `Fn+Del` returns to the menu
- In terminal mode: `Del` is backspace (`bksp`)

### Screenshot (B3)

To validate the screenshot-to-serial path end-to-end:

1) Start the host capture script:

```bash
python3 esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py \
  /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* \
  /tmp/cardputer_demo.png
```

2) On the device, press `P` (screenshot).
3) Verify the script prints `wrote /tmp/cardputer_demo.png (...)` and the PNG opens.

## Troubleshooting

### Port keeps changing

Always prefer a by-id path:

- `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...`

### Flash fails with “port is busy”

Stop any running monitor first (or use the single-command workflow):

- Use `./build.sh -p <port> flash monitor` (recommended)
- Or kill the tmux session that’s holding the port (example): `tmux kill-session -t cardputer-demo-suite`

### `No module named 'esp_idf_monitor'`

This indicates you’re not using the ESP-IDF python env. Use `./build.sh` (it sources the correct `export.sh` and invokes `${IDF_PATH}/tools/idf.py` via the IDF venv python).
