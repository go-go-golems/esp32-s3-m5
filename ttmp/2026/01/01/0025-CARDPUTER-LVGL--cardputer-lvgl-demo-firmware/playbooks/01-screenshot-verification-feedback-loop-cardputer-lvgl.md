---
Title: Screenshot verification feedback loop (Cardputer LVGL)
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: playbooks
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: A repeatable workflow to capture a screenshot via esp_console, save it on the host, OCR-verify expected UI, and iterate quickly while avoiding serial tool interference.
LastUpdated: 2026-01-02T13:38:41.494370035-05:00
WhatFor: ""
WhenToUse: ""
---

# Screenshot verification feedback loop (Cardputer LVGL)

This playbook is a repeatable “tight loop” for validating Cardputer LVGL UI changes using on-device screenshot capture + host-side OCR. It produces objective evidence you can attach to docs/PRs and use for regression checks without relying on photos.

The loop is built around:

- Device firmware: `esp32-s3-m5/0025-cardputer-lvgl-demo`
- Device console command: `screenshot` (ESP-IDF `esp_console` over USB-Serial/JTAG)
- Host tools:
  - `tools/capture_screenshot_png_from_console.py` (sends `screenshot`, captures framed PNG)
  - `pinocchio code professional --images ... "..."` (OCR)

## Prerequisites

### Hardware + port assumptions

- M5Stack Cardputer connected via USB (USB-Serial/JTAG).
- A serial port exists, typically:
  - `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*` (preferred)
  - `/dev/ttyACM0` (fallback)

### One-serial-consumer rule

Screenshots stream as binary bytes over the same serial port using a framed protocol:

```
PNG_BEGIN <len>\n
<raw PNG bytes>
\nPNG_END\n
```

If another program reads the port concurrently (e.g. `idf.py monitor`, `screen`, `minicom`), it can steal bytes and corrupt the capture. For screenshot capture, let the capture script have exclusive access to the port.

## Step 0: Build + flash

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo
./build.sh build
./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash
```

## Step 1: Capture a screenshot (automation-friendly)

This tool opens the serial port, sends `screenshot`, waits for `PNG_BEGIN`, then:
- if `<len> > 0`: reads exactly `<len>` bytes, or
- if `<len> == 0`: parses the streamed PNG until the `IEND` chunk,
then waits for `PNG_END` and writes the PNG.

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0025-cardputer-lvgl-demo
python3 tools/capture_screenshot_png_from_console.py \
  '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' \
  /tmp/cardputer_lvgl.png \
  --timeout-s 30
```

Sanity check:

```bash
file /tmp/cardputer_lvgl.png
```

Expected: `PNG image data, 240 x 135, ...`

## Step 2: OCR-verify the screenshot

```bash
pinocchio code professional --images /tmp/cardputer_lvgl.png "OCR this screenshot and list the visible UI text (titles, labels, hints)."
```

Good OCR checks:

- title matches expected screen (e.g., `LVGL Demos`, `Basics`, `POMODORO`)
- expected menu entries are visible
- hint text matches what the README claims

## Step 3: Before/after regression check

```bash
# BEFORE
python3 tools/capture_screenshot_png_from_console.py \
  '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' \
  /tmp/before.png --timeout-s 30

# AFTER (after flashing a new build)
python3 tools/capture_screenshot_png_from_console.py \
  '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' \
  /tmp/after.png --timeout-s 30

pinocchio code professional --images /tmp/before.png,/tmp/after.png "OCR both screenshots. Summarize what changed in the visible UI text and layout."
```

This catches common regressions (wrong screen, missing labels, truncated text, wrong hint strings) quickly.

## Step 4: Troubleshooting

### No `PNG_BEGIN` (script times out)

Likely causes:

- wrong port
- device not running the updated firmware
- another program is holding/reading the port (monitor)

Fix:

- close monitors, retry with correct `/dev/serial/by-id/...` path, re-flash if needed

### PNG saved but corrupted

Likely cause: another serial consumer stole bytes.

Fix:

- ensure exclusive port ownership during capture (no `idf.py monitor` running)

### Command prints `ERR: screenshot failed` or `ERR: screenshot timeout`

Likely causes:

- device wedged / UI loop not draining control-plane events
- USB-Serial/JTAG driver not ready / host not reading fast enough

Fix:

- retry once; if persistent, re-open monitor after capture attempt and inspect logs
