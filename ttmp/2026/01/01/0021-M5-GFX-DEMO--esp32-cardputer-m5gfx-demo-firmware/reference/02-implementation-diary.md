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
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp
      Note: Demo-suite firmware scaffold (display init + keyboard + list view)
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp
      Note: Cardputer matrix keyboard scanner used for input mapping
    - Path: esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/ui_list_view.h
      Note: Reusable ListView widget API (A2)
    - Path: esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md
      Note: Bug report ticket created after real-device WDT during screenshot send
ExternalSources: []
Summary: Step-by-step implementation diary for the 0021 M5GFX demo-suite firmware, separate from the initial research diary.
LastUpdated: 2026-01-02T02:20:00Z
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

## Step 9: Close out doc-only tasks (inventory + UX scope)

This step updates the ticket task list to reflect that the “inventory” and “firmware UX/scope” work is already captured in the existing analysis/design docs, so implementation can focus on remaining gaps (assets + additional demo modules).

### What I did
- Marked these tasks as complete in the ticket task list:
  - “Finish exhaustive M5GFX feature inventory”
  - “Define demo-suite firmware scope and UX”

### Why
- The work exists in the ticket workspace already; leaving those unchecked made it unclear what “next” should be.

### What worked
- N/A (documentation bookkeeping).

### What didn't work
- N/A.

### What I learned
- N/A.

### What was tricky to build
- Avoiding “task drift”: checking items off only after confirming there are no lingering TODOs/TBDs in the docs.

### What warrants a second pair of eyes
- Confirm the inventory doc is truly complete enough for your intended “coverage” definition, and whether any missing demos should be added to the plan.

### What should be done in the future
- Decide the asset strategy next (SD vs embedded) before implementing image-heavy demos, to avoid rework.

## Step 10: Bug report — screenshot-to-serial can WDT if USB-Serial/JTAG driver is not initialized

This step records a real-device failure encountered while using the B3 screenshot feature. The key outcome is that the current `serial_write_all()` implementation can spin forever when `usb_serial_jtag_write_bytes()` returns `<= 0`, and that failure mode can spiral into watchdog resets due to log spam and IDLE starvation.

I opened a dedicated bug ticket with a detailed reproduction hypothesis and pointers for a fix: `0024-B3-SCREENSHOT-WDT`.

### What I did
- Captured the reported console output showing repeated `usb_serial_jtag_write_bytes(...): The driver hasn't been initialized` and a subsequent Task WDT backtrace into:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`
  - `serial_write_all()`
- Created the bug ticket + bug report doc:
  - `esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md`

### Why
- The screenshot feature is meant to accelerate UI iteration and debugging; if it can brick the device loop, it becomes too risky to use during development.

### What worked
- The backtrace clearly implicates the screenshot path and provides line numbers in our code (`screenshot_png.cpp:13`, `:31`).

### What didn't work
- The current screenshot send loop can be an infinite retry loop when the USB-Serial/JTAG driver is unavailable, and it has no backoff/yield.

### What I learned
- `usb_serial_jtag_write_bytes()` can fail in a way that is not “temporary” (driver not initialized). In that case, retrying forever is incorrect and can cascade into WDT resets.

### What was tricky to build
- This is a cross-layer failure (ESP-IDF console config + driver init + app-level retry logic). The “right” fix likely touches both Kconfig choices and app behavior (guard, bounded timeouts, yielding).

### What warrants a second pair of eyes
- Decide whether we should:
  - explicitly install/guard the USB-Serial/JTAG driver in the app (recommended for B3), or
  - change the demo-suite’s console configuration so the driver is always installed, or
  - both.

### What should be done in the future
- Fix `serial_write_all()` to fail fast on permanent errors and yield/bound retries on transient “0 bytes written” conditions.
- Add a small on-screen toast indicating screenshot send success/failure (after the send completes).

### Code review instructions
- Start with `esp32-s3-m5/ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--cardputer-screenshot-png-to-usb-serial-jtag-can-wdt-when-driver-uninitialized/analysis/01-bug-report-usb-serial-jtag-write-bytes-not-initialized-busy-loop-causes-wdt-during-screenshot.md` and then inspect:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` (`serial_write_all`)
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/sdkconfig` (`CONFIG_ESP_CONSOLE_*`)

### Technical details
- The reported error/wdt sequence includes:
  - `usb_serial_jtag_write_bytes(269): The driver hasn't been initialized`
  - `task_wdt: ... IDLE0 (CPU 0)`

Additional log context from a later repro suggests this often happens after navigating to the “B3 Screenshot” scene (then triggering the screenshot send):

- `cardputer_m5gfx_demo_suite: open: idx=4 scene=B3 Screenshot`
- followed by repeated `usb_serial_jtag_write_bytes(...): The driver hasn't been initialized`

## Step 11: Fix B3 screenshot-to-serial WDT (install USJ driver + chunked writes + bounded retries)

This step implements the fix described in the `0024-B3-SCREENSHOT-WDT` ticket. The key outcome is that the screenshot send path can no longer busy-loop forever when `usb_serial_jtag_write_bytes()` fails, and it now writes the PNG in small chunks so large payloads don’t fail as a single ringbuffer item.

**Commit (code):** da2f85f — "Fix: harden screenshot PNG send over USB-Serial/JTAG"

### What I did
- Updated `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`:
  - install USB-Serial/JTAG driver if missing (`usb_serial_jtag_driver_install`)
  - chunk the send (`kTxChunkBytes=128`)
  - treat negative return values as fatal (no infinite retry)
  - bound 0-byte writes with a retry budget + `vTaskDelay(1)` yield
- Rebuilt and flashed the firmware.

### Why
- The prior implementation could wedge the main loop and trip Task WDT when the driver was not initialized (or when the ringbuffer couldn’t accept a large item).

### What worked
- Build succeeded and flash worked after stopping an existing monitor process that had the serial port locked.

### What didn't work
- Full end-to-end screenshot capture (press `P` + host capture script produces a valid PNG) still needs a manual validation run.

### What I learned
- `usb_serial_jtag_write_bytes()` returns an `esp_err_t` (negative) when the driver is not installed; a `<= 0` retry loop is unsafe unless negative is treated as fatal.

### What was tricky to build
- The driver uses an internal ringbuffer; `usb_serial_jtag_write_bytes(buf, huge_len, ...)` is not safe because it attempts to enqueue a single large ringbuffer item.

### What warrants a second pair of eyes
- Confirm the chosen chunk size and retry budgets are reasonable for the expected host throughput and don’t introduce “mysterious partial screenshots” under load.

### What should be done in the future
- Add an on-screen toast after screenshot send success/failure (after the raw transfer completes), so users don’t need to look at the host terminal.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` and review:
  - `ensure_usb_serial_jtag_driver_ready()`
  - `serial_write_all()`
  - `kTxChunkBytes`

## Step 12: Switch demo-suite input to `cardputer_kb` (Fn-chord nav + Fn+1 Esc, Fn+Del menu)

This step removes the demo-suite’s ad-hoc Cardputer keyboard scanner and replaces it with the reusable `cardputer_kb` component. The outcome is that navigation is consistent with the physical keyboard reality (arrows are Fn-chords), and we can share bindings across firmwares.

**Commits (code):**
- 91c94f0 — "Input: use cardputer_kb + fn esc/del bindings"
- f2161b0 — "Input: map Esc to Fn+1"

### What I did
- Updated `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite` to include and require `cardputer_kb`.
- Reimplemented `CardputerKeyboard` in `main/input_keyboard.cpp` as a thin adapter:
  - calls `cardputer_kb::MatrixScanner::scan()`
  - uses `cardputer_kb::decode_best()` with `kCapturedBindingsM5Cardputer` to emit semantic events:
    - `up/down/left/right`
    - `esc` (now `Fn+1`)
    - `del` (now `Fn+Del`)
  - still emits raw “typing” keys as 1-character strings for the terminal demo, and maps the physical `del` key to `bksp`
- Updated `main/app_main.cpp`:
  - menu navigation recognizes `up/down/left/right` events
  - `esc`/`del` both return to the menu
  - terminal scrollback uses `up/down` (instead of `Fn+w/s`)
- Flashed the new firmware to the connected Cardputer (`/dev/ttyACM0` via `/dev/serial/by-id/...`).

### Why
- Cardputer “arrow keys” are not standalone physical keys; they are Fn-combos. The demo suite should reflect that so navigation works for users without memorizing letter fallbacks.
- We now have a reusable scanner + binding table so future firmwares don’t copy/paste matrix code.

### What worked
- `./build.sh build` succeeded after the refactor.
- Flash succeeded once the serial port was not held by an existing monitor process.

### What didn't work
- End-to-end screenshot capture validation still needs a coordinated run (host capture script + press `P` on device). My automated host-side wait timed out when no screenshot was triggered.

### What I learned
- A “best match binding” layer is a clean way to unify Fn+key navigation without losing raw keys for text entry.

### What was tricky to build
- Avoiding duplicate events: when a chord binding matches, we must not also emit raw key events for the chord members.

### What warrants a second pair of eyes
- Confirm the chosen defaults in `cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h` match the intended UX contracts across demos (especially `Fn+1` for Esc/back and `Fn+Del` for menu).

### What should be done in the future
- Add a small on-screen “controls footer” that shows the current navigation chords (so users don’t need the playbook open).

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp` and then review:
  - `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings_m5cardputer_captured.h`
  - `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/bindings.h`
  - `esp32-s3-m5/components/cardputer_kb/include/cardputer_kb/scanner.h`

## Step 13: Fix screenshot crash: stack overflow in `main` during `createPng`

This step responds to a new real-device failure: triggering the screenshot send (`P`) can overflow the FreeRTOS `main` task stack and reboot. The fix avoids patching M5GFX/miniz by running the screenshot encode+send in a dedicated task with a larger stack.

### What I did
- Created ticket `0026-B3-SCREENSHOT-STACKOVERFLOW` with a bug report + investigation notes.
- Implemented a task-based screenshot sender in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`:
  - `createPng()` and the USB-Serial/JTAG transfer run in a worker task with a larger stack
  - `main` blocks until completion to avoid concurrent display access

### Why
- `LGFXBase::createPng()` (M5GFX) calls miniz’ deflate encoder (`tdefl_*`) which is likely stack-heavy.
- The crash explicitly reports a `main` task stack overflow, so moving the heavy work out of `main` is the most direct mitigation.

### What worked
- Build succeeded after the change.

### What didn't work
- Full end-to-end validation still depends on pressing `P` on-device while the host capture script is running.

### What I learned
- “Convenient” library helpers can hide large stack requirements; treating `main` as a thin control loop is safer.

### What was tricky to build
- Ensuring the display bus isn’t used concurrently while `readRectRGB()` is fetching rows for PNG encoding.

### What warrants a second pair of eyes
- Confirm the worker task stack size is sufficient; if crashes persist, increase it and/or switch to a non-deflate debug format.

### What should be done in the future
- Add a UI toast for screenshot success/failure so validation is visible without host tools.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`.

## Step 14: Add real demos: C1 Plasma + C2 Primitives

This step starts fulfilling the “demo modules” part of the ticket by adding two actual content scenes to the demo suite: a plasma effect (ported from tutorial `0011`) and a static primitives showcase. The immediate goal is to move beyond placeholder screens and validate that the scene infrastructure supports both animated and static renderers.

**Commit (code):** 25d9dbd — "Demo: add plasma and primitives scenes"

### What I did
- Added C1 Plasma demo:
  - Ported the `sin8`/palette precompute + `draw_plasma()` algorithm from `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`.
  - Implemented it as `PlasmaState` + `plasma_render()` in:
    - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_plasma.cpp`
    - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_plasma.h`
  - Integrated into the scene switcher and menu as `SceneId::C1PlasmaDemo`, and marked the body as dirty every frame to animate.
- Added C2 Primitives demo:
  - Implemented `primitives_render()` in:
    - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_primitives.cpp`
    - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_primitives.h`
  - Renders a small set of representative 2D primitives (rect/line/circle/triangle) and static labels.
- Wired both into the build:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/CMakeLists.txt`
- Updated 0021 task list to track these as explicit checkboxes:
  - `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/tasks.md`

### Why
- Plasma is a “real” pixel effect that exercises:
  - direct access to the canvas backbuffer (`getBuffer()`),
  - consistent present timing (`waitDMA()` in the main loop),
  - perf overlay behavior under continuous redraw.
- Primitives is a simple baseline to validate draw APIs and colors without worrying about animation timing or per-pixel loops.

### What worked
- `./build.sh build` succeeds after adding the new demos.

### What didn't work
- N/A in this step (no device flash requested).

### What I learned
- Keeping animated demos as “always dirty” scenes is an easy integration path; later we can refine this into a per-scene `tick()` contract that declares its desired frame rate.

### What was tricky to build
- Ensuring plasma writes into the correct pixel format: the body sprite must remain 16-bit and we must treat its buffer as `uint16_t*` in row-major order.

### What warrants a second pair of eyes
- Whether we want to clamp plasma’s redraw rate (currently it redraws every loop iteration for that scene) or let the global `vTaskDelay(10)` be the cap.

### What should be done in the future
- Add additional demos incrementally from the remaining categories: fonts/text, sprites/rotation, image decode, QR/widgets.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp` (scene wiring), then review:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_plasma.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_primitives.cpp`

## Step 15: Fill placeholder bodies for A1/B2/B3 scenes

This step removes the confusing “TODO: implement demo module” placeholder content for the A1/B2/B3 scenes by giving each its own body renderer. The goal is that every menu entry now displays something meaningful even if it’s primarily an overlay feature (HUD/perf) or a host-integrated action (screenshot).

### What I did
- Added A1 body renderer:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_a1_hud.cpp`
  - Shows current HUD/perf toggle state and live `HudState` values.
- Added B2 body renderer:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_b2_perf.cpp`
  - Shows simple bar visualizations for avg render/present/total, plus last/avg numbers.
- Added B3 body renderer:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_b3_screenshot.cpp`
  - Shows the serial framing protocol and host script usage; tracks last send status.
  - `Enter` on the B3 scene triggers screenshot send (in addition to global `P`).
- Wired renderers into the scene dispatch:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`

### Why
- Previously, A1/B2/B3 were “implemented” but still displayed a placeholder body, which makes it look unfinished.
- Giving these scenes a dedicated body makes the suite feel coherent and reduces “what does this menu item do?” friction.

### What worked
- `./build.sh build` succeeds after the changes.

### What didn't work
- N/A (no device flash requested for this step).

### What I learned
- “Overlay features” still benefit from a body page that documents the controls and shows the live data feeding the overlay.

### What was tricky to build
- Keeping B3’s status reporting simple without introducing extra protocol or requiring the host capture tool to be running.

### What warrants a second pair of eyes
- Whether we want B3 to *only* send when host is connected (and show a toast otherwise), or keep the current “best effort” behavior.

### What should be done in the future
- Add additional demo bodies for the remaining categories (fonts, sprites/transforms, image decode, QR/widgets) so task [6] can be completed fully.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`, then review:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_a1_hud.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_b2_perf.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_b3_screenshot.cpp`

## Step 16: Implement D1-D3 text demos (fonts/metrics, wrap/ellipsis, UTF-8 sanity)

This step implements the first “text/fonts” tranche of the decomposed demo-module backlog by adding three new scenes:

- D1: builtin fonts showcase + simple metrics
- D2: wrapping vs ellipsis patterns (small-screen text constraints)
- D3: UTF-8 / symbols sanity check (glyph coverage varies by font)

### What I did
- Added new scenes and menu entries:
  - `SceneId::D1TextFontsDemo`, `SceneId::D2TextWrapDemo`, `SceneId::D3TextUtf8Demo`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`
- Implemented D1 renderer:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_d1_text_fonts.cpp`
  - Shows `Font0/Font2/Font4` samples and `textWidth`/`fontHeight` metrics.
- Implemented D2 renderer:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_d2_text_wrap.cpp`
  - Shows a “hard wrap” box and a “single-line ellipsis” box.
- Implemented D3 renderer:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_d3_text_utf8.cpp`
  - Shows accented Latin, arrow symbols, and a few math/misc glyphs.
- Wired new sources into the build:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/CMakeLists.txt`
- Built successfully:
  - `cd esp32-s3-m5/0022-cardputer-m5gfx-demo-suite && ./build.sh build`

### Why
- These demos make text behavior explicit (width, wrap, truncation, glyph coverage), which is essential for designing any UI on a 240×135 display.

### What worked
- Build passes; new scenes render via the same scene dispatch used by the other demos.

### What didn't work
- N/A in this step (no device flash requested).

### What I learned
- With `-std=gnu++2b`, `u8"..."` is `char8_t*` and doesn’t match LovyanGFX’s `const char*` APIs; plain UTF-8 string literals are the simplest approach here.

### What was tricky to build
- Keeping D1 readable within the small body height (header/footer consume 32px).

### What warrants a second pair of eyes
- D2 wrap behavior is byte-based (not Unicode grapheme-safe). That’s okay for the demo, but if we later build production UI wrapping, we should decide on a UTF-8 aware strategy.

### What should be done in the future
- Next text-related demos should exercise datums/alignment more thoroughly and include a “scrollable text view” widget.

### Code review instructions
- Start in `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/app_main.cpp`, then review:
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_d1_text_fonts.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_d2_text_wrap.cpp`
  - `esp32-s3-m5/0022-cardputer-m5gfx-demo-suite/main/demo_d3_text_utf8.cpp`
