---
Title: PaperS3 WAMR AssemblyScript analysis design and implementation guide
Ticket: ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT
Status: active
Topics:
    - papers3
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - console
    - usb-serial-jtag
    - storage
    - wasm
    - assemblyscript
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/CMakeLists.txt
      Note: Current PaperS3 donor-component wiring pattern
    - Path: 0077-papers3-alphabet-graffiti/main/app_main.cpp
      Note: Minimal PaperS3 app entrypoint pattern
    - Path: 0077-papers3-alphabet-graffiti/sdkconfig.defaults
      Note: Current ESP-IDF 5.3.4 and USB Serial/JTAG baseline
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Local `esp_console` REPL startup pattern using USB Serial/JTAG
    - Path: 0067-esp-c3-led-matrix-http/main/app_main.c
      Note: Prior art for runtime bootstrap and extra console registration
    - Path: 0067-esp-c3-led-matrix-http/main/js_console.c
      Note: Prior art for a script-specific console command family
    - Path: /home/manuel/esp/esp-idf-5.3.4/components/console/esp_console.h
      Note: Official ESP-IDF 5.3.4 console API surface
ExternalSources:
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/README.md
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md
    - https://github.com/AssemblyScript/website/blob/main/src/compiler.md
    - https://github.com/AssemblyScript/website/blob/main/src/runtime.md
Summary: Evidence-backed design guide for a new PaperS3 firmware that executes precompiled AssemblyScript-generated WebAssembly demo programs through WAMR and `esp_console`.
LastUpdated: 2026-03-22T10:22:37.935625018-04:00
WhatFor: ""
WhenToUse: ""
---

# PaperS3 WAMR AssemblyScript analysis design and implementation guide

## Executive Summary

This document describes how to build the next PaperS3 firmware in this repo: a dedicated WAMR-hosting console demo that executes a curated set of precompiled AssemblyScript programs on-device. The proposed project number is `0079-papers3-wamr-assemblyscript-console`.

The most important design choices are:

1. Use a new standalone firmware project instead of modifying `0077`.
2. Keep ESP-IDF pinned to `5.3.4`, matching the current PaperS3 work.
3. Keep the interactive console on USB Serial/JTAG, not UART.
4. Compile AssemblyScript on the host ahead of time; do not compile on-device.
5. Start with embedded `.wasm` assets bundled into the firmware image.
6. Use WAMR in interpreter mode first; defer AOT until the baseline is stable.
7. Use a simple numeric ABI between host and guest first; avoid rich object/string interop in milestone 1.
8. Instantiate a fresh module per `wasm run` command; do not keep long-lived guest state initially.

The repo already contains the building blocks needed to make this tractable:

- PaperS3 app structure and donor component reuse in `0076` and `0077`
- USB Serial/JTAG `esp_console` setup in `0030-cardputer-console-eventbus`
- a script-runtime command surface in `0067-esp-c3-led-matrix-http`
- official ESP-IDF `esp_console` APIs in `/home/manuel/esp/esp-idf-5.3.4/components/console/esp_console.h`

What the repo does not yet contain is any WAMR or WebAssembly integration. That means the correct deliverable here is not "copy the existing runtime and rename it." It is a fresh embedded runtime design that deliberately fits the PaperS3 environment and this repo's conventions.

## Problem Statement

The user wants a device-side workflow like this:

```text
write AssemblyScript demos on the host
-> compile them ahead of time into WebAssembly
-> flash one PaperS3 firmware image
-> open the PaperS3 console over USB Serial/JTAG
-> run named demos from `esp_console`
-> see each demo affect the PaperS3 display and diagnostics
```

That sounds simple, but there are four separate technical problems hidden inside it:

### 1. Runtime problem

The ESP32-S3 firmware needs a WebAssembly runtime that can:

- load wasm bytecode from memory
- instantiate modules safely on a microcontroller
- call exports
- expose native host functions back into the guest
- report exceptions and memory errors clearly

### 2. Build problem

AssemblyScript programs are not source code the firmware can interpret directly. They must be compiled on the host with `asc`, and the resulting `.wasm` files must be carried into the firmware build in a deterministic way.

### 3. ABI problem

A host/guest boundary is not free. The firmware needs a stable contract for:

- how a guest module starts
- what imports it can call
- how it passes arguments
- how failures are reported
- what data types are allowed across the boundary

### 4. Product problem

This cannot be only a runtime experiment. The user wants "a bunch of AssemblyScript programs that do different things." That means the firmware needs:

- a registry of bundled demos
- a human-usable command surface
- visible results on the PaperS3 display
- documentation good enough for a new intern to extend the system

## Requested Outcome and Non-Goals

### Requested outcome

The intended milestone is a standalone PaperS3 firmware where:

- `wasm list` prints the available demos
- `wasm run <name>` executes one bundled demo
- each demo is authored in AssemblyScript and precompiled before firmware build
- the guest can call a small native drawing/diagnostic API
- the device remains debuggable through `esp_console`

### Explicit non-goals for milestone 1

These are intentionally out of scope for the first implementation pass:

- on-device AssemblyScript compilation
- arbitrary user-uploaded modules at runtime
- full WASI compatibility
- dynamic module download over Wi-Fi
- rich AssemblyScript object marshalling across the host boundary
- persistent long-running guest processes
- multitasking multiple guest modules concurrently

Those may become later tickets, but they would be a mistake to mix into the initial `0079` bring-up.

## Current-State Analysis

This section lists the concrete repo evidence that shapes the proposal.

### PaperS3 project structure already exists and should be copied, not reinvented

Observed local evidence:

- `0077-papers3-alphabet-graffiti/CMakeLists.txt` reuses the donor `M5PaperS3-UserDemo/components` tree through `EXTRA_COMPONENT_DIRS`.
- `0077-papers3-alphabet-graffiti/main/app_main.cpp` keeps the entrypoint minimal and defers real behavior to an app class.
- `0077-papers3-alphabet-graffiti/sdkconfig.defaults` already selects `esp32s3`, `16MB` flash, `SPIRAM`, and USB Serial/JTAG console defaults on `ESP-IDF 5.3.4`.

Why this matters:

- the new firmware should look like its neighboring PaperS3 apps in the repo
- the board bring-up path is already proven
- the new project can stay focused on the runtime and console problem

### The repo already has working `esp_console` patterns

Observed local evidence:

- `0030-cardputer-console-eventbus/main/app_main.cpp` starts a REPL with `esp_console_new_repl_usb_serial_jtag(...)`, registers commands with `esp_console_cmd_register(...)`, then starts the REPL with `esp_console_start_repl(...)`.
- `0067-esp-c3-led-matrix-http/main/app_main.c` registers extra commands through a dedicated callback and keeps command-family logic separate from app bootstrap.
- `0067-esp-c3-led-matrix-http/main/js_console.c` shows a clear runtime-focused command namespace (`js status`, `js eval`, `js reset`, `js stop`, `js mem`, `js examples`).

Why this matters:

- `0079` does not need a novel console architecture
- it should copy the "single namespace with subcommands" pattern from `0067`
- it should copy the USB Serial/JTAG REPL bootstrap path from `0030`

### ESP-IDF 5.3.4 already provides the console APIs we need

Observed local evidence from `/home/manuel/esp/esp-idf-5.3.4/components/console/esp_console.h`:

- `esp_console_cmd_register(...)`
- `esp_console_run(...)`
- `esp_console_register_help_command(...)`
- `esp_console_new_repl_usb_serial_jtag(...)`
- `esp_console_start_repl(...)`

Those are enough for the proposed `wasm` command family.

### The repo does not yet contain WAMR

Observed local evidence:

- searching the repo for `WAMR`, `AssemblyScript`, and `wasm-micro-runtime` produced no existing runtime integration
- the only `wasm` occurrences in the repo are unrelated examples and notes, not an embedded Wasm host

This is important because it means:

- there is no local runtime component to reuse directly
- we need to plan the vendor/component boundary carefully
- memory budget and host ABI must be chosen explicitly

## External Runtime Facts That Matter

These are the external facts that affect the design.

### WAMR

From the official WAMR sources:

- WAMR is a lightweight standalone WebAssembly runtime intended for embedded and MCU-class systems.
- It supports ESP-IDF as one of its documented platforms.
- The embedding flow is built around:
  - `wasm_runtime_full_init(...)`
  - `wasm_runtime_register_natives(...)`
  - `wasm_runtime_load(...)`
  - `wasm_runtime_instantiate(...)`
  - `wasm_runtime_lookup_function(...)`
  - `wasm_runtime_create_exec_env(...)`
  - `wasm_runtime_call_wasm(...)`
  - `wasm_runtime_get_exception(...)`
- WAMR can confine runtime allocations into a dedicated memory pool through `wasm_runtime_full_init(...)`, which is a very useful property on an ESP32-S3.

### AssemblyScript

From the official AssemblyScript sources:

- `asc` produces `.wasm` with `--outFile` and optionally `.wat` with `--textFile`.
- `asconfig.json` can define repeatable `release` and `debug` targets.
- the runtime can be selected with `--runtime incremental|minimal|stub`.
- `--exportRuntime` exposes helpers like `__new`, `__pin`, and `__collect`, but that is mainly needed when the host must manipulate managed objects directly.

These facts push the design in a clear direction:

- use `asc` as an explicit host-side build step
- keep the guest/host ABI simple enough that we do not need managed-object interop on day 1
- use WAMR's explicit runtime init and native-registration hooks rather than inventing a custom loader

## Gap Analysis

Comparing the requested outcome against the current repo:

### What already exists

- PaperS3 board and display bring-up patterns
- `esp_console` REPL setup patterns
- a precedent for script-runtime console commands
- ESP-IDF `5.3.4` environment already used by nearby projects

### What is missing

- a WAMR component in the repo
- a host-side AssemblyScript build pipeline
- a module registry for bundled `.wasm` assets
- a native host API for the guest programs
- a `wasm` console namespace
- demo programs authored specifically for PaperS3

### The main design risk

The real risk is not "can Wasm run on an ESP32-S3?" WAMR is explicit that it targets embedded systems, and it documents ESP-IDF support.

The real risk is this:

- if the first milestone tries to solve managed strings, dynamic module upload, persistent guest state, and display rendering all at once, the intern will drown in integration complexity

So the design must be staged around the simplest reliable baseline.

## Proposed Solution

The recommended solution is a new standalone firmware project:

```text
0079-papers3-wamr-assemblyscript-console/
```

That project should:

1. reuse the donor PaperS3 component stack
2. initialize a USB Serial/JTAG REPL with `esp_console`
3. bundle a small registry of precompiled `.wasm` demo modules into the firmware image
4. initialize WAMR with a bounded memory pool
5. register a minimal native host API
6. instantiate and run one module per console command
7. tear down the module instance after each run

### High-level architecture

```text
Host developer
  -> edits AssemblyScript demo
  -> runs build script / idf.py build
  -> asc compiles demo.ts -> demo.wasm + demo.wat
  -> firmware build embeds demo.wasm files
  -> flash PaperS3

USB Serial/JTAG console
  -> esp_console REPL
  -> wasm command family
  -> module registry lookup
  -> WAMR load / instantiate / run
  -> native host API calls
  -> PaperS3 display + console output
```

### Runtime control flow

```text
wasm run bars
  -> parse console args
  -> find registry entry "bars"
  -> ensure WAMR runtime initialized
  -> wasm_runtime_load(module_bytes)
  -> wasm_runtime_instantiate(module, stack, heap)
  -> wasm_runtime_lookup_function("run")
  -> wasm_runtime_create_exec_env(...)
  -> wasm_runtime_call_wasm(...)
  -> if guest imports host drawing APIs:
       host APIs update display state
  -> print return code / exception
  -> destroy exec env, instance, module
```

## Project Layout

The following file plan is recommended for `0079`.

```text
0079-papers3-wamr-assemblyscript-console/
  CMakeLists.txt
  README.md
  sdkconfig.defaults
  partitions.csv
  main/
    CMakeLists.txt
    app_main.cpp
    console_repl.h
    console_repl.cpp
    wasm_command.h
    wasm_command.cpp
    wasm_runtime_service.h
    wasm_runtime_service.cpp
    wasm_host_api.h
    wasm_host_api.cpp
    wasm_module_registry.h
    wasm_module_registry.cpp
    papers3_canvas.h
    papers3_canvas.cpp
  wasm-src/
    package.json
    asconfig.json
    shared/
      host.ts
      constants.ts
    hello-frame/
      assembly/index.ts
    nested-boxes/
      assembly/index.ts
    bars/
      assembly/index.ts
    checkerboard/
      assembly/index.ts
    radar-sweep/
      assembly/index.ts
  tools/
    build_wasm_demos.sh
```

### Responsibility of each module

- `app_main.cpp`
  - board init
  - WAMR service init
  - console start

- `console_repl.cpp`
  - `esp_console` transport selection
  - prompt config
  - help command registration

- `wasm_command.cpp`
  - implements `wasm list|info|run|status`

- `wasm_runtime_service.cpp`
  - one place where WAMR init/load/instantiate/call/unload happens
  - keeps stack/heap/pool sizing consistent

- `wasm_host_api.cpp`
  - native functions imported by the guest
  - draw primitives
  - logging/timing helpers

- `wasm_module_registry.cpp`
  - maps demo names to embedded asset symbols and metadata

- `papers3_canvas.cpp`
  - isolates PaperS3 display primitives from the runtime layer

## Why Embedded Assets First Instead of SPIFFS

There are two obvious ways to carry precompiled Wasm modules into the device:

1. embed them into the firmware image
2. store them on a filesystem partition and load them at runtime

### Recommendation for milestone 1: embed them

Reasons:

- the user asked for a curated set of demos, not arbitrary module installation
- embedded assets keep the build and test path deterministic
- it avoids a second moving part during bring-up
- it is simpler for a new intern to debug
- the `.wat` files can stay on the host for inspection without wasting flash

### When SPIFFS becomes worth it

Switch to SPIFFS or LittleFS later if the product evolves toward:

- user-uploaded modules
- module swapping without reflashing
- larger demo inventories
- downloadable content

That is a valid follow-up, but not a good starting point.

## Why WAMR Interpreter First Instead of AOT

WAMR supports multiple execution modes, including AOT. That does not mean AOT is the right first milestone.

### Recommendation for milestone 1: interpreter

Reasons:

- fewer moving parts in the build chain
- simpler debugging for the first intern
- easier to inspect `.wasm` and `.wat` side by side
- avoids introducing `wamrc` and AOT artifact management before the baseline works

### When to add AOT

Add an AOT experiment only after:

- `wasm run <name>` works reliably
- the host API is stable
- several demos are passing
- memory headroom and startup latency are measured on real hardware

## Guest ABI Strategy

This is the part most likely to create accidental complexity, so it deserves explicit rules.

### Milestone 1 ABI

Use a very small ABI:

- one required export:
  - `run(): i32`
- optional export:
  - `describe(): i32` or no-op metadata export if later needed
- host imports limited to numeric primitives and tightly controlled buffers

Recommended host imports:

```text
host.log_i32(tag: i32, value: i32) -> void
host.delay_ms(ms: i32) -> void
host.screen_clear(color: i32) -> void
host.draw_rect(x: i32, y: i32, w: i32, h: i32, color: i32) -> void
host.fill_rect(x: i32, y: i32, w: i32, h: i32, color: i32) -> void
host.present(mode: i32) -> void
```

### Why numeric-only first

AssemblyScript strings and arrays are managed runtime objects. The official AssemblyScript runtime docs explain that once the host starts interacting with managed values, concepts like `__pin`, `__unpin`, `__collect`, headers, and runtime layout become relevant.

That is exactly the complexity we should avoid in milestone 1.

Numeric-only ABI benefits:

- no guest object ownership questions
- no dependency on AssemblyScript host bindings
- simpler native signatures
- easier debugging when a demo fails

### Text support strategy

Do not make guest-to-host strings a baseline requirement for the first demos.

Instead:

1. keep demo metadata in the C++ registry on the host
2. let the host print the module name and description
3. add string or buffer imports only after the basic demos work

This is an important simplification and should be treated as intentional, not as a missing feature.

## AssemblyScript Runtime Recommendation

### Recommendation for milestone 1: `--runtime stub`

Why:

- each `wasm run` command creates a short-lived module instance
- modules are not expected to keep complex live object graphs across calls
- the `stub` runtime is the smallest and least complicated AssemblyScript runtime choice

That gives the intern a clean initial rule:

- a module runs, draws, returns, and then the host destroys the instance

### When to move beyond `stub`

Consider `minimal` or `incremental` only when:

- modules need to exchange managed values with the host
- guest-side collections live across multiple callbacks
- you want long-running or re-entrant guest sessions

## Demo Inventory

The user wants multiple programs that "do different things." The initial set should be intentionally small, visual, and diverse.

### Recommended first five demos

1. `hello-frame`
   - clears the screen
   - draws a border and a few fixed boxes
   - proves the end-to-end runtime works

2. `nested-boxes`
   - draws shrinking rectangles
   - proves loops and parameterized draw calls

3. `bars`
   - draws a bar chart
   - proves deterministic numeric computation

4. `checkerboard`
   - draws alternating tiles
   - proves two-dimensional loops

5. `radar-sweep`
   - draws a few frames with delays
   - proves a guest can call time/delay imports and present multiple frames

### Good stretch demos after baseline

- `random-walk`
- `progress-meter`
- `page-refresh-playground`
- `touch-crosshair` once input ABI exists

## Console Command Design

Follow the `0067` pattern: one command family, several subcommands.

### Proposed command surface

```text
wasm list
wasm info <name>
wasm run <name>
wasm run <name> [repeat_count]
wasm status
wasm examples
```

### Example session

```text
papers3> wasm list
hello-frame
nested-boxes
bars
checkerboard
radar-sweep

papers3> wasm info bars
name=bars
artifact=bars.wasm
bytes=1432
entry=run

papers3> wasm run bars
ok: module=bars rc=0 elapsed_ms=47
```

### Console handler pseudocode

```cpp
static int cmd_wasm(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        return wasm_module_registry_print();
    }

    if (strcmp(argv[1], "info") == 0) {
        return argc >= 3 ? wasm_runtime_print_module_info(argv[2]) : 1;
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) return 1;
        int repeat = argc >= 4 ? atoi(argv[3]) : 1;
        return wasm_runtime_run_named(argv[2], repeat);
    }

    if (strcmp(argv[1], "status") == 0) {
        return wasm_runtime_print_status();
    }

    if (strcmp(argv[1], "examples") == 0) {
        print_examples();
        return 0;
    }

    print_usage();
    return 1;
}
```

## Build Pipeline Design

The host-side build flow should be explicit and boring.

### Recommended AssemblyScript layout

```text
wasm-src/
  package.json
  asconfig.json
  shared/host.ts
  hello-frame/assembly/index.ts
  nested-boxes/assembly/index.ts
  ...
```

### Recommended `asconfig.json` approach

Use at least two targets:

- `release`
  - optimized
  - emits `.wasm`
  - optionally emits `.wat`
- `debug`
  - debug symbols
  - easier inspection during bring-up

### Suggested build command shape

```bash
asc assembly/index.ts \
  --config ../../asconfig.json \
  --target release \
  --outFile ../../build/hello-frame.wasm \
  --textFile ../../build/hello-frame.wat
```

### Suggested wrapper script

```bash
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/generated-wasm"

mkdir -p "$OUT"
pushd "$ROOT/wasm-src" >/dev/null
npm install

for demo in hello-frame nested-boxes bars checkerboard radar-sweep; do
  npx asc "$demo/assembly/index.ts" \
    --config asconfig.json \
    --target release \
    --runtime stub \
    --outFile "$OUT/$demo.wasm" \
    --textFile "$OUT/$demo.wat"
done

popd >/dev/null
```

### Firmware embedding options

There are two clean choices:

1. `EMBED_FILES` or binary-data embedding through ESP-IDF/CMake
2. a generated C++ translation unit containing byte arrays

Recommendation:

- prefer the normal ESP-IDF binary embedding path first
- only switch to generated arrays if the build becomes awkward

## WAMR Runtime Service Design

The runtime should not be spread across command handlers. It should live in one service module.

### Service responsibilities

- initialize WAMR once
- own the WAMR memory pool
- register native functions once
- run one named module on demand
- collect status and exception details

### Core service pseudocode

```cpp
bool WasmRuntimeService::Init() {
    memset(&init_args_, 0, sizeof(init_args_));
    init_args_.mem_alloc_type = Alloc_With_Pool;
    init_args_.mem_alloc_option.pool.heap_buf = runtime_pool_;
    init_args_.mem_alloc_option.pool.heap_size = sizeof(runtime_pool_);
    init_args_.native_module_name = "host";
    init_args_.native_symbols = kHostSymbols;
    init_args_.n_native_symbols = kHostSymbolCount;
    return wasm_runtime_full_init(&init_args_);
}

int WasmRuntimeService::RunModule(const ModuleAsset &asset) {
    module_ = wasm_runtime_load(asset.bytes, asset.size, error_buf_, sizeof(error_buf_));
    if (!module_) return PrintLoadError();

    module_inst_ = wasm_runtime_instantiate(module_, kGuestStackBytes, kGuestHeapBytes,
                                            error_buf_, sizeof(error_buf_));
    if (!module_inst_) return PrintInstantiateError();

    func_ = wasm_runtime_lookup_function(module_inst_, "run");
    if (!func_) return PrintMissingEntry();

    exec_env_ = wasm_runtime_create_exec_env(module_inst_, kExecEnvStackBytes);
    if (!exec_env_) return PrintExecEnvError();

    uint32_t argv[1] = {0};
    if (!wasm_runtime_call_wasm(exec_env_, func_, 0, argv)) {
        printf("exception: %s\n", wasm_runtime_get_exception(module_inst_));
        return 1;
    }

    return 0;
}
```

### Memory sizing guidance

Do not pretend the right sizes are known up front. Start with explicit constants and measure:

- runtime pool
- module stack
- module heap
- exec env stack

Initial strategy:

- choose conservative defaults
- add `wasm status` output for current pool sizes and failure mode
- adjust after real hardware testing

## Native Host API Design

This is the contract the AssemblyScript demos will import.

### Milestone 1 host API

```cpp
// All imported under module name "host"
void host_log_i32(wasm_exec_env_t exec_env, int32_t tag, int32_t value);
void host_delay_ms(wasm_exec_env_t exec_env, int32_t ms);
void host_screen_clear(wasm_exec_env_t exec_env, int32_t color);
void host_draw_rect(wasm_exec_env_t exec_env, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void host_fill_rect(wasm_exec_env_t exec_env, int32_t x, int32_t y, int32_t w, int32_t h, int32_t color);
void host_present(wasm_exec_env_t exec_env, int32_t mode);
```

### WAMR native registration sketch

```cpp
static NativeSymbol kHostSymbols[] = {
    EXPORT_WASM_API_WITH_SIG(host_log_i32, "(ii)"),
    EXPORT_WASM_API_WITH_SIG(host_delay_ms, "(i)"),
    EXPORT_WASM_API_WITH_SIG(host_screen_clear, "(i)"),
    EXPORT_WASM_API_WITH_SIG(host_draw_rect, "(iiiii)"),
    EXPORT_WASM_API_WITH_SIG(host_fill_rect, "(iiiii)"),
    EXPORT_WASM_API_WITH_SIG(host_present, "(i)")
};
```

Note:

- the exact signature strings should be verified during implementation
- the important design point is that the first API stays primitive and explicit

### AssemblyScript side sketch

```ts
@external("host", "host_screen_clear")
declare function screenClear(color: i32): void

@external("host", "host_fill_rect")
declare function fillRect(x: i32, y: i32, w: i32, h: i32, color: i32): void

@external("host", "host_present")
declare function present(mode: i32): void

export function run(): i32 {
  screenClear(0xFFFFFF)
  for (let i = 0; i < 6; i++) {
    fillRect(40 + i * 30, 80, 20, 40 + i * 10, 0x000000)
  }
  present(1)
  return 0
}
```

## PaperS3 Display Integration

This is where the Wasm runtime meets the actual device behavior.

### Recommendation

Keep the PaperS3 display logic behind a thin host-side adapter. Do not let WAMR-facing code talk directly to `M5.Display` everywhere.

Benefits:

- easier unit-like reasoning
- easier future migration if refresh policy changes
- easier to enforce drawing constraints

### Suggested host adapter responsibilities

- initialize `M5`
- normalize rotation and colors
- offer simple draw primitives
- centralize full vs partial refresh choices
- optionally track dirty regions later

### Important discipline

Do not expose the entire M5GFX surface to guest code. Expose only the specific verbs the product needs.

That keeps:

- the ABI stable
- security tighter
- debugging easier

## Design Decisions

### Decision 1: Create `0079` as a new sibling project

Reason:

- avoids destabilizing `0077`
- matches repo numbering conventions
- keeps Wasm work isolated and teachable

### Decision 2: Keep USB Serial/JTAG as the console transport

Reason:

- matches the PaperS3/Cardputer guidance in this repo
- avoids UART overlap problems
- already proven in local `esp_console` prior art

### Decision 3: Embed precompiled modules into firmware first

Reason:

- deterministic
- easy to review
- no filesystem bring-up required for milestone 1

### Decision 4: Use WAMR interpreter first

Reason:

- smallest bring-up surface
- easier debugging
- AOT can wait until baseline behavior is stable

### Decision 5: Use short-lived module instances

Reason:

- simpler lifecycle
- simpler memory hygiene
- aligns well with `--runtime stub`

### Decision 6: Start with a numeric ABI only

Reason:

- avoids immediate AssemblyScript managed-object interop
- shortens the path to the first visible demo

## Alternatives Considered

### Alternative A: Reuse the existing QuickJS-style path from `0067`

Rejected because:

- the user specifically wants AssemblyScript programs compiled to Wasm
- QuickJS solves a different runtime model
- Wasm gives a smaller, more explicit ABI boundary for the requested demos

### Alternative B: Load modules from SPIFFS on day 1

Rejected for milestone 1 because:

- it adds unnecessary complexity during bring-up
- it complicates documentation for a new intern
- the initial product only needs a curated demo set

### Alternative C: Solve text/string marshalling immediately

Rejected for milestone 1 because:

- it creates avoidable complexity around AssemblyScript runtime layout
- it is not required to prove the runtime architecture

### Alternative D: Start directly with AOT

Rejected for milestone 1 because:

- it would add another build tool and artifact type before the baseline exists
- it obscures whether failures are coming from runtime integration or code generation

## Implementation Plan

This is the recommended sequence for the intern.

### Phase 1: Scaffold `0079`

Create:

- `0079-papers3-wamr-assemblyscript-console/`
- `main/`
- `sdkconfig.defaults`
- `partitions.csv`
- `README.md`

Copy the structural patterns from `0077`.

Success criteria:

- project config builds as a PaperS3 target
- USB Serial/JTAG remains the console default

### Phase 2: Add console bootstrap

Implement:

- `console_repl.cpp`
- `wasm_command.cpp`

Copy the bootstrapping approach from `0030`.

Success criteria:

- `help` works
- `wasm examples` works even before WAMR is integrated

### Phase 3: Add WAMR as a component and initialize it

Implement:

- component/vendor integration
- runtime service
- bounded runtime pool

Success criteria:

- firmware builds
- `wasm status` reports runtime initialized

### Phase 4: Add one static demo module

Implement:

- `wasm-src/hello-frame/assembly/index.ts`
- host-side `asc` build script
- binary embedding
- `wasm run hello-frame`

Success criteria:

- one guest module loads
- one guest export runs
- one visual result appears on the PaperS3

### Phase 5: Expand the host API

Implement:

- more draw primitives
- timing helpers
- error/status reporting

Success criteria:

- at least three demos look different on-screen

### Phase 6: Add the rest of the curated demo pack

Implement:

- registry entries
- info metadata
- command examples
- README updates

Success criteria:

- `wasm list` shows the full starter pack
- each named demo runs from the console

### Phase 7: Hardware validation

Validate:

- console reliability
- WAMR memory headroom
- display refresh feel
- repeated run stability

Success criteria:

- repeated `wasm run <name>` does not degrade or crash
- no transport conflicts occur
- failures produce readable exceptions

## Testing and Validation Strategy

### Build validation

Run:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

### Host-side Wasm artifact validation

For each demo:

- check `.wasm` exists
- keep `.wat` for inspection
- optionally record file size in the registry metadata

### Console smoke tests

```text
help
wasm examples
wasm list
wasm info hello-frame
wasm run hello-frame
wasm run bars
wasm run checkerboard 3
```

### Failure-path tests

```text
wasm run missing-demo
wasm info missing-demo
```

Expected behavior:

- readable error
- no reboot
- no corrupted REPL state

### Hardware observations to record

- display latency per demo
- whether repeated runs leave visual artifacts
- whether the console remains responsive while a demo is running
- whether the chosen runtime pool sizes are sufficient

## Risks and Open Questions

### Risk 1: WAMR footprint vs PaperS3 app budget

Mitigation:

- start with interpreter only
- keep the native API small
- measure binary size after each phase

### Risk 2: Guest string interop becomes a distraction

Mitigation:

- forbid it in milestone 1
- keep human-readable names on the host side

### Risk 3: Display updates from guest actions become too noisy for e-paper

Mitigation:

- centralize display calls behind the host adapter
- keep `present(mode)` explicit

### Open question 1

Should the first host API include text drawing at all, or only rectangles/lines?

Recommendation:

- only if the host owns the strings

### Open question 2

Should the runtime eventually support persistent module state across commands?

Recommendation:

- not in `0079`

### Open question 3

When should AOT be introduced?

Recommendation:

- only after interpreter-mode demos are stable on hardware

## API Reference Quick Sheet

### ESP-IDF console APIs used by the design

```c
esp_err_t esp_console_cmd_register(const esp_console_cmd_t *cmd);
esp_err_t esp_console_run(const char *cmdline, int *cmd_ret);
esp_err_t esp_console_new_repl_usb_serial_jtag(...);
esp_err_t esp_console_start_repl(esp_console_repl_t *repl);
esp_err_t esp_console_register_help_command(void);
```

### WAMR APIs central to the design

```c
bool wasm_runtime_full_init(RuntimeInitArgs *init_args);
bool wasm_runtime_register_natives(const char *module_name, NativeSymbol *native_symbols, uint32_t n_native_symbols);
wasm_module_t wasm_runtime_load(uint8_t *buf, uint32_t size, char *error_buf, uint32_t error_buf_size);
wasm_module_inst_t wasm_runtime_instantiate(const wasm_module_t module, uint32_t default_stack_size, uint32_t host_managed_heap_size, char *error_buf, uint32_t error_buf_size);
wasm_function_inst_t wasm_runtime_lookup_function(const wasm_module_inst_t module_inst, const char *name);
wasm_exec_env_t wasm_runtime_create_exec_env(wasm_module_inst_t module_inst, uint32_t stack_size);
bool wasm_runtime_call_wasm(wasm_exec_env_t exec_env, wasm_function_inst_t function, uint32_t argc, uint32_t argv[]);
const char *wasm_runtime_get_exception(wasm_module_inst_t module_inst);
```

### AssemblyScript options central to the design

```text
--outFile
--textFile
--runtime stub
--target release
--config asconfig.json
```

## Recommended First Review Order

If a new intern starts implementing this design, they should read in this order:

1. this document
2. `0030-cardputer-console-eventbus/main/app_main.cpp`
3. `0067-esp-c3-led-matrix-http/main/js_console.c`
4. `0077-papers3-alphabet-graffiti/sdkconfig.defaults`
5. `/home/manuel/esp/esp-idf-5.3.4/components/console/esp_console.h`
6. the WAMR embedding and native API docs
7. the AssemblyScript compiler/runtime docs

## References

### Local repo references

- `0077-papers3-alphabet-graffiti/CMakeLists.txt`
- `0077-papers3-alphabet-graffiti/main/app_main.cpp`
- `0077-papers3-alphabet-graffiti/sdkconfig.defaults`
- `0030-cardputer-console-eventbus/main/app_main.cpp`
- `0067-esp-c3-led-matrix-http/main/app_main.c`
- `0067-esp-c3-led-matrix-http/main/js_console.c`
- `/home/manuel/esp/esp-idf-5.3.4/components/console/esp_console.h`

### External references

- WAMR README: `https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/README.md`
- WAMR embedding guide: `https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md`
- WAMR native API export guide: `https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md`
- WAMR product-mini README: `https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md`
- AssemblyScript compiler docs: `https://github.com/AssemblyScript/website/blob/main/src/compiler.md`
- AssemblyScript runtime docs: `https://github.com/AssemblyScript/website/blob/main/src/runtime.md`
