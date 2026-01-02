---
Title: 'Bug report + investigation: stack overflow during createPng screenshot send'
Ticket: 0026-B3-SCREENSHOT-STACKOVERFLOW
Status: active
Topics:
    - cardputer
    - esp-idf
    - esp32s3
    - m5gfx
    - screenshot
    - png
    - stack-overflow
    - debugging
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Pressing `P` to send a PNG screenshot can overflow the FreeRTOS `main` task stack (likely inside `LGFXBase::createPng` / miniz encoder), causing an immediate reboot.
LastUpdated: 2026-01-01T23:05:27.229953718-05:00
WhatFor: ""
WhenToUse: ""
---

# Bug report + investigation: stack overflow during `createPng` screenshot send

## Summary

In the demo-suite firmware (`esp32-s3-m5/0022-cardputer-m5gfx-demo-suite`), pressing `P` triggers `screenshot_png_to_usb_serial_jtag()`, which calls `m5gfx::M5GFX::createPng()` and then writes the bytes framed as `PNG_BEGIN <len>\n ... \nPNG_END\n` on USB-Serial/JTAG.

On real hardware this can crash with:

- `***ERROR*** A stack overflow in task main has been detected.`

The crash occurs after `PNG_BEGIN ...` and partial PNG bytes have already been emitted, then the device reboots.

## Observed logs (device-side)

User-provided snippet (truncated, emphasis added):

```
PNG_BEGIN 2200
...<binary PNG bytes>...
***ERROR*** A stack overflow in task main has been detected.

Backtrace: 0x40375c2d:0x3fc9b780 0x4037b75d:0x3fc9b7a0 ...
--- vApplicationStackOverflowHook ...
--- vTaskSwitchContext ...
```

Additional context from the same report:

- `ELF file SHA256: 9d390e569` (as printed by panic handler; truncated in the pasted snippet)

## Expected behavior

- Screenshot send should not crash the firmware.
- Host-side `tools/capture_screenshot_png.py` should save a valid PNG.
- If screenshot cannot be produced, fail fast (log/toast) and keep UI responsive.

## Actual behavior

- Firmware prints `PNG_BEGIN` and binary data, then panics due to `main` task stack overflow.

## Where the PNG encoder comes from (source of truth)

`m5gfx::M5GFX` is the LovyanGFX/M5GFX stack vendored in this repo. The screenshot code path ultimately calls:

- `LGFXBase::createPng(...)` in `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp`

Implementation detail (relevant excerpt):

- It allocates a DMA row buffer: `heap_alloc_dma(w * 3)`
- It calls miniz’ encoder:
  - `tdefl_write_image_to_png_file_in_memory_ex_with_cb(...)`
  - with a callback (`png_encoder_get_row`) that reads each row via `readRectRGB(...)`

So: the PNG encoding work is done by **miniz (tdefl)** on-device. That encoder can use substantial stack.

## Likely root cause

### Hypothesis A (most likely): main task stack too small for PNG encode

ESP-IDF runs `app_main()` inside the FreeRTOS task named `main`, with a configurable stack size (often small by default).

`LGFXBase::createPng()` uses miniz’ DEFLATE encoder (`tdefl_*`), which can use enough stack to overflow the default `main` task stack. The overflow may only be detected on the next context switch, which explains why the crash appears after some serial output and often right after a `vTaskDelay(...)`/yield.

### Hypothesis B: additional stack pressure from our screenshot send loop

Less likely: `screenshot_png_to_usb_serial_jtag()` itself is small, but it can call `vTaskDelay(1)` in `serial_write_all()` when the driver ringbuffer is full. That can trigger the stack overflow check at a task switch, surfacing an overflow that already happened during `createPng()`.

## Fix strategy (chosen)

Avoid running stack-heavy PNG encode in the `main` task:

- Move `createPng()` + framing send into a dedicated FreeRTOS task with a **large stack**.
- Block `main` until the task finishes (so display access remains serialized and we don’t contend with the render loop).

This avoids patching the upstream M5GFX/miniz implementation while keeping the screenshot feature.

## Fix status

Implemented in the demo-suite:

- Commit `9087658` — "Fix: avoid main stack overflow on screenshot"
  - `screenshot_png_to_usb_serial_jtag()` now runs encode+send in a worker task (`screenshot_task`) with a larger stack and blocks until completion.

## Validation results (post-fix)

Host-side capture succeeded after flashing the fix:

- Command:
  - `python3 esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00 /tmp/cardputer_demo.png`
- Result:
  - `wrote /tmp/cardputer_demo.png (2140 bytes)`
  - `/tmp/cardputer_demo.png: PNG image data, 240 x 135, 8-bit/color RGB, non-interlaced`

## Files / symbols to review first

- Demo suite:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`
    - `screenshot_png_to_usb_serial_jtag()`
    - `serial_write_all()`
- M5GFX:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp`
    - `LGFXBase::createPng(...)`
    - `tdefl_write_image_to_png_file_in_memory_ex_with_cb(...)`

## Validation checklist

1. Start host capture:
   - `python3 esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py <port> /tmp/out.png`
2. On device, press `P`:
   - host prints `wrote /tmp/out.png (...)`
   - device does not reboot / no stack overflow
3. Repeat several times; confirm still stable.

## Prevention guidance (for future work)

- Treat the ESP-IDF `main` task as “control plane” only: avoid heavy compression/encoding there.
- If using library helpers like `createPng()`, assume they may be stack-heavy:
  - run them in a purpose-built task with explicit stack budget
  - consider non-deflate formats (raw RGB, BMP, QOI) if stack/RAM is constrained.
