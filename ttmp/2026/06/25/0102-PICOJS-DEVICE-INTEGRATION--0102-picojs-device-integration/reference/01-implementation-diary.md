---
Title: Implementation Diary
Ticket: 0102-PICOJS-DEVICE-INTEGRATION
Status: active
Topics:
    - esp32-p4
    - quickjs
    - picocalc
    - visual-repl
    - javascript
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: Built-in home/sysmon/snake app sources and load command
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: Devkit compatibility API and widget subset
    - Path: ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/scripts/02-devkit-subset-probe.py
      Note: Probe for devkit subset apps
ExternalSources: []
Summary: Chronological diary for merging PicoJS DSL groundwork into the ESP32-P4 device firmware and adding console-driven validation.
LastUpdated: 2026-06-25T14:39:56.11679486-07:00
WhatFor: Use this to resume device-side PicoJS DSL firmware work without rediscovering merge, build, console, and serial-port details.
WhenToUse: Read before continuing any task in 0102-PICOJS-DEVICE-INTEGRATION, especially before flashing or probing the PicoCalc.
---


# Diary

## Goal

This diary records the step-by-step device-side integration of the PicoJS DSL into the ESP32-P4 PicoCalc firmware. It captures merge work, design decisions, console feedback-loop validation, build/flash/probe results, failures, and follow-up tasks.

## Step 1: Create the device integration ticket and implementation guide

I created a new docmgr ticket for the firmware-side continuation of the 0102 PicoJS work before changing device code. The guide records the current architecture, the feature branch material to merge, the intended console-first feedback loop, and the phased plan for converting the desktop native host into an ESP-IDF component.

The most important decision in this step is to make UART console observability part of the implementation contract. The LCD remains a target renderer, but `js smoke`, `screen dump`, and later `picojs dump` should let the agent validate behavior through serial output without requiring the user to correlate what appears on the physical display.

### Prompt Context

**User prompt (verbatim):** "ok, let's do the merging back into the main firmware. Create a new docmgr ticket for the whole device side work and add a detailed device implementation guide. then work on it, task by task, committing at appropriat eintervals, keeping a detailed diary."

**User prompt (verbatim):** "if possible, feedback loop using the console, so that you can work on your own without needing me to crosscorrelate with the display, for now."

**Assistant interpretation:** Create a new ticket for firmware/device-side PicoJS integration, document the implementation plan, then proceed in small validated tasks with commits and diary updates; prefer UART-console validation so device work can be checked without manual LCD inspection.

**Inferred user intent:** Move the JS/native-host groundwork into the main firmware safely and then implement the device runtime in an evidence-driven way that supports autonomous build/flash/probe debugging.

**Commit (code):** N/A — documentation/ticket setup only at this point.

### What I did

- Created docmgr ticket `0102-PICOJS-DEVICE-INTEGRATION`.
- Added `design-doc/01-device-implementation-guide.md`.
- Added `reference/01-implementation-diary.md`.
- Added five initial ticket tasks:
  - Merge portable JS and native-host groundwork into the main firmware worktree safely.
  - Add console-driven device feedback loop for scripted QuickJS/UI validation.
  - Port the native picoOS DSL API into an ESP-IDF component boundary.
  - Wire device renderer and keyboard adapters for DSL applications.
  - Build, flash, and validate on the PicoCalc through the UART console.
- Inspected the existing 0102 firmware app and components:
  - `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp`
  - `components/qjs_service/include/qjs_service.h`
  - `components/qjs_service/qjs_service.cpp`
  - `components/visual_repl/include/visual_repl.h`
  - `components/visual_repl/visual_repl.cpp`
  - `components/picocalc_keyboard/include/picocalc_keyboard.h`

### Why

- The previous feature branch is primarily desktop and host-side groundwork. The firmware integration needs its own task plan because it introduces serial probing, ESP-IDF builds, device renderer/input adapters, and native QuickJS binding lifetime constraints.
- The implementation guide prevents accidental copying of host-only `termios`/ANSI code into firmware and defines where native API code should live.
- The console feedback loop is needed to make device validation scriptable and reduce reliance on manual physical display inspection.

### What worked

- `docmgr status --summary-only` found the existing docs root and vocabulary.
- The new ticket and documents were created successfully.
- The existing firmware already has a UART console with useful commands (`status`, `lcd`, `screen`, `kbd`, `js`) and a `qjs_service_run()` API that can install native bindings on the QuickJS task.
- The current `visual_repl` model is fixed at 40 columns and 20 rows, which matches the desired console dump and the desktop DSL's 40-column display goal.

### What didn't work

- The main firmware worktree is not clean. At ticket creation time it had unrelated dirty/untracked paths, including `.playwright-mcp/`, `ttmp/vocabulary.yaml`, `esper`, `stoms3r/...`, and older ticket scratch files. This means broad staging commands such as `git add .` are unsafe.
- The path `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs` exists locally as an ignored directory in the main worktree. It conflicts with the feature branch's new submodule path and must be moved aside before merging.

### What I learned

- The firmware-side QuickJS service already has the right extension point for native DSL installation: `qjs_service_run()` executes a callback with `JSContext *` on the service task.
- `visual_repl` is currently history/input-oriented rather than frame-oriented. This is good enough for early console dump validation, but dashboard/game-style apps will likely need a dedicated row-matrix renderer later.
- The keyboard task already centralizes raw PicoCalc key events, so a semantic token adapter can be added without changing the low-level I2C keyboard component first.

### What was tricky to build

- The main worktree includes unrelated local state and an ignored QuickJS checkout. The immediate risk is not code complexity; it is accidentally overwriting or staging unrelated user work during merge and commits. The approach is to stage explicit paths only and to move the ignored QuickJS checkout to `/tmp` before the merge.
- The console feedback loop must not compete with `idf.py monitor` or any stale serial process. The plan records single-owner serial probing as a requirement before any flash/probe work.

### What warrants a second pair of eyes

- Whether the first renderer bridge should append lines to `visual_repl` history or add a frame-oriented `visual_repl_render_rows()` API immediately.
- Whether `picojs_runtime` should own all screen state independently or reuse parts of `visual_repl` as its screen model.
- Whether the first device app should be loaded from an embedded string, a paste buffer, or a console `picojs load hello` built-in.

### What should be done in the future

- Commit the ticket setup and guide.
- Move the ignored local QuickJS checkout aside and merge `feature/0102-js-scripts` into `main`.
- Add the first console-only smoke/dump commands before attempting full native DSL firmware porting.

### Code review instructions

- Start with `ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/design-doc/01-device-implementation-guide.md`.
- Verify that the guide matches the actual component APIs in `qjs_service`, `visual_repl`, and `picocalc_keyboard`.
- Validate documentation hygiene with:

```bash
docmgr doctor --ticket 0102-PICOJS-DEVICE-INTEGRATION --stale-after 30
```

### Technical details

Current main worktree status at the start of this ticket included:

```text
## main...origin/main [ahead 42]
 ? esper
 m ttmp/2026/04/22/ATOMS3R-ESPPROV--create-atoms3r-esp-idf-ble-provisioning-firmware/sources/atoms3r-esp-idf
 M ttmp/vocabulary.yaml
?? .playwright-mcp/
?? almanach-fontscale.png
?? almanach-mono.png
?? almanach-print-btn.png
?? almanach-studio-dev.png
?? stoms3r/cmd/almanach-render-service/.devctl/
?? stoms3r/cmd/almanach-render-service/.playwright-mcp/
?? stoms3r/cmd/almanach-render-service/almanach-render-service
?? stoms3r/cmd/almanach-render-service/examples/bundles/11-upcoming-horror-cats.zip
?? stoms3r/cmd/almanach-render-service/examples/bundles/11-upcoming-horror-cats/
?? ttmp/2026/06/11/
?? ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/scripts/
?? ttmp/2026/06/24/0102-JS-API-REPL-OS-SIM--0102-js-api-repl-and-quickjs-os-simulator/
```

Ignored QuickJS checkout detected before merge:

```text
0100-esp32-p4-quickjs-wasm/wasm-src/quickjs
.gitignore:14:**/wasm-src/quickjs/
```

## Step 2: Merge the host groundwork and split implementation into child tickets

I moved the ignored local QuickJS checkout out of the future submodule path, merged `feature/0102-js-scripts` back into the main firmware worktree, initialized the QuickJS submodule, rebuilt `qjs`, and reran the host-side validation scripts. I then split the remaining firmware work into child tickets so each phase can have its own design guide, diary, validation loop, and focused commit series.

This step establishes the baseline for device work. The main branch now contains the portable JS runtime, examples, bundle tools, interactive host, and desktop native QuickJS host prototype. The next implementation work should start with the `0102-PICOJS-CONSOLE-FEEDBACK` child ticket, because console observability is the prerequisite for autonomous device-side validation.

### Prompt Context

**User prompt (verbatim):** "alright, do it, and for each ticket write a design + implementation guide as usual (and upload to remarkable), then stsart working on tasks one by one, committing at appropriate intervals, and keeping a detailed diary."

**Assistant interpretation:** Create the child ticket set with design/implementation guides, publish the documentation to reMarkable, and then begin implementation phase-by-phase with commits and diary updates.

**Inferred user intent:** Manage the complex firmware integration as independently reviewable, well-documented vertical slices while preserving a single umbrella plan.

**Commit (code):** 2f180f5 — "Merge branch 'feature/0102-js-scripts'"

### What I did

- Moved the ignored local checkout:

```bash
mv 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs /tmp/quickjs-local-backup-1782424055
```

- Merged the feature branch:

```bash
git merge --no-ff feature/0102-js-scripts
```

- Initialized and built the new QuickJS submodule:

```bash
git submodule update --init 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs
make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs
```

- Ran host-side validation:

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
```

- Created child tickets:
  - `0102-PICOJS-CONSOLE-FEEDBACK`
  - `0102-PICOJS-RUNTIME-COMPONENT`
  - `0102-PICOJS-MINIMAL-DSL`
  - `0102-PICOJS-LAYOUT-WIDGETS`
  - `0102-PICOJS-INPUT-APP-MODE`
  - `0102-PICOJS-FRAME-TIMERS`
  - `0102-PICOJS-LCD-RENDERER`
- Added a design/implementation guide and diary to each child ticket.
- Added the child ticket map to the umbrella design guide.

### Why

- The ignored local QuickJS checkout had to be moved so Git could materialize the new submodule path cleanly.
- The feature branch groundwork needed to be present in the main firmware worktree before device-side implementation could begin.
- Child tickets prevent the native DSL port, renderer work, input routing, and console probing from becoming one unreviewable change set.

### What worked

- The merge completed with no source conflicts.
- QuickJS submodule initialization and `qjs` build completed successfully.
- Host validation passed:
  - `run-smoke.sh`: `SMOKE PASS 0`
  - `run-api-tests.sh`: core/screen/OS/UI tests passed
  - `run-bundle-smoke.sh`: `PASS bundle hello-api`, `PASS bundle calc`
  - `run-native-smoke.sh`: native hello/dashboard/layout passed

### What didn't work

- The main worktree still contains unrelated dirty/untracked files outside this work. This is expected from the shared workspace but remains a staging hazard.
- `docmgr ticket list --ticket <id>` returns success even when it prints `No tickets found`, so the ticket-creation script had to test output text rather than relying on the exit status.

### What I learned

- The host-side work is now validated from the main worktree, not only from the feature worktree.
- The child ticket split maps naturally to validation boundaries: console feedback, runtime skeleton, minimal DSL, layout/widgets, input/app mode, timers, and LCD rendering.

### What was tricky to build

- The merge itself was mechanically simple, but it required removing an ignored directory that Git would otherwise not replace with a submodule checkout. This is a common trap because ignored files are invisible in normal status output but still block paths during checkout/merge operations.
- The shared worktree remains dirty with unrelated artifacts, so all commits in this effort must continue to stage explicit paths.

### What warrants a second pair of eyes

- Whether the child ticket set is too granular or missing a separate QuickJS lifetime/error-hardening ticket. For now, lifetime hardening is part of the minimal DSL and frame/timer work, but it may deserve its own ticket if it grows.
- Whether `0102-PICOJS-LCD-RENDERER` should extend `visual_repl` or bypass it through direct `picocalc_lcd` row blits.

### What should be done in the future

- Upload the child ticket guides to reMarkable.
- Commit the child ticket documentation.
- Start `0102-PICOJS-CONSOLE-FEEDBACK` by implementing `js smoke` and `screen dump`.

### Code review instructions

- Review the merge commit first to confirm it contains only host-side JS/native-host groundwork.
- Then review the child ticket guides for scope boundaries.
- Re-run host validation from the main worktree before starting firmware code changes.

### Technical details

Merge commit:

```text
2f180f5 Merge branch 'feature/0102-js-scripts'
```

QuickJS local backup path:

```text
/tmp/quickjs-local-backup-1782424055
```


## Step 6: Start picoOS devkit compatibility in firmware

This step began turning the imported `picoos-devkit.jsx` parity assessment into firmware behavior. It adds a broader compatibility slice to the native PicoJS runtime: cheap app/panel/widget methods, minimal OS mock APIs, and built-in `home`, `sysmon`, and `snake` apps that exercise the new widgets.

The work intentionally remains an incremental subset rather than a full devkit port. The goal was to make several more devkit-shaped apps load and render on the hardware path before extracting a host SDL emulator.

### Prompt Context

**User prompt (verbatim):** "ok, do it all."

**Assistant interpretation:** Proceed with the recommended devkit-parity milestones, starting from cheap compatibility APIs and moving into widgets/apps before host SDL extraction.

**Inferred user intent:** Expand the firmware implementation toward the complete picoOS devkit app suite so host and hardware development can converge on one source-compatible DSL.

**Commit (code):** pending — implementation built locally; latest flash was blocked when the ESP32-P4 by-id serial device disappeared.

### What I did
- Added firmware runtime support for `App.state()`, `App.refresh()`, `App.dispatch()`, and `App.exit()` compatibility methods.
- Added `Panel.titleRight()` and `Panel.footer()`.
- Added `Text.dim()` and `Gauge.max()` / `Gauge.style()`.
- Added a generic widget class for early `spark`, `table`, `menu`, `list`, and `grid` rendering.
- Added minimal OS compatibility APIs/properties: `OS.clock()`, `OS.battery`, `OS.metrics`, `OS.history()`, `OS.processes()`, `OS.launch()`, `OS.cfg`, `OS.eval()`, plus no-op `reset/step/turn` placeholders.
- Added built-in `picojs load home`, `picojs load sysmon`, and `picojs load snake` firmware app sources.
- Expanded the interactive demo key handlers to include `up`, `down`, and `enter` in addition to `left`, `right`, and `a`.
- Added `scripts/02-devkit-subset-probe.py` for the new devkit subset.

### Why
- The parity assessment showed the host SDL emulator should not freeze today's incomplete firmware API.
- `home`, `sysmon`, and `snake` cover useful widget categories: menu/grid launcher, gauges/spark/table, and grid/layer game rendering.
- Cheap methods like `App.state()` and `Panel.titleRight()` unlock many devkit source snippets without committing to the full widget library yet.

### What worked
- Build passed with ESP-IDF 5.4.2:
  - `cd 0102-esp32-p4-visual-quickjs-repl && source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`
- Before the final title-rendering tweak, the device successfully loaded and dumped the new app subset:
  - `picojs load home` rendered the launcher/menu and battery gauge.
  - `picojs load sysmon` rendered CPU/memory/temp gauges, a sparkline, and a process table containing `kernel`.
  - `picojs load snake` rendered a grid with head/food layers and responded to `picojs key left`.
- The app binary still fits comfortably: latest build reported `0xe5ad0` bytes with about 78% of the 4 MB app partition free.

### What didn't work
- First validation expected `picoOS` in the home dump, but the top bar used a one-row framed panel. The runtime only rendered frame titles when the frame height was at least two rows, so the title was invisible. I fixed this by rendering titles/titleRight even for one-row or frameless panels.
- The final flash after that title fix could not run because the ESP32-P4 by-id device disappeared:
  - `Could not open /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 ... No such file or directory`
  - `/dev/serial/by-id` only showed the separate S3R device afterward.
- An earlier flash attempt failed because `idf_monitor` was still holding the port; I killed PID `1997757` before retrying.

### What I learned
- Several devkit methods can be compatibility no-ops as long as they preserve chaining and avoid syntax/runtime failures.
- One-row panels are common in the devkit; renderer title placement cannot assume a minimum frame height.
- The current 40x20 hardware screen truncates devkit status bars and some menu text; this is expected until we either adapt apps or add scrolling/alternate geometry.

### What was tricky to build
- The generic widget object needs to preserve method chaining for many different widget families while only rendering a subset. I used one native `PicoJSWidget` class with kind-specific rendering for spark/table/menu/list/grid.
- Callback argument compatibility matters: devkit callbacks often expect an app argument (`m => m.refresh()`) or key callback `(m, k)`. The runtime now passes an app object into timer/loop/compute callbacks and `(app, token)` into key callbacks.

### What warrants a second pair of eyes
- Review the generic widget API carefully; it is intentionally broad and may hide mistakes because many methods are no-ops.
- Review `OS.eval()` before exposing it widely. It currently delegates to QuickJS global evaluation and is only a placeholder for the devkit calculator helper.
- Review Unicode handling. The current screen stores ASCII and maps non-ASCII glyphs to `?`, so the built-in subset avoids most Unicode-heavy devkit visuals.

### What should be done in the future
- Reconnect/reset the ESP32-P4 and rerun `scripts/02-devkit-subset-probe.py` after flashing the latest build.
- Implement focus movement/activation for `menu`, `table`, and future form/input widgets.
- Add real `keypad`, `pad`, `progress`, `form`, `feed`, `input`, `editor`, and `viewer` widgets in separate reviewable slices.

### Code review instructions
- Start in `components/picojs_runtime/picojs_runtime.cpp` around `GenericWidget`, `install_os_compat()`, and the `make_widget_object()` bindings.
- Then review `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` for `kPicoJsHomeSource`, `kPicoJsSysmonSource`, `kPicoJsSnakeSource`, and the expanded `picojs load` command.
- Build with ESP-IDF 5.4.2 and validate with `ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/scripts/02-devkit-subset-probe.py` once the ESP32-P4 by-id port is visible again.

### Technical details
- Build command: `cd 0102-esp32-p4-visual-quickjs-repl && source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Flash command that failed after the board disappeared: `idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00 flash`.
- Visible devices after failure: only `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_B4:3A:45:BE:16:80-if00` was present.
