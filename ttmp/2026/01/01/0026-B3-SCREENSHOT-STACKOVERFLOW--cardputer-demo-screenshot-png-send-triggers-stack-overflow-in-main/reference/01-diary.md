---
Title: Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T23:05:27.33003097-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Step 1: Triage stack-overflow crash during screenshot send

This step turns a “screenshot crashes the device” report into an actionable hypothesis and a concrete fix direction (without prematurely changing formats or patching upstream libraries).

### What I did
- Collected the observed failure mode: `***ERROR*** A stack overflow in task main has been detected.` after `PNG_BEGIN ...` and partial PNG bytes.
- Located the PNG encoder implementation used by `m5gfx::M5GFX::createPng()`:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp` → `LGFXBase::createPng`
  - It calls miniz: `tdefl_write_image_to_png_file_in_memory_ex_with_cb(...)`
- Chose a fix approach: run PNG encode/send in a dedicated FreeRTOS task with a larger stack, and block `main` until completion to avoid concurrent display access.

### Why
- ESP-IDF’s `main` task stack is often relatively small by default, and miniz deflate can be stack-hungry.
- The overflow is reported in `task main`, which matches the screenshot being triggered directly from `app_main()`’s event loop.

### What worked
- Source-level confirmation that PNG generation is done on-device via miniz, not via host tools.

### What didn't work
- I can’t deterministically reproduce the crash in this environment without manual keypress timing; the fix is implemented from strong code-path evidence.

### What I learned
- `LGFXBase::createPng()` performs a full deflate encode on-device; the helper’s convenience hides a non-trivial stack budget.

### What was tricky to build
- Ensuring the fix won’t introduce display bus contention: the screenshot task must not run concurrently with the render loop unless we add locking.

### What warrants a second pair of eyes
- Stack sizing for the screenshot worker task: pick a conservative value and confirm no regressions on memory-constrained variants.

### What should be done in the future
- Consider adding an alternate screenshot format (QOI/raw) for “always safe” debug dumps if PNG continues to be fragile.
- Consider adding a small toast/overlay confirming screenshot success/failure so validation isn’t purely host-side.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` and review the task-based encode/send change.

## Step 2: Implement task-based PNG encode/send to avoid `main` stack overflow

This step implements the chosen fix: perform `createPng()` and the USB-Serial/JTAG send in a dedicated FreeRTOS task with a larger stack, and block `main` until completion.

### What I did
- Refactored the screenshot sender in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`:
  - moved the existing encode+send logic into `screenshot_png_to_usb_serial_jtag_impl()`
  - added a `screenshot_task()` that runs the implementation and notifies the caller via `xTaskNotify`
  - updated `screenshot_png_to_usb_serial_jtag()` to create the task with a large stack (`8192`) and wait for completion

### Why
- The crash is reported as a stack overflow in FreeRTOS task `main`, and the deflate-based PNG encoder is a plausible stack-heavy call.
- A dedicated task provides an explicit stack budget without patching M5GFX/miniz.

### What worked
- `./build.sh build` succeeds with the task-based refactor.

### What didn't work
- End-to-end validation still requires manual keypress (`P`) while the host capture script is running.

### What I learned
- This project already uses FreeRTOS task sizes in the `4096`/`8192` range elsewhere; using the same pattern keeps things consistent.

### What was tricky to build
- Ensuring we do not create display contention: `main` blocks while the worker runs, so the render loop doesn’t touch the display during `readRectRGB` row reads.

### What warrants a second pair of eyes
- Confirm that the chosen stack size (`8192`) is sufficient on real hardware; bump to `12288` if overflow persists.

### What should be done in the future
- If screenshot still proves fragile, consider offering a non-deflate screenshot format for debugging (QOI or raw RGB) behind a separate keybinding.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` and review:
  - `screenshot_png_to_usb_serial_jtag_impl()`
  - `screenshot_task()`
  - `ScreenshotTaskArgs`
