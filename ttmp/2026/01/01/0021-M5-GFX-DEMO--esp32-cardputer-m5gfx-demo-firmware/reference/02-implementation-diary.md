---
Title: Implementation diary
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - ui
    - keyboard
    - esp32s3
    - esp-idf
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Demo-suite firmware scaffold (display init + keyboard + list view)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.h
      Note: Reusable ListView widget API (A2)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Cardputer matrix keyboard scanner used for input mapping
ExternalSources: []
Summary: Step-by-step implementation diary for the 0021 M5GFX demo-suite firmware, separate from the initial research diary.
LastUpdated: 2026-01-02T02:10:00Z
WhatFor: ""
WhenToUse: ""
---

# Implementation diary

## Step 1: Create demo-suite project scaffold + A2 list view skeleton

This step creates a new ESP-IDF project for the demo-suite and implements the first reusable UI building block: a keyboard-driven ListView (starter scenario A2). The immediate goal is a compile-ready firmware scaffold that can serve as the home/menu for all subsequent demos.

### What I did
- Created new ESP-IDF project `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite` reusing the vendored `M5GFX` component via `EXTRA_COMPONENT_DIRS`.
- Implemented `CardputerKeyboard` (matrix scan) to emit keypress edge events (W/S/Enter/Del).
- Implemented `ListView` widget with selection + scrolling + ellipsis truncation.
- Wired it together in `app_main` so the device boots into a simple menu list.

### Why
- The ticket’s tasks require a “demo catalog app”; the ListView is the core reusable widget for navigation, settings pages, and file pickers.
- Getting bring-up, input, and stable rendering working early reduces risk for all later demo modules.

### What worked
- The project follows the same M5GFX component integration pattern as tutorials `0011` and `0015` (Cardputer autodetect + sprite present + `waitDMA()`).

### What didn't work
- N/A (initial scaffold).

### What I learned
- N/A (implementation underway).

### What was tricky to build
- Key event semantics: emitting edge events based on physical key positions (not shifted glyphs) to avoid “shift changes the key name” issues.

### What warrants a second pair of eyes
- Keyboard scan correctness on real hardware (primary vs alt IN0/IN1 autodetect).
- ListView layout math (line height + padding) against the Cardputer font metrics.

### What should be done in the future
- Add a small on-screen footer that documents the current key bindings (align with the design doc’s navigation contract).
- Add a “demo details/help overlay” screen (so `Enter` opens a per-demo help page before running the demo).

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` then review:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`

## Step 2: Compile the new firmware

This step validates that the new project builds cleanly under ESP-IDF 5.4.1 and produces a flashable binary. The main outcome is a confirmed compile baseline to iterate on for the remaining starter scenarios (A1/B2/E1/B3).

### What I did
- Built the project:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `cd esp32-s3-m5/0022-cardputer-m5gfx-demo-suite`
  - `idf.py set-target esp32s3`
  - `idf.py build`

### Why
- The ticket’s “demo suite” is only useful if it compiles and can be flashed as a known-good baseline.

### What worked
- `idf.py build` succeeded and produced `build/cardputer_m5gfx_demo_suite.bin`.

### What didn't work
- Initial compilation errors caught and fixed during bring-up:
  - Ambiguous `LovyanGFX` type due to an incorrect forward declaration (fixed by including the proper header and using `lgfx::v1::LovyanGFX`).
  - `KeyPos` visibility issue (fixed by making the struct public).

### What I learned
- The M5GFX headers use an `lgfx::v1` namespace pattern; forward declaring `lgfx::LovyanGFX` is a footgun.

### What was tricky to build
- Making the UI/widget layer type-safe without accidentally shadowing library types (namespace collisions are surprisingly easy here).

### What warrants a second pair of eyes
- Confirm that the chosen `lgfx::v1::LovyanGFX` type annotation is consistent with other code in this repo and won’t make later integration (sprites, overlays) awkward.

### What should be done in the future
- Add a minimal “scene switcher” abstraction before implementing A1/B2/E1 so the launcher remains clean.

### Code review instructions
- Rebuild from scratch and confirm no local-only state is required:
  - `idf.py fullclean && idf.py build`

## Step 3: Commit the compile-ready baseline

This step records the point where the demo-suite scaffold first compiled successfully and was committed, so subsequent work (A1/B2/E1/B3) can build on a known-good baseline.

**Commit:** `4a751b7` — "✨ Cardputer: add M5GFX demo-suite scaffold"

### What I did
- Committed the new project (`0022-cardputer-m5gfx-demo-suite`) plus the `0021-M5-GFX-DEMO` ticket workspace updates.

### Why
- The fastest way to iterate is to lock in a working baseline early, then layer scenarios on top without mixing in unrelated changes.

### What worked
- Commit includes a successful `idf.py build` baseline.

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- Keeping the commit limited to the demo-suite + its ticket docs, because the repo working tree contained unrelated modifications.

### What warrants a second pair of eyes
- Whether committing the full ticket workspace docs (analysis + design + diaries) in the same baseline commit is desirable, or if future tickets prefer a docs-only commit separate from the code scaffold.

### What should be done in the future
- Keep commits per scenario small (A1, B2, E1, B3) so regressions are easy to bisect.

### Code review instructions
- Start with `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` (menu + wiring), then:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`

## Step 4: Flash to the connected Cardputer + capture boot logs

This step validates the end-to-end “host -> flash -> device runs -> serial logs” loop on real hardware. It also confirmed the stable by-id serial path for the Cardputer’s USB-Serial/JTAG interface in this environment.

### What I did
- Detected the Cardputer serial device:
  - `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00` (symlink to `/dev/ttyACM1`)
- Flashed the firmware:
  - `cd esp32-s3-m5/0022-cardputer-m5gfx-demo-suite`
  - `./build.sh -p /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00 flash`
- Started `idf.py monitor` in tmux (monitor requires a TTY) and captured boot output.

### Why
- A compile-only baseline is not enough; we need a repeatable “flash + observe” loop before implementing more scenarios.

### What worked
- Flash succeeded over USB-Serial/JTAG and the device rebooted cleanly.
- Serial logs show:
  - `M5GFX: [Autodetect] board_M5Cardputer`
  - `display ready: width=240 height=135`

### What didn't work
- Running `idf.py monitor` from a non-interactive runner fails with:
  - `Monitor requires standard input to be attached to TTY.`
  - This is expected; the workflow uses tmux for monitoring.

### What I learned
- `idf.py monitor` holds an exclusive lock on the serial port; if monitor is running, `idf.py flash` may fail with “port busy”. This shaped the “flash monitor as a single command” recommendation in the playbook.

### What was tricky to build
- Making the build helper resilient in tmux: relying on `idf.py` from PATH can lead to python/module mismatches. Using `${IDF_PATH}/tools/idf.py` via the IDF venv python is more robust.

### What warrants a second pair of eyes
- Confirm the proposed tmux workflow is the preferred house style for firmware iteration (versus a pure one-shot `flash monitor` command each time).

### What should be done in the future
- Add a small on-screen “controls footer” so the validation checklist can be performed without referencing docs.

## Step 5: Add a repeatable debug loop (build.sh + tmux playbook)

This step focuses on shortening the edit/build/flash/observe loop. The key outcome is a single entry point (`build.sh`) and a documented tmux-based workflow that works around monitor’s TTY requirement and serial-port locking constraints.

### What I did
- Added `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/build.sh`:
  - sources ESP-IDF 5.4.1 (only when needed)
  - prefers explicit `${IDF_PYTHON_ENV_PATH}/bin/python` + `${IDF_PATH}/tools/idf.py` (tmux-safe)
  - provides `tmux-flash-monitor` to run `flash monitor` in a tmux pane
- Wrote a playbook design doc:
  - `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/07-playbook-build-flash-monitor-validate.md`

### Why
- The firmware iteration loop is dominated by friction (port selection, monitor TTY, environment drift) unless we standardize a single “happy path”.

### What worked
- `flash monitor` works reliably inside tmux (real TTY), and produces expected boot logs after flashing.

### What didn't work
- Attempting to run `idf.py monitor` from non-interactive runners fails (expected).
- Running monitor and flash concurrently leads to serial-port lock conflicts; the workflow avoids this by running `flash monitor` as a single command.

### What I learned
- For this repo’s environment, “explicit python + explicit idf.py path” avoids the most common tmux/pyenv/direnv confusion.

### What was tricky to build
- Getting the tmux workflow right without accidentally creating a “monitor holds the port forever, flash always fails” loop.

### What warrants a second pair of eyes
- Whether `tmux-flash-monitor` should be the default command name, or if we want a more general `tmux-run` wrapper for future projects.

### What should be done in the future
- Add a `build.sh run` alias (or a tiny `tools/run_fw_flash_monitor.sh`) if we want “auto-pick port + flash monitor” without passing `-p` manually.

## Step 6: Implement A1 (HUD header) + B2 (perf overlay) + scene switching

This step turns the demo-suite from a single full-screen list into a small “catalog app” that matches the ticket’s UX contract: a menu-driven launcher with consistent chrome. It also implements the perf overlay early so later demos can be debugged with on-device timings.

### What I did
- Refactored rendering into layered sprites:
  - `header` (16px), `body` (content), `footer` (16px).
- Added a minimal scene switcher:
  - `Enter` opens a demo from the menu
  - `Tab` / `Shift+Tab` cycles scenes
  - `Del` returns to the home menu
- Implemented A1: HUD header bar with screen name + fps + heap/dma.
- Implemented B2: perf footer bar with EMA-smoothed totals (update/render/present).
- Flashed to the device and confirmed boot logs + display bring-up.

### Why
- A1 (chrome) and B2 (perf) are foundational. Without them, later demos are harder to validate and you end up “blind debugging” stutter/flicker.

### What worked
- `idf.py build` succeeded for the updated firmware.
- Flash succeeded once the serial port was not held by a running monitor.
- Boot logs show Cardputer autodetect and expected resolution:
  - `M5GFX: [Autodetect] board_M5Cardputer`
  - `display ready: width=240 height=135`

### What didn't work
- Flash failed once with “port is busy” when a monitor process was holding `/dev/serial/by-id/...` open. Killing the tmux session freed the port and the retry succeeded.

### What I learned
- With USB-Serial/JTAG, exclusive port locking is strict: you can’t reliably run flash and monitor concurrently in separate processes.

### What was tricky to build
- Measuring timings while still using dirty redraws: the “frame” notion becomes “a present event” (only update perf EMA when something is actually pushed).

### What warrants a second pair of eyes
- Perf accounting: confirm the definition of `present_us` (push + waitDMA) matches how you want to reason about stutter across demos.

### What should be done in the future
- Add key-repeat support for menu navigation (holding W/S) if it feels too “clicky” with edge-only events.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` and follow:
  - sprite allocation and present order
  - key mapping and scene transitions
  - HUD/perf cadence and dirty logic

## Step 7: Implement E1 terminal/log console scene

This step implements the first “real” demo module beyond the launcher: a console UI you can reuse for logs, REPL output, and interactive text input. It exercises scrolling, wrapping, and incremental redraw patterns.

### What I did
- Added a `ConsoleState` buffer with scrollback, input line, and dirty flag.
- Implemented wrapping via `textWidth()` when appending lines to the buffer.
- Rendered the console into the `body` sprite (excluding header/footer).
- Integrated the console as a real scene (`E1TerminalDemo`) in the scene switcher.
- Flashed the updated firmware to the Cardputer.

### Why
- A console is one of the most reusable UI building blocks for firmware and is a good stress test for text metrics + redraw strategies.

### What worked
- `idf.py build` succeeded.
- Flash succeeded and the device booted with expected autodetect logs.

### What didn't work
- N/A (no new blockers beyond the known “monitor holds the port” constraint).

### What I learned
- Wrapping at append-time keeps rendering predictable, but the line budget must remain bounded to prevent memory growth (hard cap at 256 lines).

### What was tricky to build
- Key binding trade-offs: on Cardputer there’s no dedicated Backspace key, so the console uses `Del` as backspace and reserves `Fn+Del` for “back to menu”.

### What warrants a second pair of eyes
- Whether `Fn+W/S` is the right scrollback control, or if you prefer a different mapping (to avoid interfering with normal typing).

### What should be done in the future
- Add optional “fast mode” (scrollRect incremental append) once the stable redraw-on-dirty path is battle-tested.

### Code review instructions
- Start with `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp` then review its integration in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`.

## Step 8: Implement B3 screenshot-to-serial (PNG framing + host capture script)

This step adds a “developer workflow” feature: capture the current UI as a PNG over USB-Serial/JTAG. This is intended for documentation, bug reports, and rapid UI iteration without taking photos.

### What I did
- Implemented device-side PNG capture using `createPng()` and a framing protocol:
  - `PNG_BEGIN <len>\\n` + raw bytes + `\\nPNG_END\\n`
- Wired global key `P` to trigger screenshot capture.
- Added a small host helper script (`pyserial`) to capture the framed PNG into a file.
- Built and flashed the firmware to the Cardputer.

### Why
- Screenshots make UI iteration and bug reporting dramatically faster than photographing the device.

### What worked
- `idf.py build` succeeded with the new USB-Serial/JTAG write path.
- Flash succeeded.

### What didn't work
- Full end-to-end host validation (press `P` + script produces a valid PNG) still needs a quick manual run, because the capture requires a physical keypress on the device.

### What I learned
- Using `usb_serial_jtag_write_bytes(...)` is mandatory for binary output; `printf`/logs will corrupt the PNG payload.

### What was tricky to build
- Avoiding serial interleaving: any log output during the raw PNG transfer can corrupt the capture. The current implementation minimizes logging in the screenshot path but does not globally silence other logs.

### What warrants a second pair of eyes
- Whether we should temporarily suppress logging around screenshot transfer (global log level / stdout lock) to make B3 more robust under noisy firmware.

### What should be done in the future
- Add a “toast” overlay after sending the screenshot (but only after the raw transfer completes).

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` and confirm:
  - framing protocol is stable
  - `free(png)` matches the allocator used by `createPng`
