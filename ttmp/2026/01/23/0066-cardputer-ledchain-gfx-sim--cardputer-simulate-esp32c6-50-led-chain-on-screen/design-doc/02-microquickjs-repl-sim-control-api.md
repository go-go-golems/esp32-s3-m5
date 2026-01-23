---
Title: "0066: MicroQuickJS REPL + Simulator Control API (Design + Analysis)"
Slug: 0066-microquickjs-repl-sim-control-api
Short: "Design and deep analysis for adding a MicroQuickJS REPL to the Cardputer LED-chain simulator, including a JS-facing sim API and integration constraints."
Topics:
  - esp32s3
  - cardputer
  - esp-idf
  - javascript
  - microquickjs
  - console
  - simulation
  - leds
IsTemplate: false
IsTopLevel: false
ShowPerDefault: false
SectionType: DesignDoc
---

# 0066: MicroQuickJS REPL + Simulator Control API (Design + Analysis)

## 1. Problem Statement (What we are building)

We already have a Cardputer (ESP32-S3) firmware that:

- maintains a *virtual* 50‑LED WS281x strip in memory,
- computes patterns using the same pattern engine used on the ESP32‑C6 (`components/ws281x`),
- renders those 50 RGB pixels as a 10×5 on-screen “LED chain simulator” via M5GFX,
- exposes `sim ...` commands through a USB Serial/JTAG `esp_console` REPL, and
- optionally starts Wi‑Fi + a small HTTP server after STA gets an IP.

The new requirement is to add **MicroQuickJS + a REPL** (like our prior “Cardputer JS REPL” work) and expose a **JS API** that can drive the simulator’s patterns/parameters.

For this phase, the intent is not to mirror the ESP32‑C6 control protocol; instead we want a programmable, interactive surface on the Cardputer itself:

- type JS expressions,
- call into `sim.*` bindings,
- see the on‑screen LEDs change.

## 2. Relevant Prior Art (What we should reuse)

### 2.1. “JS GPIO exercizer” REPL (MicroQuickJS + custom REPL loop)

This repo already has a complete MicroQuickJS REPL implementation:

- `0039-cardputer-adv-js-gpio-exercizer/main/app_main.cpp`
  - a dedicated REPL task reading bytes from USB Serial/JTAG (not `esp_console`)
- `0039-cardputer-adv-js-gpio-exercizer/main/repl/ReplLoop.cpp`
  - minimal line editor, meta-commands (`:help`, `:mode`, `:stats`, …), evaluator dispatch
- `0039-cardputer-adv-js-gpio-exercizer/main/eval/JsEvaluator.cpp`
  - creates context via `JS_NewContext(..., &js_stdlib)`
  - evaluates lines via `JS_Eval(..., JS_EVAL_REPL | JS_EVAL_RETVAL)`
- `0039-cardputer-adv-js-gpio-exercizer/main/esp32_stdlib_runtime.c`
  - defines the native function stubs referenced by the generated stdlib table
  - includes the generated `esp32_stdlib.h` at the bottom (defining `js_stdlib`)

The most important lesson from this prior art is **not** “how to read bytes from UART”, but **how MicroQuickJS expects native bindings to exist**.

### 2.2. MicroQuickJS is “table-driven” (stdlib generation is part of the runtime)

MicroQuickJS in this repo is not “QuickJS where you can just do `JS_NewCFunction`”.
Instead:

- The global object layout and C-function catalog come from a ROM-style stdlib table (`JSSTDLibraryDef`).
- The “generator” emits a C header defining:
  - the atom table,
  - object property tables,
  - and a `js_c_function_table[]` mapping call slots to C symbol names.

This is documented explicitly in:

- `imports/esp32-mqjs-repl/mqjs-repl/docs/js.md`
- `imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh`
- `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c`

### 2.3. 0066 simulator internals (what we bind)

The simulator’s controllable state is already centralized and thread-safe:

- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_engine.h`
  - owns `led_patterns_t`, current `led_pattern_cfg_t`, virtual `led_ws281x_t`, `frame_ms`
  - protected by a FreeRTOS mutex (safe from multiple tasks)
- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_console.cpp`
  - already defines a stable vocabulary for types/parameters:
    - `off|rainbow|chase|breathing|sparkle`
    - per-pattern fields like speed/sat/spread, etc.

This makes the JS binding straightforward: we do not need to “touch pixels”; we only need to set `led_pattern_cfg_t` and `frame_ms`.

## 3. Core Constraint (The “why” behind the design)

> Callout: **MicroQuickJS native functions are not dynamically registered; they are compiled into a generated stdlib table.**

If we want JavaScript to call:

```js
sim.setPattern("rainbow")
```

then the stdlib table must contain:

- a `sim` object on the global object
- a property `setPattern` whose value points at a C function slot
- a C function table entry whose symbol name is `js_sim_setPattern` (or similar)

Therefore, the design has two inseparable parts:

1) **JS API surface**: what functions exist, what arguments they take, how they fail.
2) **Stdlib generation path**: how we produce a `js_stdlib` that contains those functions.

## 4. Design Options (and trade-offs)

### Option A: Replace `esp_console` with the custom JS REPL loop (0039-style)

Pros:
- identical REPL UX to the prior MicroQuickJS REPL (meta commands, prompt switching)
- full control over line editing behavior

Cons:
- 0066 currently relies on `wifi_console` (which owns `esp_console`) for Wi‑Fi configuration
- running two concurrent “console readers” on USB Serial/JTAG is unsafe (competing reads)
- would require porting `wifi ...` and `sim ...` commands into the custom REPL command language

### Option B: Keep `esp_console` and add a `js` command (chosen)

Pros:
- preserves current `sim ...` and `wifi ...` UX
- avoids dual readers for USB Serial/JTAG
- easy adoption: `js eval ...` and `js repl` are “just another command”

Cons:
- JS REPL loop is “nested” and more limited than the 0039 REPL
- `esp_console` tokenization is not shell-quality; quoting is limited

### Option C: Run JS in a dedicated task via `components/mqjs_service` and enqueue evals

Pros:
- formalizes the “single VM owner” invariant (good for future multi-producer JS jobs)
- provides timeouts/deadlines via the VM wrapper

Cons:
- still needs a front-end for interactive input
- more moving pieces than the MVP needs

## 5. Chosen Architecture

We implement Option B:

- Keep 0066’s existing `wifi_console`/`esp_console` REPL.
- Add a new `js` command:
  - `js eval <code...>`: evaluate one JS expression/statement
  - `js repl`: enter a simple JS prompt loop (`js> `) until `.exit`
  - `js stats`: dump MicroQuickJS memory
  - `js reset`: recreate the JS context

And we expose a JS global object:

```js
sim.status()
sim.setFrameMs(ms)
sim.setBrightness(pct)
sim.setPattern("rainbow" | "chase" | "breathing" | "sparkle" | "off")
sim.setRainbow(speed, sat, spread)
sim.setChase(speed, tail, gap, trains, fgHex, bgHex, dir, fade, briPct)
sim.setBreathing(speed, colorHex, min, max, curve)
sim.setSparkle(speed, colorHex, density, fade, mode, bgHex)
```

### Files added (0066 implementation)

- JS runtime + generated stdlib:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib_runtime.c`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/esp32_stdlib.h` (generated)
- JS engine wrapper:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_engine.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_engine.h`
- Console command:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_console.h`
- Generator script (ticket-local tooling):
  - `ttmp/2026/01/23/0066-cardputer-ledchain-gfx-sim--cardputer-simulate-esp32c6-50-led-chain-on-screen/scripts/gen_esp32_stdlib_0066.py`

## 6. Implementation Mechanics (How the pieces work)

### 6.1. Stdlib generation (why we need the script)

MicroQuickJS uses `mquickjs_build.c` (host-side) to generate a ROM table.
The generator is conceptually:

```text
JSPropDef trees + class defs
        |
        v
build_atoms("js_stdlib", js_global_object, js_c_function_decl)
        |
        v
esp32_stdlib.h (js_stdlib_table + js_c_function_table + JSSTDLibraryDef js_stdlib)
```

Our `gen_esp32_stdlib_0066.py` does three steps:

1) reads the upstream generator input (`mqjs_stdlib.c`)
2) injects an extra object definition (`js_sim_obj`) and adds it to the global object list
3) compiles a temporary generator and writes `0066/.../mqjs/esp32_stdlib.h`

### 6.2. Runtime stubs (native C functions)

The generated table references C symbols by name. Therefore `esp32_stdlib_runtime.c` must provide:

- baseline builtins we rely on:
  - `js_print`, `js_gc`, `js_date_now`, `js_performance_now`
- stubs for generator-included but unused objects (`gpio`, `i2c`)
- our simulator bindings:
  - `js_sim_status`, `js_sim_setPattern`, etc.

The binding functions only touch simulator state through:

- `sim_engine_get_cfg`
- `sim_engine_set_cfg`
- `sim_engine_get_frame_ms`
- `sim_engine_set_frame_ms`

Because `sim_engine` is mutex-protected, these operations are thread-safe.

### 6.3. Evaluation model

We evaluate using:

- `JS_Eval(ctx, code, len, "<repl>", JS_EVAL_REPL | JS_EVAL_RETVAL)`

Fundamentally:

- `JS_EVAL_REPL` allows implicit globals in assignments (REPL convenience)
- `JS_EVAL_RETVAL` returns the last value (so expressions show results)

We convert results to printable strings by temporarily redirecting the log function:

- set `JS_SetContextOpaque(ctx, std::string*)`
- set `JS_SetLogFunc(ctx, write_to_string)`
- call `JS_PrintValueF(ctx, value, JS_DUMP_LONG)`

## 7. API Semantics (What JS “means”)

### 7.1. `sim.status()`

Returns a JS object containing:

- `ok`, `led_count`, `frame_ms`, `brightness_pct`, `type`
- and always-present nested objects:
  - `rainbow`, `chase`, `breathing`, `sparkle`

This “always present” choice is deliberate: it avoids forcing users to branch on `type` just to inspect values.

### 7.2. “Setter” functions

The setters are intentionally *side-effecting* and mostly return either:

- `undefined` (for multi-arg setters), or
- a small confirmation value (e.g., new brightness/frame value)

This matches an embedded REPL’s ergonomic constraint: the primary feedback loop is the screen visualization, not a complex return type.

### 7.3. Validation philosophy

We validate only where it prevents user confusion:

- brightness must be 1..100
- frame_ms must be 1..1000
- pattern name must be in the known set
- colors must parse as `#RRGGBB`

Everything else is “accept the value and let the pattern engine do what it does”. This keeps the binding thin and preserves the semantics of the existing pattern engine.

## 8. Pseudocode Summary (End-to-end)

### Boot

```c
engine = sim_engine_init(...)
sim_ui_start(engine)

wifi_console_start({
  register_extra: () => {
    sim_console_register_commands(engine)
    mqjs_console_register_commands(engine)  // creates JS context, registers `js`
  }
})
```

### JS eval command path

```text
user types:  sim> js eval sim.setBrightness(100)

esp_console dispatch -> cmd_js(argv)
  -> MqjsEngine::Eval("sim.setBrightness(100)")
     -> JS_Eval(...)
     -> returns value or exception
  -> print result
```

### JS → simulator binding path

```text
JS calls: sim.setBrightness(100)

js_sim_setBrightness(ctx, argv):
  cfg = sim_engine_get_cfg()
  cfg.global_brightness_pct = 100
  sim_engine_set_cfg(cfg)
```

## 9. Validation

The smoke test uses USB Serial/JTAG and checks:

- `js help` prints usage
- `js eval sim.status()` returns an object
- setting brightness and patterns updates simulator state

Automated script (ticket-local):

- `ttmp/.../scripts/serial_smoke_js_0066.py`

## 10. Future Work (natural next steps)

1) Add a helper that prints status as JSON (better for remote debugging):
   - `sim.statusJson()` that returns a string
2) Add a SPIFFS-backed `load(path)` and autoload mechanism (like mqjs-repl):
   - makes the JS environment extensible without regenerating stdlib
3) Add “safe execution” knobs:
   - timeouts (interrupt handler)
   - memory usage reporting / hard limits
4) Consider `components/mqjs_service` if multiple subsystems need to enqueue JS jobs.

