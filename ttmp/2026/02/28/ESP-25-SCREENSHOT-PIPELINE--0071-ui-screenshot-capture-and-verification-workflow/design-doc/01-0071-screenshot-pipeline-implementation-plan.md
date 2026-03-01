---
Title: 0071 Screenshot Pipeline Implementation Plan (QOI Pivot)
Ticket: ESP-25-SCREENSHOT-PIPELINE
Status: active
Topics:
    - cardputer
    - cardputer-adv
    - esp32-s3
    - firmware
    - screenshot
    - ui
    - usb-serial-jtag
    - qoi
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0071-cardputer-adv-photo-timer/main/app_main.cpp
      Note: Console command wiring and UI-thread screenshot execution
    - Path: 0071-cardputer-adv-photo-timer/main/screenshot_qoi.cpp
      Note: Current QOI-based screenshot path
    - Path: 0071-cardputer-adv-photo-timer/tools/capture_screenshot_qoi_from_console.py
      Note: Host-side framed QOI capture utility
    - Path: 0025-cardputer-lvgl-demo/main/screenshot_png.cpp
      Note: Previous streamed-PNG implementation reference
    - Path: 0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py
      Note: Previous host framing/parser reference
ExternalSources: []
Summary: Replace on-device PNG screenshot encoding in 0071 with QOI to reduce CPU and memory pressure while preserving host-capture workflow and enabling reliable UI verification.
LastUpdated: 2026-03-01T16:25:00Z
WhatFor: Define the concrete migration from PNG to QOI screenshot transport for 0071.
WhenToUse: Use when implementing, reviewing, or debugging screenshot capture reliability in 0071.
---

# 0071 Screenshot Pipeline Implementation Plan (QOI Pivot)

## Executive Summary

The existing screenshot command in `0071` still fails on hardware (`PNG_BEGIN 0`, empty payload). The firmware currently attempts on-device PNG encoding, which is CPU-heavy and historically fragile under constrained memory/stack conditions.  

This ticket pivots screenshot transport to QOI (Quite OK Image), a lightweight lossless format with significantly lower encode complexity than PNG. The new design preserves the same operational model:

1. user runs `screenshot` from console,
2. firmware captures the framebuffer on the UI thread and streams framed bytes over USB Serial/JTAG,
3. host script writes output and optionally decodes for visual verification.

## Problem Statement

Current behavior:

1. screenshot command is registered and executable.
2. framing appears (`PNG_BEGIN`/`PNG_END`).
3. capture returns failure and no image bytes.

Observed consequence:

1. We cannot reliably produce screenshots for UI verification.
2. Current path depends on PNG encode behavior that is expensive and brittle on embedded targets.

## Proposed Solution

### Transport and framing

Replace PNG framing with QOI framing:

1. firmware emits `QOI_BEGIN <len>\n`,
2. emits exactly `<len>` QOI bytes,
3. emits `\nQOI_END\n`.

### On-device encoder

Implement a dedicated QOI encoder module in firmware:

1. read display rows via `readRectRGB`,
2. encode using QOI ops (`INDEX`, `RUN`, `DIFF`, `LUMA`, `RGB`),
3. support count-only pass and write pass so framed length is known before payload.

### Runtime execution model

Keep the hardened execution model:

1. screenshot command posts to UI command queue,
2. UI thread triggers encoder task with explicit stack budget,
3. command waits for completion via task notification.

### Host tooling

Replace host script with QOI-aware capture script:

1. send `screenshot`,
2. parse `QOI_BEGIN`,
3. read exact payload length,
4. verify `QOI_END`,
5. write `.qoi`,
6. optionally decode to `.ppm` for local visual checks.

## Design Decisions

1. **QOI instead of PNG**
Reason: Lower compute/memory complexity and fewer failure points than DEFLATE-based PNG on-device.

2. **Length-prefixed framing**
Reason: Avoid binary stream ambiguity; host reads exact payload without chunk heuristics.

3. **Two-pass encode (count + write)**
Reason: Avoid large temporary frame buffer while still producing precise length in `QOI_BEGIN`.

4. **Keep screenshot queue + dedicated worker task**
Reason: Maintains thread-safety and stack isolation from the main UI loop.

## Alternatives Considered

1. **Keep PNG and increase memory/stack**
Rejected: still expensive and previous hardening already showed instability risks.

2. **Raw RGB565 dump only**
Rejected for default path: simple but larger payload and less portable than QOI.

3. **Host-side screen scrape via web UI**
Rejected for this need: does not validate on-device LVGL render output.

## Implementation Plan

1. Implement QOI encode core and framed USB transport in firmware module.
2. Wire `app_main.cpp` to use QOI screenshot function and update command help strings.
3. Replace host script with QOI capture parser.
4. Add decode helper output (`.ppm`) for UI inspection.
5. Flash and validate: repeated captures, no crash/watchdog.
6. Capture screenshot and verify:
   - no horizontal scrollbar artifact at bottom,
   - settings view includes Wi-Fi IP line.

## Open Questions

1. Keep `screenshot` command name unchanged (recommended) or split into `screenshot qoi` and future variants?
2. Persist `.qoi` only, or always emit a decoded `.ppm` companion file for faster review?

## References

1. `0025-cardputer-lvgl-demo/main/screenshot_png.cpp`
2. `0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py`
3. `0071-cardputer-adv-photo-timer/main/app_main.cpp`
4. `0071-cardputer-adv-photo-timer/main/screenshot_qoi.cpp`
