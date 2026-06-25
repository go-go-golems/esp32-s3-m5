---
Title: Device Implementation Guide
Ticket: 0102-PICOJS-DEVICE-INTEGRATION
Status: active
Topics:
    - esp32-p4
    - quickjs
    - picocalc
    - visual-repl
    - javascript
    - firmware
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: Current device orchestration
    - Path: components/picocalc_keyboard/include/picocalc_keyboard.h
      Note: PicoCalc key event source for future semantic DSL input adapter
    - Path: components/qjs_service/include/qjs_service.h
      Note: QuickJS service extension point for native DSL installation jobs
    - Path: components/qjs_service/qjs_service.cpp
      Note: Runtime owner
    - Path: components/visual_repl/include/visual_repl.h
      Note: Existing 40x20 visual model API and target for console dump/render integration
ExternalSources: []
Summary: Plan for merging the PicoJS DSL groundwork into the main 0102 firmware and implementing a console-driven device validation loop.
LastUpdated: 2026-06-25T14:39:55.591028613-07:00
WhatFor: Use this when continuing the 0102 device-side PicoJS DSL integration after the desktop JS/native-host groundwork.
WhenToUse: Before changing firmware code, before flashing the PicoCalc, and when deciding whether a DSL feature belongs in JS runtime code, native QuickJS bindings, the visual renderer, or the keyboard/input adapter.
---


# Device Implementation Guide

## Executive Summary

This ticket moves the PicoJS DSL work from desktop groundwork into the main `0102-esp32-p4-visual-quickjs-repl` firmware. The feature branch already contains a portable JavaScript runtime, examples, smoke tests, bundle tooling, and a desktop C++ QuickJS host prototype. The main firmware already contains an ESP32-P4 visual QuickJS REPL that initializes the PicoCalc LCD, PicoCalc keyboard, `qjs_service`, and `visual_repl`, and exposes a UART console prompt named `0102>`.

The device-side implementation should proceed in small commits. First merge the feature branch safely into `main`, without overwriting unrelated local work and without losing the existing ignored QuickJS checkout. Then add a console feedback loop that can validate JavaScript evaluation and display-side behavior through UART logs and command output, so development can continue without requiring visual cross-correlation with the LCD. Only after that should the native picoOS DSL move into an ESP-IDF component boundary and be wired to renderer/input adapters.

The guiding architecture is: JavaScript scripts describe applications; firmware-owned C/C++ code owns QuickJS lifetime, native API bindings, timers, input dispatch, screen state, rendering, and memory limits. Desktop JS remains the API reference and test oracle. Device firmware should not depend on QuickJS `std/os`, Node, browser APIs, or file-system imports.

## Current State

### Main firmware worktree

Repository worktree:

```text
/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5
```

Relevant firmware app:

```text
0102-esp32-p4-visual-quickjs-repl/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── app_main.cpp
├── partitions.csv
├── README.md
└── sdkconfig.defaults
```

`0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` currently performs these responsibilities:

1. Initializes the PicoCalc LCD through `picocalc_lcd_init()`.
2. Initializes the fixed-cell visual REPL model through `visual_repl_init()`.
3. Starts a keyboard editor task that reads `picocalc_keyboard_poll_event()` and maps key codes into a single input line.
4. Starts `qjs_service` with memory and stack limits.
5. Registers UART console commands: `status`, `lcd`, `screen`, `kbd`, and `js`.
6. Evaluates JavaScript from the LCD input line or UART `js eval` command through `qjs_service_eval()`.
7. Appends captured output/error text to `visual_repl` and rerenders the LCD.

The current firmware already gives us the right console control plane. The device work should extend it rather than replacing it.

### Existing components

| Component | Current role | Device integration implication |
|---|---|---|
| `components/qjs_service` | Owns a QuickJS runtime on a FreeRTOS task, installs `print`, `millis`, `gc`, evaluates source, runs synchronous/asynchronous jobs, enforces interrupt deadline. | Native DSL installation should run as a `qjs_service_run()` job because it needs direct `JSContext *` access on the QuickJS task. |
| `components/quickjs_native` | Vendored QuickJS sources adapted for ESP-IDF. | Firmware native bindings should include `quickjs.h` through this component, not through the desktop submodule. |
| `components/visual_repl` | Fixed 40x20 cell model rendered to 320x320 LCD rows using a small glyph renderer. | Early DSL rendering can append or blit 40-column text snapshots; later rendering can use a dedicated screen buffer and row-diff path. |
| `components/picocalc_keyboard` | I2C keyboard polling, recovery, diagnostics, and key names. | DSL input adapter should translate raw key codes to semantic tokens such as `left`, `right`, `up`, `down`, `enter`, `esc`, printable characters. |
| `components/picocalc_lcd` | RGB565 fill/rect/blit primitives for the 320x320 LCD. | If DSL rendering bypasses history, this component is the final display sink. |

### Feature branch groundwork

Feature branch:

```text
feature/0102-js-scripts
```

Relevant feature files after merge:

```text
0102-esp32-p4-visual-quickjs-repl/js/
├── lib/00-core.js
├── lib/10-screen.js
├── lib/20-os-sim.js
├── lib/30-ui-runtime.js
├── examples/*.js
├── examples-native/*.js
├── tests/*.sh
├── tools/interactive-host.js
└── tools/native-host/src/{main.cpp,pico_native_api.cpp,pico_native_api.hpp}
```

The desktop native host contains the first C++ version of the DSL API. It is not firmware-ready as-is because `tools/native-host/src/main.cpp` uses `termios`, host file I/O, stdin, and ANSI redraw. The reusable material is the API ownership model in `pico_native_api.cpp`: native objects own apps, panels, widgets, timers, and stored QuickJS values; JavaScript wrapper objects are non-owning views.

## Problem Statement

The firmware can already evaluate JavaScript and show REPL output, but it cannot yet run the PicoJS visual app DSL on the device. The missing pieces are:

1. A safe merge of the JS/native-host groundwork into the main worktree.
2. A repeatable console-only feedback loop for device validation.
3. A firmware-native PicoJS runtime component installed into the existing QuickJS service context.
4. A renderer adapter from DSL screen state to the existing visual display path.
5. An input adapter from PicoCalc key events to DSL semantic key tokens.
6. Validation that works without relying on manually comparing the physical LCD against expected screenshots.

The console feedback loop is especially important. During firmware work the agent should be able to flash the device, send UART commands, read command output/logs, and decide whether the next fix is needed. The LCD remains useful, but it should not be the only observable state.

## Proposed Solution

### Phase 1 — Merge and preserve state

Merge `feature/0102-js-scripts` into the main firmware worktree. Before merging, move the existing ignored local QuickJS checkout out of the path that becomes a git submodule:

```bash
mv 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs \
  /tmp/quickjs-local-backup-$(date +%s)
```

Then merge:

```bash
git merge --no-ff feature/0102-js-scripts
```

After merging, initialize/build the submodule for desktop smoke tests:

```bash
git submodule update --init 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs
make -C 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs qjs
```

Run the host-side checks before touching firmware integration:

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
```

### Phase 2 — Console feedback loop

Add console commands that make device state machine behavior testable from UART. The initial console feedback should not require a DSL renderer. It should prove that the console can drive QuickJS, capture output/errors, and report a compact machine-readable result.

Recommended commands:

```text
js eval <source>          # existing command; keep it human-readable
js smoke                  # new built-in QuickJS contract test
js probe <case>           # optional named probe: print, throw, timeout, heap
screen dump               # new display model dump, 40x20 text rows over UART
picojs status             # later: native DSL runtime status
picojs load <example>     # later: load embedded DSL example
picojs frame [dt_ms]      # later: advance app and print/render a frame
picojs key <token>        # later: inject semantic key token
picojs dump               # later: dump current DSL screen buffer over UART
```

The immediate target is a command sequence that a script can run over the serial port:

```text
status
js smoke
js eval print('hello-device')
js eval throw new Error('boom')
screen demo
screen dump
```

A passing console log should include enough text to determine success without reading the LCD. `screen dump` is the important bridge: it exposes the display model as ASCII rows over UART.

### Phase 3 — Firmware-native picoOS component

Create a new ESP-IDF component, tentatively named `components/picojs_runtime`, rather than putting large native binding code into `app_main.cpp`.

Proposed layout:

```text
components/picojs_runtime/
├── CMakeLists.txt
├── include/picojs_runtime.h
├── picojs_runtime.cpp
├── picojs_bindings.cpp
├── picojs_screen.cpp
└── picojs_input.cpp
```

Public C API sketch:

```c
typedef struct picojs_runtime picojs_runtime_t;

typedef struct {
    uint16_t cols;
    uint16_t rows;
    uint32_t frame_interval_ms;
} picojs_runtime_config_t;

esp_err_t picojs_runtime_create(const picojs_runtime_config_t *cfg, picojs_runtime_t **out);
void picojs_runtime_destroy(picojs_runtime_t *rt);
esp_err_t picojs_runtime_install(JSContext *ctx, picojs_runtime_t *rt);
esp_err_t picojs_runtime_frame(picojs_runtime_t *rt, uint32_t dt_ms);
esp_err_t picojs_runtime_key(picojs_runtime_t *rt, const char *token);
esp_err_t picojs_runtime_dump_text(picojs_runtime_t *rt, char *dst, size_t dst_len);
esp_err_t picojs_runtime_render_visual_repl(picojs_runtime_t *rt);
```

Install the native API through `qjs_service_run()`:

```c
static esp_err_t install_picojs_job(JSContext *ctx, void *user) {
    return picojs_runtime_install(ctx, (picojs_runtime_t *)user);
}

qjs_job_t job = {
    .fn = install_picojs_job,
    .user = g_picojs,
    .timeout_ms = 1000,
};
ESP_ERROR_CHECK(qjs_service_run(g_qjs, &job));
```

This preserves QuickJS thread ownership. Native QuickJS APIs should only run on the `qjs_service` task.

### Phase 4 — Renderer adapter

Use a two-step renderer strategy.

Step 1: text dump only. The runtime owns a 40x20 or 40x19 screen buffer and `picojs dump` prints rows over UART. This gives a deterministic feedback loop before LCD rendering.

Step 2: visual REPL integration. Convert runtime rows into either:

1. `visual_repl_append_line()` history rows; or
2. a new `visual_repl_render_rows()` API that renders a full 40x20 cell matrix without treating it as scrollback history.

Option 2 is better for apps because dashboards and games are frame-based rather than scrollback-based. Option 1 is faster to prove because the existing renderer already handles rows and input.

### Phase 5 — Input adapter

The keyboard task should eventually route keys to two modes:

- REPL edit mode: current behavior, editing `g_input` and submitting JS snippets.
- App mode: translate raw PicoCalc key codes to DSL tokens and call `picojs_runtime_key()` through a `qjs_service_post()` or a queue owned by the PicoJS runtime.

Suggested semantic tokens:

| Raw key meaning | Token |
|---|---|
| Left arrow `0xb4` | `left` |
| Right arrow `0xb7` | `right` |
| Enter `0x0a`/`0x0d` | `enter` |
| Escape `0xb1` | `esc` |
| Printable ASCII | one-character string |
| Home/Delete/End | `home`, `delete`, `end` |

Input injection must also be exposed through UART (`picojs key <token>`) so tests do not require physical keypresses.

## Design Decisions

### Decision 1: Keep `qjs_service` as the QuickJS owner

**Decision:** Install and run native DSL bindings inside the existing `qjs_service` task using `qjs_service_run()`/`qjs_service_post()`.

**Rationale:** `qjs_service` already owns runtime creation, memory limits, stack limits, interrupt deadlines, output capture, and status reporting. Creating a second QuickJS owner would duplicate lifetime and memory policy.

**Consequence:** The PicoJS runtime API must be asynchronous-aware. Any call that touches `JSContext *` must execute on the service task.

### Decision 2: Add console dump/probe commands before LCD-only app rendering

**Decision:** Implement UART-observable `js smoke`, `screen dump`, and later `picojs dump` commands before relying on LCD output.

**Rationale:** A console feedback loop lets an agent validate behavior through serial output and logs. This reduces ambiguity during device work and avoids requiring the user to visually correlate the screen.

**Consequence:** Display models must expose a text snapshot. This is also useful for regression tests and bug reports.

### Decision 3: Port native DSL bindings as a component, not as app glue

**Decision:** Create `components/picojs_runtime` for firmware-native DSL state and QuickJS bindings.

**Rationale:** `app_main.cpp` is already responsible for device orchestration and console commands. Adding hundreds of lines of QuickJS class bindings there would make the firmware hard to review and hard to reuse.

**Consequence:** The first component commit should define a narrow public C API before porting the full widget set.

### Decision 4: Treat desktop native host as a reference, not firmware source

**Decision:** Reuse concepts and selected code from `js/tools/native-host/src/pico_native_api.cpp`, but do not copy `main.cpp` and do not assume desktop ownership rules are sufficient.

**Rationale:** The desktop host uses host terminal APIs and process lifetime assumptions. Firmware has FreeRTOS tasks, persistent runtime sessions, memory pressure, and different render/input paths.

**Consequence:** The port must explicitly handle callback storage, exception capture, and reset teardown.

## Alternatives Considered

### Bundle the JavaScript runtime into firmware first

This would load `00-core.js`, `10-screen.js`, `20-os-sim.js`, and `30-ui-runtime.js` into QuickJS and run examples unchanged. It is a useful compatibility test, but it is not the target architecture. It consumes memory, leaves more runtime behavior in JavaScript, and delays the native binding work needed for device UI integration.

### Bypass `visual_repl` and draw directly to LCD immediately

Direct LCD drawing is likely the final high-performance path, but it is not the best first device step. `visual_repl` already has a fixed-cell renderer, palette, and UART-visible status. A text dump and/or row-render bridge gives faster validation.

### Keep only the LCD as the feedback channel

This is rejected for this ticket. The user explicitly asked for a console feedback loop if possible. UART observability should be part of the implementation contract.

## Implementation Plan

### Task 1 — Merge feature branch safely

1. Record current main status in the diary.
2. Move the ignored local `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs` checkout aside.
3. Merge `feature/0102-js-scripts` into `main` with `--no-ff`.
4. Initialize the QuickJS submodule and run desktop tests.
5. Commit only intentional merge/doc state.

Validation:

```bash
git status --short --branch
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
```

### Task 2 — Add console smoke and screen dump

1. Add `js smoke` to `cmd_js`.
2. Add a `visual_repl_dump_text()` API or equivalent console-only helper.
3. Add `screen dump` to print 20 fixed-width rows over UART.
4. Add a host-side script under this ticket's `scripts/` directory to open the serial port, issue commands, and capture output.

Validation:

```text
0102> js smoke
js smoke: PASS ...
0102> screen dump
[00] ...
...
[19] > ...
```

### Task 3 — Create `picojs_runtime` skeleton

1. Add component directory and public header.
2. Add create/destroy/status/dump functions that do not yet touch QuickJS.
3. Wire the component into `0102-esp32-p4-visual-quickjs-repl/CMakeLists.txt` and `main/CMakeLists.txt`.
4. Add `picojs status` and `picojs dump` console commands.

Validation:

```text
0102> picojs status
picojs: initialized=1 cols=40 rows=20 apps=0 frames=0
```

### Task 4 — Install native API into QuickJS

1. Port the minimal `OS.app`, `App.panel`, `Panel.text`, `app.mount`, and `picojs frame` path.
2. Install the API through `qjs_service_run()` after service startup.
3. Add a small embedded JavaScript smoke program or console command that evaluates a compact app.
4. Dump rows over UART.

Validation:

```text
0102> picojs load hello
0102> picojs frame 1000
0102> picojs dump
... HELLO ...
```

### Task 5 — Add device input injection and keyboard adapter

1. Add `picojs key <token>` console command.
2. Route keyboard events to app mode when a PicoJS app is mounted.
3. Keep an escape path back to REPL edit mode.
4. Validate with an app whose state changes on `left`, `right`, and printable keys.

### Task 6 — Build, flash, and console-probe loop

1. Source ESP-IDF 5.4.2.
2. Build the 0102 project.
3. Flash through `/dev/serial/by-id/...` if available.
4. Run the serial probe script.
5. Record exact build/flash/probe output in the diary.

Commands:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/serial/by-id/<p4-uart> flash
```

## Console Feedback Loop Design

The feedback loop should be scriptable. A probe script should:

1. Open the UART port as the single owner.
2. Wait for the `0102>` prompt.
3. Send one command per line.
4. Capture output until the next prompt.
5. Assert substrings for pass/fail.
6. Close the port cleanly.

Initial probe cases:

| Command | Required evidence |
|---|---|
| `status` | Contains `quickjs:`, `visual:`, and heap line. |
| `js smoke` | Contains `js smoke: PASS`. |
| `js eval print('hello-device')` | Contains `hello-device`. |
| `js eval throw new Error('boom')` | Contains `error:` and `boom`; command may return nonzero status text but prompt must recover. |
| `screen demo` | Contains `screen demo: ESP_OK`. |
| `screen dump` | Contains row markers `[00]` and `[19]`. |

The script must not compete with `idf.py monitor` or another serial process on the same port. If prompt detection fails because the device needs a reset, stop and ask the operator to press reset rather than repeatedly opening the port.

## Review and Validation Checklist

Before each commit:

```bash
git diff --check
git status --short
```

For docs:

```bash
docmgr doctor --ticket 0102-PICOJS-DEVICE-INTEGRATION --stale-after 30
```

For host JS/native work after merge:

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-bundle-smoke.sh
0102-esp32-p4-visual-quickjs-repl/js/tests/run-native-smoke.sh
```

For firmware builds:

```bash
cd 0102-esp32-p4-visual-quickjs-repl
source ~/esp/esp-idf-5.4.2/export.sh
idf.py build
```

For device validation:

```bash
python3 ttmp/2026/06/25/0102-PICOJS-DEVICE-INTEGRATION--0102-picojs-device-integration/scripts/01-console-probe.py \
  --port /dev/serial/by-id/<p4-uart>
```

## Risks

- The main worktree contains unrelated dirty/untracked files. Stage paths explicitly and never use broad `git add .`.
- `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs` already exists locally as an ignored directory and conflicts with the new submodule path. Move it aside before merge.
- The repository has older gitlinks without `.gitmodules` entries. Full `git submodule status` may report unrelated problems; initialize only the QuickJS submodule needed by 0102 unless explicitly cleaning all submodules.
- QuickJS native binding errors can corrupt long-running device sessions if `JSValue` ownership is wrong. Treat every stored callback and every `JS_Call` result as review-critical.
- `visual_repl` is scrollback-oriented. Frame-based apps may need a row-matrix rendering API to avoid turning each frame into history.
- The ESP32-P4 console uses UART0 through CH343. Do not apply ESP32-S3 USB-Serial/JTAG console assumptions.

## Open Questions

1. Should the first firmware DSL app load from an embedded string, from UART paste, or from a compact `picojs load hello` built-in?
2. Should app mode replace the REPL input row, or should the REPL remain available as an overlay/escape mode?
3. Should the first renderer bridge use `visual_repl_append_line()` for speed or add a dedicated frame renderer immediately?
4. How much of `20-os-sim.js` should become device-native OS APIs versus remaining desktop-only simulation?

## References

- `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` — current app orchestration, console commands, keyboard edit loop, and QuickJS eval bridge.
- `components/qjs_service/include/qjs_service.h` — public QuickJS service API including `qjs_service_run()` and `qjs_service_post()`.
- `components/qjs_service/qjs_service.cpp` — QuickJS runtime owner, global installation, interrupt deadlines, eval output capture.
- `components/visual_repl/include/visual_repl.h` — 40x20 fixed-cell visual model public API.
- `components/visual_repl/visual_repl.cpp` — current row renderer and history/input model.
- `components/picocalc_keyboard/include/picocalc_keyboard.h` — keyboard event and diagnostic API.
- `0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/pico_native_api.cpp` — desktop native DSL binding reference after merge.
