---
title: Investigation Diary
doc_type: reference
ticket: M5STACKCHAN-BLINKY
topics:
  - m5stackchan
  - firmware
  - custom-app
  - blinky
  - intern-guide
status: active
---

# Diary: Blinky Custom App Implementation Guide

## Goal

Create a detailed analysis, design, and implementation guide for building and flashing the Blinky custom app, targeting a new intern. Store in the M5STACKCHAN-BLINKY ticket and upload to reMarkable.

## Step 1: Research and Write the Guide

I read the previous diary (9 steps covering ESP-NOW research and firmware build), the full developer guide (18 sections, 1500 lines), and key source files to produce a comprehensive intern guide.

### Prompt Context

- **User prompt (verbatim):** "Creat a new ticket to build and flash the blinky example, read the previous diary and the previous developer guide, then Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."
- **Assistant interpretation:** Create a new docmgr ticket, write a comprehensive intern-ready guide covering system architecture, Mooncake internals, HAL, LVGL threading, LED API, Blinky design with pseudocode and diagrams, then upload to reMarkable
- **Inferred user intent:** Produce a standalone, self-contained guide that a new team member can follow to build their first custom app on the StackChan platform

### What I did

1. Created ticket M5STACKCHAN-BLINKY with topics: m5stackchan, firmware, mooncake, custom-app, esp32-s3, blinky, intern-guide
2. Created design doc: `01-blinky-custom-app-analysis-design-and-implementation-guide.md`
3. Created reference doc: `01-investigation-diary.md` (this file)
4. Read the full developer guide (1500 lines) for precise API references and command examples
5. Read the full ESP-NOW research diary (9 steps) for context on previous work
6. Read key source files for precise line references:
   - `ability.h` — AppAbility class, AppInfo_t struct, State_t enum
   - `app_template.h/.cpp` — official template app
   - `main.cpp` — app_main entry point, app installation
   - `apps.h` — app includes
   - `main/CMakeLists.txt` — GLOB_RECURSE build system
   - `hal_mcp.cpp` — discovered the actual LED API: `GetStackChan().leftNeonLight().setColor(r, g, b)` (NOT `GetHAL().setLedColor()`)
7. Wrote the guide: 15 sections, ~40KB

### Why

The developer guide has a Blinky example in section 16.8, but it uses `GetHAL().setLedColor()` which doesn't exist in the actual codebase. The real API is `GetStackChan().leftNeonLight().setColor()` and `GetStackChan().rightNeonLight().setColor()`. This intern guide provides the correct, verified API with full system context.

### What worked

- The MCP tool definition in `hal_mcp.cpp` was the key to finding the real LED API — it's a complete working example of `setColor()` usage
- The `ability.h` header is very well-documented with Chinese/English comments explaining each ability type
- The `app_template` provides a minimal reference that's easy to compare against

### What didn't work

- The developer guide's Blinky example (section 16.8) uses the wrong API (`GetHAL().setLedColor()`) — this needs to be corrected

### What I learned

- The LED ring is split into two `CircularStrip` instances (left and right), each controlling 6 WS2812C LEDs
- The safe RGB range is 0–168 (not 0–255) to avoid brownout resets from current draw
- The `LvglLockGuard` is from `smooth_ui_toolkit`, not from LVGL itself
- The `onRunning()` callback runs at the speed of the main loop (no fixed frame rate) — timed behavior requires `millis()` checks
- The `userData` void pointer in `AppInfo_t` is used as a `uint32_t*` for theme color — this is a StackChan convention, not a Mooncake requirement

### What was tricky to build

- Finding the correct LED API required tracing through `hal_mcp.cpp` → MCP tool → `GetStackChan().leftNeonLight().setColor()`. The API is not documented anywhere in the HAL header
- The neon light has two halves that must both be set — setting only one side would look broken
- The `static` keyword is required for both the icon and theme color variables because the `AppInfo_t` stores a `void*` pointer that must remain valid for the app's lifetime

### What warrants a second pair of eyes

- The `#include <stackchan/stackchan.h>` path needs verification — the actual include path depends on how the build system resolves the StackChan subsystem headers
- The `lv_obj_delete()` call in `onClose()` — verify this is the correct LVGL v9 API (v8 used `lv_obj_del()`)
- The home indicator's `update_home_indicator()` must be called every frame — if it's missed, the spring animation won't work

### What should be done in the future

- Correct the developer guide's Blinky example (section 16.8) to use the real LED API
- Actually build and flash the Blinky app to verify all code compiles and runs
- Test the home indicator swipe-up gesture
- Add a custom icon for Blinky using the LVGL Image Converter

### Code review instructions

- Read the full guide: `design/01-blinky-custom-app-analysis-design-and-implementation-guide.md`
- Cross-reference section 8.3 (implementation) with `hal_mcp.cpp` lines 72-88 for the LED API
- Cross-reference section 3.2 (AppAbility API) with `ability.h`
- Cross-reference section 7.4 (pseudocode) with `app_template.cpp` for structural consistency

### Technical details

- Guide: 15 sections, ~40KB
- Key correction: LED API is `GetStackChan().leftNeonLight().setColor(r,g,b)` + `GetStackChan().rightNeonLight().setColor(r,g,b)`, not `GetHAL().setLedColor(r,g,b)`
- Guide sections: Executive Summary, System Architecture, Mooncake Internals, HAL & StackChan Subsystem, Common UI, Icon & Theme, Design, Implementation, Build/Flash/Verify, Extension Ideas, Key File Reference, API Quick Reference, App Template Appendix, Partition Layout Appendix, Neon Light Internals Appendix

## Step 2: Fix Blinky LED Control After Hardware Verification

The first flashed Blinky implementation proved the app registration and LVGL label path, but it did not visibly drive the physical RGB LEDs. The original attempt used `GetStackChan().leftNeonLight().setColor()` and `rightNeonLight().setColor()`, which writes into the StackChan neon-light animation abstraction rather than immediately pushing RGB bytes to the hardware. That made the test harder to reason about and depended on the broader StackChan update path.

I replaced the LED control with the direct HAL RGB API used by the official M5Stack Arduino RGB LED example: set all 12 LED indices through `GetHAL().setRgbColor(index, r, g, b)`, then call `GetHAL().refreshRgb()`. This matches the documented hardware contract: 12 RGB LEDs total, indices 0-5 on the left and 6-11 on the right, with 168 used as the safe brightness maximum.

### Prompt Context

**User prompt (verbatim):** "Your little brother has been struggling with the LED ring. Analyze what they did and fix it."

**Assistant interpretation:** Debug the previous Blinky implementation, identify why the visible LEDs were not toggling, implement a simpler and more hardware-direct fix, rebuild, flash, and verify logs.

**Inferred user intent:** Get the actual StackChan RGB LEDs toggling on hardware, not just the screen label.

### What I did
- Read the current `app_blinky.cpp` implementation.
- Identified that it used `NeonLight::setColor()` plus `GetStackChan().update()`.
- Replaced that with `set_all_rgb_leds()`, which loops over LED indices 0-11, calls `GetHAL().setRgbColor()`, then calls `GetHAL().refreshRgb()`.
- Rebuilt successfully: `stack-chan.bin binary size 0x39bdc0 bytes`, 27% app partition free.
- Flashed successfully to `/dev/ttyACM0`; flash completed with hash verification and hard reset.
- Started monitor and saw Blinky open as app id 8 with no WDT/panic/error immediately after launch.

### Why
- The official RGB LED docs use direct indexed RGB writes followed by refresh.
- For a smoke-test app, direct hardware control is clearer than the higher-level animated neon-light abstraction.
- Removing `GetStackChan().update()` avoids invoking avatar/motion/modifier updates from a small app that only wants LEDs.

### What worked
- Build completed with no compile errors.
- Flash completed successfully.
- App opens from launcher as app id 8.
- Serial log shows `[Blinky] on open` and no immediate watchdog/panic/errors.

### What didn't work
- Earlier versions either did not visibly toggle LEDs or triggered watchdog stalls around the animated home indicator path.
- The previous mental model was wrong about LED location: they are the two rows of RGB LEDs near the screen/touch board, not a base ring.

### What I learned
- Official docs state there are 12 RGB LEDs; indices 0-5 are left and 6-11 are right.
- `GetHAL().setRgbColor()` only stages the LED color; `GetHAL().refreshRgb()` is required to push buffered colors to hardware.
- The StackChan `NeonLight` abstraction animates toward a target color, which is fine for production animations but less direct for a hardware test.

### What was tricky to build
- The name `leftNeonLight/rightNeonLight` suggested a direct LED driver, but it is actually an animation layer over the low-level RGB API.
- The previous home-indicator integration caused WDT symptoms, so the test app now uses a simple LVGL quit button instead.

### What warrants a second pair of eyes
- Physical LED confirmation is still required from the operator.
- Once direct LED toggling is confirmed, revisit whether a lock-safe home indicator pattern can be restored.

### What should be done in the future
- Update the intern guide to prefer direct RGB HAL calls for first hardware tests.
- Add a second Blinky variant that demonstrates `NeonLight` animation intentionally.
- Fix/document the home-indicator watchdog issue separately.

### Code review instructions
- Start at `build/firmware/main/apps/app_blinky/app_blinky.cpp`, especially `set_all_rgb_leds()` and `onRunning()`.
- Validate with `idf.py build`, `idf.py -p /dev/ttyACM0 flash`, then open Blinky from the launcher.
- Expected behavior: label toggles LED: ON/OFF and the two rows of RGB LEDs near the screen toggle amber/off.

### Technical details
- Build log: `/tmp/stackchan-blinky-direct-rgb-build.log`
- Flash log: `/tmp/stackchan-blinky-direct-rgb-flash.log`
- Monitor log: `/tmp/stackchan-blinky-direct-rgb-monitor.log`
- Direct RGB API: `GetHAL().setRgbColor(index, r, g, b)` + `GetHAL().refreshRgb()`
