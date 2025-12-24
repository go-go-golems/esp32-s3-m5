---
Title: Diary
Ticket: 004-CARDPUTER-M5GFX
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - display
    - m5gfx
    - st7789
    - spi
    - animation
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: Ground-truth Cardputer panel/bus/backlight config via M5GFX autodetect
    - Path: M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Vendor demo’s M5GFX + sprite usage pattern
    - Path: echo-base--openai-realtime-embedded-sdk/ttmp/2025/12/22/004-CARDPUTER-M5GFX--cardputer-using-m5gfx-bring-up-plasma-animation/analysis/01-how-to-use-m5gfx-on-m5cardputer-from-userdemo-prior-m5gfx-investigations.md
      Note: Ticket analysis doc used as starting point
    - Path: esp32-s3-m5/0010-atoms3r-m5gfx-canvas-animation/main/hello_world_main.cpp
      Note: Plasma implementation + waitDMA present loop we’ll port
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T22:17:06.340333007-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Capture a step-by-step narrative of building a **Cardputer** ESP-IDF chapter project under `esp32-s3-m5/` that uses **M5GFX** to bring up the **ST7789** display and run a full-screen **plasma** animation without DMA-overlap “flutter”.

## Context

This ticket builds on the existing analysis doc (M5GFX autodetect ground-truth for Cardputer wiring + offsets/rotation) and ports the working “plasma via `M5Canvas` + `waitDMA()`” approach from AtomS3R tutorial `0010`.

## Quick Reference

N/A (this is an implementation diary; copy/paste-ready snippets live in the chapter project itself).

## Usage Examples

N/A

## Related

- See `analysis/01-how-to-use-m5gfx-on-m5cardputer-from-userdemo-prior-m5gfx-investigations.md` for the bring-up notes and golden config values.

## Step 1: Create tasks + diary scaffolding

This step set up the ticket so the rest of the work is traceable and easy to continue. The key outcome is a concrete task list plus a diary document that’s already related to the ground-truth code files we’ll reference while implementing the Cardputer chapter.

There were no firmware changes yet; this is pure documentation + workflow setup.

### What I did
- Added a real task breakdown to the ticket with `docmgr task add`
- Created this diary document via `docmgr doc add`
- Related the key source-of-truth files to the diary via `docmgr doc relate`

### Why
- Make the upcoming firmware work easy to audit/review and easy to continue later without re-deriving Cardputer display details

### What worked
- `docmgr` ticket is active and now has an actionable task list
- Diary doc exists and is linked to the code we will port/reference

### What didn't work
- N/A

### What I learned
- The repo-level `.ttmp.yaml` points docmgr at `echo-base--openai-realtime-embedded-sdk/ttmp`, so ticket bookkeeping should happen there even when the implementation files live elsewhere in the repo.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Start with the task list in `tasks.md`
- Use the RelatedFiles on this diary doc to jump to the “ground truth” M5GFX Cardputer config and the AtomS3R plasma reference

### Technical details
- Commands used (high level):
  - `docmgr task add --ticket 004-CARDPUTER-M5GFX --text "..."`
  - `docmgr doc add --ticket 004-CARDPUTER-M5GFX --doc-type reference --title "Diary"`
  - `docmgr doc relate --doc .../reference/01-diary.md --file-note "/abs/path:Reason" ...`

### What I'd do differently next time
- N/A

## Step 4: Confirm device re-enumeration risk; switch to /dev/serial/by-id

This step validated a second failure mode consistent with USB re-enumeration: even when we forced a lower flash baud rate, esptool failed immediately because `/dev/ttyACM0` was missing at open time. This reinforces that the issue is transport stability (device resets/disconnects) rather than flash layout.

The concrete mitigation is to use the stable `/dev/serial/by-id/...` symlink (same physical device even if `/dev/ttyACM*` numbering changes) and keep the baud conservative while we’re stabilizing the workflow.

### What I did
- Attempted flashing at low baud:
  - `idf.py -p /dev/ttyACM0 -b 115200 flash`
- Observed failure:
  - `/dev/ttyACM0` missing (`Errno 2 No such file or directory`)
- Identified the stable device path:
  - `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00`
- Updated the flash-debug analysis doc with this mitigation.

### Why
- If the device node itself is unstable, we need to remove that variable before we can judge whether baud/no-stub fixes the mid-transfer disconnect.

### What worked
- We found a stable by-id path that should survive re-enumeration better than hardcoding `/dev/ttyACM0`.

### What didn't work
- Low baud alone didn’t help because the port path was missing at open time.

### What I learned
- On Linux, `/dev/serial/by-id/*` is the most reliable way to reference USB serial devices across disconnect/reconnect cycles.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Update `0011` README with the by-id flash command once we confirm it works reliably.

### Code review instructions
- See `analysis/02-debug-flashing-0011-aborts-around-60-70.md` for the exact errors and the recommended flash command.

## Step 5: Flash successfully with stable by-id path at 115200 baud

This step confirmed the root cause and the practical workaround: flashing succeeds reliably when we use the stable `/dev/serial/by-id/...` path and keep baud at 115200 (no baud hop to 460800 mid-stub). This removes the “device node disappeared” failure mode and completes the write+verify sequence.

With flashing stabilized, we can proceed to the actual chapter goal: implementing the plasma animation loop.

### What I did
- Flashed tutorial `0011` using the stable by-id path at 115200 baud:
  - `idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash`

### Why
- Avoid `/dev/ttyACM0` disappearing mid-flash and avoid high-baud instability on USB-Serial/JTAG.

### What worked
- Flash completed successfully (write + hash verify for bootloader/app/partition table).

### What didn't work
- N/A (this was the successful run).

### What I learned
- The “by-id + conservative baud” approach is the most robust baseline for this setup.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Update `0011` README to recommend the by-id flash command as the default.

## Step 6: Implement plasma animation in tutorial 0011

This step completes the chapter’s “fun demo” goal: after the solid-color bring-up, the firmware now allocates a full-screen `M5Canvas` sprite and renders a classic plasma effect into its RGB565 buffer each frame. Present is done via `pushSprite(0,0)` and (optionally) `display.waitDMA()` to avoid overlapping transfers.

**Commit (code):** bbb22f5 — "Tutorial 0011: add plasma animation loop"

### What I did
- Added plasma rendering utilities (palette + sine table + draw loop) to `0011`’s `main/hello_world_main.cpp`
- Allocated `M5Canvas canvas(&display)` sized to `display.width()/height()` after autodetect/rotation
- Implemented per-frame render into `canvas.getBuffer()` (RGB565) + present via `canvas.pushSprite(0,0)`
- Added optional `display.waitDMA()` controlled by `CONFIG_TUTORIAL_0011_PRESENT_WAIT_DMA`
- Updated `0011` README with expected output + by-id flash tip

### Why
- Provide the Cardputer equivalent of AtomS3R `0010`’s “obviously alive” animation, using the safe canvas present pattern.

### What worked
- Firmware builds and links successfully after adding the plasma loop.

### What didn't work
- N/A (implementation/build succeeded).

### What I learned
- Using `display.width()/height()` after M5GFX autodetect keeps the chapter robust to Cardputer rotation/offset details.

### What was tricky to build
- Ensuring the present path doesn’t overlap frame rendering; the `waitDMA()` hook is the key guardrail.

### What warrants a second pair of eyes
- Confirm `display.waitDMA()` is sufficient for Cardputer ST7789 present stability under sustained full-frame updates.

### What should be done in the future
- Add a short “performance tuning” note (baud/port + frame_delay_ms) once we measure FPS on real hardware.

## Step 2: Scaffold Cardputer tutorial 0011 (M5GFX autodetect bring-up)

This step created the new chapter project under `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation`, mirroring the proven structure from AtomS3R `0010` but switching the display init to **M5GFX autodetect** (Cardputer ST7789 wiring + offsets/rotation/backlight). The immediate goal is a reliable “solid colors” bring-up that compiles cleanly before layering in the full plasma animation.

At this point, the project builds successfully, but flashing to hardware appears to abort partway through (reported as ~60–70% into upload). Before changing anything, we need to capture the exact `idf.py flash` output and compare flashing/partition settings against the known-good Cardputer tutorials.

**Commit (code):** c0bff4e — "Tutorial 0011: scaffold Cardputer M5GFX project"

### What I did
- Added `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/` with:
  - `CMakeLists.txt` using `EXTRA_COMPONENT_DIRS` to reuse `M5Cardputer-UserDemo/components/M5GFX`
  - `main/hello_world_main.cpp` bring-up scaffold: `display.init()` + solid color fills
  - `sdkconfig.defaults` + `partitions.csv` (Cardputer-style 4M app + 1M storage)
  - Kconfig knobs for frame delay / DMA wait / heartbeat logging (for later animation loop)
- Built successfully with ESP-IDF 5.4.1:
  - `idf.py set-target esp32s3 && idf.py build`

### Why
- Establish a clean, reproducible Cardputer chapter baseline that uses the M5GFX “golden path” (autodetect) before implementing the plasma render/present loop.

### What worked
- Project compiles and links cleanly under ESP-IDF 5.4.1.

### What didn't work
- Flashing to the device seems to abort around ~60–70% into upload (need exact error output).

### What I learned
- Cardputer tutorial `0001` / `0007` use the same `partitions.csv` layout (4M factory + 1M storage) and default flash size 8MB, so the partition table in `0011` is unlikely to be the root cause by itself.

### What was tricky to build
- Keeping the new chapter small while still integrating M5GFX correctly (via `EXTRA_COMPONENT_DIRS`) so we reuse the vendored M5GFX without copying it into each chapter.

### What warrants a second pair of eyes
- Confirm that M5GFX autodetect reliably selects `board_M5Cardputer` in this ESP-IDF component integration setup (no hidden dependency on vendor init ordering).

### What should be done in the future
- Capture the exact `idf.py flash` failure output and, if it’s a serial transport issue, document the “lower baud / ensure port not in use” workaround in the chapter README.

### Code review instructions
- Start with `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
- Then check `sdkconfig.defaults` and `partitions.csv` for “Cardputer-style” defaults

### Technical details
- The bring-up uses:
  - `m5gfx::M5GFX display; display.init();`
  - `display.fillScreen(TFT_RED/...)` to validate panel init + backlight

### What I'd do differently next time
- Record the exact flash failure output immediately (copy/paste) before hypothesizing.

## Step 3: Capture flashing failure output (loss of connection at ~63%)

This step captured the exact `idf.py flash` failure output for `0011` so we can debug from evidence rather than guesswork. The failure happens mid-write (around 63%) right after esptool switches the port to **460800 baud**, then pySerial reports an **Input/output error** when trying to configure the port, which strongly suggests the USB CDC device disconnected/re-enumerated mid-transfer.

This points away from partition layout and toward USB/serial transport stability (baud, cable/hub, port contention). Next we’ll rerun the flash at a lower baud and, if needed, try `--no-stub`.

### What I did
- Collected the failing flash log for `/dev/ttyACM0` (loss of connection at ~63%)
- Added the log + interpretation to the ticket analysis doc `analysis/02-debug-flashing-0011-aborts-around-60-70.md`

### Why
- We need the exact error text and timing to choose the right mitigation (transport vs runtime crash)

### What worked
- The failure mode is now clearly identified as a USB serial transport exception (pySerial)

### What didn't work
- Flash did not complete at 460800 baud due to port I/O error mid-transfer

### What I learned
- When flashing over **USB-Serial/JTAG** (`/dev/ttyACM*`), a mid-flash disconnect often presents as a pySerial “could not configure port” error after esptool retries.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Document a “known-stable flash command” in the `0011` README once confirmed (e.g. `idf.py -b 115200 flash`).

### Code review instructions
- Read the failure log + debug playbook in `analysis/02-debug-flashing-0011-aborts-around-60-70.md`

### Technical details
- Failing point: `Writing at 0x0003d827... (63 %)` then “Lost connection”
- Error: `Could not configure port: (5, 'Input/output error')`

### What I'd do differently next time
- N/A
