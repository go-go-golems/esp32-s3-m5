---
Title: Diary
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.cursor/commands/diary.md
      Note: Diary format and required sections
    - Path: ../../../../../../../../../../.cursor/commands/docmgr.md
      Note: Workflow for creating ticket/docs/tasks
    - Path: 0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Key event decoding decisions referenced in Step 1
    - Path: 0025-cardputer-lvgl-demo/build.sh
      Note: Step 3 flash workflow + tmux monitor guidance
    - Path: 0025-cardputer-lvgl-demo/main/action_registry.cpp
      Note: Step 10 action list and CtrlEvent mapping
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: |-
        Step 2 UI loop + stable UiState callbacks
        Step 11 perf counters for SysMon
    - Path: 0025-cardputer-lvgl-demo/main/command_palette.cpp
      Note: Step 10 palette implementation
    - Path: 0025-cardputer-lvgl-demo/main/console_repl.cpp
      Note: |-
        Step 8 adds menu/basics/pomodoro/setmins commands
        Step 10 uses ActionRegistry for command registration and adds palette command
    - Path: 0025-cardputer-lvgl-demo/main/control_plane.h
      Note: Step 8 expands CtrlType/CtrlEvent for UI-thread-safe demo control
    - Path: 0025-cardputer-lvgl-demo/main/demo_basics.cpp
      Note: Step 6 timer cleanup on screen delete
    - Path: 0025-cardputer-lvgl-demo/main/demo_manager.cpp
      Note: Step 4 screen switching and shared group
    - Path: 0025-cardputer-lvgl-demo/main/demo_menu.cpp
      Note: |-
        Step 4 menu screen implementation
        Step 9 menu lists Console entry
    - Path: 0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp
      Note: |-
        Step 4 Pomodoro screen implementation
        Step 6 timer cleanup on screen delete
    - Path: 0025-cardputer-lvgl-demo/main/demo_split_console.cpp
      Note: Step 9 SplitConsole implementation
    - Path: 0025-cardputer-lvgl-demo/main/demo_system_monitor.cpp
      Note: Step 11 SysMon implementation
    - Path: 0025-cardputer-lvgl-demo/main/input_keyboard.cpp
      Note: Step 5 Fn+1 Esc override
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp
      Note: Step 2 display flush + tick timer
    - Path: 0025-cardputer-lvgl-demo/sdkconfig.defaults
      Note: Step 2 LVGL Kconfig defaults (disable example build)
    - Path: components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h
      Note: Step 5 corrected Back/Esc chord (Fn+`)
    - Path: imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp
      Note: Flush/tick/handler reference used in Step 1
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T22:49:10.13641006-05:00
WhatFor: ""
WhenToUse: ""
---












# Diary

## Goal

Capture the research and implementation journey for building a Cardputer LVGL demo firmware: what I looked at, what I changed, why, what failed, and what to validate.

## Context

This ticket builds an ESP-IDF firmware project for M5Stack Cardputer that integrates LVGL using M5GFX as the display backend and the `cardputer_kb` scanner (plus the existing `CardputerKeyboard` wrapper from `0022-cardputer-m5gfx-demo-suite`) as the input source.

## Step 1: Research the existing “known-good” Cardputer setup and LVGL port patterns

I started by identifying the smallest set of existing code in this repo that already solves the hard parts we need (Cardputer display bring-up, keyboard scanning, and an LVGL<->M5GFX flush path). The key decision from this step is to reuse the project/app scaffold from `0022-cardputer-m5gfx-demo-suite` and adapt the LVGL emulator port’s display flush approach to ESP-IDF.

The biggest uncertainty going in is the RGB565 byte order: LVGL and LovyanGFX both support swapped/non-swapped pixel order, and getting that wrong yields “working but wrong colors”. The plan is to implement the simplest correct flush path first, then adjust `LV_COLOR_16_SWAP` / `setSwapBytes()` if needed.

### What I did
- Read `~/.cursor/commands/docmgr.md` and `~/.cursor/commands/diary.md` to follow the ticket+diary workflow.
- Read the Cardputer demo suite scaffold (`0022-cardputer-m5gfx-demo-suite`) to copy its ESP-IDF project wiring patterns.
- Read the LVGL emulator port (`imports/lv_m5_emulator`) to reuse its flush/tick/handler patterns conceptually.
- Reviewed the shared Cardputer keyboard component (`components/cardputer_kb`) and how `CardputerKeyboard` turns scans into semantic events.

### Why
- Avoid reinventing bring-up code that already works in this repo.
- Minimize scope: build the smallest LVGL port that can render and accept keyboard input.

### What worked
- Found a clean “Cardputer baseline” in `0022-cardputer-m5gfx-demo-suite`:
  - `m5gfx::M5GFX display; display.init();`
  - `CardputerKeyboard keyboard; keyboard.init();`
- Found a directly relevant LVGL flush implementation that targets `M5GFX` in the emulator import.

### What didn't work
- N/A (research-only step)

### What I learned
- The repo already has a good separation of concerns for keyboard:
  - `components/cardputer_kb/*` = stable physical scan + action decoding
  - `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.*` = “event stream” suitable for app/UI layers

### What was tricky to build
- LVGL’s input model expects a polling callback (`read_cb`) while the Cardputer keyboard layer emits “edge events”. This requires a small queue/adapter between the two.

### What warrants a second pair of eyes
- The chosen mapping from `KeyEvent.key` strings to LVGL keycodes (to ensure we preserve normal text entry while enabling navigation).
- The display flush pixel format assumptions (RGB565 swap correctness).

### What should be done in the future
- Once the baseline works, consider adding a small note/playbook on how to debug color swap issues (photos + the specific knob that fixed it).

### Code review instructions
- Start with the analysis plan: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md`
- Then review the scaffold we’re reusing: `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`
- Then review the LVGL port pattern reference: `esp32-s3-m5/imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp`

### Technical details
- Key files referenced in this step:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/CMakeLists.txt`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
  - `esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp`
  - `esp32-s3-m5/imports/lv_m5_emulator/src/utility/lvgl_port_m5stack.cpp`

### What I'd do differently next time
- N/A

## Step 2: Scaffold the ESP-IDF project and implement a minimal LVGL UI loop

This step created the actual firmware project (`0025-cardputer-lvgl-demo`) and implemented the smallest LVGL “happy path”: bring up M5GFX, register an LVGL display driver that flushes to M5GFX, register a keyboard indev, and render a simple screen with a textarea and a button.

The key correctness fix in this step was ensuring that LVGL callback `user_data` points at a stable object. LVGL stores pointers verbatim; passing the address of a stack local in `create_demo_ui()` would have been a use-after-return bug. The current implementation uses a static `UiState` in `app_main` and passes its address into LVGL callbacks/timers.

### What I did
- Added a new ESP-IDF project at `esp32-s3-m5/0025-cardputer-lvgl-demo/` with the same “EXTRA_COMPONENT_DIRS” setup as `0022-cardputer-m5gfx-demo-suite` (M5GFX + cardputer_kb).
- Added an ESP-IDF Component Manager dependency on `lvgl/lvgl` via `main/idf_component.yml`.
- Implemented:
  - `lvgl_port_m5gfx` (draw buffers + flush callback + esp_timer tick)
  - `lvgl_port_cardputer_kb` (KEYPAD indev backed by a small key queue)
  - `app_main` UI loop with a textarea + “Clear” button + status label
- Ran a local build to confirm the toolchain and component dependency resolution.

### Why
- Establish a reusable baseline integration that future Cardputer UI work can build on.
- Make the first demo prove the integration end-to-end: draw + input + periodic updates.

### What worked
- `./build.sh build` successfully fetched and built LVGL and produced `build/cardputer_lvgl_demo.bin`.
- The LVGL managed component resolved to `lvgl/lvgl (8.4.0)` with the current semver constraint.

### What didn't work
- First build compiled a large set of LVGL examples (`CONFIG_LV_BUILD_EXAMPLES=y`), which massively increases compile time for a tiny demo. I added a default override in `sdkconfig.defaults` to disable it going forward.

### What I learned
- The ESP-IDF `lvgl/lvgl` managed component defaults to `CONFIG_LV_CONF_SKIP=y` (it builds without requiring a project-provided `lv_conf.h`).
- For LVGL integration, pointer lifetime is a real footgun: callback `user_data` needs a stable address for the lifetime of the objects/timers.

### What was tricky to build
- Choosing an input model that preserves normal typing while still providing navigation: the adapter uses “semantic” actions (`up/down/...`) when the bindings decoder emits them, and otherwise forwards single-character keys as ASCII.

### What warrants a second pair of eyes
- RGB565 swap correctness: the integration currently defaults `swap_bytes=false` and relies on `M5GFX::setSwapBytes(false)`. If colors are wrong on-device, we should flip the appropriate knob and document it.
- Memory choices for draw buffers (DMA heap vs normal heap) and whether PSRAM should be used.

### What should be done in the future
- Add a short “color swap debugging” note after the first on-device run (photo + setting used).

### Code review instructions
- Start in `esp32-s3-m5/0025-cardputer-lvgl-demo/main/app_main.cpp`
- Then review the display port in `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp`
- Then review key mapping/queue behavior in `esp32-s3-m5/0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp`
- Build: `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh build`

### Technical details
- `sdkconfig.defaults` sets `# CONFIG_LV_BUILD_EXAMPLES is not set` and increases `CONFIG_LV_MEM_SIZE_KILOBYTES` to reduce compile time and avoid small-heap surprises.

## Step 3: Flash smoke test on hardware

This step validated the firmware on a real Cardputer by flashing it over the USB-Serial/JTAG port. The demo screen appears usable: text entry works in the textarea and the UI is responsive enough for an initial baseline.

I didn’t capture a full interactive `idf.py monitor` session from this CLI environment (it requires an attached TTY). For future debugging, the project’s `./build.sh tmux-flash-monitor` target is the intended workflow because it runs monitor in a proper terminal.

### What I did
- Flashed the firmware:
  - `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash`
- Verified the UI behavior directly on device (manual).

### Why
- Confirm the display flush, tick, and keyboard indev are working end-to-end on real hardware.

### What worked
- Flash succeeded and the demo appears to work well on device (user-reported).

### What didn't work
- Running `idf.py monitor` from this harness fails due to missing stdin TTY (“Monitor requires standard input to be attached to TTY”).

### What I learned
- This repo’s `build.sh tmux-flash-monitor` pattern is the right way to run a stable flash+monitor workflow for device work.

### What was tricky to build
- N/A (validation step)

### What warrants a second pair of eyes
- Confirm on-device colors are correct (RGB565 swap). If not, we should decide whether to change `swap_bytes` (M5GFX) or LVGL’s swap setting and document it.

### What should be done in the future
- Run a monitor session in a real terminal and capture the boot log + any LVGL warnings into the diary for future regressions.

### Code review instructions
- Flash+monitor (recommended): `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh tmux-flash-monitor`

## Step 4: Add demo menu + Pomodoro screen

This step turns the firmware from a single-screen bring-up into a tiny “demo catalog” by introducing a menu screen and a lightweight screen manager. The menu allows selecting demos from the keyboard, and we add a Pomodoro screen that uses LVGL’s `lv_arc` with a big time label and timer-driven updates to feel smooth on the Cardputer display.

The key technical choice here is to keep a single shared `lv_group_t` and re-bind focusable objects per screen. That keeps keypad routing predictable and avoids accumulating groups. Another important detail is that “Esc returns to menu” is implemented globally in the app loop so it works even when focus is inside a textarea.

### What I did
- Added a demo manager and screens:
  - Menu screen (select demo)
  - Basics screen (textarea + clear button, refactored from the original single-screen demo)
  - Pomodoro screen (arc ring + big MM:SS + timer-based tick)
- Updated keyboard delivery so the app can see the translated LVGL keycode (for global `Esc` handling and “last key” display).
- Built and flashed the updated firmware.

### Why
- Make the firmware useful as a multi-demo baseline.
- Provide a polished Pomodoro example that exercises LVGL widgets, fonts, and timers.

### What worked
- `./build.sh build` succeeds after adding the new screens and manager.
- `./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_* flash` succeeds.

### What didn't work
- Still cannot capture `idf.py monitor` output from this harness (requires TTY). Use `tmux-flash-monitor` for live logs.

### What I learned
- Managing focus via a single shared `lv_group_t` is the simplest way to keep LVGL keypad routing sane across screen switches.

### What was tricky to build
- LVGL key events go to the focused object. For the Basics demo, we must add the textarea and button (not just the screen root) to the group so typing and “Enter to click” both work.
- Font availability depends on LVGL config. The Pomodoro screen uses a small helper to pick the “best available” font at compile time.

### What warrants a second pair of eyes
- Screen switching safety: calling `lv_scr_load()` from an LVGL event callback is generally OK, but we should watch for any LVGL re-entrancy edge cases.
- Global `Esc` handling: confirm it doesn’t interfere with any future text-entry needs (it currently bypasses LVGL and immediately returns to menu).

### What should be done in the future
- If we add more demos, consider turning the menu into a scrollable list and adding a “demo description” panel (still keyboard-first).
- Capture a boot log in a real terminal and paste it into the diary for regression checks.

### Code review instructions
- Start in:
  - `esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_manager.cpp`
  - `esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_menu.cpp`
  - `esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp`
- Build: `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh build`
- Flash: `cd esp32-s3-m5/0025-cardputer-lvgl-demo && ./build.sh flash`

## Step 5: Fix Esc chord mapping (Fn+`)

This step fixes two on-device issues: (1) the “Back/Esc” chord was mapped to the wrong keynum for this Cardputer (`Fn+\`` is keynum 29 + 1, not 29 + 2), and (2) the main loop was using `vTaskDelay(pdMS_TO_TICKS(5))`, which can round down to 0 ticks depending on the FreeRTOS tick rate, starving `IDLE0` and triggering task watchdog warnings.

Rather than adding “escape hatch” overrides in the app, the correct fix is to make the shared binding table match reality and to ensure the main task actually blocks for at least one RTOS tick each iteration.

### What I did
- Updated `cardputer_kb` captured bindings to use Back/Esc = `Fn(29) + keynum 1` (backtick key).
- Removed the earlier `Fn+1` override logic from `CardputerKeyboard::poll()`.
- Changed the app loop delay to `vTaskDelay(1)` to guarantee a real block/yield (avoids task_wdt spam).
- Updated UI hints/README to say `Fn+\`` instead of “Esc”.
- Built the firmware to ensure it compiles.

### Why
- `Fn+\`` is the “menu/back” chord in this firmware; it must map correctly.
- Task watchdog warnings are noise and can hide real problems.

### What worked
- `./build.sh build` succeeds with the change.

### What didn't work
- Flashing from this harness can fail if the serial port is held by another running monitor session (“port is busy”). Close the other monitor or use a single `tmux-flash-monitor` session.

### What warrants a second pair of eyes
- Confirm `Fn+\`` is the correct Back/Esc chord across Cardputer variants and doesn’t regress other firmwares that rely on `cardputer_kb`.

## Step 6: Fix crash when switching demos (LVGL timer use-after-free)

This step fixes a hard crash (Guru Meditation / `LoadProhibited`) that occurs after switching away from the Basics demo. The panic showed `lv_label_set_text()` called from `status_timer_cb`, and the LVGL object pointer was clearly poisoned (`0xa1b2c3d4` pattern), which is consistent with use-after-free: the screen was destroyed, but the LVGL timer kept running and continued updating a freed label.

The fix is to treat timers as screen-scoped resources: each demo allocates a per-screen state struct and registers an `LV_EVENT_DELETE` handler on the screen root to delete the timer and free the state. This prevents timers from firing after a screen is deleted and also avoids reusing a single global state struct across multiple screen instances.

### What I did
- Refactored Basics and Pomodoro demos to allocate per-screen state (`new ...State`) instead of reusing a single static struct.
- Added `LV_EVENT_DELETE` cleanup callbacks on demo roots to:
  - `lv_timer_del()` any active timers
  - clear pointers
  - `delete` the state
- Built and flashed.

### Why
- LVGL timers are not automatically tied to object lifetimes; deleting a screen does not delete its timers.

### What worked
- Reproduced symptom from backtrace: timer callback touching freed LVGL objects.
- After cleanup-on-delete, switching between Menu/Basics/Pomodoro no longer leaves background timers running.

### What warrants a second pair of eyes
- Confirm `lv_obj_del_async()` + `LV_EVENT_DELETE` timer deletion is safe under LVGL’s internal timer scheduling (no edge-case double-delete).

## Step 7: Add `esp_console` screenshot command (host-scriptable capture + OCR validation)

This step adds a host-scriptable screenshot flow to the LVGL demo by starting an `esp_console` REPL over USB-Serial/JTAG and registering a `screenshot` command. The key constraint is correctness under concurrency: the REPL runs in its own FreeRTOS task, but LVGL and display access must remain single-threaded and predictable. The solution is to treat `screenshot` as a control-plane request: the console command enqueues an event, and the main UI loop executes the capture and notifies the console task when it’s done.

This step also validates the end-to-end use case on real hardware: we flashed the firmware, invoked `screenshot` via the serial console, captured the framed PNG on the host, and used OCR to confirm the image contains the expected LVGL menu text.

**Commit (code+docs):** feed537b — "Cardputer LVGL: esp_console screenshot + OCR playbook"

### What I did
- Implemented an `esp_console` REPL over USB-Serial/JTAG in the LVGL demo project:
  - Added `0025-cardputer-lvgl-demo/main/console_repl.{h,cpp}` with `heap` + `screenshot` commands.
  - Updated console configuration to enable `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` (REPL supported by ESP-IDF’s `esp_console_new_repl_usb_serial_jtag`).
- Implemented a small control-plane queue so the REPL never touches LVGL directly:
  - Added `0025-cardputer-lvgl-demo/main/control_plane.{h,cpp}` (FreeRTOS queue of `CtrlEvent`).
  - In `0025-cardputer-lvgl-demo/main/app_main.cpp`, drained `CtrlEvent`s on the UI thread and executed screenshot capture there.
- Ported the hardened screenshot sender into the LVGL demo:
  - Added `0025-cardputer-lvgl-demo/main/screenshot_png.{h,cpp}` (framed PNG over USB-Serial/JTAG).
  - Kept the safety properties from the demo-suite’s implementation:
    - driver install guard
    - chunked writes
    - bounded retries with yields
    - PNG encode/send in a dedicated task (avoid `main` stack overflow)
- Added host capture tools and documented the workflow:
  - Added `0025-cardputer-lvgl-demo/tools/capture_screenshot_png.py` (capture-only)
  - Added `0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py` (sends `screenshot` then captures)
  - Updated `0025-cardputer-lvgl-demo/README.md` with usage and OCR validation command.

### Why
- We want deterministic “inspect what is going on” debugging without relying on camera photos of the screen.
- `esp_console` enables scripting/automation (e.g., capture screenshots repeatedly during UI changes).
- The repo already learned the hard lessons:
  - USB-Serial/JTAG driver can be uninitialized (WDT wedge risk) if you assume it exists.
  - `createPng()` can overflow the `main` task stack if run directly.
  - LVGL must not be mutated from arbitrary tasks.

### What worked
- Build succeeded:
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`
- Flash succeeded:
  - `./build.sh -p /dev/ttyACM0 flash`
- End-to-end capture succeeded (host triggered the console command and saved the PNG):
  - `python3 0025-cardputer-lvgl-demo/tools/capture_screenshot_png_from_console.py /dev/ttyACM0 /tmp/cardputer_lvgl.png --timeout-s 30`
  - `file /tmp/cardputer_lvgl.png` reports: `PNG image data, 240 x 135, ...`
- OCR validation matched the expected UI:
  - `pinocchio code professional --images /tmp/cardputer_lvgl.png "OCR the screenshot and tell me what UI text is visible (title/menu labels)."`
  - OCR output included: `LVGL Demos`, `Basics`, `Pomodoro`, `Up/Down select  Enter open`

### What didn't work
- Python PIL wasn’t installed in this environment (so we used `file` + OCR instead of `PIL.Image` for validation).

### What I learned
- ESP-IDF’s `esp_console_new_repl_usb_serial_jtag` is compiled only when `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`, so enabling the Kconfig option is required for a USB REPL.
- Waiting inside the console command (instead of printing progress) is important so we don’t interleave text output with the binary PNG payload.

### What was tricky to build
- Keeping the PNG stream “clean” for host capture while still using a REPL on the same port:
  - The command handler blocks while the UI thread sends the framed PNG.
  - Only after `PNG_END` does the command print `OK len=...`.
- Ensuring screenshot capture doesn’t race with LVGL:
  - the REPL enqueues a request; the UI loop performs capture; the PNG encoder runs in a worker task while the UI loop waits.

### What warrants a second pair of eyes
- Confirm the chosen console configuration (USB-Serial/JTAG as primary) doesn’t regress any preferred debugging workflows for this repo (e.g., `idf.py monitor` assumptions).
- Confirm no unexpected LVGL/display concurrency remains (the current design should serialize capture behind the UI loop, but it’s worth sanity-checking).

### What should be done in the future
- Add a `screenshot` “save-to-SD” option once MicroSD mounting exists (so captures can persist without a host).
- Expand the command set beyond `heap`/`screenshot` (menu navigation, demo switching, param tweaks) using the same control-plane queue pattern.

## Step 8: Add v0 esp_console demo-control commands (menu/basics/pomodoro/setmins)

This step extends the host-scriptable control plane from “screenshot-only” into a minimal but useful v0 command set: open the Menu/Basics/Pomodoro screens and set the Pomodoro duration in minutes. The goal is to make it easy to drive UI state deterministically from scripts (or a human REPL) while preserving the core architectural constraint: **LVGL remains single-threaded**.

The key technical decision is to keep using the control-plane/queue boundary: console commands enqueue `CtrlEvent`s, and the LVGL loop drains and applies them. For Pomodoro duration, we store a “default minutes” in `DemoManager` so `setmins` can be called before opening Pomodoro, while still applying immediately if Pomodoro is already active.

**Commit (code):** a623875 — "Cardputer LVGL: add esp_console demo control commands"

### What I did
- Extended `CtrlType`/`CtrlEvent` to support demo control:
  - OpenMenu/OpenBasics/OpenPomodoro
  - PomodoroSetMinutes (with `arg`)
- Implemented UI-thread handling in the `app_main` LVGL loop:
  - screen loads via `demo_manager_load(...)`
  - Pomodoro minutes updates via `demo_manager_pomodoro_set_minutes(...)`
- Added esp_console commands that enqueue events:
  - `menu`, `basics`, `pomodoro`, `setmins <minutes>`
- Made Pomodoro duration configurable via `DemoManager::pomodoro_minutes` and applied it when:
  - creating the Pomodoro screen, and
  - receiving `PomodoroSetMinutes` while Pomodoro is active.

### Why
- Host scripting should be able to put the UI into a known state (“open pomodoro”, “set duration 15”, “screenshot”) for fast verification and regressions.
- Keeping LVGL single-threaded avoids “works until it doesn’t” crashes from cross-thread UI mutation.

### What worked
- Build succeeded:
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`
- Flash succeeded:
  - `./build.sh -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' flash`
- REPL commands successfully switched screens and updated Pomodoro duration:
  - `pomodoro` → `OK`
  - `setmins 15` → `OK minutes=15`
  - `basics` → `OK`
  - `menu` → `OK`
- Screenshot + OCR validated the screen state transitions:
  - Menu screenshot OCR: `LVGL Demos`, `Basics`, `Pomodoro`, `Up/Down select`, `Enter open`
  - Pomodoro screenshot OCR: `POMODORO`, `25:00`, `PAUSED`
  - After `setmins 15`, OCR diff confirmed timer changed `25:00` → `15:00`

### What didn't work
- OCR sometimes truncates the bottom hint string on Pomodoro (small font); it still consistently captures the title + timer + status, which are the main regression signals.

### What I learned
- Treating “default state” (Pomodoro minutes) as part of `DemoManager` makes the control plane more predictable: `setmins` works even if the screen isn’t currently active.

### What was tricky to build
- Preserving the LVGL single-thread rule while still allowing the console task to “wait for completion” (we only do that for `screenshot`; demo-control commands just enqueue and return).
- Making `setmins` both:
  - update the current Pomodoro screen when it’s active, and
  - persist as the default for future Pomodoro screen creation.

### What warrants a second pair of eyes
- Confirm the `CtrlType` expansion and UI-thread dispatch in `app_main` can’t starve the UI loop under repeated host commands (queue depth, per-event work, no blocking).
- Confirm `demo_pomodoro_apply_minutes(...)` cannot accidentally apply to a stale screen (it relies on `LV_EVENT_DELETE` cleanup nulling the static pointer).

### What should be done in the future
- Add a “command registry” abstraction so the esp_console command list and future LVGL SplitConsole/command-palette can share a single source of truth (names/help/keywords).
- Add a small “reply” mechanism for non-screenshot commands if we later want deterministic “screen switch complete” acknowledgements (without touching LVGL from the REPL task).

### Code review instructions
- Start with `0025-cardputer-lvgl-demo/main/console_repl.cpp` (new commands + argument parsing).
- Then review `0025-cardputer-lvgl-demo/main/app_main.cpp` (CtrlEvent drain + dispatch on UI thread).
- Then review `0025-cardputer-lvgl-demo/main/demo_manager.cpp` + `0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp` (Pomodoro minutes default/apply).
- Validate:
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`
  - flash and run `menu`, `pomodoro`, `setmins 15`, `screenshot`
  - capture: `python3 tools/capture_screenshot_png_from_console.py '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' /tmp/after.png --timeout-s 30`

### Technical details
- Commands and outputs observed (REPL over USB-Serial/JTAG @ 115200):
  - `pomodoro` → `OK`
  - `setmins 15` → `OK minutes=15`
  - `basics` → `OK`
  - `menu` → `OK`
- Screenshot verification loop commands used:
  - `python3 tools/capture_screenshot_png_from_console.py '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' /tmp/cardputer_lvgl_pomodoro.png --timeout-s 30`
  - `pinocchio code professional --images /tmp/cardputer_lvgl_pomodoro.png \"OCR this screenshot and list the visible UI text (titles, labels, hints).\"`
  - `python3 tools/capture_screenshot_png_from_console.py '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' /tmp/cardputer_lvgl_pomodoro_15.png --timeout-s 30`
  - `pinocchio code professional --images /tmp/cardputer_lvgl_pomodoro.png,/tmp/cardputer_lvgl_pomodoro_15.png \"OCR both screenshots. Summarize what changed (titles, labels, hints, layout cues).\"`

### What I'd do differently next time
- Add a tiny host helper to send arbitrary console commands (not just `screenshot`) so this workflow doesn’t need ad-hoc `pyserial` snippets.

## Step 9: Add LVGL SplitConsole demo screen (output + input + scrollback)

This step adds a new LVGL demo screen called “Console”: a split-pane UI with a scrollable output area and a single-line input at the bottom. The intent is to have an on-device console surface that’s keyboard-first and can grow into a richer debugging UX (history, autocomplete, log sinks) without violating the core constraint that LVGL remains single-threaded.

The main architectural choice is to keep all UI mutation on the LVGL loop and treat the SplitConsole as just another `DemoManager` screen. For validation and automation, the host `esp_console` REPL now also exposes a `console` command that switches to this screen via the existing control-plane queue.

**Commit (code):** 3c6d8ea — "Cardputer LVGL: add SplitConsole demo screen"

### What I did
- Added `DemoId::SplitConsole` and registered it in `DemoManager` and the menu.
- Implemented `demo_split_console.cpp`:
  - read-only output `lv_textarea` with scrollbars
  - one-line input `lv_textarea` with `Enter` submit
  - in-memory scrollback (bounded line count) + follow-tail behavior based on scroll position
  - minimal v0 command parsing for on-device use (`help`, `heap`, `menu|basics|pomodoro`, `setmins`)
- Added a small UI append API (`split_console_ui_log_line`) for future UI-thread responses (not wired to global `ESP_LOG*`).
- Added an `esp_console` command `console` to open the SplitConsole screen from the host REPL (enqueues a CtrlEvent; UI thread performs the screen switch).

### Why
- The on-device split-pane console is a “debug UX multiplier”: it reduces reliance on serial monitors and makes interactive inspection possible directly on the device.
- Keeping it as a demo screen keeps lifecycle rules consistent (create screen, bind group focus, cleanup on delete).

### What worked
- Build succeeded:
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`
- Flash succeeded:
  - `./build.sh -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' flash`
- Screenshot/OCR validation confirmed the new UI text:
  - Menu screenshot includes the new entry `Console`.
  - SplitConsole screenshot shows `Console`, `lvgl console ready; type 'help'`, and the hint string `Enter submit  Tab focus  Fn+backtick menu`.

### What didn't work
- N/A (no runtime failures observed during this step)

### What I learned
- `lv_obj_get_scroll_bottom(out)` is a convenient follow-tail signal: when it reaches ~0, the textarea is scrolled to the bottom and auto-follow can re-enable.

### What was tricky to build
- Avoiding screen-switch hazards: commands that call `demo_manager_load(...)` must be the last thing done in the handler, because the current screen is deleted asynchronously.
- Keeping the textarea-based scrollback deterministic: `lv_textarea_add_text` is convenient but doesn’t support trimming head content, so we keep a bounded line deque and re-render.

### What warrants a second pair of eyes
- Confirm the scrollback strategy (re-render full text) is acceptable on-device under rapid appends, and tune limits if needed.
- Confirm focus behavior is ergonomic: `Tab` cycles output/input, and output scrolling behaves as expected with the Cardputer key mapping.

### What should be done in the future
- Add Ctrl+P palette integration by sharing a single Action/Command registry across:
  - esp_console registrations
  - SplitConsole parsing/autocomplete
  - command palette overlay
- Add command history + simple line editing UX rules (likely by porting the data model patterns from `0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp`).

### Code review instructions
- Start in `0025-cardputer-lvgl-demo/main/demo_split_console.cpp` (UI + command parsing + scrollback model).
- Then review menu integration in `0025-cardputer-lvgl-demo/main/demo_menu.cpp` and screen switching in `0025-cardputer-lvgl-demo/main/demo_manager.cpp`.
- Validate with:
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`
  - `./build.sh -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' flash`
  - `python3 tools/capture_screenshot_png_from_console.py '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' /tmp/menu.png --timeout-s 30`
  - `python3 -c 'import serial,time; ser=serial.Serial(\"/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*\",115200,timeout=0.2); ser.write(b\"console\\r\\n\"); ser.flush(); time.sleep(0.2); print(ser.read(4096).decode(\"utf-8\",errors=\"replace\")); ser.close()'`
  - `python3 tools/capture_screenshot_png_from_console.py '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' /tmp/console.png --timeout-s 30`
  - `pinocchio code professional --images /tmp/menu.png,/tmp/console.png \"OCR both screenshots. Summarize what changed (titles, labels, hints, layout cues).\"`

### Technical details
- Commands used for validation:
  - `python3 tools/capture_screenshot_png_from_console.py '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' /tmp/cardputer_lvgl_menu_console.png --timeout-s 30`
  - `pinocchio code professional --images /tmp/cardputer_lvgl_menu_console.png \"OCR this screenshot and list the visible UI text (titles, labels, hints).\"`
  - `python3 - <<'PY'\nimport serial, time\nport = '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00'\nwith serial.Serial(port, 115200, timeout=0.2) as ser:\n    ser.reset_input_buffer()\n    ser.write(b\"console\\r\\n\")\n    ser.flush()\n    time.sleep(0.2)\n    print(ser.read(4096).decode('utf-8', errors='replace'))\nPY`
  - `python3 tools/capture_screenshot_png_from_console.py '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' /tmp/cardputer_lvgl_split_console.png --timeout-s 30`
  - `pinocchio code professional --images /tmp/cardputer_lvgl_split_console.png \"OCR this screenshot and list the visible UI text (titles, labels, hints).\"`

## Step 10: Add Ctrl+P command palette overlay (ActionRegistry + LVGL modal)

This step adds a command palette overlay (Ctrl+P) to the Cardputer LVGL demo. The palette is a modal LVGL UI on `lv_layer_top()` that accepts a query, filters a small set of actions, and runs the selected action. This turns the demo catalog into something that feels closer to a “micro OS”: fast navigation and discoverability without leaving the keyboard.

The key architecture constraint remains unchanged: LVGL is single-threaded. Even though the palette runs on the UI thread, actions are still represented as `CtrlEvent`s so the same action set can be reused by host scripting (`esp_console`) without cross-thread LVGL access.

**Commit (code):** 5ed4599 — "Cardputer LVGL: add Ctrl+P command palette overlay"

### What I did
- Implemented a shared `ActionRegistry` (`ActionId`, titles, keywords) and a mapping from `ActionId` → `CtrlEvent`.
- Implemented the palette overlay module:
  - backdrop + panel on `lv_layer_top()`
  - query `lv_textarea` + 5 visible result rows
  - substring filtering over titles/keywords
  - Up/Down selection, Enter run, Esc close
  - focus capture/restore by temporarily binding the keyboard indev to a dedicated LVGL group
- Wired Ctrl+P chord detection in `app_main` using `KeyEvent.ctrl` (pre-LVGL feed), and added a `palette` host command that toggles the overlay via `CtrlEvent::TogglePalette`.
- Updated `esp_console` command registration to reuse `ActionRegistry` for navigation commands (menu/basics/pomodoro/console), while keeping special commands (`screenshot`, `setmins`, `heap`) as explicit handlers.
- Updated `0025-cardputer-lvgl-demo/README.md` with palette usage + updated command list.

### Why
- A command palette is a high-leverage UX primitive for embedded devices: it makes features discoverable and reduces “menu diving”.
- The action registry is reusable infrastructure for the next planned features (command palette, split console, and future actions).
- Keeping actions as `CtrlEvent`s preserves the “LVGL single-thread” rule across both UI and `esp_console`.

### What worked
- Build succeeded:
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`

### What didn't work
- Flash + screenshot/OCR validation was blocked in this harness because the device node disappeared mid-run.
  - Flash failure:
    - `A fatal error occurred: Could not open ... the port is busy or doesn't exist.`
  - After that, `ls -la /dev/serial/by-id` returned: `No such file or directory`, and no `/dev/ttyACM*` devices were present.

### What I learned
- Modifier-aware shortcuts like Ctrl+P are easiest to implement at the `KeyEvent` layer (pre-LVGL feed) because the current LVGL key translation intentionally drops modifier state.
- A dedicated LVGL group for a modal overlay is simpler than trying to “borrow” the demo’s group: it avoids Tab/selection routing into underlying screens.

### What was tricky to build
- Sequencing: for actions that switch demos, the safest ordering is “enqueue action, close palette, then let the UI loop process the event” so we don’t restore focus into soon-to-be-deleted objects.
- Ensuring the palette doesn’t interfere with the global “Esc to menu” behavior: the app loop bypasses the global Esc handler while the palette is open so Esc closes the palette instead.

### What warrants a second pair of eyes
- Focus restoration correctness:
  - verify `lv_obj_is_valid(prev_focused)` is sufficient for guarding stale focus pointers
  - verify no edge cases when an action switches screens immediately after closing the palette
- `ActionRegistry`/`esp_console` integration:
  - confirm the “dispatch by argv[0]” approach is acceptable and won’t create confusing error messages for future commands.

### What should be done in the future
- Add fuzzy matching (or better scoring) if simple substring filtering feels limiting.
- Add ActionRegistry reuse in the on-device SplitConsole (so typed commands and palette actions share a single source of truth).
- Once device access is stable in this harness, add screenshot/OCR regression anchors for the palette UI (e.g., `Search...`, `Open Menu`) using the existing playbook.

### Code review instructions
- Start in `0025-cardputer-lvgl-demo/main/command_palette.cpp` and review:
  - lifecycle (open/close), filtering, selection behavior, focus capture/restore
- Then review `0025-cardputer-lvgl-demo/main/action_registry.cpp` for action list and CtrlEvent mapping.
- Then review `0025-cardputer-lvgl-demo/main/app_main.cpp` for Ctrl+P chord detection + global key interactions.
- Build: `cd 0025-cardputer-lvgl-demo && ./build.sh build`
- Hardware validation (once `/dev/serial/by-id/...` exists again):
  - flash
  - run `palette` command from host REPL, then capture a screenshot and OCR it for `Search...` and action titles.

### Technical details
- Commands run (build):
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`
- Commands attempted (flash, failed due to missing device node):
  - `cd 0025-cardputer-lvgl-demo && ./build.sh -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*' flash`

### What I'd do differently next time
- Add a tiny host command to “force open palette” earlier in development (before wiring Ctrl+P) to keep screenshot/OCR regression checks fully automated.

## Step 11: Add System Monitor demo (sparklines for heap/DMA/fps-ish)

This step adds a new “System Monitor” demo screen (`SysMon`) to the LVGL demo catalog. It’s intended as a lightweight “top-like” dashboard while developing other demos: it shows free heap, DMA-capable free heap, an approximate UI loop rate, and an “fps-ish” responsiveness signal derived from LVGL handler time. The visualization is a set of small `lv_chart` sparklines updated at a ~250ms cadence.

The design deliberately keeps everything on the LVGL UI thread: sampling happens in a screen-local `lv_timer` which is deleted on `LV_EVENT_DELETE`. We do export a few timing counters from `app_main` (loop counter + LVGL handler duration) so the monitor can display a useful responsiveness metric without having to instrument LVGL internals.

**Commit (code):** c93aace — "Cardputer LVGL: add System Monitor demo screen"

### What I did
- Added `DemoId::SystemMonitor` and registered it in:
  - demo menu (`SysMon` entry)
  - demo manager (screen creation + group focus)
- Implemented `demo_system_monitor.cpp`:
  - header label with `heap/dma/loop Hz/lv us/fps`
  - three `lv_chart` sparklines (heap KiB, dma KiB, fps estimate)
  - `lv_timer` sampling at 250ms; timer deleted via `LV_EVENT_DELETE`
- Added lightweight perf counters in `app_main`:
  - `g_ui_loop_counter` incremented each loop
  - `g_lvgl_handler_us_last` / `g_lvgl_handler_us_avg` measured around `lv_timer_handler()` with EMA smoothing
- Exposed an action/command to open the screen:
  - `sysmon` in host `esp_console` (via ActionRegistry + CtrlEvent)
  - “Open System Monitor” in the Ctrl+P palette
  - `sysmon` command in the on-device SplitConsole v0 parser

### Why
- A system monitor makes performance/memory regressions obvious while iterating on UI work (especially when adding heavier features like screenshot, palette, future SD/Wi‑Fi).
- Tracking DMA free is specifically valuable for LVGL+M5GFX because DMA exhaustion tends to show up as graphics instability before general heap runs out.

### What worked
- Build succeeded:
  - `cd 0025-cardputer-lvgl-demo && ./build.sh build`

### What didn't work
- Screenshot/OCR validation was intentionally deferred for this step (per request to validate once the whole chunk is done). Hardware device nodes were also unstable earlier in this harness, so flashing is best done once `/dev/serial/by-id/...` is present again.

### What I learned
- Measuring LVGL handler duration around `lv_timer_handler()` is a pragmatic “responsiveness signal” that correlates with UI load even if it isn’t a literal display FPS.
- `lv_chart_set_next_value(...)` gives an easy scrolling sparkline without managing a ring buffer manually.

### What was tricky to build
- Avoiding cross-thread state: all LVGL object updates live in the SysMon screen timer; perf counters are simple globals updated in the same thread.
- Avoiding lifecycle hazards: screen-local `lv_timer_t` must be deleted on `LV_EVENT_DELETE` to prevent use-after-free (same pattern as earlier demos).

### What warrants a second pair of eyes
- Confirm the EMA smoothing and chart ranges are sensible for the Cardputer’s actual memory profile (heap KiB range `0..512`, dma KiB `0..128`, fps `0..60`).
- Confirm the loop counter metric remains meaningful under palette usage and screenshot capture load (no overflow/precision issues in the displayed “loop Hz”).

### What should be done in the future
- Add a Wi‑Fi panel (event-loop driven) if/when a demo enables networking (task 47).
- Add a screenshot/OCR regression anchor for the SysMon screen once device capture is stable (`System Monitor`, `heap=`, `dma=` visible).
- Consider a “reset charts” key binding (e.g., `R`) if the screen becomes a long-running dashboard.

### Code review instructions
- Start in `0025-cardputer-lvgl-demo/main/demo_system_monitor.cpp` (timer lifecycle + chart updates).
- Then review `0025-cardputer-lvgl-demo/main/app_main.cpp` (perf counters around `lv_timer_handler()`).
- Then review `0025-cardputer-lvgl-demo/main/action_registry.cpp` (sysmon action/command mapping).
- Build: `cd 0025-cardputer-lvgl-demo && ./build.sh build`

### Technical details
- SysMon sampling cadence: `lv_timer_create(..., 250, ...)`
- LVGL handler EMA: `avg = avg - (avg >> 3) + (us >> 3)`

## Quick Reference

### LVGL key mapping (proposed)

`KeyEvent.key` → `lv_indev_data_t.key`

- `"up"` → `LV_KEY_UP`
- `"down"` → `LV_KEY_DOWN`
- `"left"` → `LV_KEY_LEFT`
- `"right"` → `LV_KEY_RIGHT`
- `"enter"` → `LV_KEY_ENTER`
- `"tab"` → `LV_KEY_NEXT`
- `"esc"` → `LV_KEY_ESC`
- `"bksp"` → `LV_KEY_BACKSPACE`
- `"space"` → `' '`
- Single-character strings (e.g. `"a"`, `"A"`, `"/"`) → ASCII codepoint of the first char

### LVGL display flush (shape)

```c++
static void flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
  M5GFX& display = *static_cast<M5GFX*>(drv->user_data);
  int w = area->x2 - area->x1 + 1;
  int h = area->y2 - area->y1 + 1;
  uint32_t pixels = w * h;

  display.startWrite();
  display.setAddrWindow(area->x1, area->y1, w, h);
  display.writePixels(reinterpret_cast<lgfx::rgb565_t*>(color_p), pixels);
  display.endWrite();

  lv_disp_flush_ready(drv);
}
```

## Usage Examples

- Validate input quickly:
  - Boot the firmware, focus the textarea, press `a`, verify it appears.
  - Press `Enter` on the button, verify the bound action runs (e.g. clear textarea).

## Related

- Ticket index: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/index.md`
- Analysis plan: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/analysis/01-lvgl-on-cardputer-esp-idf-m5gfx-integration-plan.md`
