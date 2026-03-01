---
Title: Implementation Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0071-cardputer-adv-photo-timer/main/screenshot_qoi.cpp
      Note: On-device QOI encoder and framed USB serial sender
    - Path: 0071-cardputer-adv-photo-timer/main/app_main.cpp
      Note: Console command wiring and UI view switch command
    - Path: 0071-cardputer-adv-photo-timer/tools/capture_screenshot_qoi_from_console.py
      Note: Host-side framed capture and decode helper
ExternalSources: []
Summary: Chronological implementation notes for migrating 0071 screenshot transport from PNG to QOI and validating with hardware captures.
LastUpdated: 2026-03-01T16:40:00Z
WhatFor: Preserve exact implementation/debugging history and validation evidence.
WhenToUse: Use when reviewing or extending screenshot capture behavior in 0071.
---

# Implementation Diary

## Goal

Record the end-to-end migration from failing on-device PNG screenshots to reliable QOI screenshots for 0071, including exact commands, failures, and validation artifacts.

## Step 1: Capture failure mode and choose QOI

The first task was to confirm that the screenshot command path was wired correctly and isolate where it failed. We verified command registration and framing, then observed zero-length PNG payload output from the device despite successful command execution.  

Given the repeated PNG fragility in this firmware and prior tickets, I selected QOI as the new format because it is lossless but far cheaper to encode on-device than PNG/DEFLATE.

### Prompt Context

**User prompt (verbatim):** "QOI. Add tasks, then work on them"

**Assistant interpretation:** Implement a QOI-based screenshot pipeline immediately, track it as explicit tasks, and complete the work.

**Inferred user intent:** Make screenshot capture reliable on constrained hardware and unblock UI verification.

### What I did

- Reproduced and logged current failure path:
  - command worked
  - output showed `PNG_BEGIN 0` then `PNG_END`
  - command returned `ERR: screenshot failed`
- Added explicit QOI tasks in `ESP-25` and updated implementation plan to a QOI pivot.

### Why

- PNG path still produced empty payloads on this board/firmware.
- QOI gives deterministic, lightweight encoding with smaller implementation surface.

### What worked

- Existing screenshot queue/task structure was reusable.
- Existing host serial framing model was reusable.

### What didn't work

- On-device PNG path did not produce payload bytes on this firmware despite command wiring being correct.

### What I learned

- The failure was in the encode/output path, not in command registration or serial transport setup.

### What was tricky to build

- Distinguishing transport issues from encoding issues required direct monitor checks of command output (`help`, `screenshot`) before changing architecture.

### What warrants a second pair of eyes

- Ensure no hidden dependency still expects PNG framing names or payload type.

### What should be done in the future

- Consider keeping a secondary raw RGB565 capture command for worst-case debugging if QOI ever regresses.

### Code review instructions

- Review task and plan updates first:
  - `ttmp/2026/02/28/ESP-25-SCREENSHOT-PIPELINE--0071-ui-screenshot-capture-and-verification-workflow/tasks.md`
  - `ttmp/2026/02/28/ESP-25-SCREENSHOT-PIPELINE--0071-ui-screenshot-capture-and-verification-workflow/design-doc/01-0071-screenshot-pipeline-implementation-plan.md`

### Technical details

- Key observed failure output before QOI migration:
  - `PNG_BEGIN 0`
  - `PNG_END`
  - `ERR: screenshot failed`

## Step 2: Implement QOI encoder, host capture, and hardware validation

This step replaced the screenshot implementation with a dedicated QOI path, kept the hardened queue/task execution model, and validated captures directly from hardware. I also added a small `ui` console command to switch views without manual encoder interaction so both timer and settings screens could be captured deterministically.  

The final result: QOI capture succeeds consistently and decoded images verified both requested UI outcomes (settings IP visible, no horizontal bottom scrollbar).

### Prompt Context

**User prompt (verbatim):** (same as Step 1)

**Assistant interpretation:** Complete the QOI migration fully and prove it with actual captures.

**Inferred user intent:** Replace unstable screenshot behavior with a robust, practical verification workflow.

### What I did

- Implemented firmware QOI transport:
  - `0071-cardputer-adv-photo-timer/main/screenshot_qoi.cpp`
  - `0071-cardputer-adv-photo-timer/main/screenshot_qoi.h`
- Updated runtime wiring:
  - switched `app_main.cpp` screenshot call to `screenshot_qoi_to_usb_serial_jtag_ex`
  - updated help text to QOI
  - added `ui timer|settings` console command to switch views for capture verification
- Updated build and docs:
  - `main/CMakeLists.txt` now compiles `screenshot_qoi.cpp`
  - `README.md` now documents QOI capture script usage
- Replaced host capture script with QOI parser + decoder helpers:
  - `tools/capture_screenshot_qoi_from_console.py`
  - supports `.qoi` output and optional decoded `.bmp`/`.ppm` output
- Ran build/flash/validation commands:
  - `idf.py -C 0071-cardputer-adv-photo-timer build`
  - `idf.py -C 0071-cardputer-adv-photo-timer -p /dev/serial/by-id/... flash`
  - `/home/manuel/.espressif/python_env/idf5.4_py3.12_env/bin/python tools/capture_screenshot_qoi_from_console.py ...`

### Why

- QOI lowers encoding complexity and memory pressure compared to PNG.
- Keeping existing queue/task execution avoids reintroducing LVGL/display thread-safety risks.

### What worked

- Successful hardware captures:
  - `/tmp/0071-ui.qoi` and decoded `/tmp/0071-ui.bmp`
  - `/tmp/0071-ui-settings.qoi` and decoded `/tmp/0071-ui-settings.bmp`
  - `/tmp/0071-ui-timer.qoi` and decoded `/tmp/0071-ui-timer.bmp`
- Settings screenshot confirms IP line:
  - `Wi-Fi IP: 192.168.0.196`
- Timer screenshot confirms no horizontal scrollbar at bottom.

### What didn't work

- Flash initially failed once with:
  - `Could not exclusively lock port ... Resource temporarily unavailable`
  - Cause: monitor session still held the serial port.
  - Fix: kill tmux monitor session and rerun flash.

### What I learned

- QOI payload sizes are compact enough for reliable USB Serial/JTAG transfer in this workflow (~6-8 KB captured in this UI state).

### What was tricky to build

- Need deterministic view switching for verification without manual encoder input.  
  Added `ui timer|settings` command to avoid manual interaction dependency during captures.

### What warrants a second pair of eyes

- QOI encoder correctness against edge cases (long runs, diff/luma boundaries) and portability of decoded output expectations.

### What should be done in the future

- Optionally add host helper to emit PNG directly (via Pillow) for teams/tools that do not accept BMP/PPM.

### Code review instructions

- Start with:
  - `0071-cardputer-adv-photo-timer/main/screenshot_qoi.cpp`
  - `0071-cardputer-adv-photo-timer/tools/capture_screenshot_qoi_from_console.py`
- Then review wiring:
  - `0071-cardputer-adv-photo-timer/main/app_main.cpp`
  - `0071-cardputer-adv-photo-timer/main/CMakeLists.txt`
  - `0071-cardputer-adv-photo-timer/README.md`
- Validate on hardware:
  - flash firmware
  - run capture script twice:
    - timer view
    - `ui settings` then screenshot

### Technical details

- Successful capture output example:
  - `wrote /tmp/0071-ui-settings.qoi (6353 bytes)`
  - `wrote /tmp/0071-ui-settings.bmp (240x135)`

## Related

- `ttmp/2026/02/28/ESP-25-SCREENSHOT-PIPELINE--0071-ui-screenshot-capture-and-verification-workflow/design-doc/01-0071-screenshot-pipeline-implementation-plan.md`
- `ttmp/2026/02/28/ESP-25-SCREENSHOT-PIPELINE--0071-ui-screenshot-capture-and-verification-workflow/tasks.md`
