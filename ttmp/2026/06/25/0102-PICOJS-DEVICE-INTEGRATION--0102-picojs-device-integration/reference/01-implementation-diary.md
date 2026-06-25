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
RelatedFiles: []
ExternalSources: []
Summary: "Chronological diary for merging PicoJS DSL groundwork into the ESP32-P4 device firmware and adding console-driven validation."
LastUpdated: 2026-06-25T14:39:56.11679486-07:00
WhatFor: "Use this to resume device-side PicoJS DSL firmware work without rediscovering merge, build, console, and serial-port details."
WhenToUse: "Read before continuing any task in 0102-PICOJS-DEVICE-INTEGRATION, especially before flashing or probing the PicoCalc."
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
