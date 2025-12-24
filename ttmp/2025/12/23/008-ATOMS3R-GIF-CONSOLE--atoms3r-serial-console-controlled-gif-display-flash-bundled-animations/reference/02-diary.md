---
Title: Diary
Ticket: 008-ATOMS3R-GIF-CONSOLE
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - display
    - m5gfx
    - animation
    - gif
    - assets
    - serial
    - usb-serial-jtag
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-23T15:00:50.269180196-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Capture a step-by-step narrative of scoping and documenting an AtomS3R firmware feature: select and play flash-bundled animations (originating from real GIFs) via a serial console command interface.

## Step 1: Create ticket 008 + seed docs (architecture + asset pipeline)

This step created the ticket workspace and the two core documents we’ll use to consolidate the design: one document focuses on the on-device architecture (display bring-up, serial command loop, playback engine), and the other focuses on the “how do we get from GIFs to flash assets?” pipeline.

### What I did
- Created ticket `008-ATOMS3R-GIF-CONSOLE`
- Created documents:
  - `analysis/01-brainstorm-architecture-serial-controlled-animation-playback-on-atoms3r.md`
  - `reference/01-asset-pipeline-gif-flash-bundled-animation-playback.md`
  - `reference/02-diary.md`

### Why
- Keep the work grounded in repo “known-good” AtomS3R display and asset patterns before implementing new firmware.

### What worked
- Ticket and docs created cleanly via `docmgr`.

### What didn't work
- N/A

### What I learned
- The repo has strong AtomS3R display ground truth already (`0008`/`0009`/`0010`), plus an existing “assetpool partition + parttool.py write_partition” pattern we can reuse for bundling animations.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

## Step 2: Write up AssetPool, esp_console, and button control patterns (non-library research)

This step captured the most reusable “plumbing patterns” we already have in the repo that will make the eventual AtomS3R animation player straightforward: the **AssetPool flash-mapped blob pattern**, the **`esp_console` REPL pattern**, and a safe **button→next animation** control path that doesn’t violate the “single writer owns the display” invariant.

The key outcome is that we can now talk concretely about how assets are stored and addressed (struct fields vs directory tables), and we can describe control inputs (console + button) without hand-waving.

### What I did
- Expanded the architecture doc with:
  - an explicit `esp_console` section (what it provides and why it’s worth using)
  - a “next animation” button plan (GPIO ISR + queue pattern)
  - an AssetPool deep-dive (what it is, how it’s mmapped, how assets are addressed)
- Updated the asset pipeline doc to clarify the difference between “AssetPool blob” and “directory of animations”

### Why
- These details are the difference between “we think we can do it” and “we have a credible plan that won’t collapse during implementation.”

### What worked
- The repo already contains a clean AssetPool mapping example (partition find + `esp_partition_mmap` + struct pointer injection).
- The repo already vendors a helper for `esp_console` REPL initialization over multiple transports.

### What didn't work
- N/A

### What I learned
- AssetPool is fundamentally **field-addressed**, not name-addressed; for animation selection we must add an explicit directory (or use a filesystem).

### What was tricky to build
- Writing the AssetPool section in a way that preserves the intent (“simple, mmap, no FS”) while making its tradeoffs explicit (“no directory, size coupling”).

### What warrants a second pair of eyes
- Button GPIO choice defaults for AtomS3R variants (BOOT button vs a vendor `BUTTON_A` pin) and how to make that configurable cleanly.

### What should be done in the future
- N/A (remaining work is tracked as ticket tasks)

## Step 3: Start implementation Phase A (mock animations + button + console)

This step begins the firmware work for ticket 008 by building a “control-plane-first” MVP: **mock animations** (colored squares / simple patterns) that can be selected via **serial console commands** and advanced via a **hardware button**. The point is to validate the concurrency model (single display owner) and the user interaction loop before introducing real GIF decoding or asset bundling.

### What I did
- Created a new tutorial chapter scaffold: `esp32-s3-m5/0013-atoms3r-gif-console/` by copying the known-good M5GFX canvas chapter (`0010`)
- Began refactoring it toward:
  - mock animation registry (`list`/`play`/`stop`/`next`/`info`)
  - button ISR → queue → playback controller (debounced)
  - `esp_console` REPL over USB Serial/JTAG

### Why
- This isolates the “interaction + scheduling” risk early, while the rendering payload remains trivial and deterministic.

### What worked
- The `0010` display bring-up + canvas present pattern provides a stable base to build on.

### What didn't work
- Initial build failed due to incorrect assumptions about `esp_console` header names (ESP-IDF 5.4.1 exposes REPL APIs in `esp_console.h`, not `esp_console_repl.h`). Fixing this is part of the next step.

### What I learned
- Even when using `esp_console`, ESP-IDF header/API names differ across versions; using the concrete ESP-IDF 5.4.1 headers avoids “cargo-cult includes”.

### What should be done in the future
- Finish compiling `0013` and then validate on hardware (serial commands + button cycling).

## Step 4: Fix console crash on hardware (ESP-IDF `esp_console` double-init) + speed up boot sanity test

This step responds to the first real-device test of `0013`. The device booted, created the canvas, and initialized the button, but crashed when starting the USB Serial/JTAG REPL with `ESP_ERR_INVALID_STATE`.

The root cause was a subtle ESP-IDF contract: `esp_console_new_repl_usb_serial_jtag()` **already initializes** `esp_console`. Our code also called `esp_console_init()` beforehand, causing a double-init and the invalid-state error.

### What I did
- Updated `0013` console bring-up to:
  - call `esp_console_new_repl_usb_serial_jtag()` first
  - only register commands after REPL creation
  - avoid `ESP_ERROR_CHECK` hard-crash for console start (log + continue instead)
- Reduced the visual sanity test delay from 500ms per color to ~120ms per color

### Why
- We need a stable “control plane” MVP (console + button) before adding GIF decoding.
- Faster boot makes iteration/testing much easier during this phase.

### What worked
- Using the actual ESP-IDF 5.4.1 source (`components/console/esp_console_repl_chip.c`) made the correct init sequence unambiguous.

### What didn't work
- The initial console init path assumed a separate `esp_console_init()` call was required (it wasn’t for USB Serial/JTAG REPL).

### What should be done next
- Re-flash `0013` and validate:
  - prompt appears (e.g. `gif>`)
  - `list/play/stop/next/info` work
  - button press triggers `next` without resetting

## Step 5: Rebuild 0013 in a fresh shell (ESP-IDF env) + record a clean compile in git

This step made the current status unambiguous: the Phase A “mock GIF console” tutorial (`esp32-s3-m5/0013-atoms3r-gif-console/`) **builds cleanly** against ESP-IDF 5.4.1 when the environment is exported correctly. This is important because earlier “build failures” were environment problems (missing `IDF_PATH` / toolchain on PATH), not actual code issues.

I also committed the small `sdkconfig` delta produced by `idf.py set-target esp32s3`, so we have a reproducible “known-good” target configuration in git.

**Commit (code):** a7df43aab69aa77a2a7b8987886d29c0fc131664 — "0013: set target esp32s3"

### What I did
- Exported ESP-IDF 5.4.1 in the shell:
  - `source ~/esp/esp-idf-5.4.1/export.sh`
- Ensured the tutorial target is set correctly:
  - `idf.py set-target esp32s3`
  - Note: this ran `fullclean` and renamed the prior `sdkconfig` to `sdkconfig.old`
- Confirmed a clean compile:
  - `idf.py build`
- Committed the regenerated `sdkconfig` (only) in the `esp32-s3-m5` git repo.

### Why
- We want “Phase A is done” to mean **it builds**, not “it built on one machine once.”
- `idf.py set-target` is an easy place for silent drift: if target isn’t pinned, later work can fail in confusing ways.

### What worked
- `idf.py build` completed successfully and produced:
  - `build/atoms3r_gif_console_mock.bin`
  - app size report: `0x5f980` bytes, leaving ~91% free in the smallest 4MB app partition.

### What didn't work
- Earlier attempts to rebuild via `ninja` (without sourcing ESP-IDF) failed with missing includes like `/tools/cmake/project.cmake` and missing compilers (`xtensa-esp32s3-elf-gcc` not in PATH). These were environment issues, resolved by exporting ESP-IDF.

### What I learned
- When you see ESP-IDF CMake includes resolve to `/tools/...`, it’s a strong signal you’re not in an ESP-IDF-exported shell (or you exported the wrong one).
- `idf.py set-target` is destructive-ish (it triggers `fullclean` and renames the previous `sdkconfig`), so treat it as an intentional step worth recording.

### What was tricky to build
- N/A (this was mostly environment + build hygiene).

### What warrants a second pair of eyes
- Confirm the chosen default button GPIO (`CONFIG_TUTORIAL_0013_BUTTON_GPIO`, default 41) matches the actual AtomS3R variant in use. If it’s wrong, the console should still work, but the “next” button UX will be dead.

### What should be done in the future
- Flash + validate on hardware (prompt + commands + button) to finish Phase A acceptance.
- Consider adding a short note in the tutorial README about the `sdkconfig.old` artifact created by `set-target` (so people don’t accidentally commit it).

### Code review instructions
- Start with `esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp`:
  - `console_start_usb_serial_jtag()`
  - command handlers (`cmd_list`, `cmd_play`, `cmd_next`, ...)
  - button ISR → queue → debounce path
- Then check `esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild` for the knobs we rely on.

### Technical details
- Commands run (from `esp32-s3-m5/0013-atoms3r-gif-console/`):
  - `source ~/esp/esp-idf-5.4.1/export.sh`
  - `idf.py set-target esp32s3`
  - `idf.py build`

## Step 6: Document running `esp_console` over AtomS3R GROVE UART (G1/G2)

This step answered a concrete workflow question for ticket 008: “is `esp_console` stuck on USB Serial/JTAG, or can we run it on the GROVE port UART?” The key outcome is that the REPL transport is a build/runtime choice in ESP-IDF 5.x: we can run the console over **USB Serial/JTAG** *or* over an external **UART**, and AtomS3R’s GROVE port exposes two GPIOs (`G1/G2`) that are suitable for that UART wiring.

I intentionally grounded this in repo-local evidence (ESP-IDF helper code + an existing project’s `sdkconfig`) and then updated the playbook so future work can copy/paste a known-good configuration.

### What I did
- Read the existing playbook:
  - `playbooks/01-esp-console-repl-usb-serial-jtag-quickstart.md`
- Confirmed repo-local “official” usage patterns:
  - `echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/console_simple_init/console_simple_init.c`
    - shows `ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT()` + `esp_console_new_repl_uart(...)`
- Confirmed the Kconfig knob names for UART console pins/baud by inspecting an existing project:
  - `ATOMS3R-CAM-M12-UserDemo/sdkconfig` (has `CONFIG_ESP_CONSOLE_UART_CUSTOM=y`, `CONFIG_ESP_CONSOLE_UART_NUM=...`, `..._TX_GPIO`, `..._RX_GPIO`, `..._BAUDRATE`)
- Looked up AtomS3/AtomS3R GROVE `G1/G2` mapping in M5Stack docs (external source)
- Updated the playbook to include:
  - GROVE `G1/G2` → `GPIO1/GPIO2` mapping + wire colors
  - minimal `sdkconfig.defaults` snippet for `CONFIG_ESP_CONSOLE_UART_CUSTOM`
  - UART adapter wiring notes (TX↔RX crossing, 3.3V TTL)
  - added ExternalSources links for the above

### Why
- We want the “control plane” (REPL) to be flexible:
  - USB Serial/JTAG is great during dev, but an external UART is sometimes more stable / integrates better with other hardware.
- AtomS3R has a convenient GROVE port; documenting it avoids repeated pinout rediscovery.

### What worked
- Repo helper code demonstrates the intended API surface: `esp_console_new_repl_uart()` exists and is used in upstream components.
- Existing `sdkconfig` in this repo shows the real Kconfig option names for UART console pin/baud selection.

### What didn't work
- N/A (this was research + documentation)

### What I learned
- `esp_console` REPL is transport-agnostic in ESP-IDF 5.x (USB Serial/JTAG, UART, USB CDC).
- For AtomS3R, GROVE `G1/G2` map to `GPIO1/GPIO2`, and **RX/TX direction is determined by configuration**, not by the labels themselves.

### What was tricky to build
- Avoiding “cargo-cult UART init”: the clean path is to let `ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT()` pick up Kconfig (pins/baud/UART num), rather than hand-installing drivers.

### What warrants a second pair of eyes
- Confirm, on real AtomS3R hardware, that `GPIO1/GPIO2` are free in the final firmware configuration (no peripheral conflicts), and that the chosen UART number (UART1 vs UART2) matches expectations.

### What should be done in the future
- If we decide we need **logs on USB** but **REPL on GROVE UART**, capture that as a deliberate design choice and document the exact IDF settings (secondary console / log routing). N/A for now.

### Code review instructions
- Start with the updated playbook:
  - `playbooks/01-esp-console-repl-usb-serial-jtag-quickstart.md`
- Then inspect the repo helper that informed the API usage:
  - `echo-base--openai-realtime-embedded-sdk/components/esp-protocols/components/console_simple_init/console_simple_init.c`

## Step 7: Deep-dive `esp_console` internals (ESP-IDF 5.4.1) and write a contributor guidebook

This step answered a more “systems” question that came up naturally once we started using `esp_console` as a control-plane tool: where exactly is the global state, what is per-task vs global in ESP-IDF, and what does that mean for running multiple REPLs at once. Rather than relying on folklore (or web snippets that contradict each other), I read the ESP-IDF 5.4.1 source directly and then wrote a guidebook that points to the exact files and symbols.

The key outcome is clarity: `esp_console` is implemented as a **global command engine** plus a **convenience REPL task**. The “new_repl” helpers call the global init and the delete helpers call the global deinit, which makes “two concurrent `esp_console_new_repl_*` REPLs” unsupported out of the box.

### What I did
- Located the actual ESP-IDF checkout used by our `0013` build by parsing `compile_commands.json`.
- Read core implementation files in ESP-IDF 5.4.1:
  - `$IDF_PATH/components/console/commands.c`
  - `$IDF_PATH/components/console/esp_console_common.c`
  - `$IDF_PATH/components/console/esp_console_repl_chip.c`
  - `$IDF_PATH/components/console/linenoise/linenoise.c`
  - `$IDF_PATH/components/esp_vfs_console/vfs_console.c`
  - `$IDF_PATH/components/esp_driver_uart/src/uart_vfs.c`
  - `$IDF_PATH/components/esp_driver_usb_serial_jtag/src/usb_serial_jtag_vfs.c`
- Wrote a new guidebook document for contributors:
  - `reference/03-esp-console-internals-guidebook.md`
- Added a reusable ticket script:
  - `scripts/extract_idf_paths_from_compile_commands.py`

### Why
- We need a trustworthy mental model of `esp_console` to make good architectural decisions (especially around multi-port console I/O and avoiding invalid-state crashes).
- Having a guidebook drastically lowers the barrier for future contributors to make safe changes in this area.

### What worked
- `commands.c` makes global state and init semantics unambiguous (`s_tmp_line_buf` is the “already initialized” sentinel).
- `esp_console_repl_task()` shows explicitly how the REPL uses `stdin/stdout` (and where per-task rebinding happens for non-default UART channels).
- `esp_vfs_console` provides a clean alternative for “output to two ports” without trying to run two REPLs.

### What didn't work
- My first attempt to fetch ESP-IDF sources via GitHub “tag” URLs returned 404 (the repo layout/refs didn’t match the assumed paths). The local ESP-IDF checkout used by our builds is the correct source of truth.

### What I learned
- The `esp_console` module core is global (command registry + parse scratch buffer + config).
- `linenoise` also uses global state (completion callbacks, history, dumb-mode).
- The REPL “all-in-one” helpers are example-oriented by design; they make strong lifecycle choices (init/deinit) that aren’t compatible with multiple concurrent REPL instances.

### What was tricky to build
- Separating “what’s global” from “what’s per-task” without oversimplifying. ESP-IDF uses Newlib reentrancy so `stdin/stdout` can be per-task, but the console command engine itself is still global.

### What warrants a second pair of eyes
- The multi-REPL conclusion: while global init/deinit strongly suggests “not supported”, it’s worth double-checking whether ESP-IDF has any officially supported patterns for multiple interactive endpoints (beyond `/dev/console` secondary output).

### What should be done in the future
- If we truly need two independent interactive endpoints, treat it as a deliberate design/engineering effort:
  - either implement a second lightweight parser loop for the second transport, or
  - propose an upstream ESP-IDF change (refcounted init/deinit, re-entrant parsing, clarified per-REPL vs global semantics).

### Code review instructions
- Start with `reference/03-esp-console-internals-guidebook.md` and verify that each claim is backed by a specific file/symbol.

## Step 8: Fix GROVE UART console selection (REPL helper gated by ESP-IDF console channel choice)

This step fixed a confusing real-device symptom: after selecting “UART on GROVE” in the tutorial settings, the firmware still printed `W ... UART console support is disabled in sdkconfig (CONFIG_ESP_CONSOLE_UART_*); no console started` and no REPL prompt appeared on the GROVE UART. The device itself was healthy (display, canvas, button, storage mount all worked), but the console subsystem wasn’t being started.

The crux is that ESP-IDF’s `esp_console_new_repl_uart()` helper is **compiled only when the global ESP-IDF “Channel for console output” choice is set to UART** (default or custom). Our tutorial-level “binding” choice initially didn’t enforce that, so it was easy to end up with a mismatch: tutorial wants UART, but ESP-IDF is still configured for USB Serial/JTAG console output, so UART REPL support is compiled out.

### What I did
- Reproduced the on-device warning and captured the relevant snippet:
  - `W (956) atoms3r_gif_console: UART console support is disabled in sdkconfig (CONFIG_ESP_CONSOLE_UART_*); no console started`
- Read ESP-IDF Kconfig and implementation sources to confirm the gating:
  - `$IDF_PATH/components/console/esp_console_repl_chip.c`
    - `esp_console_new_repl_uart()` is guarded by `#if CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM`
  - `$IDF_PATH/components/esp_system/Kconfig`
    - the console channel is a `choice` (`choice ESP_CONSOLE_UART`), so we cannot reliably “select” it from project Kconfig
- Attempted an initial workaround: manually registering `/dev/uart` VFS + spawning a custom REPL task.
  - This was a wrong direction for our goals: it made the code more complex and drifted away from the standard ESP-IDF REPL helper.
  - It also caused build errors (missing UART types / `uart_port_t` vs `int`) which reinforced that we were rebuilding the helper in userland.
- Reverted back to the standard approach:
  - Keep using `esp_console_new_repl_uart()` / `esp_console_new_repl_usb_serial_jtag()`
  - Add **Kconfig constraints** so the tutorial binding options are only selectable when the corresponding ESP-IDF console channel is enabled.
- Updated the tutorial Kconfig and rebuilt:
  - `idf.py reconfigure && idf.py build`

### Why
- Our objective is a clean compile-time switch between USB Serial/JTAG and GROVE UART using standard ESP-IDF mechanisms.
- A “manual REPL” workaround makes maintenance harder and increases risk (especially for future ESP-IDF upgrades).

### What worked
- The source-level confirmation removed ambiguity: the helper really is compiled out unless `CONFIG_ESP_CONSOLE_UART_*` is enabled.
- Using `depends on ESP_CONSOLE_UART_CUSTOM` / `depends on ESP_CONSOLE_USB_SERIAL_JTAG` in our tutorial Kconfig prevents mismatched selections.
- After reverting to the helper-based implementation, the project builds cleanly again.

### What didn't work (failures worth recording)
- Trying to “select the ESP-IDF console channel choice symbol” from project Kconfig:
  - ESP-IDF warns that selecting a `choice` symbol has no effect.
- The “manual REPL task” workaround:
  - Increased complexity and created compilation issues; ultimately not needed for our intended workflow.

### What I learned
- ESP-IDF’s REPL convenience helpers (`esp_console_new_repl_*`) have compile-time guards that reflect the **global console output channel choice**, not just whether the `console` component is present.
- Because the channel is a `choice`, the right way to keep UX clean is:
  - make our tutorial options depend on the underlying IDF selection, and
  - set sane defaults in `sdkconfig.defaults` (but remember: existing `sdkconfig` files won’t be overwritten automatically).

### What was tricky to build
- “Two layers of configuration” can easily drift:
  - tutorial-level “binding” (our Kconfig)
  - ESP-IDF-level “Channel for console output” (IDF Kconfig choice)
  When they disagree, compilation guards silently remove the REPL helper and you only see the failure at runtime.

### What warrants a second pair of eyes
- Confirm the UX in `menuconfig` feels right:
  - If a user wants GROVE UART but hasn’t set the ESP-IDF console channel to Custom UART, the tutorial option should be unavailable (or clearly explained).

### What should be done in the future
- Consider adding a short “first-time setup” note in the tutorial README:
  - when switching console transports, run `idf.py menuconfig` and check `Channel for console output`, because an existing `sdkconfig` overrides `sdkconfig.defaults`.

### Code review instructions
- `esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild`:
  - `choice TUTORIAL_0013_CONSOLE_BINDING` and its `depends on` constraints
- `esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp`:
  - `console_start_uart_grove()` uses `esp_console_new_repl_uart()`
  - `console_start_usb_serial_jtag()` remains the USB path

## Step 9: Add a non-invasive UART RX heartbeat (prove whether bytes are arriving on GROVE RX)

This step addressed the next debugging fork after the GROVE UART REPL started emitting output: **output works, but input appears dead** (typing from a Cardputer serial console produces no visible action on the AtomS3R). At this point it’s easy to waste time in higher-level parsing assumptions, but the first question is more basic:

> Is the AtomS3R *physically* seeing activity on the UART RX pin at all?

To answer that without disturbing `esp_console` or consuming bytes, I added a “UART RX heartbeat” which counts GPIO edges on the configured RX pin (default: `GPIO2` / GROVE `G2`). This gives an immediate signal:

- **edges stay at 0** while typing: wiring / ground / wrong pin / TX↔RX not crossed
- **edges increase** while typing: electrical RX is alive; focus shifts to baud/UART selection/line endings/REPL binding

### What I did
- Added a new tutorial Kconfig debug option:
  - `CONFIG_TUTORIAL_0013_CONSOLE_RX_HEARTBEAT_ENABLE` (default `y` when GROVE UART binding is selected)
  - `CONFIG_TUTORIAL_0013_CONSOLE_RX_HEARTBEAT_INTERVAL_MS` (default `1000`)
- Implemented a GPIO ISR edge counter on the selected RX GPIO:
  - `GPIO_INTR_ANYEDGE` on `CONFIG_TUTORIAL_0013_CONSOLE_UART_RX_GPIO`
  - counters: total edges + rises + falls (UART idle is high, so traffic should generally show both)
- Added a low-priority FreeRTOS task that logs once per interval:
  - `uart rx heartbeat: rx_gpio=<N> level=<0|1> edges=<total> (d=<delta>) rises=<d> falls=<d>`
- Rebuilt `0013` successfully with the heartbeat enabled.

### Why
- Reading from UART in a second task would race/steal bytes from `esp_console` and make the situation harder to reason about.
- Edge counting is intentionally “dumb” but reliable: it tells us whether there’s real signal on the wire.

### What worked
- The heartbeat is non-invasive and should not interfere with the UART driver’s RX path (we never call `uart_read_bytes` or touch the UART driver at all).
- Logging a delta per interval provides a clear “yes/no” signal about RX activity.

### What didn't work
- N/A (this is additive instrumentation)

### What I learned
- For UART debugging, it’s valuable to separate “bytes arriving electrically” from “bytes being parsed”.
- A GPIO-edge heartbeat is a fast way to validate wiring assumptions before diving into line-ending or REPL issues.

### What was tricky to build
- Ensuring the heartbeat doesn’t perturb the UART: using GPIO interrupts (not UART RX reads) avoids consuming bytes.
- Making the ISR counter safe: use atomic increments in the ISR so the task can read consistent counters without locks.

### What warrants a second pair of eyes
- Confirm that enabling a GPIO interrupt on the RX pin does not cause issues on any AtomS3R board variants (it *shouldn’t*, but worth validating).

### What should be done in the future
- If we confirm edges are present but the REPL still doesn’t respond, add a *separate* “application-level echo” debug mode (not enabled by default) that prints raw received bytes **only when esp_console is disabled**, to avoid interference.

### Code review instructions
- Start here:
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/Kconfig.projbuild` (Console Debug menu)
  - `esp32-s3-m5/0013-atoms3r-gif-console/main/hello_world_main.cpp`:
    - `uart_rx_heartbeat_start()`
    - `uart_rx_edge_isr()`
    - `uart_rx_heartbeat_task()`

## Step 10: Regression notes (RX heartbeat toggle changes console behavior + resets persist)

After adding the RX heartbeat, we got new field observations while toggling the feature:

- **RX heartbeat disabled**:
  - Build compiles.
  - The REPL behavior is inconsistent across runs/attempts:
    - In one attempt: the console “worked”, but keystrokes appeared repeated (likely terminal-side local echo).
    - In later attempts: the console “didn’t work” (user could see output but input/commands weren’t accepted, or the prompt didn’t behave interactively).
- **RX heartbeat enabled**:
  - Build compiles.
  - The device still resets (software restart signature).
  - The serial console becomes worse (in some runs we don’t even get interactive serial working / the expected prompt/input path is not stable).

This suggests we have **two separate issues** that accidentally coupled during instrumentation:

1. **The reset/restart issue** is likely independent of the REPL transport and is still present even if the UART REPL can sometimes work.
2. **The RX heartbeat instrumentation itself** can change behavior, either by:
   - perturbing the RX pin’s configuration (pin mux / pull-up / GPIO interrupt routing), or
   - creating an interrupt load pattern that changes timing/latency of the REPL task.

At this point, the diary branches into a “back to fundamentals” analysis (see the analysis document added next), because continuing to iterate on instrumentation without a clear model risks chasing symptoms.

## Context

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
