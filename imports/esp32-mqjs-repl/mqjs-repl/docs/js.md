---
Title: Cardputer JS REPL: JS Component Reference
Slug: mqjs-js-component-reference
Short: Developer reference for MicroQuickJS integration (JsEvaluator, stdlib runtime, load/autoload)
Topics:
  - javascript
  - esp32s3
  - esp-idf
  - debugging
  - console
  - cardputer
  - spiffs
IsTemplate: false
IsTopLevel: false
ShowPerDefault: false
SectionType: GeneralTopic
---

# Cardputer JS REPL: JS Component Reference

## Overview

The “JS component” is the MicroQuickJS-backed evaluator that implements:

- `:mode js` → evaluate JS lines (`JS_Eval`)
- `:reset` → recreate the JS context
- `:stats` → dump MicroQuickJS memory usage (`JS_DumpMemory`)
- `:autoload [--format]` → mount SPIFFS (optionally format), load `/spiffs/autoload/*.js`
- `load(path)` inside JS → read a SPIFFS file and `JS_Eval` it

This doc is for developers who want to add JS-facing capabilities, improve REPL UX, or change how code is loaded.

## Narrative Walkthrough (Mental Model)

At runtime, “JS mode” is just a different evaluator plugged into the same REPL loop. The REPL doesn’t know anything about JavaScript; it simply forwards completed lines to `JsEvaluator::EvalLine`. That separation is what keeps the firmware debuggable: if JS is broken, the repeat evaluator still proves that input/output and prompt handling work.

MicroQuickJS is not QuickJS-in-a-library. The important conceptual shift is that a lot of what normally feels “built into the engine” (atoms, builtins, global objects, and even some parsing behavior around keywords) is described by a ROM-style stdlib table. On ESP32-S3, getting those tables right is the difference between “real JS” and bizarre parse errors that look unrelated (like `var` not being recognized).

The current implementation deliberately keeps the JS story simple:

- there is exactly one context owned by the REPL task,
- the heap is a fixed 64 KiB arena,
- `load(path)` and `:autoload` are the basic primitives for growing the interpreter with libraries,
- and “risky” operations (SPIFFS formatting) are always explicit.

## MicroQuickJS Constraints (Critical)

MicroQuickJS in this repo is **table-driven**:

- The “stdlib” is not just library objects; it also includes the **atom table** that affects parsing (keywords like `var`).
- On ESP32-S3, tables must be generated for **32-bit** (`-m32`) and must match the atom header.

Golden invariant:

- `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h` and
  `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_atom.h`
  are generated together via:
  `imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh`

Do not hand-edit generated headers; regenerate them.

## Code Map (Start Here)

### Evaluator integration

- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h`
  - `class JsEvaluator final : public IEvaluator`
  - Key methods:
    - `EvalLine(std::string_view)`
    - `Reset(std::string*)`
    - `GetStats(std::string*)`
    - `Autoload(bool format_if_mount_failed, std::string* out, std::string* error)`

- `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
  - `JsEvaluator::JsEvaluator()` creates context:
    - `ctx_ = JS_NewContext(js_mem_buf_, kJsMemSize, &js_stdlib);`
  - `JsEvaluator::EvalLine`:
    - uses `JS_EVAL_REPL | JS_EVAL_RETVAL`
    - prints non-`undefined` return values
    - formats exceptions via `JS_GetException` + `JS_PrintValueF`
  - `JsEvaluator::Autoload`:
    - mounts SPIFFS (`SpiffsEnsureMounted(format_if_mount_failed)`)
    - reads each `.js` from `/spiffs/autoload`
    - evaluates with `JS_Eval(ctx_, buf, len, path, flags=0)`

### JS stdlib runtime stubs (native functions)

- `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
  - Defines C functions that the generated stdlib references (examples):
    - `js_print(...)` → prints to stdout (USB Serial/JTAG routes via ESP-IDF)
    - `js_gc(...)` → runs `JS_GC(ctx)`
    - `js_date_now(...)`, `js_performance_now(...)` → ESP timer-based time
    - `js_load(...)` → SPIFFS-backed `load(path)`
  - Includes the generated stdlib at the end:
    - `#include "esp32_stdlib.h"`

## `load(path)` Contract

Implemented in `esp32_stdlib_runtime.c` as `js_load(...)`.

Current behavior:

- Signature: `load(path)`; path must be a JS string.
- Mounts SPIFFS non-destructively (no formatting):
  - mount failure error suggests using `:autoload --format` once.
- Reads whole file into memory and evaluates it:
  - current size limit: **32 KiB** (`load: file too large`).

Suggested enhancements (future work):

- Stream/compile incrementally (if MicroQuickJS supports it) or implement chunked reads.
- Enforce path policy (e.g., require `/spiffs/` prefix) to avoid surprising behavior.
- Add a `loadRelative("foo.js")` convention (or track a “cwd” variable).

## Autoload Contract (`:autoload`)

Autoload is implemented in `JsEvaluator::Autoload`.

Current behavior:

- Directory: `/spiffs/autoload`
- File filter: `*.js`
- Formatting is explicit:
  - `:autoload` → mount without formatting, errors if not mountable
  - `:autoload --format` → format+mount if mount fails

Seeded SPIFFS image:

- `imports/esp32-mqjs-repl/mqjs-repl/spiffs_image/autoload/00-seed.js`
  - sets `globalThis.AUTOLOAD_SEED = 123`
  - prints a line via `print(...)`

The device and QEMU smoke tests assert that:

- `:autoload --format` prints `autoload: ...`
- evaluating `globalThis.AUTOLOAD_SEED` returns `123`

## Memory Model (What’s Allocated Where)

- JS heap: fixed 64 KiB arena in `JsEvaluator`:
  - `static constexpr size_t kJsMemSize = 64 * 1024;`
  - `static uint8_t js_mem_buf_[kJsMemSize];`
- SPIFFS load/autoload currently uses heap allocations for file buffers (reads whole file).
- `:stats` prints ESP heap and then dumps MicroQuickJS memory via `JS_DumpMemory`.

## How to Validate Changes

### QEMU

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/test_js_repl_qemu_uart_stdio.sh --timeout 120
```

### Device (Cardputer)

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./tools/set_repl_console.sh usb-serial-jtag
./build.sh build
./tools/flash_device_usb_jtag.sh --port auto
python3 ./tools/test_js_repl_device_uart_raw.py --port auto --timeout 90
```

## Extending JS Capabilities (Practical Paths)

### Path A (recommended for now): ship JS libraries in SPIFFS

Add a file under:
- `imports/esp32-mqjs-repl/mqjs-repl/spiffs_image/autoload/NN-name.js`

Then rebuild + flash; the SPIFFS image is regenerated as `build/storage.bin`.

### Path B (native functions): regenerate the stdlib

If you need a new native callable function in the global object, you must:

1) update the generator inputs (host-side tooling / build description), and
2) regenerate `main/esp32_stdlib.h` and `components/mquickjs/mquickjs_atom.h` together:
   - `imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh`

Then ensure the runtime stubs (`esp32_stdlib_runtime.c`) provide any referenced symbols.

## Usage Sketches (Snippets)

### Run JS and inspect the result

In the REPL:
```text
:mode js
var x = 1 + 2; x
```

Expected output includes `3`.

### Load a script from SPIFFS

In JS mode:
```text
load("/spiffs/autoload/00-seed.js")
globalThis.AUTOLOAD_SEED
```

This exercises the `load(path)` primitive directly.

### Add a shipped “stdlib-in-files” library

Add a file under `spiffs_image/autoload/`:
```js
// spiffs_image/autoload/10-mylib.js
globalThis.MyLib = {
  add: (a, b) => a + b,
};
```

Rebuild and flash:
```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./build.sh build
./tools/flash_device_usb_jtag.sh --port auto
```

Then in the REPL:
```text
:mode js
:autoload --format
MyLib.add(2, 3)
```

### Tighten `load(path)` policy (example sketch)

If you decide to restrict `load()` to SPIFFS-only paths, do it in `js_load(...)`:
```c
if (strncmp(path, "/spiffs/", 8) != 0) {
  return JS_ThrowTypeError(ctx, "load: path must start with /spiffs/");
}
```

Validate by attempting `load("relative.js")` (should error) and `load("/spiffs/autoload/00-seed.js")` (should work).
