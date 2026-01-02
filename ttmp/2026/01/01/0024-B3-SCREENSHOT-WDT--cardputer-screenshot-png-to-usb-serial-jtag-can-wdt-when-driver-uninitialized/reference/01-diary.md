---
Title: Diary
Ticket: 0024-B3-SCREENSHOT-WDT
Status: active
Topics:
    - cardputer
    - m5gfx
    - screenshot
    - usb-serial-jtag
    - watchdog
    - esp-idf
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: Target file for the fix (serial_write_all)
    - Path: esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md
      Note: Repro logs/backtrace and proposed fix
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T21:27:48.835080107-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Capture the step-by-step investigation and fix work for the B3 screenshot-to-serial watchdog issue, including exact logs, commands, and the code paths involved.

## Step 1: Create bug ticket + consolidate repro evidence

This step turned an observed “device wedges after screenshot” report into a structured bug ticket with pointers for investigation. The immediate outcome is a grounded hypothesis: a driver-not-initialized error (`usb_serial_jtag_write_bytes`) combined with an infinite retry loop in our app code can starve IDLE and trip Task WDT.

### What I did
- Created the docmgr ticket `0024-B3-SCREENSHOT-WDT`.
- Wrote a bug report document capturing:
  - the backtrace pointing at `serial_write_all()` in `screenshot_png.cpp`
  - an additional log snippet showing the error tends to occur after navigating to the “B3 Screenshot” scene (then triggering screenshot)

### What worked
- The backtrace has line numbers in our code, making this a tight, actionable report.

### What didn't work
- N/A (documentation + triage step only).

### What I learned
- ESP-IDF’s `usb_serial_jtag_write_bytes()` returns a negative error code when the driver is not installed, and our current `<= 0 => continue` loop can spin forever in that case.

### What warrants a second pair of eyes
- Confirm whether the preferred “house style” fix is:
  - app-level `usb_serial_jtag_driver_install` + bounded retries (likely), or
  - enforcing a Kconfig console choice so the driver is always installed.

### Code review instructions
- Start with `esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md`.

## Related

- Bug report: `esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md`
- Origin ticket diary: `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/reference/02-implementation-diary.md`

## Step 2: Fix screenshot send loop (driver guard + chunked writes + bounded retries)

This step implements the minimal safety fix so the screenshot feature cannot wedge the device loop. The key change is that we no longer spin forever on `usb_serial_jtag_write_bytes()` returning an error (negative) or a timeout (0); instead we install the USB-Serial/JTAG driver if missing, write in small chunks, and bail out after a bounded number of timeouts with a small `vTaskDelay(1)` yield.

**Commit (code):** da2f85f — "Fix: harden screenshot PNG send over USB-Serial/JTAG"

### What I did
- Updated `serial_write_all()` in `screenshot_png.cpp`:
  - treat `n < 0` as fatal and return immediately (prevents infinite retry when driver is uninitialized)
  - treat `n == 0` as “couldn’t enqueue” and retry with:
    - bounded retry count
    - `vTaskDelay(1)` yield (avoid IDLE starvation)
  - chunk writes (`kTxChunkBytes=128`) so we don’t attempt to push an entire PNG as a single ringbuffer item
- Added `ensure_usb_serial_jtag_driver_ready()` to install the driver when missing (tx buffer 4096).
- Rebuilt and flashed to the connected Cardputer; captured boot logs via tmux monitor.

### Why
- The previous behavior could trip Task WDT because the app could spin in a tight retry loop while the driver simultaneously emitted an error log each iteration.
- Even with the driver installed, sending a large buffer in one `usb_serial_jtag_write_bytes()` call can fail because the driver uses a ringbuffer; chunking is required for large payloads like PNGs.

### What worked
- `idf.py build` succeeded after the change.
- Flash succeeded after stopping an existing `esp_idf_monitor` process that was holding `/dev/ttyACM0` (port busy lock).

### What didn't work
- Full end-to-end validation (capturing a real PNG via the host script) still requires a physical keypress (`P`) on the device while `capture_screenshot_png.py` is running.

### What I learned
- ESP-IDF’s `usb_serial_jtag_write_bytes()` returns:
  - a negative `esp_err_t` code when the driver is not installed (e.g. `ESP_ERR_INVALID_ARG`)
  - `0` when it times out acquiring internal resources / ringbuffer space
  - `size` when the ringbuffer enqueue succeeds

### What warrants a second pair of eyes
- Chunking/timeout policy: confirm `kTxChunkBytes=128` and the retry budgets are acceptable for real-world host throughput, and that this won’t make screenshot sends feel “stuck” when the host isn’t reading.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` and review:
  - `ensure_usb_serial_jtag_driver_ready()`
  - `serial_write_all()`
  - `kTxChunkBytes`

## Step 3: Follow-up crash observed during screenshot send (stack overflow in `main`)

After the WDT-specific fix, a different failure mode was observed on real hardware: triggering the screenshot path can overflow the FreeRTOS `main` task stack and reboot.

### What I did
- Recorded the new symptom:
  - `***ERROR*** A stack overflow in task main has been detected.`
  - it occurs after `PNG_BEGIN ...` and partial PNG bytes are emitted
- Identified the likely root: `m5gfx::M5GFX::createPng()` calls miniz’ deflate encoder (`tdefl_*`), which can be stack-hungry.
- Opened a new dedicated ticket for this separate issue: `0026-B3-SCREENSHOT-STACKOVERFLOW`.

### Why this is a separate ticket
- 0024 is specifically about WDT caused by uninitialized USB-Serial/JTAG driver + busy-loop retry.
- The new crash is not a WDT; it’s a stack overflow/panic likely inside PNG encoding.

### What warrants a second pair of eyes
- Whether the preferred long-term approach is:
  - a dedicated screenshot task with an explicit larger stack budget (recommended), or
  - increasing `CONFIG_ESP_MAIN_TASK_STACK_SIZE`, or
  - switching screenshot format away from PNG (QOI/raw) for a debug-safe path.

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
