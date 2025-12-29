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
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T13:24:53.129865004-05:00
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
- Check imported firmware: `imports/esp32-mqjs-repl/mqjs-repl/main/main.c`
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

- Read the embedded firmware entry point (`imports/esp32-mqjs-repl/mqjs-repl/main/main.c`) to see how the VM is created and used from FreeRTOS.
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
