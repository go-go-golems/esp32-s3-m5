---
Title: JS API REPL and OS Simulator Design and Implementation Guide
Ticket: 0102-JS-API-REPL-OS-SIM
Status: active
Topics:
    - esp32-p4
    - quickjs
    - javascript
    - visual-repl
    - picocalc
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/README.md
      Note: Defines JS-only scope
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/host-shim.js
      Note: Desktop implementation of print/millis/gc for QuickJS scripts
    - Path: 0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
      Note: Baseline qjs test runner and missing-qjs failure path
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: Firmware input editor and current Phase 5 eval placeholder
    - Path: components/qjs_service/qjs_service.cpp
      Note: Firmware QuickJS globals
    - Path: components/visual_repl/include/visual_repl.h
      Note: Current firmware visual terminal geometry and public model API
ExternalSources: []
Summary: Design and intern implementation guide for a portable JavaScript TUI DSL, desktop QuickJS OS simulator, and future ESP32-P4 visual REPL integration.
LastUpdated: 2026-06-24T23:30:00Z
WhatFor: Use this when implementing the JS-side picoOS API runtime, examples, self-tests, and QuickJS simulator for ticket 0102.
WhenToUse: Before editing 0102-esp32-p4-visual-quickjs-repl/js/** or wiring JS demos into the ESP32-P4 PicoCalc visual QuickJS REPL.
---


# JS API REPL and OS Simulator Design and Implementation Guide

## Executive summary

This ticket is the JS-side plan for building a portable PicoCalc visual REPL API and a desktop QuickJS simulator for it. The immediate work should stay in `0102-esp32-p4-visual-quickjs-repl/js/**`: scripts, runtime shims, example apps, and tests that can run under the desktop `qjs` CLI and later be pasted into or embedded by the device firmware. Firmware files are reference material for this design, not edit targets for the JS worktree.

The target API is a fluent JavaScript TUI DSL inspired by the supplied React prototype: `OS.app(name)`, `app.layout(...)`, `app.panel(id)`, widgets such as `text`, `gauge`, `spark`, `table`, `menu`, `grid`, `input`, and a simulated OS object containing mock battery, process, file, chat, music, calculator, and snake subsystems. The simulator should execute in QuickJS without Node, browser, modules, `require`, imports, filesystem access, or network access. The public portable contract remains tiny: `print(...args)`, `millis()`, and `gc()` are the only assumed host globals.

The recommended implementation is a layered JS package made of plain script files: a host shim, a deterministic screen buffer, a small OS simulation model, the TUI runtime/builders, preset examples, and self-tests. A small concatenation runner should load these files into QuickJS in order. On the desktop this gives a credible 40-column simulator and test harness; on the device the same source can be bundled as one script or embedded as text evaluated by `qjs_service_eval()`.

## Problem statement and scope

### Problem

The firmware branch is actively bringing up the ESP32-P4 PicoCalc LCD, keyboard, visual terminal model, and native QuickJS service. The JS worktree needs to move in parallel without touching firmware. The missing piece is a portable JS API layer that lets a JavaScript-focused contributor prototype the visual REPL experience and author examples now, while the device-side renderer and eval bridge mature.

The user supplied a full browser/React devkit prototype with a simulated 40×30 LCD, a `createRuntime()` function, an `OS` object, an `App`/`Panel`/widget builder hierarchy, and presets for dashboard, sysmon, music, snake, calc, settings, chat, notes, and hello. That prototype is useful as a behavior sketch but cannot be copied directly into the portable QuickJS runtime because it depends on React, DOM events, `new Function`, browser rendering, `Date`, `performance`, `requestAnimationFrame`, HTML/CSS, and `Math.random()`-driven browser state.

### Scope for this ticket

In scope:

- Build a portable JS runtime under `0102-esp32-p4-visual-quickjs-repl/js/**`.
- Simulate the OS services needed by examples: metrics, clock/tick, files, chat, music, settings, calculator, and snake.
- Implement a 40-column-friendly TUI DSL with fluent builders.
- Provide examples and self-tests that run with the existing smoke-test style.
- Keep scripts single-file or concatenation-friendly so they can be pasted into the device visual REPL.
- Document future firmware bindings as comments or docs, not C/C++ changes.

Out of scope unless explicitly requested:

- Editing `main/`, `components/qjs_service`, `components/visual_repl`, or `components/picocalc_*`.
- Adding QuickJS modules, `std`, `os`, Node, browser, filesystem, network, or async features.
- Building a browser IDE in this worktree.
- Replacing the firmware visual renderer.

## Current-state analysis with evidence

### JS worktree contract

The JS playbook states that this directory is for JavaScript that can be developed on a desktop and later run on the ESP32-P4 PicoCalc visual QuickJS REPL (`js/README.md:1-5`). It also states that this work should stay mostly under `0102-esp32-p4-visual-quickjs-repl/js/**` (`js/README.md:21-25`) and that firmware code should be avoided unless explicitly coordinated (`js/README.md:27-34`).

The runtime contract is intentionally small. The only portable globals are `print(...args)`, `millis()`, and `gc()` (`js/README.md:36-44`). The same section forbids console, modules/imports, Node APIs, QuickJS `std`/`os`, and browser APIs (`js/README.md:46-53`). Therefore every JS API layer in this ticket must be a plain script that can run in a restricted global environment.

The README recommends a single-file script with an explicit `main()` entrypoint (`js/README.md:55-64`) and explicitly recommends tiny self-tests that print `PASS`/`FAIL` and examples that fit a 40-column display (`js/README.md:100-107`). It also warns against large libraries and scripts requiring more than about one second (`js/README.md:109-114`).

### Existing desktop test loop

The repository already contains a desktop host shim. It installs `print` if missing, joining arguments with spaces (`host-shim.js:3-12`), provides `millis()` from a local start time (`host-shim.js:15-20`), and installs a no-op `gc()` if missing (`host-shim.js:22-24`). This is enough for the current portable scripts, but it uses `console`, `std`, and `Date` internally as host-shim implementation details. Application scripts must not depend on those APIs.

The current smoke example verifies arithmetic, arrays, objects, `millis()`, `print()`, and `gc()` (`examples/smoke.js:1-22`). The smoke runner chooses a global `qjs` if present, otherwise looks for `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs/qjs` (`tests/run-smoke.sh:7-15`) and executes `host-shim.js` before the smoke example (`tests/run-smoke.sh:18`). In this worktree, the smoke test initially failed because `qjs` was not installed and the local vendored QuickJS checkout is absent from `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs`; the sibling active firmware worktree does contain a built vendored tree. That failure should be recorded as an environment issue, not a script failure.

### Device firmware integration points

The 0102 firmware README says this firmware combines PicoCalc LCD/keyboard work from 0099 with native QuickJS service work from 0101, and that the first skeleton initializes extracted components and a UART debug console while the visual REPL model and renderer are built (`0102 README:1-6`). The same README describes an ESP-IDF 5.4.2 ESP32-P4 build and an early expected prompt containing LCD init, keyboard init, QuickJS service init, and `0102>` (`0102 README:8-25`).

The main app configures QuickJS with a 2 MiB memory limit, 64 KiB stack limit, 1000 ms eval timeout, and 2048-byte max eval source constants (`main/app_main.cpp:21-27`). `start_quickjs_service()` creates a service task named `qjs0102`, with a 32768-word task stack, queue length 8, the same memory/stack limits, and `can_block=false` (`main/app_main.cpp:258-276`). These constraints imply that demo scripts should be short, synchronous, and bounded.

Keyboard input is already translated into an editable input line in firmware. `handle_editor_key()` handles backspace, enter, escape, left/right, home/delete/end, and printable ASCII (`main/app_main.cpp:170-215`). At the current checkpoint, submit does not evaluate JS: `submit_input_line()` appends the prompt and a status line saying `ENTER captured; QuickJS eval is Phase 5` (`main/app_main.cpp:157-168`). This means JS-side examples can be designed now, but the firmware must later connect submit to `qjs_service_eval()`.

### QuickJS service capabilities

The service header exposes `qjs_service_eval()`, `qjs_service_run()`, `qjs_service_post()`, `qjs_service_reset()`, and `qjs_service_get_status()` (`qjs_service.h:68-83`). Eval results carry `ok`, `timed_out`, `elapsed_ms`, captured `output`, and `error` (`qjs_service.h:35-42`). Status contains memory, atom, heap, eval, and reset counters (`qjs_service.h:44-58`).

The native QuickJS service installs exactly the three globals the JS contract names: `print`, `millis`, and `gc` (`qjs_service.cpp:156-164`). `print` converts each argument with `JS_ToCString`, joins with spaces, appends a newline, and either captures output or writes to stdout (`qjs_service.cpp:105-125`). `millis` returns `esp_timer_get_time() / 1000` (`qjs_service.cpp:127-132`). `gc` calls `JS_RunGC` (`qjs_service.cpp:134-141`). The runtime also sets memory limit, stack limit, blocking behavior, and an interrupt handler during runtime creation (`qjs_service.cpp:181-203`).

During eval, the service captures printed output into a string, sets an interrupt deadline, calls `JS_Eval(..., JS_EVAL_TYPE_GLOBAL)`, records elapsed time, marks timeouts, returns printed output, and appends a non-undefined return value to the output (`qjs_service.cpp:304-345`). This is the exact behavior the desktop JS tests should mimic at the API level: stdout is line-oriented text, exceptions become error text, and all examples must finish before the timeout.

### Visual REPL model

The firmware visual REPL currently defines 40 columns, 20 rows, 8×16 cells, 80 history rows, and 160 input characters (`visual_repl.h:13-18`). Styles are row-level semantic values: system, prompt, input, output, error, and status (`visual_repl.h:20-27`). The implementation keeps history rows as a style plus a fixed text buffer (`visual_repl.cpp:17-29`) and renders rows through a 5×7 glyph set scaled into 8×16 cells (`visual_repl.cpp:79-184`). It renders a prompt row by prefixing `> ` and placing a cursor (`visual_repl.cpp:236-259`), and it renders the visible history plus input line (`visual_repl.cpp:261-283`).

The component README confirms the current checkpoint: 320×320 RGB565, 8×16 cells, 40×20 rows, first 19 rows as scrollback/output, final row as editable input, and one style per row (`visual_repl/README.md:5-14`). The user’s React prototype uses 40×30 at a 6×8 font. The portable JS simulator should therefore make the logical dimensions configurable and default to the user’s desired 40×30 for examples, while keeping a 40×20 compatibility mode for firmware-aligned snapshots.

### LCD and keyboard hardware boundaries

The LCD component README states that the PicoCalc display is 320×320 RGB565 over SPI, with a high-speed SPLL path and 32 KiB maximum transfer size (`picocalc_lcd/README.md:18-28`). Its public API includes full fills, clipped rectangle fills, rectangular blits, row-band blits, and actual SPI frequency reporting (`picocalc_lcd/README.md:30-39`). That explains why the firmware visual REPL is row-buffer-oriented.

The keyboard component README states that the keyboard controller is an I2C device at `0x1f`, using status register `0x04` and FIFO register `0x09` (`picocalc_keyboard/README.md:5-15`). Its public API is low-level polling plus diagnostics and key-name helpers (`picocalc_keyboard/README.md:16-23`). It explicitly says higher-level visual REPL code should translate raw key events into semantic editor events such as character, enter, backspace, cursor, and scroll (`picocalc_keyboard/README.md:23`). The JS simulator should use the semantic token layer, not hardware keycodes.

## Target architecture

### Layer diagram

```text
+-------------------------------------------------------------+
| JS examples and self-tests                                  |
|  hello.js, dashboard.js, sysmon.js, snake.js, api tests      |
+-----------------------------+-------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| picoOS JS API runtime                                        |
|  OS.app, App, Layout, Panel, widgets, focus/key dispatch     |
+-----------------------------+-------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| Simulated device services                                    |
|  battery, metrics, processes, clock, files, chat, music,     |
|  settings, calculator, snake, deterministic RNG              |
+-----------------------------+-------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| Screen model and renderer                                    |
|  40x30 or 40x20 cells, row text dumps, PASS/FAIL snapshots   |
+-----------------------------+-------------------------------+
                              |
                              v
+-------------------------------------------------------------+
| Host contract                                                 |
|  print(...args), millis(), gc() only                         |
+-----------------------------+-------------------------------+
                              |
              +---------------+----------------+
              |                                |
              v                                v
+-----------------------------+    +---------------------------+
| Desktop QuickJS qjs          |    | ESP32-P4 qjs_service_eval |
| host-shim + script bundle    |    | future paste/embed route  |
+-----------------------------+    +---------------------------+
```

### Recommended file layout

All implementation files should stay under `0102-esp32-p4-visual-quickjs-repl/js/**`.

```text
0102-esp32-p4-visual-quickjs-repl/js/
  README.md
  host-shim.js
  lib/
    00-core.js              # assert helpers, clamp, pad, deterministic RNG
    10-screen.js            # makeScreen(), renderText(), frame helpers
    20-os-sim.js            # createOS(seed), mock services, input tokens
    30-ui-runtime.js        # App/Layout/Panel/widget builders
    40-presets.js           # preset factory functions or source strings
    90-test-harness.js      # runTest(), expectFrame(), printSummary()
  examples/
    smoke.js
    hello-api.js
    dashboard.js
    sysmon.js
    snake.js
    calc.js
  tests/
    run-smoke.sh
    run-api-tests.sh
    api-smoke.js
    screen-snapshot.js
```

This layout is still paste/embed friendly because `run-api-tests.sh` can concatenate `lib/*.js` and the target example into one temporary script before invoking `qjs`. QuickJS global evaluation preserves top-level definitions across input files, so the runner can also pass files in order like the current smoke runner does.

### Runtime ownership

The JS runtime should expose one top-level namespace: `picoOS` or `Pico`. It should install `OS` only when running examples. This avoids making every helper global and keeps future firmware embedding safer.

Recommended shape:

```js
var Pico = (function () {
  function createRuntime(options) {
    var screen = makeScreen(options.cols || 40, options.rows || 30);
    var os = createOS(options.seed || 1);
    var ui = createUIRuntime(os, screen);
    return {
      OS: os,
      screen: screen,
      runFrame: function (dt) { os._evolve(dt); ui.frame(dt); },
      renderText: function () { return screen.toText(); },
      sendKey: function (token) { ui.sendKey(token); },
      tap: function (x, y) { ui.tap(x, y); }
    };
  }
  return { createRuntime: createRuntime };
})();
```

For examples intended to paste directly into the REPL, provide a tiny bootstrap:

```js
var rt = Pico.createRuntime({ cols: 40, rows: 30, seed: 1 });
var OS = rt.OS;
// app code here
for (var i = 0; i < 3; i++) rt.runFrame(33);
print(rt.renderText());
```

## Public JS API reference

### Host globals

| Name | Contract | Notes |
|---|---|---|
| `print(...args)` | Convert args to text, join with spaces, append newline. | Installed by firmware and host shim. |
| `millis()` | Return milliseconds since boot/start. | Use for simple elapsed-time demos only. |
| `gc()` | Request garbage collection. | Safe to call in tests; no return value. |

Application scripts must not use `console.log`, `require`, `import`, `std`, `os`, `fs`, `path`, `process`, `Buffer`, `window`, `document`, `fetch`, or DOM APIs.

### Screen API

```js
var screen = makeScreen(40, 30);
screen.clear();
screen.set(x, y, ch, { fg: "green", bold: true });
screen.text(x, y, "hello", { fg: "white" });
screen.hline(x, y, w, "─", style);
screen.vline(x, y, h, "│", style);
screen.box(x, y, w, h, "rounded", style);
print(screen.toText());
```

Screen invariants:

- Coordinates are integer cell coordinates.
- Writes outside the screen are clipped.
- `toText()` returns exactly `rows` newline-separated lines, each no wider than `cols`.
- Text snapshots must be display friendly at 40 columns.

### OS simulation API

Core properties and methods:

```js
OS.battery                 // number 0..100
OS.metrics.cpu             // number 0..100
OS.metrics.mem             // number 0..100
OS.metrics.tmp             // degrees C-ish value
OS.history(name, n)        // recent numeric history
OS.processes()             // stable array copy
OS.clock("HH:mm:ss")       // deterministic or millis-derived clock
OS.launch(name)            // records toast/status
OS.toast                   // latest toast text
```

Subsystems:

```js
// Music
OS.library.current
OS.position
OS.volume
OS.mode.shuffle
OS.next(); OS.prev(); OS.playing = false; OS.fft(30);

// Snake
OS.snake.cells; OS.snake.head; OS.food; OS.ate; OS.dead;
OS.turn("↑"); OS.step(); OS.reset();

// Chat
OS.room.messages; OS.room.online; OS.send("hello"); OS.colorOf("ada");

// Files/settings/calculator
OS.cwd; OS.ls(OS.cwd); OS.selected; OS.select(file);
OS.cfg.bright; OS.cfg.theme; OS.cfg.haptics;
OS.eval("sin(45) × 2");
```

The simulator should prefer deterministic data. Use a seedable linear congruential generator instead of `Math.random()` if tests assert snapshots.

### App and builder API

```js
var app = OS.app("hello");
var st = app.state({ n: 0 });

app.layout(function (l) {
  l.row(1, "bar").row("*", "body");
});

var panel = app.panel("body").frame("rounded").title(" hello ");
panel.text("picoOS DSL").at("center", 2).bold().fg("cyan");
panel.gauge().at(2, 4).label("batt").value(function () { return OS.battery; }).width(10).showPct();

app.key("a", function () { st.last = "a"; });
app.on("tick", 1000, function () { st.n++; });
app.statusbar("a working starter");
app.mount();
```

Builder design rules:

- Every fluent method returns the builder object unless it intentionally returns a child builder.
- Values can be literals or zero-argument functions.
- Widgets render from current state every frame; no browser-style reactivity is required.
- Key tokens are semantic strings: `"↑"`, `"↓"`, `"←"`, `"→"`, `"⏎"`, `"⌫"`, `"esc"`, and printable characters.
- Examples should not require callbacks from browser events. Tests call `rt.sendKey(token)` directly.

## Key flows and pseudocode

### Desktop test flow

```text
run-api-tests.sh
  locate qjs or fail with the same helpful message as run-smoke.sh
  assemble ordered JS files:
    host-shim.js
    lib/00-core.js
    lib/10-screen.js
    lib/20-os-sim.js
    lib/30-ui-runtime.js
    tests/api-smoke.js
  execute qjs with those files
  rely on printed PASS/FAIL lines
```

Pseudocode:

```js
function runTest(name, fn) {
  try {
    fn();
    print("PASS", name);
  } catch (e) {
    print("FAIL", name, e && e.message ? e.message : e);
    throw e;
  }
}

runTest("hello app renders", function () {
  var rt = Pico.createRuntime({ cols: 40, rows: 30, seed: 1 });
  var OS = rt.OS;
  var app = OS.app("hello");
  app.panel("main").frame("rounded").title(" hello ")
    .text("picoOS DSL").at("center", 2);
  app.mount();
  rt.runFrame(16);
  assertContains(rt.renderText(), "picoOS DSL");
});
```

### Device paste/embed flow

```text
Developer chooses example
  -> runner concatenates lib + example into one JS file
  -> developer pastes into visual REPL or firmware embeds as C string
  -> firmware submit path eventually calls qjs_service_eval(...)
  -> qjs_service captures print output
  -> firmware appends prompt/output/error/status rows to visual_repl
  -> visual_repl renders dirty rows via LCD row blits
```

Device-side pseudocode for future firmware work, included here only as context:

```c
// Not in scope for JS-only ticket.
qjs_eval_result_t result = {};
esp_err_t err = qjs_service_eval(g_qjs, source, strlen(source), 1000, "<visual-repl>", &result);
visual_repl_append_line(VISUAL_REPL_STYLE_PROMPT, submitted_line);
if (err != ESP_OK) {
  visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, esp_err_to_name(err));
} else if (!result.ok) {
  visual_repl_append_line(VISUAL_REPL_STYLE_ERROR, result.error);
} else {
  append_each_output_line(result.output, VISUAL_REPL_STYLE_OUTPUT);
}
visual_repl_render();
qjs_eval_result_free(&result);
```

### Frame flow

```text
runFrame(dt)
  OS._evolve(dt)
    update deterministic metrics/history/player/snake timers
  app._frame(screen, dt)
    fire due app timers
    run due fixed-rate loops
    run compute callbacks
    clear hit targets
    clear screen
    draw panels and widgets sorted by z
    draw statusbar
```

### Key dispatch flow

```text
sendKey(token)
  if app has explicit key binding for token:
    call binding(app, token)
  else if focused widget exists:
    if token is arrow and widget.move exists: widget.move(token)
    else if token is enter and widget.activate exists: widget.activate()
    else if widget.type exists: widget.type(token)
```

This mirrors the user-supplied prototype and fits the firmware keyboard boundary, where raw keycodes should be translated before reaching higher-level UI code.

## Implementation phases

### Phase 0 — Make the smoke loop reliable

Goal: ensure `tests/run-smoke.sh` can run on this worktree or fails with an actionable message.

Tasks:

1. Keep `host-shim.js`, `examples/smoke.js`, and `tests/run-smoke.sh` as the minimum compatibility baseline.
2. If the local vendored QuickJS directory is absent, either document that dependency clearly or update the runner to optionally use the sibling active worktree path only if that is acceptable to the repo owner.
3. Do not hide missing `qjs`; a clear failure is better than silently using the wrong binary.

Validation:

```bash
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh
```

Expected output includes `SMOKE PASS`.

### Phase 1 — Core utilities and screen buffer

Files:

- `js/lib/00-core.js`
- `js/lib/10-screen.js`
- `js/tests/screen-snapshot.js`

Implement:

- `assert`, `assertEqual`, `assertContains`, `runTest`.
- `clamp`, `pad`, `resolve`, `resolveX`.
- Deterministic RNG.
- `makeScreen(cols, rows)` with `set`, `text`, `hline`, `vline`, `box`, `clear`, and `toText`.

Important bug to avoid: the user prototype's `inB` helper checks `x < cols && y < cols && y < rows`; the duplicate `y < cols` is harmless for 40×30 but wrong in principle. The JS implementation should use `x < cols && y < rows`.

Validation:

- Draw a box at `(0,0,40,30)` and assert border characters.
- Write off-screen text and assert no exception.
- Assert every rendered row is at most 40 characters.

### Phase 2 — OS simulation model

Files:

- `js/lib/20-os-sim.js`
- `js/tests/os-sim-test.js`

Implement:

- Stable metrics, history, process list, clock, toast, and launch.
- Snake state with `reset`, `turn`, and `step`.
- Music state with current track, position, volume, playback, and FFT.
- Chat room and send.
- Tiny in-memory filesystem and settings object.
- `OS.eval(expr)` for calculator examples, with a deliberately tiny math scope.

Constraints:

- Avoid `new Function()` if possible because it widens the language surface and makes future device hardening harder. Prefer a tiny expression parser for calculator examples, or keep `OS.eval()` test-only and documented as unsafe.
- Avoid `Date` in examples. If `clock()` uses `millis()`, make the base time configurable.
- Avoid nondeterministic `Math.random()` in snapshot tests; use seeded RNG.

### Phase 3 — App, layout, panel, and base widgets

Files:

- `js/lib/30-ui-runtime.js`
- `js/tests/ui-runtime-test.js`

Implement first:

- `OS.app(name)` returning an `App`.
- `App.state`, `layout`, `panel`, `key`, `on('tick')`, `loop`, `compute`, `statusbar`, `mount`, `exit`.
- `Layout.row` and `Layout.col` single-axis splits.
- `Panel.frame`, `title`, `titleRight`, `footer`, `content`.
- `Text`, `Gauge`, `Spark`, `Progress`.

Defer complex widgets until the base is stable:

- `Table`, `Menu`, `Grid`, `Input`, `Form`, `Feed`, `Editor`, `Viewer`.

Validation:

- Hello app renders centered text.
- A gauge updates when `OS.battery` changes.
- A timer increments state after enough simulated milliseconds.
- Statusbar left/center/right rendering is clipped to 40 columns.

### Phase 4 — Examples and self-tests

Files:

- `js/examples/hello-api.js`
- `js/examples/dashboard.js`
- `js/examples/sysmon.js`
- `js/examples/snake.js`
- `js/examples/calc.js`
- `js/tests/api-smoke.js`
- `js/tests/run-api-tests.sh`

Start with examples that map cleanly to 40 columns:

1. `hello-api.js`: one panel, text, timer, key binding.
2. `dashboard.js`: frame, title, battery gauge, menu grid.
3. `sysmon.js`: gauges, sparkline, process table.
4. `snake.js`: fixed grid, loop, arrow-key tokens.
5. `calc.js`: expression string and deterministic eval/parser.

Each example should include a header:

```js
// Purpose: Demonstrates App/Panel/Text/Gauge on the portable picoOS DSL.
// Expected output: 40-column screen snapshot containing "picoOS DSL".
// Assumptions: Runs under host-shim.js + lib/*.js; no firmware-only bindings.
// Required globals: print, millis, gc.
```

### Phase 5 — Bundle/paste workflow

Files:

- `js/tests/bundle-example.sh`
- `js/README.md` update, if allowed under the JS edit rule.

Implement a script that concatenates selected runtime files and one example into a single temporary file. This supports both desktop tests and manual paste into the firmware REPL.

Pseudocode:

```bash
#!/usr/bin/env bash
set -euo pipefail
example="$1"
cat js/lib/00-core.js \
    js/lib/10-screen.js \
    js/lib/20-os-sim.js \
    js/lib/30-ui-runtime.js \
    "$example"
```

Validation:

```bash
bundle-example.sh js/examples/hello-api.js > /tmp/hello.bundle.js
qjs js/host-shim.js /tmp/hello.bundle.js
```

### Phase 6 — Future firmware handoff notes

No firmware code should be changed in this phase. Produce a short markdown note under `js/` describing:

- Which examples are ready to paste/embed.
- Maximum observed runtime and output line counts.
- Which examples require a future device binding.
- Whether the example expects 40×30 or 40×20.

## Decision records

### Decision: Keep the JS API runtime as plain scripts, not modules

- **Context:** The portable contract forbids `require`, `import`, Node APIs, and QuickJS `std`/`os`. Device integration may paste or embed one script.
- **Options considered:** ES modules; CommonJS-like loader; plain ordered scripts; generated bytecode.
- **Decision:** Use plain ordered scripts and optional concatenation for paste/embed.
- **Rationale:** This matches the current smoke runner, avoids adding a loader, and works under both desktop `qjs` and `qjs_service_eval()`.
- **Consequences:** File order matters. Tests must load files consistently. Namespacing must be manual.
- **Status:** proposed.

### Decision: Simulate screen output as text snapshots first

- **Context:** The device renderer is currently 40×20 row-based RGB565, while the user prototype models a 40×30 LCD-like text grid.
- **Options considered:** Pixel-perfect renderer; ANSI terminal renderer; text snapshot renderer; SVG/HTML renderer.
- **Decision:** Implement a cell buffer with text snapshot output first, then optionally add ANSI later.
- **Rationale:** Text snapshots are easy to test in QuickJS, can be printed through the existing output contract, and keep examples display-aware.
- **Consequences:** Color/bold/dim are represented in cells but not visible in plain snapshots unless a debug renderer is added.
- **Status:** proposed.

### Decision: Make dimensions configurable with 40×30 default and 40×20 compatibility mode

- **Context:** The user request and React prototype target 40×30 at 6×8; current firmware visual REPL is 40×20 at 8×16.
- **Options considered:** Hard-code 40×30; hard-code 40×20; make dimensions runtime options.
- **Decision:** Default the simulator to 40×30 for the requested API gallery, but accept `{ cols: 40, rows: 20 }` for firmware-aligned tests.
- **Rationale:** This preserves the user's desired app designs while keeping an obvious path to current firmware limits.
- **Consequences:** Examples should declare their expected geometry. Tests should include at least one 40×20 smoke case.
- **Status:** proposed.

### Decision: Use deterministic simulation state for tests

- **Context:** The React sketch uses random metrics, FFT, and food placement. Snapshot tests need stable output.
- **Options considered:** Use `Math.random()`; seed a local RNG; remove evolving data.
- **Decision:** Use a small seedable RNG owned by the OS simulator.
- **Rationale:** This keeps examples lively while making tests repeatable under QuickJS and device eval.
- **Consequences:** Examples that intentionally animate should assert structure, not exact every-frame glyphs, unless the seed is fixed.
- **Status:** proposed.

### Decision: Treat the React prototype as a behavioral reference, not source code to port verbatim

- **Context:** The prototype contains browser/React-specific code and uses APIs outside the portable contract.
- **Options considered:** Commit the React app; mechanically port classes; rewrite a small QuickJS-native runtime.
- **Decision:** Rewrite the runtime as portable QuickJS scripts while preserving API shape and example behavior.
- **Rationale:** A rewrite avoids accidental browser dependencies and produces code suitable for device embedding.
- **Consequences:** Some UI affordances such as clicking, CSS colors, and browser animation become semantic test functions (`tap`, `sendKey`, `runFrame`) instead of DOM behavior.
- **Status:** proposed.

## Testing and validation strategy

### Test categories

1. **Contract tests:** verify that scripts only require `print`, `millis`, and `gc`.
2. **Screen tests:** verify drawing primitives, clipping, boxes, text placement, and row width.
3. **OS simulation tests:** verify deterministic metrics, clock formatting, snake movement, chat send, settings mutation, and calculator behavior.
4. **UI runtime tests:** verify app mount, frame rendering, timers, loops, compute callbacks, focus, key dispatch, and widget rendering.
5. **Example smoke tests:** run every example and assert key screen text appears.
6. **Bundle tests:** concatenate runtime plus example and run as one file.

### Commands

```bash
# Existing baseline
0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh

# Proposed new suite
0102-esp32-p4-visual-quickjs-repl/js/tests/run-api-tests.sh

# Proposed single example bundle check
0102-esp32-p4-visual-quickjs-repl/js/tests/bundle-example.sh \
  0102-esp32-p4-visual-quickjs-repl/js/examples/hello-api.js \
  > /tmp/hello-api.bundle.js
qjs 0102-esp32-p4-visual-quickjs-repl/js/host-shim.js /tmp/hello-api.bundle.js
```

### Acceptance criteria

- The existing smoke test prints `SMOKE PASS` once a valid `qjs` binary is available.
- New test scripts print `PASS` lines and fail nonzero on the first failed assertion.
- Example output stays within 40 columns.
- No example uses forbidden APIs.
- At least one example runs as separate files and as a concatenated bundle.
- The simulator can render both 40×30 and 40×20 without code changes.

## Risks, alternatives, and open questions

### Risks

- **Environment drift:** This worktree lacks the vendored QuickJS checkout expected by `run-smoke.sh`. The runner may fail until `qjs` is installed or the vendored tree is restored.
- **Geometry mismatch:** The requested 40×30 simulator differs from current firmware 40×20. Configurable dimensions and per-example notes reduce this risk.
- **Script size:** A large JS runtime may exceed comfortable paste size or the current 2048-byte eval-source constant in firmware. Bundling examples for device use may require firmware-side embedded strings or a larger input path later.
- **Unsafe calculator eval:** A direct `new Function` calculator is easy but not aligned with a small trusted API surface. Prefer a tiny parser or mark it as demo-only.
- **Over-building widgets:** The React prototype has many widgets. Implement the minimal core first, then add widgets based on examples and tests.

### Alternatives considered

- **Browser IDE first:** Useful for developer ergonomics, but it violates the immediate portable QuickJS requirement and duplicates the supplied prototype.
- **Firmware bindings first:** Would prove hardware integration, but the user explicitly constrained this worktree to JS-side development unless asked.
- **ANSI terminal UI:** Good for desktop visual inspection, but ANSI escape handling is not part of the device visual REPL contract. Plain text snapshots are more portable.
- **Full React feature parity:** Too large for the first QuickJS pass. The correct target is the API and behavior, not the browser app.

### Open questions

1. Should the device-side visual REPL remain 40×20, or will a smaller font enable the requested 40×30 mode later?
2. Should examples be optimized for paste size, embedded firmware strings, or both?
3. Should `OS.eval()` exist in the portable runtime, or should calculator examples use a parser-only helper named `calcEval()` to avoid confusion with QuickJS eval?
4. Should colors be retained as metadata only, or should text snapshots include optional style markers for debugging?
5. Will future firmware expose drawing bindings to JS, or will JS continue to print textual screen snapshots for the C renderer to display?

## Intern onboarding guide

Start here if you are the intern implementing this ticket.

1. Read `0102-esp32-p4-visual-quickjs-repl/js/README.md` first. The most important rule is that application scripts only assume `print`, `millis`, and `gc`.
2. Run `git branch --show-current` and confirm `feature/0102-js-scripts`.
3. Run `0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh`. If it says `qjs not found`, do not edit firmware. Install/build QuickJS or ask for the correct vendored checkout.
4. Implement Phase 1 before touching widgets. A reliable screen buffer makes every later bug easier to see.
5. Keep examples tiny and self-testing. Prefer `PASS name` and a final snapshot over verbose logs.
6. After each change, run the smoke/API tests and check `git diff` to ensure only `0102-esp32-p4-visual-quickjs-repl/js/**` changed.

## File references

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/README.md` — JS worktree contract, forbidden APIs, desktop loop, and JS-only workflow.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/host-shim.js` — desktop implementation of the three portable globals.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/examples/smoke.js` — current minimal portable script style.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/js/tests/run-smoke.sh` — current qjs runner and failure mode.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` — firmware input editor, QuickJS service config, current non-eval submit path.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/qjs_service/include/qjs_service.h` — C API for eval, reset, status, and result ownership.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/qjs_service/qjs_service.cpp` — installed JS globals, eval capture, runtime limits, interrupt deadline.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/visual_repl/include/visual_repl.h` — current visual terminal dimensions and style API.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/visual_repl/visual_repl.cpp` — row history, glyph renderer, prompt row, and demo screen.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/picocalc_lcd/README.md` — LCD geometry, RGB565, SPI, and row-blit guidance.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5-js/components/picocalc_keyboard/README.md` — keyboard I2C boundary and semantic-event guidance.
