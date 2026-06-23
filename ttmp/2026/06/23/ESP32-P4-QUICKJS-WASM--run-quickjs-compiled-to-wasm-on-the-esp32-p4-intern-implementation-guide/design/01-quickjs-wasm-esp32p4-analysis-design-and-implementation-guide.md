---
Title: ""
Ticket: ""
Status: ""
Topics: []
DocType: ""
Intent: ""
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: Pattern ported for NativeSymbol registration (env module)
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: Pattern ported for load/instantiate/lookup/call lifecycle
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: Pattern ported for WAMR init + PSRAM pool + status
    - Path: 0099-esp32-p4-picocalc-display-keyboard/sdkconfig.defaults
      Note: ESP32-P4 target/console/PSRAM baseline copied into 0100
    - Path: 0100-esp32-p4-quickjs-wasm/main/app_main.cpp
      Note: Target firmware scaffold entrypoint
    - Path: ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/sources/06-wamr-embed-wamr.md
      Note: WAMR embedding lifecycle and buffer-passing (module_dup_data) used in eval flow
    - Path: ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/sources/09-wamr-wasm-export-header.h
      Note: Authoritative WAMR C API (wasm_export.h) referenced throughout the design
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# QuickJS Compiled to WASM on the ESP32-P4 — Analysis, Design, and Implementation Guide

**Ticket:** ESP32-P4-QUICKJS-WASM
**Target firmware directory:** `0100-esp32-p4-quickjs-wasm`
**Audience:** New intern — you can read C/C++ and use a terminal, but you have never touched this codebase or run WebAssembly on a microcontroller.
**Goal:** Understand the complete stack and implement firmware `0100` that runs a JavaScript engine (QuickJS), compiled to WebAssembly, sandboxed inside the WAMR runtime, on an ESP32-P4 — driven from a USB console `js eval` command.

---

## 1. Executive Summary

This guide explains how to run **JavaScript on the ESP32-P4** by compiling the **QuickJS** engine
to a **WebAssembly (WASM)** module and executing that module with the **WebAssembly Micro Runtime
(WAMR)** embedded in ESP-IDF firmware. The result is a "JS-in-WASM-in-WAMR" stack: user JavaScript
is evaluated by a QuickJS engine that itself runs inside a WASM sandbox, which is in turn driven by
a WAMR runtime living inside ESP-IDF.

You do **not** write QuickJS or WAMR. You build QuickJS into a `.wasm` file on your PC, embed that
file into firmware `0100`, wire WAMR to load and call it, and expose a console command that feeds
JavaScript source into it. Everything you need is already patterned in this workspace: the WAMR
embedding is copied from project `0079-papers3-wamr-assemblyscript-console`, and the ESP32-P4 target
conventions are copied from `0099-esp32-p4-picocalc-display-keyboard`.

The guide is organized in layers:

1. **System architecture** — what runs where and why, with the central "two host boundaries" idea.
2. **Background concepts** — QuickJS, WebAssembly/WASI, WAMR, and the ESP32-P4, each explained for a newcomer.
3. **Prior art and gap analysis** — what the workspace already has and what is missing.
4. **Design** — the build pipeline, firmware layout, runtime init, host API, eval flow, and console.
5. **API references** — the exact WAMR and QuickJS C functions and signature strings you will call.
6. **Pseudocode** — copy-pasteable skeletons for every firmware source file.
7. **Diagrams** — call flow, memory layout, and build pipeline.
8. **File references** — every claim is anchored to an absolute path.
9. **Phased plan, build/flash/verify, testing, risks, decisions, and references.**

---

## 2. System Architecture

### 2.1 What you are building, in one paragraph

You compile QuickJS (a small C JavaScript engine) into a single WebAssembly module called
`quickjs.wasm` using the `wasi-sdk` toolchain. That module exports a tiny C API — `qjs_init()` and
`qjs_eval(ptr, len)` — and imports a handful of host functions (`host_print`, `host_gpio_write`,
…). You embed `quickjs.wasm` into ESP-IDF firmware `0100` as a binary blob, link the `espressif/wasm-micro-runtime`
component, initialize WAMR with a memory pool in PSRAM, register the host functions under the
WASM import module `"env"`, then load → instantiate → call `qjs_eval` with a JavaScript string the
user types at the console. QuickJS parses and runs the JavaScript; when the script calls
`print("hello")`, that call travels back out through the WASM import boundary into your ESP-IDF
host code, which prints to the console.

### 2.2 Hardware overview — the ESP32-P4

The ESP32-P4 is Espressif's highest-performance microcontroller to date. Key facts you must know:

- **CPU:** 32-bit RISC-V, dual-core **HP** cluster up to **400 MHz** (with AI/vector extensions and
  an FPU), plus a single-core **LP** (low-power) core up to 40 MHz. You only use the HP cores here.
- **On-chip memory:** 128 KB HP ROM, **768 KB HP SRAM (called L2MEM)**, 32 KB LP SRAM, **8 KB SPM**
  (scratchpad, deterministic latency), 16 KB LP ROM.
- **External memory:** **32 MB stacked PSRAM** (on the `ESP32-P4NRW32` package used by the
  PicoCalc/Waveshare boards) and **16 MB SPI flash**.
- **Peripherals:** 55 programmable GPIOs, MIPI-CSI, MIPI-DSI, USB 2.0 OTG, Ethernet, SDIO 3.0,
  SPI, I2S, I2C, LED PWM, MCPWM, RMT, ADC, and more.
- **Cache:** separate L1 instruction and data caches, backed by an L2 cache that serves both
  internal SRAM and external PSROM/PSRAM.

**Why this matters for the design:** 768 KB of internal SRAM is *large* for an MCU but *small* for a
JavaScript engine. QuickJS plus its heap needs hundreds of KB to megabytes, so the WAMR runtime
pool and the QuickJS guest heap must live in **PSRAM**, not internal SRAM. The 400 MHz dual-core
RISC-V CPU is fast enough to make an interpreted JS engine usable, and the large PSRAM is what
makes the whole "JS-in-WASM" idea feasible where it would not be on a smaller chip.

### 2.3 The board and console

This guide targets the **PicoCalc / Waveshare ESP32-P4-WIFI6** adapter (the same board used by
workspace projects `0097`–`0099`). Two hardware facts shape the firmware:

- **Console = UART0, not USB Serial/JTAG.** The ESP32-P4 has *no* native USB Serial/JTAG console
  (unlike the ESP32-S3). The board uses an external **CH343 USB-UART bridge** on **UART0 (GPIO37
  TX / GPIO38 RX)**. So `sdkconfig.defaults` must select `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`, and
  the S3-centric "prefer USB Serial/JTAG" guidance in `AGENTS.md` does **not** apply to the P4.
- **PSRAM is hex/200 MHz stacked PSRAM** (`CONFIG_SPIRAM_MODE_HEX=y`, `CONFIG_SPIRAM_SPEED_200M=y`),
  verified in the `0099` bring-up.

### 2.4 The software stack — the "JS-in-WASM-in-WAMR" idea

This is the single most important diagram in the guide. Read it carefully:

```
        ┌──────────────────────────────────────────────────────────────┐
        │  USER TYPES AT CONSOLE:  js eval print(1+2)                  │
        └─────────────────────────────────┬────────────────────────────┘
                                          │  JS source string
        ┌─────────────────────────────────▼────────────────────────────┐
        │  ESP-IDF FIRMWARE (0100)   — native C/C++ on ESP32-P4         │
        │                                                              │
        │   esp_console ──> js_eval_command(src,len)                   │
        │         │                                                    │
        │         │  wasm_runtime_module_dup_data(ctx, src, len)       │
        │         ▼                                                    │
        │  ┌──────────────────────────────────────────────────────┐   │
        │  │  WAMR RUNTIME  (espressif/wasm-micro-runtime)        │   │
        │  │   wasm_runtime_full_init  (pool in PSRAM)           │   │
        │  │   wasm_runtime_load(quickjs.wasm)                   │   │
        │  │   wasm_runtime_instantiate(stack, heap)             │   │
        │  │   wasm_runtime_register_natives("env", host_symbols)│   │
        │  │   wasm_runtime_call_wasm(qjs_eval, [ptr,len])       │   │
        │  └───────────────────────┬──────────────────────────────┘   │
        │                          │   WASM import call (module "env")│
        │          ┌───────────────▼────────────────┐                 │
        │          │  quickjs.wasm  (WASM guest)    │                 │
        │          │   qjs_eval(ptr,len)            │                 │
        │          │      │                          │                 │
        │          │      ▼  QuickJS engine (C)     │                 │
        │          │   JS_Eval(ctx, src, ...)        │                 │
        │          │      │  user JS calls print()   │                 │
        │          │      ▼                          │                 │
        │          │   js_print()  ──host_print()──► │  ── import ──►  │
        │          └─────────────────────────────────┘                 │
        │                                  │                          │
        │   host_print()  ──>  printf / esp_log  ──>  UART0 console    │
        └──────────────────────────────────│──────────────────────────┘
                                         ▼
                              CONSOLE OUTPUT:  3
```

### 2.5 The two host boundaries (the key concept)

A newcomer's most common mistake is to think "JavaScript calls ESP-IDF directly." It does not.
There are **two separate host boundaries**, and the data crosses each one differently:

1. **Boundary A — WAMR ↔ QuickJS-wasm (the WASM boundary).**
   WAMR is the *host*; `quickjs.wasm` is the *guest*. The guest imports functions (e.g.
   `host_print`) from a WASM import module we name `"env"`. WAMR satisfies those imports with
   **native symbols** you register via `wasm_runtime_register_natives("env", ...)`. Arguments
   cross this boundary as raw WASM types (i32 pointers into the guest's linear memory, lengths,
   etc.). Strings/buffers passed *into* the guest must be allocated in the guest's own memory with
   `wasm_runtime_module_dup_data`.

2. **Boundary B — QuickJS ↔ user JavaScript (the JS boundary).**
   QuickJS is the *host*; the user's JavaScript is the *guest*. The user JS calls globals like
   `print(...)` or `gpio.write(...)`. Those globals are **C functions you define inside the wasm**
   (registered via `JS_NewCFunction`), and *those* C functions in turn call the `env` imports
   (Boundary A) to reach real hardware.

So a single `print("hi")` travels: user JS → `js_print` (C, inside wasm) → `host_print` (wasm
import) → WAMR native symbol → ESP-IDF `printf` → UART0. **Memorize this chain**; it explains every
design decision below.

### 2.6 Why compile QuickJS to WASM at all?

You might ask: "Why not just compile QuickJS directly to the ESP32-P4 (native RISC-V), like the
workspace's `microquickjs` work did on an ESP32?" Good question. Wrapping QuickJS in WASM adds a
sandbox and a layer of indirection, at a performance cost. The benefits:

- **Memory safety / sandboxing.** WASM gives QuickJS a private linear memory it cannot escape. A
  buggy or malicious script cannot corrupt the ESP-IDF heap, task stacks, or peripherals. On a
  device that also drives a display and radios, that isolation is valuable.
- **Portability of the engine artifact.** The same `quickjs.wasm` runs on your PC (for testing),
  on the P4, and on any other WAMR host. You debug the engine once.
- **Uniform extension model.** If you later want to run other languages (Lua, Python) you compile
  them to WASM too and reuse the same WAMR host + console plumbing.
- **The P4 can afford it.** 400 MHz + 32 MB PSRAM means the WASM interpreter overhead is tolerable
  for an interactive/scripting use case, where it would be painful on a smaller chip.

The tradeoff (performance) is addressed later by WAMR's **AOT** mode: you can pre-compile
`quickjs.wasm` to native RISC-V with `wamrc` to remove the interpreter overhead (see Decision DR-2).

---

## 3. Background Concepts (for the intern)

### 3.1 QuickJS — the JavaScript engine

QuickJS is a small, embeddable JavaScript engine by Fabrice Bellard. Facts you need:

- Supports the **ES2025** specification (modules, async generators, proxies, nearly 100% of test262).
- **Small:** a few C files, no external dependencies; ~367 KiB of x86 code for a hello-world.
- **GC:** reference counting with cycle removal — deterministic and low-memory.
- **Embeddable C API:** you create a `JSRuntime`, then a `JSContext`, then `JS_Eval` a string. You
  register C functions as JS globals with `JS_NewCFunction`.
- **License:** MIT. **Official repo:** `https://github.com/bellard/quickjs`.
- **Micro QuickJS:** Bellard also maintains `https://github.com/bellard/mquickjs`, a JS engine for
  bare microcontrollers. *This guide does not use it* — we use full QuickJS compiled to WASM —
  but note the workspace already has `microquickjs` experience (ticket `ESP-30`).

The QuickJS C API you will use (signatures in §6):

| Function | Purpose |
|---|---|
| `JS_NewRuntime()` | Create a runtime (holds GC, memory limits). |
| `JS_NewContext(rt)` | Create a context (holds globals, modules). |
| `JS_Eval(ctx, src, len, filename, flags)` | Parse and run a JS string. |
| `JS_GetGlobalObject(ctx)` | Get the global object (`globalThis`). |
| `JS_SetPropertyStr(ctx, obj, "name", val)` | Set a global property (e.g. register `print`). |
| `JS_NewCFunction(ctx, fn, "name", min_args)` | Wrap a C function as a JS function value. |
| `JS_ToCString(ctx, val)` / `JS_FreeCString` | Read a JS string into a C `char*`. |
| `JS_FreeValue(ctx, val)` | Release a reference (refcounting). |
| `JS_SetMemoryLimit(rt, bytes)` | Cap the QuickJS heap. |
| `JS_FreeContext` / `JS_FreeRuntime` | Tear down. |

### 3.2 WebAssembly and WASI — the bytecode format and its "system calls"

**WebAssembly (WASM)** is a portable binary instruction format for a stack-based virtual machine. A
`.wasm` file contains: imports (functions it needs from the host), exports (functions the host can
call), a linear memory (a flat byte array the code lives in), and the code itself. The key property
for us: **the guest cannot touch host memory except through declared imports.** That is the sandbox.

**WASI (WebAssembly System Interface)** is a standard set of imports (a "libc for WASM") — `fd_write`,
`args_get`, `clock_time_get`, `proc_exit`, etc. A C program compiled with the `wasi-sdk` and
`wasi-libc` becomes a WASI module: its `malloc`, `printf`, file I/O, etc. are implemented over those
WASI imports. WAMR implements WASI (toggle `CONFIG_WAMR_ENABLE_LIBC_WASI=y`), so a WASI-compiled
QuickJS gets a working `malloc`/`free` and basic libc for free.

- **`wasi-sdk`:** a Clang/LLVM/LLD toolchain configured to target `wasm32-wasi` by default.
  Installed at `/opt/wasi-sdk` (the conventional path). Its libc is `wasi-libc` (musl-derived).
- **Reactor vs command modules:** A WASI *command* exports `_start` (like a program `main`). A
  *reactor* (library) module exports `_initialize` and other named functions, with no program entry.
  We build QuickJS as a **reactor** so we can call `qjs_eval` repeatedly without re-launching a
  process — see Decision DR-1.

### 3.3 WAMR — the WebAssembly Micro Runtime

WAMR (`bytecodealliance/wasm-micro-runtime`) is a lightweight, embeddable WASM runtime suited to
embedded and IoT. Properties:

- **Multiple execution modes:** interpreter (default, *fast* interp), **AOT** (pre-compiled to
  native via `wamrc`), and JIT (Fast-JIT / LLVM-JIT on supported targets).
- **Embeddable in C/C++:** you call a small C API (`wasm_export.h`) to init, load, instantiate,
  and call modules.
- **Native API:** the host registers C functions that the wasm guest can import and call — this is
  Boundary A.
- **WASI + libc-builtin:** built-in libc (no external deps) so `malloc`/`printf` work in the guest.
- **Memory containment:** the runtime can be given a fixed memory pool (a raw byte buffer) so all
  wasm allocations are bounded and cannot starve the system — essential on embedded.
- **ESP-IDF component:** Espressif publishes `espressif/wasm-micro-runtime` on the IDF Component
  Registry; the workspace already uses version `2.4.0~1` in project `0079`.

The WAMR embedding lifecycle (Boundary A host side) is:

```
wasm_runtime_full_init(args)     // init runtime + pool + register natives
  → wasm_runtime_load(bytes)      → wasm_module_t
  → wasm_runtime_instantiate(...)  → wasm_module_inst_t   (linear memory ready)
  → wasm_runtime_lookup_function(inst, "qjs_eval") → func
  → wasm_runtime_create_exec_env(inst, stack)        → exec_env
  → wasm_runtime_module_dup_data(inst, src, len)     → wasm-side pointer
  → wasm_runtime_call_wasm(exec_env, func, 2, argv)  // argv = {ptr, len}
  ... teardown: destroy_exec_env, deinstantiate, unload, destroy
```

### 3.4 The ESP32-P4 memory model (what goes where)

ESP-IDF on the P4 distinguishes memory by **bus** (instruction vs data) and **location** (internal
vs external). For this project the practical map is:

| Region | Size | Properties | Use in this project |
|---|---|---|---|
| **DRAM (internal SRAM)** | ~768 KB L2MEM (shared with IRAM/code) | Fast, executable-capable when in IRAM; the runtime heap | Task stacks, critical code, DMA buffers. **Do not** put the WAMR pool or QuickJS heap here — too small. |
| **IRAM** | subset of internal SRAM | Executable at fixed latency | ISRs, time-critical code (`IRAM_ATTR`). |
| **IROM / DROM** | 16 MB flash via MMU cache | Read-only, cached | Firmware code + const data + the embedded `quickjs.wasm` blob. |
| **PSRAM** | 32 MB external, cached via L2 | Large, slower, not executable by default | **WAMR runtime pool + QuickJS guest heap.** Allocated via `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`. |
| **SPM** | 8 KB | Deterministic latency, software-managed | Optional: hot interpreter data. Not used initially. |

> **Rule of thumb:** anything that can be large or grow (the WAMR pool, the QuickJS heap, big JS
> strings) goes in **PSRAM**. Anything that must be fast and small (task stacks, DMA buffers,
> ISRs) stays in **internal SRAM**. The `0099` firmware already enables
> `CONFIG_SPIRAM_USE_MALLOC=y` so `malloc` can spill into PSRAM automatically.

---

## 4. Prior Art and Gap Analysis

### 4.1 What the workspace already has

| Project / Ticket | What it proves | Reuse for `0100` |
|---|---|---|
| `0079-papers3-wamr-assemblyscript-console` | Embeds `espressif/wasm-micro-runtime` v2.4.0~1, inits WAMR with a 512 KB pool in PSRAM, registers `host` native symbols, runs wasm modules from `EMBED_FILES`, drives an `esp_console` REPL. | **Copy the host/runtime/runner split** (`wasm_runtime_service.cpp`, `wasm_host_api.cpp`, `wasm_module_runner.cpp`). |
| `0082-papers3-wamr-allocator-control` | Same WAMR core, focused on allocator behavior (pool vs system allocator). | Confirms pool-in-PSRAM is the stable choice. |
| `ESP-30-M5DIAL-MQJS-LAIN-DSL` (ticket) + `microquickjs` vocab | Native QuickJS embedding on an ESP32. | Shows the JS side of the story; this ticket is the WASM-sandboxed equivalent on a bigger chip. |
| `0097/0098/0099-esp32-p4-*` | ESP32-P4 bring-up: target `esp32p4`, IDF 5.4.2, UART0 console, hex 200 MHz PSRAM, PicoCalc keyboard/LCD wiring. | **Copy `sdkconfig.defaults`, console choice, `CMakeLists.txt`, partition layout.** |

### 4.2 What is missing (the gap)

1. **A QuickJS → WASM build pipeline.** Nobody in the workspace has compiled QuickJS with
   `wasi-sdk`. This guide defines it (§5.2) but the intern must execute it on a host PC.
2. **A reactor-style `wasm_main.c`** exposing `qjs_init`/`qjs_eval` and declaring `env` imports.
   Does not exist yet.
3. **A `js eval` console command** that passes a JS string *into* wasm memory via
   `wasm_runtime_module_dup_data` and calls `qjs_eval`. The `0079` runner calls zero-arg exports;
   it does not pass a string buffer. This is the main new host code.
4. **ESP32-P4 + WAMR config tuning.** `0079` targets `esp32s3`; the P4 needs `esp32p4` target,
   UART console, and memory-cap adjustments (larger pool feasible thanks to 32 MB PSRAM).
5. **Memory/perf validation** for an interpreted JS engine on the P4 (no numbers yet).

### 4.3 Non-goals

- Not building a full JS app framework (the `wamr-app-framework` is available but unnecessary).
- Not wiring the LCD/keyboard into JS yet — Phase 1 is a headless `print`/`eval` console.
- Not native (non-WASM) QuickJS — that is the `microquickjs` lineage, a different ticket.

---

## 5. Proposed Design

### 5.1 Component breakdown

```
0100-esp32-p4-quickjs-wasm/
├── CMakeLists.txt                  # project() + set-target esp32p4 (from 0099)
├── sdkconfig.defaults              # P4 console + PSRAM + WAMR toggles (from 0099 + new)
├── partitions.csv                  # large app partition (from 0099)
├── idf_component.yml               # depends on espressif/wasm-micro-runtime (from 0079)
├── main/
│   ├── CMakeLists.txt              # EMBED_FILES quickjs.wasm; REQUIRES wmr + console + pthread
│   ├── app_main.cpp                # init console, init WAMR, register js commands
│   ├── wasm_runtime_service.{h,cpp}# WAMR init + status (port from 0079)
│   ├── wasm_host_api.{h,cpp}       # "env" native symbols: host_print, host_gpio_write...
│   ├── wasm_runner.{h,cpp}        # load qjs.wasm, eval a JS string, return result
│   ├── js_command.{h,cpp}         # esp_console "js eval ..." / "js repl"
│   └── quickjs-embed.h            # extern decls of the embedded wasm blob
├── wasm-src/                       # HOST-SIDE QuickJS build (not compiled by ESP-IDF)
│   ├── quickjs/                    # vendored bellard/quickjs source
│   ├── wasm_main.c                 # reactor wrapper: qjs_init, qjs_eval, js_print
│   ├── CMakeLists.txt              # wasm32-wasi toolchain build
│   └── build-quickjs-wasm.sh       # one-shot: configure wasi-sdk, build, emit .wasm
└── wasm-build/
    └── quickjs.wasm                # produced artifact, copied into main/ for EMBED_FILES
```

### 5.2 The WASM build pipeline (host PC)

This runs on your laptop, **not** in ESP-IDF. It produces `wasm-build/quickjs.wasm`.

**Step 1 — install wasi-sdk** (one-time):

```bash
# Download a wasi-sdk release (e.g. wamr/LLVM 19) and extract to /opt/wasi-sdk
# (or set $WASI_SDK_PATH). Verify:
$WASI_SDK_PATH/bin/clang --version
# clang version ... (Target: wasm32-wasi)
```

**Step 2 — vendor QuickJS:**

```bash
cd 0100-esp32-p4-quickjs-wasm/wasm-src
git clone https://github.com/bellard/quickjs.git quickjs
cd quickjs && git checkout <release-tag>   # pin a release
```

**Step 3 — write `wasm_main.c`** (the reactor wrapper). Pseudocode in §7.1; the essence:

```c
#include "quickjs.h"

/* Boundary A imports: provided by WAMR native symbols, module "env". */
__attribute__((import_module("env"), import_name("host_print")))
extern void host_print(const char *s, int len);

__attribute__((import_module("env"), import_name("host_millis")))
extern int host_millis(void);

static JSRuntime *rt;
static JSContext  *ctx;

static JSValue js_print(JSContext *c, JSValueConst ths, int argc, JSValueConst *argv) {
    const char *s = JS_ToCString(c, argv[0]);
    host_print(s, (int)strlen(s));
    JS_FreeCString(c, s);
    return JS_UNDEFINED;
}

void qjs_init(void) {                       // EXPORTED
    rt  = JS_NewRuntime();
    JS_SetMemoryLimit(rt, 256 * 1024);      // cap QuickJS heap (inside wasm linear mem)
    ctx = JS_NewContext(rt);
    JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "print",
                      JS_NewCFunction(ctx, js_print, "print", 1));
}

int qjs_eval(const char *src, int len) {    // EXPORTED
    JSValue r = JS_Eval(ctx, src, (size_t)len, "<console>",
                       JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_PRINT_ONLY);
    int ok = JS_IsException(r) ? -1 : 0;
    JS_FreeValue(ctx, r);
    return ok;
}
```

**Step 4 — build as a reactor (library) module:**

```bash
# Build QuickJS as a static lib with the wasi toolchain, then link our wrapper as a reactor.
$WASI_SDK_PATH/bin/clang \
  --target=wasm32-wasi \
  -O3 -flto \
  -I wasm-src/quickjs \
  wasm-src/quickjs/*.c  wasm-src/wasm_main.c \
  -o wasm-build/quickjs.wasm \
  -Wl,--no-entry \
  -Wl,--export=qjs_init \
  -Wl,--export=qjs_eval \
  -Wl,--allow-undefined \
  -Wl,--export=__heap_base \
  -msimd128  # optional; see DR-3
```

Key flags explained:

- `--target=wasm32-wasi` — produce a WASI module (gets `wasi_snapshot_preview1` imports for libc).
- `--no-entry` — reactor, not a command; there is no `_start`/`main`.
- `--export=qjs_init,qjs_eval` — these are the functions WAMR will `lookup_function`.
- `--allow-undefined` — our `host_*` imports (Boundary A) are unresolved at link time; WAMR
  supplies them at runtime via `wasm_runtime_register_natives("env", ...)`.
- `__heap_base` export — gives the host a handle to where wasm heap begins (useful for sizing).

> **Validation check:** `wasm-objdump -x quickjs.wasm` should show an `import` section listing
> `env.host_print`, `env.host_millis`, … and `wasi_snapshot_preview1.*`, and an `export` section
> listing `qjs_init` and `qjs_eval`. Run the module on your PC with `iwasm` (host test, §10)
> *before* flashing.

### 5.3 Firmware embedding

**`main/CMakeLists.txt`** (modeled on `0079`):

```cmake
idf_component_register(
    SRCS
        "app_main.cpp"
        "wasm_runtime_service.cpp"
        "wasm_host_api.cpp"
        "wasm_runner.cpp"
        "js_command.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        espressif__wasm-micro-runtime
        console
        pthread
    EMBED_FILES
        "quickjs.wasm"
)
```

`EMBED_FILES quickjs.wasm` turns the wasm blob into `_binary_quickjs_wasm_start`/`_end` symbols
accessible from C, exactly as `0079` embeds its AssemblyScript `.wasm` assets. Copy the built
`wasm-build/quickjs.wasm` into `main/quickjs.wasm` before `idf.py build`.

**`idf_component.yml`** (from `0079`):

```yaml
dependencies:
  idf:
    version: ">=5.4.2,<5.5.0"        # P4 needs 5.4.2+ (see 0099)
  espressif/wasm-micro-runtime:
    version: "2.4.0~1"
```

### 5.4 `sdkconfig.defaults` (P4 + WAMR)

Built by merging `0099`'s P4 settings with the WAMR toggles the component exposes (the exact
symbol names come from `0079/.../managed_components/espressif__wasm-micro-runtime/build-scripts/esp-idf/wamr/Kconfig`):

```kconfig
# --- Target + console (from 0099) ---
CONFIG_IDF_TARGET="esp32p4"
CONFIG_ESP_CONSOLE_UART_DEFAULT=y          # CH343 USB-UART bridge on UART0 GPIO37/38
CONFIG_ESP_CONSOLE_SECONDARY_NONE=y

# --- Flash + PSRAM (from 0099) ---
CONFIG_ESPTOOLPY_FLASHMODE_DIO=y
CONFIG_ESPTOOLPY_FLASHFREQ_80M=y
CONFIG_ESPTOOLPY_FLASHSIZE_32MB=y
CONFIG_IDF_EXPERIMENTAL_FEATURES=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_HEX=y
CONFIG_SPIRAM_SPEED_200M=y
CONFIG_SPIRAM_USE_MALLOC=y

# --- WAMR runtime (see the component Kconfig) ---
CONFIG_WAMR_BUILD_RELEASE=y
CONFIG_WAMR_ENABLE_INTERP=y
CONFIG_WAMR_INTERP_FAST=y                  # fast interpreter
CONFIG_WAMR_INTERP_LOADER_NORMAL=y
CONFIG_WAMR_ENABLE_LIBC_BUILTIN=y          # guest malloc/printf over builtin libc
CONFIG_WAMR_ENABLE_LIBC_WASI=y             # QuickJS-wasm is a WASI module
CONFIG_WAMR_ENABLE_LIB_PTHREAD=y           # optional guest threads
CONFIG_WAMR_ENABLE_APP_FRAMEWORK=n         # we don't need app-mgr; keep it small
CONFIG_WAMR_ENABLE_MULTI_MODULE=n
CONFIG_WAMR_ENABLE_AOT=n                   # Phase 2 optimization (see DR-2)
CONFIG_WAMR_ENABLE_REF_TYPES=n
CONFIG_WAMR_ENABLE_SHARED_MEMORY=n
CONFIG_WAMR_APP_THREAD_STACK_SIZE_MAX=65536
```

### 5.5 Runtime initialization (WAMR pool in PSRAM)

Port `0079/main/wasm_runtime_service.cpp` almost verbatim, but enlarge the pool — the P4 has 32 MB
PSRAM, so a 1–2 MB WAMR pool is cheap and gives QuickJS room to breathe:

```cpp
constexpr std::size_t kRuntimePoolSizeBytes = 2 * 1024 * 1024;   // 2 MB in PSRAM
// allocate in PSRAM, fall back to internal if PSRAM unavailable (as 0079 does):
g_runtime_pool_buffer = heap_caps_malloc(kRuntimePoolSizeBytes,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
RuntimeInitArgs init_args = {};
init_args.mem_alloc_type           = Alloc_With_Pool;
init_args.running_mode             = Mode_Interp;
init_args.mem_alloc_option.pool.heap_buf  = g_runtime_pool_buffer;
init_args.mem_alloc_option.pool.heap_size = kRuntimePoolSizeBytes;
// register natives here OR separately (see §5.6):
init_args.native_module_name = "env";
init_args.n_native_symbols   = sizeof(kHostSymbols)/sizeof(kHostSymbols[0]);
init_args.native_symbols      = kHostSymbols;
wasm_runtime_full_init(&init_args);
```

> The `0079` code registers natives separately with `wasm_runtime_register_natives` *after*
> `wasm_runtime_full_init`. Both orders work; the doc `embed_wamr.md` notes registration must
> finish **before** `wasm_runtime_load`. Pick one and keep it consistent.

### 5.6 The host API (Boundary A: WAMR native symbols)

Port `0079/main/wasm_host_api.cpp`. The native symbols live under module `"env"` to match the
`__attribute__((import_module("env")))` declarations in `wasm_main.c`. Signatures use WAMR's compact
notation (legend in §6.2):

```cpp
// First param of every native fn is wasm_exec_env_t (NOT written in the signature string).
static void host_print_native(wasm_exec_env_t, const char *s) {
    fputs(s, stdout);                 // goes to UART0 console
}
static int  host_millis_native(wasm_exec_env_t) {
    return (int)esp_timer_get_time() / 1000;
}
static void host_gpio_write_native(wasm_exec_env_t, int pin, int val) {
    gpio_set_level((gpio_num_t)pin, val ? 1 : 0);
}

static NativeSymbol kHostSymbols[] = {
    { "host_print",      (void*)host_print_native,      "($)",  nullptr }, // string, no ret
    { "host_millis",     (void*)host_millis_native,     "()i",  nullptr }, // no args, i32 ret
    { "host_gpio_write", (void*)host_gpio_write_native,  "(ii)", nullptr }, // 2x i32, no ret
};
```

Register with module name `"env"`:

```cpp
wasm_runtime_register_natives("env", kHostSymbols,
                              sizeof(kHostSymbols)/sizeof(kHostSymbols[0]));
```

> **String passing note:** `host_print` receives `const char *s` because the signature `$` tells
> WAMR this is a *string in the guest's linear memory* — WAMR converts the guest pointer to a host
> pointer and (importantly) NUL-terminates a copy so it is safe to read as a C string. For raw
> binary buffers use `*` (buffer) + `~` (length) instead.

### 5.7 The eval flow (passing a JS string into the guest)

This is the main new host code (0079 only calls zero-arg exports). To let QuickJS read the JS
source, you must place it **inside the wasm instance's linear memory** — you cannot pass a host
pointer (the sandbox forbids it). WAMR provides `wasm_runtime_module_dup_data`:

```cpp
WasmEvalResult run_js(const char *src, size_t len) {
    WasmEvalResult r = {};
    char err[128];

    wasm_module_t mod = wasm_runtime_load((uint8_t*)quickjs_wasm, quickjs_wasm_len,
                                           err, sizeof err);
    if (!mod) { r.stage="load"; r.msg=err; return r; }

    wasm_module_inst_t inst = wasm_runtime_instantiate(mod,
          /*stack*/ 16*1024, /*heap*/ 512*1024, err, sizeof err);
    if (!inst) { r.stage="instantiate"; r.msg=err; goto unload; }

    // Copy JS source INTO the guest's linear memory; returns a wasm-side pointer.
    uint64_t wasm_ptr = wasm_runtime_module_dup_data(inst, src, len);
    if (!wasm_ptr) { r.stage="dup"; r.msg="dup_data failed"; goto deinst; }

    wasm_function_inst_t fn = wasm_runtime_lookup_function(inst, "qjs_eval");
    wasm_exec_env_t env = wasm_runtime_create_exec_env(inst, 16*1024);
    uint32_t argv[2] = { (uint32_t)wasm_ptr, (uint32_t)len };
    if (!wasm_runtime_call_wasm(env, fn, 2, argv)) {
        r.stage="eval"; r.msg = wasm_runtime_get_exception(inst);
    } else {
        r.ok = (argv[0] == 0);     // qjs_eval returns 0 on success
    }
    wasm_runtime_module_free(inst, wasm_ptr);
    wasm_runtime_destroy_exec_env(env);
deinst:
    wasm_runtime_deinstantiate(inst);
unload:
    wasm_runtime_unload(mod);
    return r;
}
```

Design choices here:

- **Keep the module loaded, instantiate per session (or per eval).** For Phase 1, instantiate once
  at `js repl` start and reuse the instance across evals (call `qjs_init` once). Re-instantiate
  only on `js reset`. This mirrors how a JS REPL keeps one context.
- **`qjs_init` vs `qjs_eval`:** call `qjs_init` (zero-arg) right after instantiate to create the
  runtime/context and register `print`. Then call `qjs_eval` for each input line.

### 5.8 The console (`esp_console` + `js` commands)

Use `esp_console` (the `console` component) exactly like `0079`'s `wasm` commands. Suggested
commands:

```
js status              # WAMR runtime status + pool/heap high-water
js eval "<src>"        # one-shot: qjs_init + qjs_eval("<src>")
js repl                # line-by-line REPL: init once, eval each line
js reset               # deinstantiate + re-instantiate (fresh context)
js run -f <name>       # load a JS program from an embedded manifest (Phase 3)
```

`js eval` pseudocode (§7.4). The console itself is configured for **UART0** (P4 board), so all
`printf`/`esp_log` output reaches the user over the CH343 bridge.

### 5.9 Memory layout at runtime

```
INTERNAL SRAM (~768 KB)                 PSRAM (32 MB, cached)
┌──────────────────────┐               ┌──────────────────────────────────┐
│ FreeRTOS tasks/stacks │               │ WAMR runtime pool  (2 MB)        │
│ esp_console buffers  │  heap_caps ──▶│   └─ module instances, exec_env   │
│ DMA buffers           │   malloc      │ QuickJS guest heap (inside wasm   │
│ IRAM ISRs             │   spills ──▶  │   linear memory, capped 256 KB)  │
└──────────────────────┘               │ IDF heap (remainder)             │
                                       └──────────────────────────────────┘
FLASH (16 MB, MMU-cached)
┌──────────────────────────────────────────┐
│ bootloader | partitions | app firmware   │
│   └─ code (IROM), const data (DROM)      │
│   └─ embedded quickjs.wasm blob (DROM)   │  ← EMBED_FILES
└──────────────────────────────────────────┘
```

---

## 6. API References

### 6.1 WAMR native API (`wasm_export.h`)

Source: `ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--.../sources/09-wamr-wasm-export-header.h`.
Canonical doc: `sources/06-wamr-embed-wamr.md` and `sources/05-wamr-export-native-api.md`.

| Function | Signature (abridged) | Returns / Effect |
|---|---|---|
| `wasm_runtime_full_init` | `bool (RuntimeInitArgs *args)` | Init runtime with pool + natives + max threads. |
| `wasm_runtime_init` | `bool (void)` | Init with default (os) allocator. |
| `wasm_runtime_register_natives` | `bool (const char *module, NativeSymbol *syms, uint32 n)` | Register host fns for an import module. Call before `load`. |
| `wasm_runtime_load` | `wasm_module_t (const uint8 *buf, uint32 size, char *err, uint32 err_len)` | Parse wasm bytes → module. |
| `wasm_runtime_instantiate` | `wasm_module_inst_t (wasm_module_t, uint32 stack, uint32 heap, char *err, uint32 err_len)` | Create instance; linear memory ready. |
| `wasm_runtime_lookup_function` | `wasm_function_inst_t (wasm_module_inst_t, const char *name)` | Find an export by name. |
| `wasm_runtime_create_exec_env` | `wasm_exec_env_t (wasm_module_inst_t, uint32 stack)` | Execution env for calling. |
| `wasm_runtime_call_wasm` | `bool (wasm_exec_env_t, wasm_function_inst_t, uint32 argc, uint32 *argv)` | Call; args/results in 32-bit `argv`. |
| `wasm_runtime_call_wasm_a` | `bool (..., wasm_val_t *results, uint32 n_args, wasm_val_t *args)` | Typed-value variant. |
| `wasm_runtime_module_dup_data` | `uint64_t (wasm_module_inst_t, const char *src, uint64 size)` | Alloc in guest mem + copy host data; returns guest ptr. |
| `wasm_runtime_module_malloc` / `module_free` | `uint64_t` / `void` | Alloc/free inside guest linear memory. |
| `wasm_runtime_get_exception` | `const char * (wasm_module_inst_t)` | Last guest exception string. |
| `wasm_runtime_get_mem_alloc_info` | `bool (mem_alloc_info_t *)` | Pool total/free/highmark (status cmd). |
| `wasm_runtime_destroy_exec_env` / `deinstantiate` / `unload` / `destroy` | `void` | Teardown. |

### 6.2 `NativeSymbol` and signature strings

```c
typedef struct NativeSymbol {
    const char *symbol;   // import field name the guest uses
    void *func_ptr;        // host C function
    const char *signature; // "(args)return"
    void *attachment;      // optional userdata (NULL usually)
} NativeSymbol;
```

**Every native function's first parameter is `wasm_exec_env_t` (not written in the signature).**
Signature letters (from `sources/05-wamr-export-native-api.md`):

| Letter | Meaning |
|---|---|
| `i` | i32 |
| `I` | i64 |
| `f` | f32 |
| `F` | f64 |
| `r` | externref / GC reference |
| `*` | a buffer address in the guest |
| `~` | byte length of the preceding `*` buffer (must follow `*`) |
| `$` | a string in the guest (WAMR copies + NUL-terminates) |

Examples: `(ii)i` = two i32 → i32; `($)` = one string, no return (our `host_print`); `(*~)` =
buffer + length, no return.

### 6.3 QuickJS C API (guest side)

Canonical: `https://bellard.org/quickjs/quickjs.html`. The functions you call *inside* `wasm_main.c`:

```c
JSRuntime *JS_NewRuntime(void);
JSContext *JS_NewContext(JSRuntime *rt);
void       JS_SetMemoryLimit(JSRuntime *rt, size_t limit);
JSValue    JS_Eval(JSContext *ctx, const char *input, size_t input_len,
                   const char *filename, int eval_flags);
JSValue    JS_GetGlobalObject(JSContext *ctx);
int        JS_SetPropertyStr(JSContext *ctx, JSValue this_obj,
                             const char *prop, JSValue val);
JSValue    JS_NewCFunction(JSContext *ctx, JSCFunction *func,
                           const char *name, int min_args);
const char* JS_ToCString(JSContext *ctx, JSValueConst val);
void       JS_FreeCString(JSContext *ctx, const char *ptr);
void       JS_FreeValue(JSContext *ctx, JSValue v);
void       JS_FreeContext(JSContext *ctx);
void       JS_FreeRuntime(JSRuntime *rt);

typedef JSValue (*JSCFunction)(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv);
```

`eval_flags`: `JS_EVAL_TYPE_GLOBAL` (run in global scope), `JS_EVAL_FLAG_STRICT`, etc.

### 6.4 ESP-IDF APIs used

- `heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)` — allocate the WAMR pool in PSRAM.
- `esp_console_cmd_register(&cmd)` — register `js` commands (component `console`).
- `esp_timer_get_time()` — `host_millis` implementation.
- `gpio_set_level(pin, level)` — `host_gpio_write` (Phase 2).

---

## 7. Pseudocode

### 7.1 `wasm-src/wasm_main.c` (the WASM guest wrapper)

```c
#include "quickjs.h"
#include <string.h>

/* ---- Boundary A imports (resolved by WAMR native symbols, module "env") ---- */
__attribute__((import_module("env"), import_name("host_print")))
extern void host_print(const char *s, int len);
__attribute__((import_module("env"), import_name("host_millis")))
extern int host_millis(void);
__attribute__((import_module("env"), import_name("host_gpio_write")))
extern void host_gpio_write(int pin, int val);

/* ---- Boundary B: JS globals implemented in C, calling the imports ---- */
static JSRuntime *rt;
static JSContext  *ctx;

static JSValue js_print(JSContext *c, JSValueConst ths, int argc, JSValueConst *argv) {
    if (argc >= 1) {
        const char *s = JS_ToCString(c, argv[0]);
        if (s) { host_print(s, (int)strlen(s)); JS_FreeCString(c, s); }
    }
    return JS_UNDEFINED;
}
static JSValue js_millis(JSContext *c, JSValueConst ths, int argc, JSValueConst *argv) {
    return JS_NewInt32(c, host_millis());
}
static JSValue js_gpio_write(JSContext *c, JSValueConst ths, int argc, JSValueConst *argv) {
    int pin = 0, val = 0;
    if (argc >= 2) { JS_ToInt32(c, &pin, argv[0]); JS_ToInt32(c, &val, argv[1]); }
    host_gpio_write(pin, val);
    return JS_UNDEFINED;
}

/* ---- Exported entry points (called by WAMR host) ---- */
void qjs_init(void) {
    rt = JS_NewRuntime();
    JS_SetMemoryLimit(rt, 256 * 1024);
    ctx = JS_NewContext(rt);
    JSValue g = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, g, "print",      JS_NewCFunction(ctx, js_print,      "print", 1));
    JS_SetPropertyStr(ctx, g, "millis",     JS_NewCFunction(ctx, js_millis,     "millis", 0));
    JS_SetPropertyStr(ctx, g, "gpio_write", JS_NewCFunction(ctx, js_gpio_write, "gpio_write", 2));
    JS_FreeValue(ctx, g);
}

int qjs_eval(const char *src, int len) {
    if (!ctx) qjs_init();
    JSValue r = JS_Eval(ctx, src, (size_t)len, "<console>", JS_EVAL_TYPE_GLOBAL);
    int ok = JS_IsException(r) ? -1 : 0;
    if (ok != 0) {
        JSValue e = JS_GetException(ctx);
        const char *m = JS_ToCString(ctx, e);
        if (m) { host_print(m, (int)strlen(m)); host_print("\n", 1); JS_FreeCString(ctx, m); }
        JS_FreeValue(ctx, e);
    }
    JS_FreeValue(ctx, r);
    return ok;
}
```

### 7.2 `main/wasm_host_api.cpp` (Boundary A host side — port of `0079`)

```cpp
#include "wasm_export.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <cstdio>

static void host_print(wasm_exec_env_t, const char *s)      { fputs(s, stdout); fflush(stdout); }
static int  host_millis(wasm_exec_env_t)                    { return (int)(esp_timer_get_time()/1000); }
static void host_gpio_write(wasm_exec_env_t, int p, int v)  { gpio_set_level((gpio_num_t)p, v?1:0); }

static NativeSymbol kHostSymbols[] = {
    { "host_print",      (void*)host_print,      "($)",  nullptr },
    { "host_millis",     (void*)host_millis,     "()i",  nullptr },
    { "host_gpio_write", (void*)host_gpio_write, "(ii)", nullptr },
};

bool init_wasm_host_api(void) {
    return wasm_runtime_register_natives("env", kHostSymbols,
            sizeof(kHostSymbols)/sizeof(kHostSymbols[0]));
}
```

### 7.3 `main/wasm_runtime_service.cpp` (init + status — port of `0079`)

```cpp
#include "wasm_export.h"
#include "esp_heap_caps.h"
constexpr size_t kPool = 2*1024*1024;
static void *g_pool = nullptr;

bool init_wasm_runtime(void) {
    g_pool = heap_caps_malloc(kPool, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!g_pool) g_pool = heap_caps_malloc(kPool, MALLOC_CAP_8BIT);  // fallback
    RuntimeInitArgs a = {};
    a.mem_alloc_type = Alloc_With_Pool;
    a.running_mode   = Mode_Interp;
    a.mem_alloc_option.pool.heap_buf  = g_pool;
    a.mem_alloc_option.pool.heap_size = kPool;
    return wasm_runtime_full_init(&a);
}
```

### 7.4 `main/js_command.cpp` (the `js eval` console command)

```cpp
#include "esp_console.h"
#include "wasm_export.h"
#include "wasm_runtime_service.h"
#include "wasm_host_api.h"

extern const uint8_t quickjs_wasm_start[] asm("_binary_quickjs_wasm_start");
extern const uint8_t quickjs_wasm_end[]   asm("_binary_quickjs_wasm_end");

static wasm_module_t      g_mod  = nullptr;
static wasm_module_inst_t g_inst = nullptr;

static bool ensure_session(void) {
    if (g_inst) return true;
    char err[128];
    if (!g_mod)
        g_mod = wasm_runtime_load((uint8_t*)quickjs_wasm_start,
                  quickjs_wasm_end - quickjs_wasm_start, err, sizeof err);
    if (!g_mod) return false;
    g_inst = wasm_runtime_instantiate(g_mod, 16*1024, 512*1024, err, sizeof err);
    if (!g_inst) return false;
    // call qjs_init
    wasm_function_inst_t f = wasm_runtime_lookup_function(g_inst, "qjs_init");
    wasm_exec_env_t e = wasm_runtime_create_exec_env(g_inst, 16*1024);
    wasm_runtime_call_wasm(e, f, 0, nullptr);
    wasm_runtime_destroy_exec_env(e);
    return true;
}

static int cmd_js_eval(int argc, char **argv) {
    if (argc < 2) { printf("usage: js eval \"<src>\"\n"); return 1; }
    if (!ensure_session()) { printf("wasm session failed\n"); return 1; }
    const char *src = argv[1];
    size_t len = strlen(src);
    uint64_t p = wasm_runtime_module_dup_data(g_inst, src, len);
    wasm_function_inst_t f = wasm_runtime_lookup_function(g_inst, "qjs_eval");
    wasm_exec_env_t e = wasm_runtime_create_exec_env(g_inst, 16*1024);
    uint32_t argv2[2] = { (uint32_t)p, (uint32_t)len };
    bool ok = wasm_runtime_call_wasm(e, f, 2, argv2);
    if (!ok) printf("exception: %s\n", wasm_runtime_get_exception(g_inst));
    wasm_runtime_module_free(g_inst, p);
    wasm_runtime_destroy_exec_env(e);
    return ok ? 0 : 1;
}

void register_js_commands(void) {
    const esp_console_cmd_t c = { .command="js", .help="js eval|repl|reset|status",
                                   .func=cmd_js_dispatch };
    esp_console_cmd_register(&c);
}
```

### 7.5 `main/app_main.cpp`

```cpp
#include "esp_console.h"
extern "C" void app_main(void) {
    esp_console_repl_t *repl; esp_console_repl_config_t cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_new_repl_uart(&cfg, &repl);     // UART0 console (P4 board)
    init_wasm_runtime();                        // WAMR pool in PSRAM
    init_wasm_host_api();                        // register "env" natives
    register_js_commands();                      // js eval/repl/reset/status
    esp_console_start_repl(repl);
}
```

---

## 8. Diagrams (reference set)

### 8.1 End-to-end call flow

```
[console] js eval "print(1+2)"
   │
   ▼
cmd_js_eval  ──dup_data──▶  [wasm linear mem: "print(1+2)"]
   │                                │
   │ call_wasm(qjs_eval, ptr,len)   │
   ▼                                ▼
WAMR interpreter runs qjs_eval (inside quickjs.wasm)
   │
   │ QuickJS: JS_Eval("print(1+2)")
   │   └─ resolves global "print" → js_print (C in wasm)
   │       └─ js_print calls host_print(s,len)   [wasm import "env"]
   ▼
WAMR dispatches "env.host_print" → host_print_native (ESP-IDF)
   │
   ▼
fputs(s, stdout) ──▶ UART0 ──▶ CH343 ──▶ host terminal:  3
```

### 8.2 Build pipeline

```
  HOST PC                                  EMBEDDED (ESP32-P4)
  ───────                                  ──────────────────
  bellard/quickjs  ┐
  wasm_main.c      ├─ wasi-sdk clang ──▶ quickjs.wasm ──┐
  (-Wl,--no-entry, │   (wasm32-wasi)        │            │ copy into main/
   --export=...)   ┘                        │            ▼
                                      wasm-objdump    EMBED_FILES
                                      (verify)          │
                                                        ▼
                                              idf.py build  ──▶ flash ──▶ P4
```

### 8.3 Module dependency graph

```
app_main.cpp
 ├── wasm_runtime_service  ── WAMR (espressif__wasm-micro-runtime)
 ├── wasm_host_api         ── WAMR native symbols ("env")
 ├── wasm_runner           ── WAMR load/instantiate/call
 ├── js_command            ── esp_console
 └── quickjs.wasm (EMBED)  ── imported by WAMR at runtime
```

---

## 9. File References (all absolute)

**This ticket's research sources:**
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/sources/09-wamr-wasm-export-header.h` — the authoritative WAMR C API header.
- `.../sources/06-wamr-embed-wamr.md` — embedding lifecycle + buffer passing.
- `.../sources/05-wamr-export-native-api.md` — native symbol signature legend.
- `.../sources/07-wamr-build-options.md` — WAMR cmake/config options.
- `.../sources/10-vercel-quickjs-wasi-readme.md` — QuickJS-as-WASM prior art.
- `.../sources/12-esp32-p4-memory-types.md` — P4 memory model.
- `.../sources/02-quickjs-bellard-home.md` — QuickJS facts/version.

**Local prior art (copy these patterns):**
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp` — WAMR init + pool + status.
- `.../0079.../main/wasm_module_runner.cpp` — load/instantiate/lookup/call lifecycle.
- `.../0079.../main/wasm_host_api.cpp` — `NativeSymbol` registration + command queueing.
- `.../0079.../main/idf_component.yml` — WAMR component dependency (v2.4.0~1).
- `.../0079.../main/CMakeLists.txt` — `EMBED_FILES` + `REQUIRES espressif__wasm-micro-runtime`.
- `.../0079.../managed_components/espressif__wasm-micro-runtime/build-scripts/esp-idf/wamr/Kconfig` — exact `CONFIG_WAMR_*` symbols.
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0099-esp32-p4-picocalc-display-keyboard/sdkconfig.defaults` — P4 target/console/PSRAM baseline.
- `.../0099.../CMakeLists.txt` and `README.md` — P4 build/flash/monitor commands.

**Target firmware (created by this ticket):**
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm/` — scaffold ready for the intern.

---

## 10. Phased Implementation Plan

**Phase 0 — Host-side wasm (do this first, on your PC):**
1. Install `wasi-sdk`; verify `$WASI_SDK_PATH/bin/clang`.
2. Vendor `bellard/quickjs` into `0100/.../wasm-src/`.
3. Write `wasm_main.c` (§7.1) and `build-quickjs-wasm.sh` (§5.2).
4. Build `quickjs.wasm`; verify with `wasm-objdump -x` (imports `env.*` + `wasi_*`, exports `qjs_init`/`qjs_eval`).
5. **Host test with `iwasm`:** write a tiny host C program that registers `host_print` and calls `qjs_eval("print(1+2)")`; expect console output `3`. Do not flash until this passes.

**Phase 1 — Minimal firmware `0100`:**
1. Create the scaffold (provided by this ticket).
2. Port `wasm_runtime_service.cpp` and `wasm_host_api.cpp` from `0079` (§7.2–7.3).
3. Write `wasm_runner.cpp` (instantiate once, `qjs_init`, `qjs_eval`).
4. Write `js_command.cpp` (`js eval`, `js status`).
5. Copy `quickjs.wasm` into `main/` and `idf.py build`.
6. Flash and run `js eval "print('hello from wasm quickjs')"`.

**Phase 2 — REPL + peripherals:**
1. `js repl` (line-buffered, persistent context) and `js reset`.
2. Add `host_gpio_write` + `host_millis` + `gpio_write`/`millis` JS globals.
3. Add a `js bench` command (eval a loop, measure latency) for validation.

**Phase 3 — Polish:**
1. AOT compilation with `wamrc --target=riscv32` (DR-2).
2. JS program manifest (`js run -f name`) over `EMBED_FILES`.
3. Memory profiling (`CONFIG_WAMR_ENABLE_MEMORY_PROFILING=y`) surfaced in `js status`.

---

## 11. Build, Flash, and Verify

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm
source ~/esp/esp-idf-5.4.2/export.sh
idf.py set-target esp32p4
idf.py build
PORT=/dev/serial/by-id/usb-1a86_USB_Single_Serial_*-if00   # CH343 bridge (from 0099)
idf.py -p "$PORT" flash monitor
```

Expected at the console (UART0):

```
I (xxx) 0100_qjs: WAMR ready (version=W.W.W, interp=yes, ...)
qjs> js status
runtime=ready  pool_size=2097152  pool_buffer_external=yes  wamr.heap_free=...
qjs> js eval "print(1+2)"
3
qjs> js eval "for(let i=0;i<3;i++) print('row',i, millis())"
row 0 0
row 1 1
row 2 2
```

If you see `exception: ...` from `wasm_runtime_get_exception`, check: (a) the wasm imports match
the registered `env` symbol names exactly, (b) signatures match (a `$` host fn receiving a number
will misread), (c) `qjs_init` was called before `qjs_eval`.

---

## 12. Testing and Validation Strategy

- **Unit (host):** `iwasm` smoke test of `qjs_eval` for `print`, arithmetic, a 1k-iteration loop,
  and a thrown exception (verify the exception string reaches `host_print`).
- **Integration (device):** `js eval` of a fixed JS corpus; assert exact console output.
- **Memory:** `js status` reports WAMR pool free/high-water and (with memory profiling) the QuickJS
  guest heap high-water. Confirm the pool stays in PSRAM (`pool_buffer_external=yes`).
- **Stability:** run `js repl` for 100 evals including allocations and exceptions; confirm no
  watchdog, no heap corruption (`heap_caps_check_integrity`), and `js reset` recovers cleanly.
- **Performance baseline:** `js bench` measures eval latency for a known script (target: sub-100 ms
  for small scripts on the 400 MHz P4 interp; record the number for DR-2).

---

## 13. Risks, Alternatives, Open Questions

**Risks**
- **PSRAM latency vs interpreter overhead:** wasm interp + QuickJS interp is two layers of
  interpretation; heavy JS may be slow. Mitigation: AOT (DR-2); keep Phase-1 scripts small.
- **Memory pressure:** a 2 MB pool + 256 KB QuickJS heap is an estimate; too small → `qjs_eval`
  throws OOM; too large → wasteful. Validate with profiling.
- **WASI coverage:** QuickJS may use WASI calls WAMR doesn't fully implement (e.g. certain `clock`
  or `random`). If instantiation fails on a missing import, check `wasm-objdump` imports and the
  WAMR WASI support matrix.
- **Reactor `_initialize`:** wasi-libc may emit an `_initialize` for global ctors; ensure WAMR runs
  it (or the build avoids it). Validate in Phase 0.

**Alternatives considered**
- Native QuickJS (no WASM) — the `microquickjs` path; loses sandboxing/portability (see DR-1).
- Use `vercel-labs/quickjs-wasi`'s prebuilt `quickjs.wasm` — fast start, but it is a JS-facing
  reactor designed for the web; building our own thin `wasm_main.c` is simpler to bridge to WAMR
  native symbols.
- `wamr-app-framework` — overkill for a single embedded engine.

**Open questions**
- Exact `wamrc --target=` string for ESP32-P4 RISC-V (likely `riscv32` with the P4's mabi/march).
- Whether SIMD (`-msimd128`) helps QuickJS on WAMR's interp (likely no; SIMD needs AOT/JIT).

---

## 14. Decision Records

**DR-1 — Build QuickJS as a WASI reactor (library) module, not a command.**
- *Context:* WASI commands export `_start`; reactors export `_initialize` + named fns.
- *Options:* (a) command — reuse QuickJS `qjs` main, needs WAMR WASI FS to pass JS via file; (b)
  reactor — export `qjs_eval`, pass JS via memory, no FS.
- *Decision:* (b) reactor.
- *Rationale:* matches the workspace's existing WAMR pattern (`0079` calls named exports and
  passes data in memory), avoids a virtual filesystem, and gives a persistent JS context for a REPL.
- *Consequences:* requires writing `wasm_main.c`; must handle `_initialize`/ctors. Status: accepted.

**DR-2 — Interpreter baseline now; AOT later.**
- *Context:* WAMR interp is simplest; AOT pre-compiles wasm→native for speed.
- *Decision:* Phase 1 uses `Mode_Interp`; Phase 3 evaluates `wamrc --target=riscv32`.
- *Rationale:* ship a correct, debuggable baseline first; AOT adds a host build step and
  target-arch complexity best added once the interp path is proven.
- *Consequences:* perf is worse until Phase 3. Status: accepted.

**DR-3 — WAMR pool in PSRAM, 2 MB.**
- *Context:* 768 KB internal SRAM is too small for QuickJS+WAMR.
- *Decision:* allocate the 2 MB WAMR pool with `MALLOC_CAP_SPIRAM` (fall back to internal), as `0079` does.
- *Rationale:* 32 MB PSRAM makes this cheap; pool containment keeps wasm allocations bounded.
- *Consequences:* pool access is cached PSRAM latency; acceptable for scripting. Status: accepted.

**DR-4 — Console = UART0 (not USB Serial/JTAG).**
- *Context:* P4 has no native USB Serial/JTAG; the board uses a CH343 bridge on UART0.
- *Decision:* `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` (GPIO37/38), from `0099`.
- *Rationale:* matches the verified P4 bring-up; the S3 AGENTS.md console guidance does not apply.
- *Consequences:* none beyond the board's existing wiring. Status: accepted.

---

## 15. References

- WAMR: `https://github.com/bytecodealliance/wasm-micro-runtime` and `https://bytecodealliance.github.io/wamr.dev/`
- WAMR app framework: `https://github.com/bytecodealliance/wamr-app-framework`
- QuickJS: `https://bellard.org/quickjs/` and `https://github.com/bellard/quickjs`
- Micro QuickJS: `https://github.com/bellard/mquickjs`
- wasi-sdk: `https://github.com/WebAssembly/wasi-sdk`
- QuickJS-as-WASM prior art: `https://github.com/vercel-labs/quickjs-wasi`
- ESP32-P4: `https://www.espressif.com/en/products/socs/esp32-p4` and ESP-IDF P4 memory-types
- Local prior art: `0079-papers3-wamr-assemblyscript-console`, `0082-papers3-wamr-allocator-control`, `0099-esp32-p4-picocalc-display-keyboard`
- Primary sources archived in this ticket: `sources/01..14-*.md` and `sources/09-wamr-wasm-export-header.h`
