---
Title: Diary
Ticket: 0023-CARDPUTER-KB-SCANCODES
Status: active
Topics:
    - cardputer
    - keyboard
    - scancodes
    - esp32s3
    - esp-idf
    - debugging
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp
      Note: Vendor matrix scan ground truth
    - Path: esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c
      Note: Raw scan_state/in_mask logging and IN0/IN1 variant handling
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Existing scanner patterns we may reuse
    - Path: esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp
      Note: GPIO0 mode toggle and on-screen prompt rendering
    - Path: esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/wizard.cpp
      Note: Wizard prompt/capture/confirm implementation
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T21:41:57.721916763-05:00
WhatFor: ""
WhenToUse: ""
---



# Diary

## Goal

Step-by-step implementation diary for the `0023-cardputer-kb-scancode-calibrator` firmware (a standalone tool firmware for discovering Cardputer keyboard matrix scancodes and Fn-combos).

## Context

- Ticket workspace: `esp32-s3-m5/ttmp/2026/01/02/0023-CARDPUTER-KB-SCANCODES--cardputer-keyboard-scancode-calibration-firmware/`
- Primary design/plan: `analysis/01-keyboard-scancode-calibration-firmware.md`
- Key prior art:
  - `esp32-s3-m5/0007-cardputer-keyboard-serial/main/hello_world_main.c` (raw scan logging)
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` (scanner patterns used elsewhere)
  - `M5Cardputer-UserDemo/main/hal/keyboard/keyboard.cpp` (vendor scan ground truth)

## Quick Reference

N/A (this diary is prose-first; copy/paste snippets live in analysis docs).

## Usage Examples

N/A (see per-step “Code review instructions”).

## Related

- Analysis/plan: `esp32-s3-m5/ttmp/2026/01/02/0023-CARDPUTER-KB-SCANCODES--cardputer-keyboard-scancode-calibration-firmware/analysis/01-keyboard-scancode-calibration-firmware.md`

## Step 1: Create the calibrator firmware scaffold + live matrix view

This step bootstraps a new, dedicated ESP-IDF firmware project for keyboard scancode calibration. The immediate outcome is a compile-ready program that brings up the Cardputer display and shows a live 4×14 matrix view, highlighting currently pressed physical keys and printing the corresponding vendor-style `keyNum` list to the serial log on change. This establishes the “ground truth” scan pipeline we can build the interactive calibration wizard on top of.

**Commit (code):** 1fc1f77 — "✨ Cardputer: add keyboard scancode calibrator scaffold"

### What I did
- Created new project: `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator`
- Implemented a raw matrix scanner with:
  - multi-key support (collect all asserted input bits across scan states)
  - alt IN0/IN1 autodetect (switches from `{13,15,...}` to `{1,2,...}` when activity is observed)
  - pressed-set outputs: `std::vector<KeyPos>` + `std::vector<uint8_t keyNum>`
- Implemented a minimal UI:
  - 4×14 grid labeled with the vendor legend
  - highlights pressed keys
  - prints `pressed: [keynums...]` in the footer and logs on change
- Added a `build.sh` helper consistent with other Cardputer projects (tmux-friendly).

### Why
- We need a standalone “measurement tool” firmware that does not depend on guessed semantic mappings (e.g. treating `W/S` as arrows). A live matrix display makes it obvious what the hardware is actually doing.

### What worked
- `./build.sh build` succeeded and produced a flashable binary.

### What didn't work
- N/A (baseline bring-up step).

### What I learned
- The simplest stable scancode representation for our codebase is vendor `keyNum` derived from `KeyPos` (4×14), not characters.

### What was tricky to build
- Cardputer has a known IN0/IN1 wiring variance; autodetect must only switch on observed activity, otherwise both pinsets read “idle high” and are indistinguishable.

### What warrants a second pair of eyes
- Validate the scan mapping math (`scan_state` + `in_mask` → `KeyPos`) against vendor implementation and physical reality on multiple devices.

### What should be done in the future
- Implement the actual calibration wizard (prompt → stable capture → confirm/redo/skip) and config export (JSON + header snippet).

### Code review instructions
- Start in `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/kb_scan.cpp`:
  - `KbScanner::scan()` mapping and autodetect behavior
- Then review the UI wiring in `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp`.

## Step 2: Flash to device + set up serial sniff loop (tmux) + extract reusable scanner component

This step makes the calibration firmware actually usable on hardware: it’s flashed to the connected Cardputer, and a durable monitoring workflow is put in place so keypresses can be “sniffed” from logs while you press keys/combo chords. It also extracts the raw matrix scan + legend into a reusable ESP-IDF component (`cardputer_kb`) so other firmwares can share the same “physical key” representation (`KeyPos`/`keyNum`) without copy/paste drift.

**Commit (code):** cdb68fa — "Cardputer: add reusable keyboard matrix scanner component"

### What I did
- Flashed the calibrator to the Cardputer using the stable by-id port:
  - `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00`
- Started a tmux-based monitor session (monitor needs a TTY and locks the port):
  - session name: `cardputer-kb-scancodes`
  - command: `./build.sh -p <port> monitor`
- Extracted a reusable component:
  - `esp32-s3-m5/components/cardputer_kb/`
  - `cardputer_kb::MatrixScanner` returns `ScanSnapshot{pressed KeyPos, pressed_keynums, use_alt_in01}`
  - `cardputer_kb::kKeyLegend` provides the 4×14 legend table for UI/debug output
- Updated the calibrator firmware to use `cardputer_kb` instead of local `kb_scan.*`.

### Why
- “Sniffing” key scancodes requires a stable host loop. In practice `idf.py monitor` must run in tmux, and it will block flashing until stopped.
- Multiple projects need consistent key identity. The reusable component is the foundation for a “decoder config” shared across firmwares.

### What worked
- Flash succeeded and boot logs appear over the USB-Serial/JTAG port.
- The live matrix UI updates as keys are pressed, and logs emit `pressed keynums=[...]` on changes.
- IN0/IN1 autodetect logs once when the alternate wiring is detected.

### What didn't work
- Fully deriving the “complete” semantic key setup (especially arrows/Fn-combos) still requires physical keypress sampling on hardware; we can’t infer it purely from static code.

### What I learned
- ESP-IDF’s serial monitor holds an exclusive lock on the device node; treat “stop monitor before flash” as a hard operational constraint.

### What was tricky to build
- Component discovery: ESP-IDF only discovers local components if they’re on the component search path; the project’s `CMakeLists.txt` must include `../components/cardputer_kb` in `EXTRA_COMPONENT_DIRS`.

### What warrants a second pair of eyes
- Confirm the component boundary is the right one (scanner + layout/legend). We may later want to add a stable “binding/decoder” API here as well.

### Code review instructions
- Start in `esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp`.
- Then see the consumer in `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp`.

### Technical details
- To sniff chords as you press keys:
  - attach: `tmux attach -t cardputer-kb-scancodes`
  - stop (to free the port for flashing): `tmux kill-session -t cardputer-kb-scancodes`

## Step 3: Add on-device wizard prompts + mode toggle on external button GPIO0

This step adds a second “mode” to the firmware: a guided wizard that prompts for specific semantic actions (Up/Down/Left/Right, Back/Esc, Enter, Tab, Space) and captures the corresponding physical chord (`required_keynums`) once it’s held stable. The goal is to stop guessing where arrows/esc live and instead record them explicitly on-device.

Mode switching is bound to the external `GPIO0` button (G0): press it to toggle between the existing live matrix mode and the new wizard mode.

### What I did
- Added a wizard state machine (`Waiting → Stabilizing → CapturedAwaitConfirm → Done`) with a default 400ms stability hold.
- Added `GPIO0` input setup + debounced edge detection to toggle modes.
- In wizard mode:
  - prompt text appears in the footer
  - after capture, `Enter=accept`, `Del=redo`, `Tab=skip`
  - when done, pressing Enter prints a JSON config blob framed by `CFG_BEGIN ... CFG_END`.

### Why
- Arrow keys and other Fn functionality are not discoverable from the key legend; they are *semantic bindings* that must be measured as physical chords on the real device.

### What worked
- Build succeeded and the firmware flashes and boots.
- tmux monitor shows mode toggle log: `mode=wizard (GPIO0 toggle)` / `mode=live (GPIO0 toggle)`.

### What warrants a second pair of eyes
- The assumption that `GPIO0` is safe to use as a runtime “mode toggle” on all target devices (it is a strapping pin).

### Code review instructions
- Start in `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/wizard.cpp`.
- Then review the GPIO0 handling and UI wiring in `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/main/app_main.cpp`.

## Step 4: Capture semantic bindings (arrows/back) and turn them into a reusable decoder table

This step takes the wizard output (the actual physical keyNum chords the user pressed for “NavUp/Down/Left/Right/Back/Enter/Tab/Space”) and makes it reusable across firmwares. The key idea is that “arrows” are not a hardware scancode on this device; they’re a semantic mapping to physical keys, so we store them explicitly as binding rules (action → required_keynums chord).

Captured wizard output (from serial):

- `NavUp` = `[40]`
- `NavDown` = `[54]`
- `NavLeft` = `[53]`
- `NavRight` = `[55]`
- `Back` = `[1]`
- `Enter` = `[42]`
- `Tab` = `[15]`
- `Space` = `[56]`

### What I did
- Added a reusable “binding decoder” API:
  - `cardputer_kb::Binding` + `cardputer_kb::decode_best()` (subset test; most-specific chord wins)
- Checked in the captured bindings in two formats:
  - JSON: `esp32-s3-m5/components/cardputer_kb/config/m5cardputer_bindings_captured.json`
  - C++ header table: `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h`
- Added a host helper to convert a captured wizard JSON blob into a header:
  - `esp32-s3-m5/0023-cardputer-kb-scancode-calibrator/tools/cfg_to_header.py`

### Why
- This turns the calibrator’s “one-off measurement” into a stable artifact that other firmware (e.g. the demo-suite) can include to get consistent navigation behavior.

### What warrants a second pair of eyes
- The captured mapping uses punctuation keys for navigation (consistent with vendor apps) rather than Fn-chords. If the goal is “real arrows via Fn”, we should run the wizard again and press Fn+<key> for the arrow prompts.

### Code review instructions
- Start in `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings.h`.
- Then inspect the captured table in `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h`.
