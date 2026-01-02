---
Title: Diary
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.local/bin/remarkable_upload.py
      Note: Uploader script used to convert ticket markdown to PDF and upload to reMarkable
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c
      Note: Implements build_atoms and supports -m32/-m64; key to generating an ESP32-safe 32-bit stdlib
    - Path: imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_atom.h
      Note: Atom offset header used by the parser; must be generated with -m32/-a to match esp32_stdlib.h
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp
      Note: New C++ REPL-only entrypoint wiring console+editor+repeat evaluator (commit 1a25a10)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/console/UartConsole.cpp
      Note: |-
        UART-backed console abstraction used for QEMU bring-up (commit 1a25a10)
        QEMU input fix via FIFO polling (commit 7bc80f2)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/console/UsbSerialJtagConsole.cpp
      Note: USB Serial/JTAG console implementation for Cardputer REPL (commit d3e9f19)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h
      Note: Host-generated stdlib header (64-bit shaped by default); useful for comparing against esp32_stdlib.h
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h
      Note: Generated 32-bit ESP32-safe stdlib header (created via esp_stdlib_gen -m32)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c
      Note: Firmware-side stubs + include site that defines js_stdlib from esp32_stdlib.h
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib_gen
      Note: Host stdlib generator binary; used by tools/gen_esp32_stdlib.sh
    - Path: imports/esp32-mqjs-repl/mqjs-repl/legacy/minimal_stdlib.h
      Note: Legacy bring-up-only stdlib (kept for historical reference; not used by current firmware)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/mqjs_stdlib.c
      Note: Generator program main() that calls build_atoms() to emit esp_stdlib.h
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp
      Note: Core REPL loop + meta-commands + evaluator dispatch (commit 1a25a10)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: JavaScript evaluator (JS_NewContext + JS_Eval + JS_PrintValueF capture)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/main/eval/ModeSwitchingEvaluator.cpp
      Note: Implements :mode repeat|js switching
    - Path: imports/esp32-mqjs-repl/mqjs-repl/partitions.csv
      Note: Cardputer partition layout baseline (4MB app + 1MB SPIFFS) for commit 881a761
    - Path: imports/esp32-mqjs-repl/mqjs-repl/sdkconfig.defaults
      Note: Cardputer build defaults (8MB flash
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/set_repl_console.sh
      Note: Local helper to switch REPL console transport via sdkconfig (commit d3e9f19)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh
      Note: tmux-driven device repeat REPL smoke test (commit 5aeb539)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh
      Note: Regenerates the ESP32 32-bit stdlib header + atom header deterministically (this step)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_uart_raw.py
      Note: UART-direct device test using pyserial (commit bf99dc4)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh
      Note: |-
        tmux-driven QEMU repeat REPL smoke test (commit 5aeb539)
        Validated prompt-detection; input still not delivered under QEMU (commit 167c629)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh
      Note: UART-direct QEMU stdio test to bypass idf_monitor (commit bf99dc4)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh
      Note: UART-direct QEMU TCP test to bypass idf_monitor (commit bf99dc4)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh
      Note: UART-direct QEMU JS smoke test (proves `var` parses and runs)
    - Path: imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py
      Note: UART-direct device JS smoke test (pyserial)
    - Path: ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/analysis/03-microquickjs-stdlib-atom-table-split-why-var-should-parse-current-state-ideal-structure.md
      Note: Primary analysis of stdlib/atom-table split and -m32 generation plan
    - Path: ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md
      Note: Design doc for splitting firmware main into C++ components and adding RepeatEvaluator
    - Path: ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/design-doc/03-qemu-uart-rx-root-cause-workaround-plan.md
      Note: Consolidated analysis+plan from intern research
    - Path: ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer/sources/intern-research-qemu-uart-results.md
      Note: Evidence for QEMU UART RX interrupt limitations
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---









# Diary

## Goal

Document the step-by-step process of analyzing and porting the MicroQuickJS REPL firmware to Cardputer, focusing on actions taken, decisions made, challenges encountered, and lessons learned.

## Step 1: Initial Exploration and Documentation Setup

Started by exploring the imported firmware and setting up documentation structure to track findings and analysis.

### What I did

- Created ticket `0014-CARDPUTER-JS` using docmgr
- Created diary document for tracking process
- Examined imported firmware structure in `imports/esp32-mqjs-repl/`
- Read delivery documentation to understand what was delivered
- Searched codebase for Cardputer hardware specs and console patterns

### Why

Need to establish baseline understanding before making changes. Also wanted to capture analysis in structured documents rather than ad-hoc notes.

### What worked

- Found comprehensive delivery docs (`DELIVERY_SUMMARY.md`, `FINAL_DELIVERY_README.md`) that explain the firmware well
- Located Cardputer reference implementations (`0015-cardputer-serial-terminal`) showing USB Serial JTAG usage
- Identified key differences: console interface (UART vs USB Serial JTAG), flash size (2MB vs 8MB), memory constraints

### What didn't work

- Initially put too much detailed analysis in the diary - user correctly pointed out that analysis should be in separate documents
- Had to restructure: moved detailed technical analysis to separate analysis documents

### What I learned

- ESP-IDF has distinction between "console output" (where `printf` goes) and "REPL" (interactive command loop). Current firmware uses raw UART driver, not `esp_console` framework.
- Cardputer tutorials consistently use 8MB flash, 4MB app partition, 1MB storage partition pattern
- USB Serial JTAG requires driver installation before use, different from UART which is always available

### What was tricky

- Understanding the console vs REPL distinction - had to search multiple examples to clarify
- Determining what belongs in diary vs analysis documents - user feedback helped clarify

### What warrants a second pair of eyes

- Memory budget calculation: need to verify 64KB JS heap + buffers fits in Cardputer's 512KB SRAM
- Console migration approach: should we keep UART option for QEMU testing?

### What should be done in the future

- Create separate analysis documents for detailed technical findings (done)
- Test QEMU execution of current firmware to verify it works
- Create implementation plan document outlining code changes needed

### Code review instructions

- Review analysis documents: `analysis/01-current-firmware-configuration-analysis.md` and `analysis/02-cardputer-port-requirements.md`
- Check imported firmware: `imports/esp32-mqjs-repl/mqjs-repl/legacy/main.c`
- Compare with Cardputer reference: `0015-cardputer-serial-terminal/main/hello_world_main.cpp`

## Step 2: Make builds reproducible + run the baseline build

This step makes it easy to build the imported `mqjs-repl` project without remembering to source ESP-IDF in each shell. It also validates that the project still compiles cleanly in *this* environment before we start Cardputer-specific changes.

**Commit (code):** 1a42a4e7049383498a3473765889c219815137ad — "Build: add ESP-IDF auto-source wrapper for mqjs-repl"

### What I did

- Found the repo’s standard “auto-source ESP-IDF” wrapper script at:
  - `0017-atoms3r-web-ui/build.sh`
  - `0016-atoms3r-grove-gpio-signal-tester/build.sh`
- Copied that pattern into the imported project as `imports/esp32-mqjs-repl/mqjs-repl/build.sh` and committed it.
- Ran a baseline build using the wrapper:

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./build.sh set-target esp32s3
./build.sh build
```

- Attempted a QEMU run (to validate the workflow on this machine):

```bash
./build.sh qemu monitor
```

### Why

- I want every subsequent compile step to be “one command”, so we can iterate quickly and keep commits tightly scoped.
- Before porting to Cardputer, we need a known-good baseline build in our local toolchain (ESP-IDF 5.4.1).

### What worked

- `./build.sh set-target esp32s3 && ./build.sh build` succeeded and produced `build/mqjs-repl.bin`.

### What didn’t work

- QEMU run failed with:
  - `qemu-system-xtensa is not installed. Please install it using "python $IDF_PATH/tools/idf_tools.py install qemu-xtensa"`

### What I learned

- The wrapper script does what we want: it detects `ESP_IDF_VERSION` and sources `~/esp/esp-idf-5.4.1/export.sh` only when needed.
- On this environment, ESP-IDF is installed, but the **QEMU Xtensa binary** is not yet installed via `idf_tools.py`.

### What was tricky

- Keeping work scoped: the git repo has unrelated local changes, so every commit needs explicit `git add <paths...>` to avoid staging noise.

### What warrants a second pair of eyes

- Whether we want to keep QEMU support as a first-class workflow for Cardputer (USB Serial JTAG vs UART/monitoring differences may matter).

### What should be done in the future

- Install the required ESP-IDF QEMU tool (`qemu-xtensa`) and re-run `./build.sh qemu monitor` to confirm we can boot into the REPL before starting the Cardputer port.

## Step 3: Install QEMU tool + run monitor in tmux (interactive)

This step installs the missing ESP-IDF-managed Xtensa QEMU binary and sets up an interactive workflow via tmux. The key nuance is that after installing a new ESP-IDF tool, you need a freshly-exported environment so `idf.py` can find the new binaries.

### What I did

- Installed the ESP-IDF QEMU tool:

```bash
cd ~/esp/esp-idf-5.4.1
. ./export.sh >/dev/null
python "$IDF_PATH/tools/idf_tools.py" install qemu-xtensa
```

- Tried to run QEMU again via:

```bash
cd imports/esp32-mqjs-repl/mqjs-repl
./build.sh qemu monitor
```

### What didn’t work

- `idf.py qemu monitor` still reported:
  - `qemu-system-xtensa is not installed`

### What I learned

- The failure is caused by environment drift: if `ESP_IDF_VERSION` is already set in your shell, our `build.sh` **skips sourcing** `export.sh`, and `idf.py` may not pick up newly installed tool paths.
- Running `qemu monitor` in a **fresh shell** (or forcing `export.sh` to re-run) should resolve it.

### What I’d do next (recommended interactive workflow)

- Run QEMU in tmux so the monitor stays up and you can interact with it:

```bash
tmux new-session -s mqjs-qemu -c /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl
```

Then inside tmux, run:

```bash
unset ESP_IDF_VERSION
./build.sh qemu monitor
```

This forces `build.sh` to source `~/esp/esp-idf-5.4.1/export.sh` again (with qemu now installed), and `idf.py` should stop complaining about missing `qemu-system-xtensa`.

## Step 4: Validate firmware under QEMU (found SPIFFS/partition mismatch)

This step attempts the actual firmware validation we care about: boot in QEMU and reach the `js>` prompt. We got QEMU + `idf_monitor` running interactively in tmux, but the firmware exited early because SPIFFS couldn’t find its partition.

### What I did

- Started `idf.py qemu monitor` in a detached tmux session so we can interact with it:

```bash
tmux new-session -d -s mqjs-qemu -c /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl \
  "unset ESP_IDF_VERSION; ./build.sh qemu monitor"
```

- Captured the monitor output from tmux to confirm boot behavior.
- Decoded the generated partition-table binary to see what QEMU actually flashed:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl
unset ESP_IDF_VERSION
. /home/manuel/esp/esp-idf-5.4.1/export.sh >/dev/null
python "$IDF_PATH/components/partition_table/gen_esp32part.py" build/partition_table/partition-table.bin | head -50
```

### What worked

- QEMU launched and `idf_monitor` attached to `socket://localhost:5555`, showing ESP-IDF boot logs.

### What didn’t work

- Firmware aborted before starting the REPL:
  - `E (...) SPIFFS: spiffs partition could not be found`
  - `E (...) mqjs-repl: Failed to initialize SPIFFS`

### What I learned

- The built partition table **does not include the `storage` partition**, despite the repo having a `partitions.csv` that defines it.
- Root cause is in `sdkconfig`: it’s currently set to `CONFIG_PARTITION_TABLE_SINGLE_APP=y` and `CONFIG_PARTITION_TABLE_FILENAME="partitions_singleapp.csv"`, so the build ignores our custom `partitions.csv`.

### What was tricky

- This is a good example of why “runs in QEMU” needs to be validated in *our* environment: the project’s source `partitions.csv` looked correct, but the generated binary used by QEMU didn’t match.

### What should be done next

- Switch the project to use the custom partition table:
  - Enable `CONFIG_PARTITION_TABLE_CUSTOM=y`
  - Set `CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"`
  - Disable `CONFIG_PARTITION_TABLE_SINGLE_APP`
- Rebuild, confirm `gen_esp32part.py` shows the `storage` entry, then re-run QEMU and validate we reach `js>` and can evaluate expressions.

## Step 5: MicroQuickJS extension surface deep dive (stdlib tables, C functions, user classes)

This step zooms in on the *actual* MicroQuickJS embedding model used by our firmware, specifically how native functions/classes are exposed to JS. The key outcome is that MicroQuickJS on embedded is **not** “QuickJS with `JS_NewCFunction()`”; instead it relies on a ROM-style **stdlib table** (`JSSTDLibraryDef`) plus fixed-index C function tables, with explicit GC reference helpers for keeping JS values alive.

In practice this means: if we want to expose “higher-level ESP32 functionality” (GPIO/I2C/WiFi/etc), we should plan to add bindings via the **stdlib generator path** (host-side `mquickjs_build.c`) and then pass our generated `js_stdlib` into `JS_NewContext()` on device.

### What I did

- Read the legacy embedded firmware entry point (`imports/esp32-mqjs-repl/mqjs-repl/legacy/main.c`) to see how the VM was created and used from FreeRTOS before the split.
- Read MicroQuickJS public API (`imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`) to understand what’s available (and what’s *not*).
- Read the stdlib build utility interfaces (`.../components/mquickjs/mquickjs_build.h` + `mquickjs_build.c`) to understand how `esp_stdlib.h` is generated.
- Read MicroQuickJS internal wiring for stdlib and C-function dispatch (`.../components/mquickjs/mquickjs.c`) at the call sites for `stdlib_def->stdlib_table`, `c_function_table`, `c_finalizer_table`.
- Read the upstream “how to extend” example (`.../components/mquickjs/example.c` + `example_stdlib.c`) to see a complete user class + finalizer + C closure pattern.

### Why

- We need a repo-accurate “how to add native bindings” manual before adding any Cardputer-specific “JS APIs”.
- The design constraints (no dynamic C function creation, fixed function indices, explicit GC refs) materially affect how we should architect bindings and inter-task communication.

### What worked

- Found the core embedding contract:
  - `JS_NewContext(void *mem_start, size_t mem_size, const JSSTDLibraryDef *stdlib_def)` (memory pool + stdlib definition).
  - `JSSTDLibraryDef` includes `stdlib_table`, `c_function_table`, `c_finalizer_table`, and offsets used during `stdlib_init()` inside the engine.
- Confirmed how “native callable functions” work:
  - The engine only exposes `JS_NewCFunctionParams()` (no `JS_NewCFunction()`), so callable native functions are tied to the **C function table** entries and indices.
- Found a concrete user-class pattern (allocation + `JS_SetOpaque` + finalizer table + `JS_NewObjectClassUser`) in `example.c`.
- Found the callback / “keep JS value alive” pattern via `JSGCRef` + `JS_AddGCRef` / `JS_DeleteGCRef` (also used by the host REPL’s timer implementation).

### What didn’t work / surprises

- The embedded firmware currently includes `minimal_stdlib.h` (an empty stdlib table) rather than `esp_stdlib.h`. That’s fine for a minimal REPL, but it means “adding real native APIs” will likely require switching to a generated stdlib definition (or building our own) rather than trying to bolt on JS-callable functions ad-hoc at runtime.

### What I learned

- **MicroQuickJS is table-driven**: exposing builtins and native functions is primarily a compile-time (or “host-time”) concern using `mquickjs_build.c`, not a runtime “register module” approach.
- **User objects need finalizers in the stdlib**: to safely free `opaque` pointers, finalizers are looked up by `(class_id - JS_CLASS_USER)` in `c_finalizer_table`.
- **Calling JS from C is stack-based**: you must `JS_StackCheck()`, then `JS_PushArg()` values in the order expected by `JS_Call()`.
- **Cross-task access is a correctness risk**: `main/main.c` creates `js_ctx` in `app_main()` then uses it from `repl_task()`. That’s OK because only one task actually uses it after init, but it’s a reminder that we must enforce “one context, one owning task” once we add more tasks.

### What was tricky to build (conceptually)

- Translating QuickJS instincts (“register native module at runtime”) into MicroQuickJS reality (“generate stdlib table + fixed function indices”).
- Understanding lifetimes without `JS_FreeValue()` / refcount APIs: the right tool is `JSGCRef`, not “dup/free”.

### What warrants a second pair of eyes

- If we decide to keep `minimal_stdlib.h` for footprint reasons, we’ll need a careful plan for adding *just enough* stdlib entries (and not accidentally ballooning the global object / atom tables).
- Concurrency rules: whether MicroQuickJS is safe to create in one task and execute in another (it likely is, but we should treat “context is single-threaded” as the real contract).

### What should be done in the future

- Write a dedicated reference/playbook documenting:
  - How to add a new `ESP.*` namespace (functions + user objects) via the stdlib generator.
  - Memory + GC reference rules and “safe patterns” for callbacks and async-ish APIs.
  - FreeRTOS ownership rules for JS contexts and message passing.

### Code review instructions

- Start with `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h` (public API surface).
- Then read `.../components/mquickjs/mquickjs_build.h` to understand the declarative stdlib description format.
- Use `.../components/mquickjs/example.c` + `example_stdlib.c` as the canonical “how to add native stuff” sample.

## Step 6: Write the extension playbook + FreeRTOS multi-VM brainstorm docs

This step converts the “how it works” understanding into two documents that we can use as a working manual while implementing Cardputer features. The key output is a repo-specific playbook that tells us exactly how to expose ESP-IDF functionality to JS (given MicroQuickJS’ table-driven model), and a design brainstorm that frames safe ways to run multiple scripts/VMs under FreeRTOS and let them communicate.

### What I did

- Wrote a detailed reference manual: `reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`
- Wrote an analysis + design brainstorm: `design-doc/01-microquickjs-freertos-multi-vm-multi-task-architecture-communication-brainstorm.md`
- Cross-linked both docs and anchored them to the key code locations in `imports/esp32-mqjs-repl/mqjs-repl/`.

### Why

- We need a stable “source of truth” for extension mechanics and FreeRTOS safety rules before we start implementing higher-level Cardputer APIs (keyboard, display, speaker, storage).

### What worked

- The playbook could be written directly against real repo code (not generic QuickJS folklore), especially:
  - `JSSTDLibraryDef` + stdlib generation (`mquickjs_build.*`)
  - user classes + finalizers and `JSGCRef` patterns (upstream example)

### What warrants a second pair of eyes

- Sanity-check the recommended “VM ownership + message passing” architecture against ESP-IDF/FreeRTOS realities (WDT, ISR constraints, timer callbacks).

## Step 7: Track down stdlib generator provenance (why our stdlib is 64-bit) and how to generate an ESP32-safe 32-bit version

This step was prompted by a concrete symptom: our firmware autoload scripts start with `var ...`, but QEMU logs show parse errors like `expecting ';' at 1:5`. That pushed us to scrutinize *how* the MicroQuickJS stdlib/atom table is generated and whether we’re accidentally using a host-architecture table on a 32-bit target.

The key outcome is a practical recipe: we already have a host generator binary in-tree (`esp_stdlib_gen`) and it supports `-m32` to emit a 32-bit ROM table layout suitable for ESP32-S3. The 64-bit table we have today exists because the generator defaulted to `-m64` on an x86_64 host.

### What I did

- Read the diary and code to confirm where stdlib selection happens (`minimal_stdlib.h` included by firmware).
- Located the generator program source in the imported tree:
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/mqjs_stdlib.c`
- Located the generator implementation and verified it supports `-m32` / `-m64`:
  - `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c`
- Confirmed the prebuilt generator binary is a 64-bit host executable:
  - Command: `file imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib_gen`
  - Result: `ELF 64-bit ... x86-64 ...`
- Verified the generator is what emits `uint64_t` vs `uint32_t` for `js_stdlib_table` by reading the print logic:
  - It prints `uint%u_t` where `%u = JSW*8`.
  - `JSW` defaults to pointer width, but can be forced with `-m32` or `-m64`.
- Captured these findings in the analysis doc so they don’t get lost:
  - `analysis/03-microquickjs-stdlib-atom-table-split-why-var-should-parse-current-state-ideal-structure.md`

### Why

- We need a stdlib/atom table that matches the embedded engine’s `JSWord` size (ESP32-S3 is 32-bit).
- The symptom (`var` not parsing) is consistent with the “empty stdlib table” lacking keyword atoms, but we also need to avoid accidentally switching to a host-generated 64-bit stdlib table that won’t match the embedded layout.

### What worked

- Found a direct lever in the generator: `-m32` is explicitly supported and designed to generate a 32-bit ROM table layout from a host tool.
- Confirmed our checked-in `esp_stdlib.h` is 64-bit in shape (`uint64_t js_stdlib_table[]`), which is consistent with it being produced by running the generator on x86_64 without `-m32`.

### What didn’t work

- The existing diary (before this step) explained the “stdlib table” concept, but didn’t record the crucial “where 64-bit comes from” provenance nor the explicit `-m32` mechanism. That gap likely would have cost time later.

### What I learned

- MicroQuickJS’s stdlib generator (`build_atoms`) is architecture-aware:
  - it defaults to host pointer width, but has a supported override (`-m32`/`-m64`).
- This means we can generate correct ESP32 tables without needing a 32-bit host toolchain:
  - `./esp_stdlib_gen -m32 > esp32_stdlib.h` is the conceptual move.

### What was tricky to build

- Separating two independent issues that look similar in logs:
  1) “minimal stdlib has no keyword atoms so `var` may not parse”, and
  2) “full stdlib exists but our checked-in version is likely host-64-bit layout and not safe to use on ESP32”.

### What warrants a second pair of eyes

- Confirm the exact contract between ROM table layout and target `JSWord` size:
  - we should validate (by inspection and/or a small runtime test) that a `-m32` generated header is correct for ESP32-S3 and doesn’t assume host endianness or pointer tagging details beyond `JSW`.

### What should be done in the future

- Introduce a reproducible regeneration path for an ESP32-safe stdlib header and wire build-time selection so we never accidentally compile in a 64-bit stdlib on ESP32.

### Code review instructions

- Start with `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_build.c` and search for `-m32` / `-m64` to see how the generator sets `JSW`.
- Then open `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/esp_stdlib.h` and confirm it declares `uint64_t` to understand why it’s host-shaped.

### Technical details

Example command for generating a 32-bit header from the existing host binary:

```bash
cd imports/esp32-mqjs-repl/mqjs-repl/main
./esp_stdlib_gen -m32 > esp32_stdlib.h
```

Expected sanity check:

- `esp32_stdlib.h` starts with `static const uint32_t ... js_stdlib_table[]`.

### What I’d do differently next time

- When we first noticed `minimal_stdlib.h` was in use, I should have immediately checked whether the generator supports `-m32` and recorded it in the diary before moving on.

## Step 8: Write a C++ split design + stdlib analysis and publish PDFs to reMarkable for review

This step turned the “we should decouple REPL I/O from JS/storage” intuition into two long-form documents: a design doc for splitting the firmware into C++ components with pluggable evaluators, and an analysis doc explaining the stdlib/atom-table situation (including the 64-bit generator provenance and the `-m32` plan).

The impact is that we now have a shareable reference artifact (PDF on reMarkable) that can guide implementation and review without re-reading the codebase from scratch.

### What I did

- Created and filled a design document for modularizing the firmware:
  - `design-doc/02-split-firmware-main-into-c-components-pluggable-evaluators-repeat-js.md`
  - Included proposed module boundaries, interfaces, meta-commands, diagrams, and a “RepeatEvaluator” to validate REPL I/O without JS.
- Created and filled an analysis document on stdlib/atom tables:
  - `analysis/03-microquickjs-stdlib-atom-table-split-why-var-should-parse-current-state-ideal-structure.md`
  - Added an explicit section on “where 64-bit comes from” and how to generate 32-bit via `-m32`.
- Uploaded all ticket analysis + design docs as PDFs to reMarkable using:
  - `python3 /home/manuel/.local/bin/remarkable_upload.py`
  - Using `--ticket-dir ... --mirror-ticket-structure` to avoid name collisions.

### Why

- We need a clear implementation plan that separates “REPL correctness” from “JS correctness”.
- We need a durable explanation of the stdlib/keyword situation so future work doesn’t keep rediscovering the same conclusions.

### What worked

- The uploader workflow (dry-run, then upload) produced PDFs under `ai/2025/12/29/...` mirroring the ticket structure, which is ideal for annotation.

### What didn’t work

- N/A (no failures encountered during doc generation and upload).

### What I learned

- Writing the docs forced clarity on the *first* test milestone: a REPL that works even when JS and storage are disabled.

### What was tricky to build

- Keeping the docs “intern-friendly” while also being precise about MicroQuickJS’s table-driven model (C function indices, finalizer tables, and GC ref patterns).

### What warrants a second pair of eyes

- Reviewers should sanity-check that the proposed REPL split doesn’t accidentally reintroduce global shared state or encourage cross-task `JSContext` access.

### What should be done in the future

- Once the REPL-only firmware exists, confirm it behaves the same across:
  - QEMU (TCP serial)
  - Cardputer USB Serial JTAG

### Code review instructions

- Start with the design doc and confirm the proposed interfaces match what ESP-IDF can realistically implement (UART, USB Serial JTAG).
- Then check the analysis doc’s generator details against `mquickjs_build.c` to ensure the `-m32` plan is correct.

### Technical details

Uploader invocation pattern used:

```bash
ticket_dir="/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2025/12/29/0014-CARDPUTER-JS--port-microquickjs-repl-to-cardputer"
python3 /home/manuel/.local/bin/remarkable_upload.py --ticket-dir "$ticket_dir" --mirror-ticket-structure --dry-run "$ticket_dir"/analysis/*.md "$ticket_dir"/design-doc/*.md
python3 /home/manuel/.local/bin/remarkable_upload.py --ticket-dir "$ticket_dir" --mirror-ticket-structure "$ticket_dir"/analysis/*.md "$ticket_dir"/design-doc/*.md
```

### What I’d do differently next time

- Draft the REPL split interfaces earlier (even as pseudocode) before diving into deeper engine internals, since the REPL split governs how we debug everything else.

## Step 9: Convert design into an actionable task plan (REPL-only first, then JS, then storage)

After writing the design, the next risk was “we have a doc but no actionable checklist”. This step translated the design into docmgr tasks so progress can be tracked, delegated, and resumed without re-deriving scope.

The output is a concrete set of tasks that start with the smallest shippable milestone: a “REPL-only firmware variant” with a repeat evaluator (no MicroQuickJS, no SPIFFS) to validate the console + line discipline first.

### What I did

- Added docmgr tasks to ticket `0014-CARDPUTER-JS` covering:
  - C++ component split
  - `IConsole`/`UartConsole`
  - `LineEditor`/`ReplLoop`
  - `IEvaluator` + `RepeatEvaluator`
  - meta-commands (`:help`, `:mode`, `:prompt`)
  - REPL-only build variant and QEMU smoke test
- Added docmgr tasks for the stdlib generation work:
  - provenance + `-m32` regeneration
  - script/target for regeneration
  - build-time selection between minimal and generated stdlib
  - validation that `var` parses once wired

### Why

- The project has multiple interleaved workstreams (Cardputer port, QEMU input, SPIFFS, stdlib correctness). Tasks let us pick a path (REPL-only) without losing the other threads.

### What worked

- Tasks are now explicit and ordered so that we can unblock REPL correctness first, then revisit MicroQuickJS and storage.

### What didn’t work

- N/A.

### What I learned

- Making the “stdlib generation” work explicit as tasks reduces the chance that we keep assuming `esp_stdlib.h` is safe for ESP32 when it may be host-shaped.

### What was tricky to build

- Choosing task granularity: too coarse and we can’t track progress; too fine and we drown in checklist items.

### What warrants a second pair of eyes

- Review the task ordering: ensure “REPL-only firmware” genuinely avoids storage + JS initialization so it can serve as a clean transport test.

### What should be done in the future

- When we start implementing these tasks, capture each major milestone as a diary step with build/test commands and any QEMU/Cardputer behavioral differences.

### Code review instructions

- Open `tasks.md` and confirm tasks for:
  - REPL-only milestone
  - stdlib regeneration (`-m32`)
  - build-system updates

### Technical details

Commands used:

```bash
docmgr task list --ticket 0014-CARDPUTER-JS
docmgr task add --ticket 0014-CARDPUTER-JS --text "..."
```

## Step 10: Add Cardputer flash/CPU defaults + enlarge partition table

This step updates the imported `mqjs-repl` firmware project so its default configuration matches Cardputer expectations: 8MB flash, 240MHz CPU, a larger main task stack, and a partition table that can actually hold the REPL firmware plus a usable SPIFFS storage region.

The practical unlock is that we stop fighting the “2MB flash” default and can move forward with the Cardputer port (USB Serial JTAG console, storage, and JS fixes) without repeatedly hitting partition sizing errors.

**Commit (code):** 881a761 — "mqjs-repl: Cardputer flash+partition defaults"

### What I did
- Added `imports/esp32-mqjs-repl/mqjs-repl/sdkconfig.defaults` with Cardputer-oriented defaults (8MB flash, 240MHz CPU, 8k main task stack)
- Updated `imports/esp32-mqjs-repl/mqjs-repl/partitions.csv` to:
  - `factory` app = 4MB
  - `storage` SPIFFS = 1MB
- Rebuilt the project with `./build.sh build` to confirm the partition table passes size checks

### Why
- Cardputer typically ships with 8MB flash; keeping a 2MB flash config guarantees partition table failures once we allocate a realistic app + storage layout.
- A 4MB app partition leaves room for the MicroQuickJS REPL + future C++ split work without immediately running into “app too big” constraints.

### What worked
- `./build.sh build` succeeds and reports the generated flash args using `--flash_size 8MB`.

### What didn't work
- Before the flash-size config was aligned, the build failed when generating the partition table with:
  - `Partitions tables occupies 5.1MB of flash (5308416 bytes) which does not fit in configured flash size 2MB.`

### What I learned
- `sdkconfig.defaults` is the right place to carry “board defaults” for reproducible builds, especially since `sdkconfig` is gitignored in this repo and won’t be present/consistent across environments.

### What was tricky to build
- Avoiding the trap of “it builds for me” by relying on a local `sdkconfig` file that isn’t tracked; defaults must be sufficient on their own for a clean configure/build.

### What warrants a second pair of eyes
- Confirm the Cardputer partition sizing is the right baseline (4MB app + 1MB SPIFFS) and whether we should reserve additional partitions (e.g., coredump) before we start depending on the layout.

### What should be done in the future
- Add a `Cardputer`-specific bring-up note: how to apply these defaults from a clean tree (e.g., delete build artifacts and regenerate config) so contributors don’t accidentally keep a stale 2MB flash config.

### Code review instructions
- Review `imports/esp32-mqjs-repl/mqjs-repl/sdkconfig.defaults` and `imports/esp32-mqjs-repl/mqjs-repl/partitions.csv`
- Validate with:
  - `cd imports/esp32-mqjs-repl/mqjs-repl && ./build.sh build`

## Step 11: Start the C++ split + REPL-only bring-up (RepeatEvaluator)

This step begins the “split main into components” work by introducing a minimal C++ REPL stack: a console abstraction, a line editor, and a REPL loop with a pluggable evaluator. To keep iteration fast and isolate I/O issues (especially under QEMU), the default firmware path is now **REPL-only** and uses a `RepeatEvaluator` that simply echoes submitted lines.

The key unlock is that we can validate prompt/echo/backspace/newline behavior without pulling in SPIFFS, autoload scripts, or the MicroQuickJS engine. Once the REPL harness is trusted, we can layer JS evaluation back in as a separate evaluator and reintroduce storage/autoload as optional plumbing.

**Commit (code):** 1a25a10 — "mqjs-repl: start C++ REPL-only split"

### What I did
- Updated the `main` component to build a new C++ entrypoint and modules:
  - `console/IConsole` + `console/UartConsole`
  - `repl/LineEditor` + `repl/ReplLoop`
  - `eval/IEvaluator` + `eval/RepeatEvaluator`
- Made the REPL print a prompt (`repeat> `), accept input bytes from UART, and echo completed lines
- Added minimal meta-commands:
  - `:help`
  - `:mode`
  - `:prompt TEXT`
- Built the firmware and did a short QEMU boot smoke check

### Why
- We need a transport-agnostic REPL core before swapping UART → USB Serial JTAG for Cardputer.
- QEMU input issues and SPIFFS/autoload JS parse issues are easier to debug when the REPL itself can run without JS/storage.

### What worked
- `./build.sh build` succeeds with the new C++ modules.
- `timeout 15s ./build.sh qemu` boots to the `repeat> ` prompt and shows the updated partition table.

### What didn't work
- N/A in this step (we intentionally did not attempt interactive QEMU input yet; that’s still tracked in ticket `0015-QEMU-REPL-INPUT`).

### What I learned
- Keeping the bring-up evaluator trivial (repeat) makes the REPL harness useful immediately even while JS/storage layers are still unstable.

### What was tricky to build
- Getting the component build wiring right (`main/CMakeLists.txt`) while keeping the module boundaries clean enough for the next steps (USB Serial JTAG console + JS evaluator).

### What warrants a second pair of eyes
- The REPL line-discipline behavior: confirm Enter/backspace handling is correct across UART/USB Serial JTAG and doesn’t regress when we add JS evaluation output.
- Whether meta-commands should be parsed before trimming (to preserve intentional leading/trailing whitespace use-cases for JS).

### What should be done in the future
- Implement `UsbSerialJtagConsole` (Cardputer input) and make the console selectable.
- Add a `JsEvaluator` that wraps MicroQuickJS and supports `:mode js` once stdlib/atom-table issues are resolved.
- Add a QEMU interactive smoke test once the monitor input path is fixed (ticket `0015`).

### Code review instructions
- Start at:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/app_main.cpp`
  - `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`
- Validate with:
  - `cd imports/esp32-mqjs-repl/mqjs-repl && ./build.sh build`
  - `timeout 15s ./build.sh qemu`

## Step 12: Add tmux-based smoke tests for RepeatEvaluator REPL (QEMU + device)

This step adds two small automation scripts that use tmux to drive the `idf.py monitor` console like a human would: wait for the `repeat> ` prompt, send a few lines (`:mode`, a test string, and `:prompt`), and assert the expected output shows up in the captured console buffer. The intent is to make REPL bring-up repeatable and to have a “known-good” check when we swap consoles (UART → USB Serial JTAG) or change line editing behavior.

In this Codex sandbox environment, tmux cannot create/connect to its socket (EPERM), so the scripts couldn’t be executed here; they are intended to be run on a normal dev host.

**Commit (code):** 5aeb539 — "mqjs-repl: add tmux REPL smoke tests"

### What I did
- Added:
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh`
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh`
- Each script:
  - starts a tmux session running `./build.sh ... monitor`
  - waits for `repeat> `
  - sends `:mode`, a `hello-*` line, and `:prompt test> `
  - captures output to `imports/esp32-mqjs-repl/mqjs-repl/build/*.log`

### Why
- We want a fast regression check for the REPL loop before adding JS evaluation and before changing transports for Cardputer.

### What worked
- Script logic is self-contained and writes a captured log on both success and failure for easy debugging.

### What didn't work
- Running tmux inside this sandbox failed with:
  - `error connecting ... (Operation not permitted)`

### What warrants a second pair of eyes
- Confirm the patterns we grep for are stable across `idf_monitor` output formatting (timestamps, log prefixes, echo behavior).

### Code review instructions
- Start in:
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_tmux.sh`
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_tmux.sh`

## Step 13: Run the tmux QEMU script — prompt appears, but input still doesn’t reach UART RX

This step is a quick validation of the new tmux automation scripts in a less-restricted environment. The script can now start `idf.py qemu monitor`, wait for the `repeat>` prompt, and capture logs reliably — but it still cannot get the firmware to echo any typed bytes. That means the original “QEMU REPL can’t receive interactive input” problem is **not** caused by MicroQuickJS parsing, SPIFFS, or autoload scripts: it reproduces even in the REPL-only RepeatEvaluator firmware.

This is a strong signal that the underlying issue remains in the QEMU UART RX path or the `idf_monitor` → socket → QEMU wiring, and should continue to be treated as ticket `0015-QEMU-REPL-INPUT`.

**Commit (code):** 167c629 — "mqjs-repl: harden tmux REPL tests"

### What I did
- Ran the QEMU script:
  - `cd imports/esp32-mqjs-repl/mqjs-repl && ./tools/test_repeat_repl_qemu_tmux.sh --timeout 90`
- Confirmed the script sees the prompt, then fails waiting for `mode: repeat` after sending `:mode`
- Hardened the prompt matching in the scripts to look for `repeat>`/`test>` (without relying on a trailing space), since the console output can interleave logs and wrap

### What worked
- The QEMU script consistently detects the REPL prompt and captures the full console output to `imports/esp32-mqjs-repl/mqjs-repl/build/*.log`.

### What didn't work
- The firmware did not echo any of the input sent by the script (`:mode`, `hello-qemu`, `:prompt ...`), even though the REPL echoes every printable byte as it is typed.

### What I learned
- The QEMU input problem reproduces in the minimal REPL-only build, so it is a transport/UART RX issue, not a JS/stdlib/storage issue.

### What warrants a second pair of eyes
- Confirm whether this is a QEMU machine/UART routing issue (UART0 vs USB-Serial vs console) or an `idf_monitor` socket transport issue.

## Step 14: Isolate further — raw UART tests still show no input echo under QEMU

This step adds “UART-direct” tests to eliminate `idf_monitor` and tmux from the equation. The goal is to answer: is the failure happening because `idf_monitor` doesn’t forward input correctly, or because the QEMU ESP32-S3 UART RX path simply isn’t working?

The result strongly points at **QEMU UART RX**. In both cases below, we can reliably see the firmware print the `repeat>` prompt, but we cannot get the firmware to echo *any* characters back (and therefore never see `mode: repeat`). Since the RepeatEvaluator REPL echoes each received byte immediately, this indicates the firmware is not receiving UART RX bytes at all.

**Commit (code):** bf99dc4 — "mqjs-repl: add UART-direct REPL tests"

### What I did
- Added a QEMU UART-on-stdio test (no idf_monitor):
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh`
- Added a QEMU raw TCP UART test (no idf_monitor; connect via plain TCP socket):
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_tcp_raw.sh`
- Added a device-side direct UART test (no idf_monitor; uses pyserial):
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_device_uart_raw.py`

### What worked
- Both QEMU scripts can observe the REPL prompt deterministically and capture logs.

### What didn't work
- QEMU UART RX still appears non-functional:
  - QEMU prints `repeat>` but never echoes `:mode` and never prints `mode: repeat`, even when bypassing `idf_monitor` entirely.

### What I learned
- The interactive-input bug reproduces across:
  - `idf_monitor` (tmux script),
  - raw TCP socket input (no monitor),
  - and qemu stdio mode (no monitor),
  which makes it very likely the issue is in the QEMU ESP32-S3 UART RX implementation or wiring, not in our REPL code.

### What warrants a second pair of eyes
- Confirm whether any alternative QEMU serial device mapping exists for ESP32-S3 (UART0 vs UART1) and whether ESP-IDF’s QEMU glue expects a different console transport for RX.

## Step 15: Create an intern-facing internet research guide (QEMU UART RX + Cardputer console)

This step packages what we need from external research into a single, intern-friendly guide: specific questions to answer, copy/paste search queries, source-priority rules, and a reporting template that makes the results actionable. The goal is to quickly determine whether the QEMU UART RX limitation is a known unsupported feature (or a known bug with an upstream fix), and to document the correct interactive console transport for Cardputer under ESP-IDF.

**Commit (code):** N/A — documentation only

### What I did
- Added a new reference doc:
  - `reference/03-intern-research-guide-qemu-uart-rx-cardputer-console.md`
- Included:
  - research questions (UART RX support, QEMU serial mapping, ESP-IDF console expectations, Cardputer console path)
  - search queries
  - evidence checklist + reporting template
  - pointers to our local reproduction scripts

### Why
- We need authoritative answers fast, and we don’t want engineering time spent chasing low-signal sources or repeating searches.

### What should be done in the future
- Once the intern returns findings, convert “proposed actions” into concrete repo tasks (e.g., change QEMU serial mapping, implement USB Serial JTAG console for Cardputer, or stop relying on QEMU for RX).

## Step 16: Fix QEMU interactive RX by polling UART FIFO (and keep tmux tests green)

This step turns the intern’s “RX timeout interrupt missing” research into a practical fix we can validate locally: instead of depending on UART RX interrupts to move bytes into the driver’s ring buffer, the REPL console now polls the UART RX FIFO directly when no buffered bytes are available. This restores interactive input under QEMU across all of our harnesses (tmux + idf_monitor, raw TCP UART, and stdio).

In addition, I attempted device-side testing on the attached Cardputer at `/dev/ttyACM0`. Flashing and monitoring works (boot logs + `ESP_LOGI` lines show up), but the REPL prompt does not appear because our REPL console currently writes to UART0 via `uart_write_bytes`, while `/dev/ttyACM0` is the USB Serial/JTAG console. This confirms the next required step for real-device REPL is implementing a USB Serial/JTAG-backed console transport.

**Commit (code):** 7bc80f2 — "mqjs-repl: poll UART RX FIFO for QEMU"

### What I did
- Implemented a robust `UartConsole::Read()` path that:
  - first reads any buffered bytes via `uart_read_bytes(..., timeout=0)`,
  - then polls the hardware RX FIFO via `uart_ll_get_rxfifo_len()` + `uart_ll_read_rxfifo()` when interrupts don’t deliver bytes (QEMU),
  - blocks with a small `vTaskDelay(1)` loop to honor the requested timeout.
- Re-ran QEMU smoke tests:
  - `tools/test_repeat_repl_qemu_tmux.sh`
  - `tools/test_repeat_repl_qemu_uart_tcp_raw.sh`
  - `tools/test_repeat_repl_qemu_uart_stdio.sh`
- Flashed and monitored on Cardputer:
  - `tools/test_repeat_repl_device_tmux.sh --port /dev/ttyACM0 --flash` (flash succeeds; monitor shows logs but no REPL prompt)

### Why
- QEMU appears to not deliver RX reliably through the ESP-IDF UART driver’s interrupt-driven path for short inputs, even after reducing RX FIFO threshold. Polling the FIFO avoids dependence on missing/quirky IRQ behavior in the emulator.
- Cardputer REPL needs a console implementation that matches the device’s actual interactive transport (USB Serial/JTAG on `/dev/ttyACM*`).

### What worked
- QEMU interactive REPL input works again via all harnesses; `:mode` prints `mode: repeat` and `hello-*` echoes.
- Cardputer flashing and monitor connectivity on `/dev/ttyACM0` works.

### What didn't work
- Cardputer REPL prompt/output is not visible on `/dev/ttyACM0` because the REPL console currently writes to UART0 (not USB Serial/JTAG).

### What I learned
- Treat QEMU as “UART RX is not interrupt-reliable”: a polling fallback makes the REPL usable without waiting on emulator feature completeness.
- Device console output path matters: `ESP_LOG*` can show up on USB Serial/JTAG while UART0 traffic remains invisible unless separately wired.

### What was tricky to build
- Making the console read path work in both worlds:
  - real ESP-IDF driver ring buffer when interrupts do work, and
  - raw FIFO polling when they don’t.

### What warrants a second pair of eyes
- The FIFO polling loop in `UartConsole::Read()` for correctness/perf: ensure it doesn’t starve other tasks or spin too aggressively under high throughput.
- Whether we should limit FIFO polling to “QEMU builds only” vs keeping it always-on as a resilience feature.

### What should be done in the future
- Implement `UsbSerialJtagConsole` and select it for Cardputer builds so `repeat>` is visible and interactive on `/dev/ttyACM0`.
- Once Cardputer console is in place, re-run:
  - `tools/test_repeat_repl_device_tmux.sh --port /dev/ttyACM0 --flash`
  - `tools/test_repeat_repl_device_uart_raw.py --port /dev/ttyACM0`

## Step 17: Cardputer REPL on /dev/ttyACM0 via USB Serial/JTAG console

This step makes the REPL actually usable on real Cardputer hardware by switching the interactive console transport from UART0 to the ESP32-S3 USB Serial/JTAG peripheral. Previously, flashing and log monitoring worked on `/dev/ttyACM0`, but the REPL prompt didn’t show up because our REPL used `uart_write_bytes()` (UART0), which is not the USB Serial/JTAG console.

With a dedicated `UsbSerialJtagConsole` implementation and a build-time selection knob, the REPL prompt and echo now work on `/dev/ttyACM0`. This closes the loop on the “device validation” tasks and gives us a reliable interactive dev loop for the next REPL and JS bring-up steps.

**Commit (code):** d3e9f19 — "mqjs-repl: add USB Serial/JTAG console"

### What I did
- Added `UsbSerialJtagConsole` implementing `IConsole` via `usb_serial_jtag_{read,write}_bytes`
- Added a Kconfig choice for REPL console transport (UART0 vs USB Serial/JTAG)
- Added a helper script `tools/set_repl_console.sh` to update local (gitignored) `sdkconfig`
- Updated the device tmux smoke test script to select the desired console mode before flash/monitor
- Validated on Cardputer `/dev/ttyACM0`:
  - `tools/test_repeat_repl_device_tmux.sh --port /dev/ttyACM0 --flash --timeout 220` (passes)
  - `tools/test_repeat_repl_device_uart_raw.py --port /dev/ttyACM0 --timeout 20` (passes; sends `:mode` without relying on seeing the initial prompt)

### Why
- Cardputer’s interactive console is USB Serial/JTAG (`/dev/ttyACM*`); using UART0 for the REPL made the prompt invisible during normal development.

### What worked
- The REPL prompt appears and is interactive on `/dev/ttyACM0`.

### What didn't work
- N/A in this step.

### What I learned
- “Monitor works” doesn’t imply “UART0 REPL works”: console output can be on USB Serial/JTAG while UART0 is effectively disconnected from the developer workflow.

### What warrants a second pair of eyes
- Confirm that using `usb_serial_jtag_is_connected()` to gate output is the right behavior (tradeoff: avoids blocking when unplugged vs potentially dropping early boot output).

## Step 18: Write a device-first handoff guide for the next developer

This step is documentation-only. The goal is to make handoff clean: a new developer should be able to find the real entrypoints (firmware wiring, REPL loop, console transports), know which scripts to run to validate changes on Cardputer, and understand what each open task actually means and which prior docs to consult before changing behavior.

**Commit (code):** N/A — documentation only

### What I did
- Added a device-first handoff guide:
  - `reference/04-device-first-developer-handoff-guide.md`
- Included:
  - “marks” (key files + symbols)
  - device-first smoke tests and console selection commands
  - a task map describing the intent of the remaining open tasks

### Why
- This ticket has a lot of historical context (QEMU RX, stdlib generation, REPL split, Cardputer console). Without a guide, new contributors will re-derive the same context and waste time.

### What should be done in the future
- Keep the handoff guide updated as we introduce JS evaluation and storage/autoload back into the system.

## Step 19: Generate and commit the ESP32 32-bit stdlib header (+ regen script + Kconfig knob)

This step unblocks the next “JS actually works” milestone by adding a generated 32-bit stdlib table for ESP32. The previously checked-in `main/esp_stdlib.h` is a 64-bit table (generated on a 64-bit host), which is known to be unsafe for ESP32 and contributes to parse failures when we later re-enable JavaScript evaluation.

With `main/esp32_stdlib.h` generated via `esp_stdlib_gen -m32`, we now have an ESP32-safe table that includes keyword atoms like `var`, `function`, and `return`. The important follow-up is that the parser also depends on `mquickjs_atom.h`, which must be generated with the same word size (`-m32 -a`) or `var` still won’t parse.

**Commit (code):** 53f5c26 — "mqjs-repl: JS mode, stats, and smoke tests"

### What I did
- Generated a 32-bit stdlib header:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib.h`
- Added a deterministic regen helper:
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh`
- Generated the matching 32-bit atom header (via `-m32 -a`):
  - `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs_atom.h`

### Why
- MicroQuickJS’s ROM stdlib tables are word-size sensitive; the 64-bit-generated stdlib table should not be used on ESP32.
- To make the “enable JS evaluation” work item actually actionable, we need a checked-in, ESP32-safe stdlib header plus a regen mechanism.

### What worked
- `esp_stdlib_gen -m32` produces a `uint32_t` table and includes keyword atoms (`"var"`, `"function"`, `"return"`).

### What didn't work
- The generator prints a warning while building hash tables:
  - `Too many properties, consider increasing ATOM_ALIGN`
  It still generates output, but this should be understood and (if needed) addressed before we treat the generated tables as a locked-down contract.

### What I learned
- We do *not* need a 32-bit host toolchain: `mquickjs_build.c` supports a runtime `-m32` flag and emits a 32-bit table even when the generator binary itself is 64-bit.

### What was tricky to build
- Avoiding accidental symbol/table mixing: the generated headers define concrete data (not just declarations), so future wiring needs to ensure exactly one stdlib header is compiled into the firmware.

### What warrants a second pair of eyes
- Whether we should increase `ATOM_ALIGN` (currently 64) for the generated tables to avoid the “Too many properties…” warning (tradeoff: alignment vs hash table size vs memory/perf).
- Confirm that `main/esp32_stdlib.h` is the *only* stdlib header we should ever use on ESP32 once JS evaluation is enabled (and that we should treat `main/esp_stdlib.h` as host/reference only).

### What should be done in the future
- Wire the generated ESP32 stdlib + atom header into the eventual `JsEvaluator` path and run a minimal script that starts with `var` on both:
  - Cardputer (USB Serial/JTAG)
  - QEMU (UART0)

## Step 20: Add `JsEvaluator`, mode switching (`:mode js`), and prove `var` parses in QEMU

This step turns the bring-up REPL into a real JS REPL by adding a `JsEvaluator` and a mode switcher so the REPL can flip between repeat echo and JavaScript evaluation. The key lesson here is that enabling JS evaluation on ESP32 is a *pair* of generated artifacts: the stdlib table (`esp32_stdlib.h`) and the parser atom offsets (`mquickjs_atom.h`). If they don’t match, the lexer/parser misclassifies tokens and `var` fails in confusing ways.

To keep things simple and avoid fallback paths, the firmware now always compiles with the generated ESP32 stdlib table + matching atom header. With both in place, a QEMU smoke test that runs `var x = 1 + 2; x` succeeds and prints `3`.

**Commit (code):** 53f5c26 — "mqjs-repl: JS mode, stats, and smoke tests"

### What I did
- Added `JsEvaluator` (`JS_NewContext` + `JS_Eval`) and printed values/exceptions via `JS_PrintValueF` capture.
- Added `ModeSwitchingEvaluator` and wired it into `app_main` so the REPL supports `:mode repeat|js`.
- Defined `js_stdlib` in a C compilation unit (`main/esp32_stdlib_runtime.c`) that provides the required stubs (`js_print`, `js_gc`, etc) and includes `main/esp32_stdlib.h`.
- Updated `tools/gen_esp32_stdlib.sh` to regenerate both:
  - `main/esp32_stdlib.h` (via `-m32`)
  - `components/mquickjs/mquickjs_atom.h` (via `-m32 -a`)
- Added JS smoke tests:
  - QEMU: `tools/test_js_repl_qemu_uart_stdio.sh`
  - Device: `tools/test_js_repl_device_uart_raw.py`

### Why
- We need an actual JS evaluator to make progress on storage/autoload and on-device JS workflows.
- `var` parsing failures are an atom-table mismatch problem; the fix must include the atom header, not just the stdlib table.

### What worked
- QEMU passes: `tools/test_js_repl_qemu_uart_stdio.sh` prints `3` for `var x = 1 + 2; x`.

### What didn't work
- Before regenerating `components/mquickjs/mquickjs_atom.h` for 32-bit, JS mode consistently failed to parse `var` (confusing errors like `[mtag] ...`).

### What I learned
- `mquickjs_atom.h` is part of the JS parsing contract; it must be generated with the same `-m32` word-size as the stdlib table.

### What was tricky to build
- The generated stdlib header must be compiled as C (including it from C++ triggers narrowing errors).
- `JS_PrintValueF` writes through `ctx->write_func`/`ctx->opaque`; capturing output requires temporarily repointing `ctx->opaque`.

### What warrants a second pair of eyes
- Confirm no other code in this project relies on the previous `mquickjs_atom.h` offsets (this change is intentionally not backwards compatible).
- Sanity-check that the `JsEvaluator` log capture doesn’t break if multiple contexts are used in the future.

### What should be done in the future
- Run the device JS smoke test on Cardputer (`/dev/ttyACM0`) and keep it green while reintroducing storage/autoload.

## Step 21: Validate JS + `:stats` on Cardputer and tighten REPL ergonomics

This step closes the loop on “JS interpreter on the real device” by actually flashing Cardputer and running an automated smoke test against `/dev/ttyACM0`. Once that was green, I added a `:stats` command (heap stats + JS memory dump) and a few small line editor improvements that make interactive work less frustrating (Ctrl+C cancel, Ctrl+U kill line, ignore arrow-key escape sequences).

The main gotcha was that our QEMU test script switches the build to UART0 via `sdkconfig`, so device tests will silently fail unless we flip back to USB Serial/JTAG and re-flash first. I captured that failure mode here so it’s easy to recognize next time.

**Commit (code):** N/A (not committed in this session)

### What I did
- Flashed Cardputer and ran the device JS smoke test:
  - `./tools/set_repl_console.sh usb-serial-jtag`
  - `./build.sh build`
  - `./build.sh -p /dev/ttyACM0 flash`
  - `./tools/test_js_repl_device_uart_raw.py --port /dev/ttyACM0 --timeout 25`
- Added `:stats` (heap + JS dump) and `:reset`:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`
- Improved line editing:
  - ignore ANSI escape sequences (arrow keys)
  - `Ctrl+C` cancels line, `Ctrl+U` kills line, `Ctrl+L` clears screen
  - `imports/esp32-mqjs-repl/mqjs-repl/main/repl/LineEditor.cpp`
- Extended smoke tests to validate `:stats` output:
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py`
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh`

### What worked
- Device: JS eval (`var x = 1 + 2; x`) prints `3` and `:stats` prints `heap_free=...`.
- QEMU: JS + `:stats` smoke test passes.

### What didn't work
- Device test initially timed out waiting for `mode: js` when the firmware on the device hadn’t been flashed yet.
- After running the QEMU tests, device tests timed out again until switching the console back to USB Serial/JTAG (because QEMU uses UART0).

### What was tricky to build
- Keeping device vs QEMU console selection straight: `sdkconfig` is local and scripts mutate it.

### What warrants a second pair of eyes
- Ensure `:stats` output is actually meaningful for the “memory budget” task (heap metrics are system heap; JS heap is fixed-buffer and reported via `JS_DumpMemory`).

### What should be done in the future
- Add a dedicated “device JS tmux” script (flash+monitor+smoke) similar to the existing RepeatEvaluator tmux harness so interactive workflows are one-command.

## Step 22: Reintroduce SPIFFS + autoload (device-first)

This step reintroduces storage in a way that doesn’t block or brick the REPL: SPIFFS is mounted lazily and autoload is explicitly triggered via a meta-command. The user-visible contract is `:autoload` (non-destructive; requires an already-formatted partition) and `:autoload --format` (formats+mounts SPIFFS if needed, then loads any `*.js` files in `/spiffs/autoload/`).

I also wired up `load(path)` inside JS mode to actually read from SPIFFS, which is the core primitive needed to grow this into a usable on-device interpreter (autoload just becomes a thin loop over `load()`).

**Commit (code):** 18d4409 — "mqjs-repl: SPIFFS load() and autoload"

### What I did
- Added SPIFFS support and file reading helpers:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp`
- Implemented `:autoload`/`:autoload --format` in the REPL and hooked it into `JsEvaluator`:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp`
  - `imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp`
- Implemented JS `load(path)` to read a file from SPIFFS and `JS_Eval` it:
  - `imports/esp32-mqjs-repl/mqjs-repl/main/esp32_stdlib_runtime.c`
- Extended smoke tests to exercise autoload:
  - QEMU: `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh`
  - Device: `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py`
- Validated on Cardputer (USB Serial/JTAG on `/dev/ttyACM0`) and QEMU:
  - `:autoload --format` prints an `autoload:` summary and returns to prompt.

### Why
- To make JS usable as an interpreter (ability to load code) without adding hidden boot-time work or risky automatic formatting.

### What worked
- `:autoload --format` works on a brand-new device partition (formats if required) and returns to the REPL prompt.
- `load("/spiffs/autoload/foo.js")` evaluates JS from SPIFFS, enabling a “stdlib-in-files” workflow.

### What didn't work
- N/A (no hard failures in this step; prior autoload parse-error work is tracked in ticket `0016-SPIFFS-AUTOLOAD`).

### What I learned
- Storage UX matters on embedded: making formatting an explicit opt-in (`--format`) keeps experimentation safe and reviewable.

### What was tricky to build
- Keeping SPIFFS behavior safe: default `:autoload` won’t format; `--format` is the explicit “yes, wipe if needed” path.
- Avoiding REPL lockups: everything runs synchronously but is user-triggered (no hidden boot-time blocking).

### What warrants a second pair of eyes
- The SPIFFS formatting semantics: confirm `:autoload --format` is the right UX and that we never format unless explicitly requested.
- The `load(path)` size limit (currently 32 KiB) and whether that’s appropriate for expected scripts/libs.

### What should be done in the future
- Add an explicit size/CPU-time budget story for `load()` + `:autoload` (e.g., chunked reads, tighter limits, and error messaging for oversized scripts).
- Consider an autoload seed story (even a one-file example) so first-run isn’t “empty directory”.

### Code review instructions
- Start with `imports/esp32-mqjs-repl/mqjs-repl/main/repl/ReplLoop.cpp` for `:autoload` parsing/dispatch.
- Review SPIFFS helpers in `imports/esp32-mqjs-repl/mqjs-repl/main/storage/Spiffs.cpp`.
- Validate with `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh --timeout 120` and `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_device_uart_raw.py --port /dev/ttyACM0 --timeout 45`.

### Technical details
- Autoload directory: `/spiffs/autoload/` (loads `*.js` files; prints `autoload:` summary).
- `load(path)` currently enforces a 32 KiB limit to avoid large allocations on constrained SRAM.

### What I'd do differently next time
- N/A

## Step 23: Finish split cleanup (archive legacy monolith; relocate stdlib generator)

This step finishes the “split cleanup” by removing leftover, confusing artifacts from the firmware `main/` folder while keeping the old monolithic implementation available for historical reference. The goal is to make it obvious what code is actually built and where the supporting host tools live.

It’s intentionally non-behavioral: the running firmware doesn’t change, but the repo structure becomes easier to navigate and less likely to regress into “accidentally editing the legacy file”.

**Commit (code):** 2d482af — "mqjs-repl: move legacy firmware and stdlib generator"

### What I did
- Moved the legacy monolithic firmware sources out of `imports/esp32-mqjs-repl/mqjs-repl/main/` into `imports/esp32-mqjs-repl/mqjs-repl/legacy/`.
- Moved the host stdlib generator artifacts into `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/`.
- Updated `imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh` to point at the relocated generator binary.

### Why
- To make the “real” firmware code paths unambiguous (everything built lives under `main/`, not in leftover `main/main.c`).
- To keep host tooling (the stdlib generator) out of the firmware source tree and reduce accidental coupling.

### What worked
- QEMU smoke tests still pass after the cleanup (repeat + JS).

### What didn't work
- N/A

### What I learned
- Doing a “structure-only” cleanup right after functional milestones pays off: it prevents stale files from becoming a second, divergent truth.

### What was tricky to build
- Keeping historical references intact (docs and analysis) while still making `main/` clearly “the build”.

### What warrants a second pair of eyes
- Confirm that no scripts/CI/docs still assume the old generator location under `main/`.

### What should be done in the future
- If we ever want `esp_stdlib_gen` to be rebuildable (rather than checked-in as an ELF), add a small host build script and document toolchain expectations.

### Code review instructions
- Verify the moved paths and the generator script update:
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/gen_esp32_stdlib.sh`
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/esp_stdlib_gen/`
  - `imports/esp32-mqjs-repl/mqjs-repl/legacy/`
- Run:
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_repeat_repl_qemu_uart_stdio.sh --timeout 120`
  - `imports/esp32-mqjs-repl/mqjs-repl/tools/test_js_repl_qemu_uart_stdio.sh --timeout 120`

### Technical details
- Firmware build remains driven by `imports/esp32-mqjs-repl/mqjs-repl/main/CMakeLists.txt`; `legacy/` is reference-only and not compiled.

### What I'd do differently next time
- N/A
