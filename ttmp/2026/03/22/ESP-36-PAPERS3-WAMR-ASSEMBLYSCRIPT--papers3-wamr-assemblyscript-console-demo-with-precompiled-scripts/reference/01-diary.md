---
Title: Diary
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
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ttmp/2026/03/22/ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT--papers3-wamr-assemblyscript-console-demo-with-precompiled-scripts/index.md
      Note: Ticket overview updated during this documentation pass
    - Path: ttmp/2026/03/22/ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT--papers3-wamr-assemblyscript-console-demo-with-precompiled-scripts/tasks.md
      Note: Task plan for implementation and delivery
    - Path: 0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults
      Note: New firmware baseline including USB Serial/JTAG console and interpreter-first WAMR defaults
    - Path: 0079-papers3-wamr-assemblyscript-console/main/app_main.cpp
      Note: PaperS3 boot path for the new console demo
    - Path: 0079-papers3-wamr-assemblyscript-console/main/console_repl.cpp
      Note: esp_console bring-up over USB Serial/JTAG
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp
      Note: Initial wasm command namespace and placeholder runtime commands
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: Runtime initialization and status-reporting service for WAMR
    - Path: 0077-papers3-alphabet-graffiti/sdkconfig.defaults
      Note: PaperS3 baseline used to anchor ESP-IDF 5.3.4 and console defaults
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Local REPL startup reference
    - Path: 0067-esp-c3-led-matrix-http/main/js_console.c
      Note: Local runtime-command reference
ExternalSources:
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md
    - https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md
    - https://github.com/AssemblyScript/website/blob/main/src/compiler.md
    - https://github.com/AssemblyScript/website/blob/main/src/runtime.md
Summary: Chronological diary for the ticket creation, scaffold work, and runtime integration work behind the PaperS3 WAMR AssemblyScript design guide and firmware.
LastUpdated: 2026-03-22T11:49:00-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Capture the investigation and documentation work for the planned PaperS3 WAMR AssemblyScript console demo so another engineer can see what was inspected, what decisions were made, what almost caused confusion, and how to continue into implementation.

## Step 1: Create the ticket and turn the request into an implementation-ready design

This step was documentation-first rather than code-first. The user asked for a new `docmgr` ticket, a diary, a very detailed implementation guide for a new intern, and a reMarkable upload. The immediate job was therefore to map the current repo, identify the right neighboring tickets, confirm the PaperS3 baseline is still `ESP-IDF 5.3.4`, and then turn the request into a concrete architecture instead of writing vague future notes.

The main output of this step is a new ticket, `ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT`, with a detailed guide for a future `0079` PaperS3 firmware. The guide deliberately narrows the first implementation milestone: use WAMR interpreter mode, embed precompiled `.wasm` demo modules, expose them through USB Serial/JTAG `esp_console`, and keep the guest ABI numeric at first so the intern is not immediately buried under AssemblyScript managed-object interop.

### Prompt Context

**User prompt (verbatim):**

> Create anew docmgr to create a WAMR assembly script demo on the S3Paper (read the last couple of tickets, we need 5.3.4).
>
> Keep a diary.
>
> Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
>   references.
>   It should be very clear and detailed. Store in the ticket and the nupload to remarkable.
>
> We want to have a bunch fo assemblyscript programs that do different things and get compiled and then we can use esp_console to run them.

**Assistant interpretation:** Create a new ticket for the next PaperS3 firmware, base it on the recent PaperS3 work and `ESP-IDF 5.3.4`, keep a diary, and produce an intern-ready design guide that explains how a WAMR-hosted AssemblyScript demo system should be built and operated.

**Inferred user intent:** Establish a clean, teachable path from PaperS3 firmware bring-up to a console-driven WebAssembly demo platform where several AssemblyScript programs can be compiled ahead of time and run on-device without inventing the architecture later.

**Commit (code):** `N/A` - documentation-only ticket creation and design pass

### What I did

- created ticket `ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT`
- added new docmgr vocabulary topics for `papers3`, `wasm`, and `assemblyscript`
- reviewed the recent PaperS3 tickets, especially `ESP-32`, `ESP-33`, `ESP-34`, and `ESP-35`
- inspected the current PaperS3 project structure in `0077-papers3-alphabet-graffiti`
- inspected local `esp_console` prior art in `0030-cardputer-console-eventbus`
- inspected local script-runtime console prior art in `0067-esp-c3-led-matrix-http`
- inspected local ESP-IDF `5.3.4` console headers
- pulled the official WAMR embedding/native-API docs and AssemblyScript compiler/runtime docs
- wrote the primary analysis/design/implementation guide
- updated the ticket index, tasks, and changelog

### Why

- the request was about creating a ticketed implementation plan, not about jumping into code blindly
- the "last couple of tickets" instruction meant the design had to stay aligned with the current PaperS3 work rather than drifting into a generic embedded-Wasm document
- the guide needed to be useful to a new intern, which required reducing ambiguity around project layout, ABI choices, and phase boundaries

### What worked

- the recent PaperS3 tickets already established the right repo conventions for numbering, structure, and `ESP-IDF 5.3.4`
- `0030` provided a clean local example of USB Serial/JTAG REPL startup
- `0067` provided a good model for a runtime-specific command namespace under `esp_console`
- the official WAMR APIs mapped cleanly onto the desired device-side lifecycle
- the official AssemblyScript docs made it easy to justify a host-side compile step and a simple `--runtime stub` starting point

### What didn't work

- my first attempt to open the older JS runtime design doc used the wrong path; the actual file name was `01-mquickjs-matrix-scripting-api-architecture-and-integration-blueprint.md`
- the repo has no existing WAMR integration, so there was no local runtime component to trace directly
- the obvious next PaperS3 project number, `0078`, was already referenced in the neighboring layout-OS ticket, so I intentionally avoided reusing it and proposed `0079` instead

### What I learned

- the cleanest path is not "host rich AssemblyScript objects on day 1"; it is "run short-lived guest modules with a small numeric ABI first"
- PaperS3 + `esp_console` + precompiled wasm is a tractable combination as long as module loading and display verbs stay narrow
- the correct first milestone is closer to a curated demo launcher than to a full Wasm application platform

### What was tricky to build

- the sharpest design edge was the ABI boundary between AssemblyScript and native host code
- AssemblyScript becomes much more complicated once the host wants to manipulate guest-managed strings and arrays, because concepts like `__pin`, `__collect`, and runtime layout become relevant
- the design therefore had to explicitly reject that complexity for milestone 1 and explain why that is a strength rather than a missing feature

### What warrants a second pair of eyes

- whether embedding `.wasm` artifacts into the firmware is still the right milestone-1 choice, or whether the user wants SPIFFS-backed module loading immediately
- whether `WAMR` interpreter mode is sufficient for the intended demo pack on real PaperS3 hardware, or whether AOT will be needed earlier than expected
- whether the first host API should include any text drawing at all, given the desire to avoid early string marshalling complexity

### What should be done in the future

- implement `0079-papers3-wamr-assemblyscript-console`
- vendor or otherwise integrate WAMR into the repo
- add the host-side AssemblyScript build script and starter demo modules
- decide, after hardware validation, whether to stay with embedded modules or move toward a filesystem-backed module store
- measure real runtime pool, guest heap, and stack requirements on the PaperS3

### Code review instructions

- start with the ticket guide in `design-doc/01-papers3-wamr-assemblyscript-analysis-design-and-implementation-guide.md`
- then inspect `0077-papers3-alphabet-graffiti/sdkconfig.defaults` to confirm the PaperS3 baseline
- inspect `0030-cardputer-console-eventbus/main/app_main.cpp` for the USB Serial/JTAG REPL startup shape
- inspect `0067-esp-c3-led-matrix-http/main/js_console.c` for the command-family structure to copy
- finally read the external WAMR embedding/export-native docs and the AssemblyScript compiler/runtime docs referenced in the guide

### Technical details

Commands used:

```bash
docmgr status --summary-only
docmgr vocab add --category topics --slug wasm --description "WebAssembly runtimes, modules, and embedding"
docmgr vocab add --category topics --slug assemblyscript --description "AssemblyScript language, compiler, and runtime integration"
docmgr vocab add --category topics --slug papers3 --description "M5Stack PaperS3 board and firmware patterns"
docmgr ticket create-ticket --ticket ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT --title "PaperS3 WAMR AssemblyScript console demo with precompiled scripts" --topics papers3,esp32-s3,esp32s3,firmware,m5stack,m5gfx,console,usb-serial-jtag,storage,wasm,assemblyscript
docmgr doc add --ticket ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT --doc-type design-doc --title "PaperS3 WAMR AssemblyScript analysis design and implementation guide"
docmgr doc add --ticket ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT --doc-type reference --title "Diary"
```

Key local evidence inspected:

- `0077-papers3-alphabet-graffiti/CMakeLists.txt`
- `0077-papers3-alphabet-graffiti/main/app_main.cpp`
- `0077-papers3-alphabet-graffiti/sdkconfig.defaults`
- `0030-cardputer-console-eventbus/main/app_main.cpp`
- `0067-esp-c3-led-matrix-http/main/app_main.c`
- `0067-esp-c3-led-matrix-http/main/js_console.c`
- `/home/manuel/esp/esp-idf-5.3.4/components/console/esp_console.h`

Key external references inspected:

- `https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/README.md`
- `https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md`
- `https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md`
- `https://github.com/AssemblyScript/website/blob/main/src/compiler.md`
- `https://github.com/AssemblyScript/website/blob/main/src/runtime.md`

## Step 2: Expand the task list into execution-sized milestones before coding

Before creating `0079`, I converted the high-level task bullets into concrete implementation milestones with explicit substeps and commit boundaries. This makes the ticket usable as an actual runbook instead of a vague wish list.

That matters for this work because the request is not just "build something with WAMR." It is "work on them one by one and commit at appropriate intervals." The task list therefore had to name the intervals: scaffold, console bootstrap, WAMR integration, AssemblyScript pipeline, registry, first end-to-end demo, demo-pack expansion, and hardware validation.

### Prompt Context

**User prompt (verbatim):** "Create detailed atasks in the ticket and then work on them one by one and commit at appropriate intervals and keep a detailed diary as you work."

**Assistant interpretation:** Refine the ticket into concrete implementation tasks first, then start executing the first tasks with focused commits and diary updates.

**Inferred user intent:** Make the WAMR PaperS3 effort reviewable and resumable, with clear stopping points and artifacts at each milestone instead of one large ambiguous change.

**Commit (code):** `pending` - this step is being bundled into the first documentation/scaffold checkpoint

### What I did

- rewrote `tasks.md` into phased execution tasks
- added explicit substeps and expected commit boundaries
- separated pure documentation completion from future firmware implementation tasks
- aligned the task order with the dependency chain:
  - scaffold
  - console
  - WAMR
  - AssemblyScript build pipeline
  - embedded module registry
  - first runnable demo
  - expanded demo pack
  - hardware validation

### Why

- the earlier task list was directionally correct but still too coarse to drive implementation safely
- a repo with existing unrelated uncommitted changes benefits from smaller, auditable checkpoints
- this structure makes it easier to stop after a good milestone if the next milestone uncovers a blocker

### What worked

- the task tree maps cleanly onto the architecture already documented in the design guide
- the commit boundaries are now obvious enough that future diary entries can point at them directly

### What didn't work

- nothing failed in this step

### What I learned

- the cleanest implementation path is to treat "first runnable demo" as a milestone of its own, not as a footnote inside generic WAMR integration

### What was tricky to build

- the main subtlety was deciding how fine-grained the tasks should be without turning the ticket into bureaucracy
- I kept the substeps concrete enough to guide execution, but broad enough that one commit can still cover a meaningful slice of work

### What warrants a second pair of eyes

- whether the current milestone split is the right balance between reviewability and momentum

### What should be done in the future

- start executing Task 1 immediately by scaffolding `0079`

### Code review instructions

- review `tasks.md` and confirm that each phase has a real artifact and a reasonable commit boundary

### Technical details

Updated files:

- `ttmp/2026/03/22/ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT--papers3-wamr-assemblyscript-console-demo-with-precompiled-scripts/tasks.md`
- `ttmp/2026/03/22/ESP-36-PAPERS3-WAMR-ASSEMBLYSCRIPT--papers3-wamr-assemblyscript-console-demo-with-precompiled-scripts/reference/01-diary.md`

## Step 3: Scaffold `0079` and prove the first console-oriented firmware baseline builds

This step turned the ticket from a plan into a real firmware project. I created `0079-papers3-wamr-assemblyscript-console`, copied the expected PaperS3 project shape, pinned it to the local `ESP-IDF 5.3.4` toolchain assumptions, and brought up a small `esp_console` REPL over USB Serial/JTAG with a placeholder `wasm` command family.

The scope here was intentionally narrow. I did not try to build the full runtime service yet. The goal was to make sure the repo could host a new PaperS3 target, resolve WAMR through the component manager, boot a display-backed app, and compile cleanly enough that Task 3 can focus on runtime behavior instead of project plumbing.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Start executing the detailed task list by creating the new firmware, wiring the console skeleton, and committing at a sensible checkpoint once the baseline builds.

**Inferred user intent:** Establish a trustworthy first milestone that proves the architecture is viable on this repo and board family before deeper WAMR-specific logic is added.

**Commit (code):** `638bc95` - `feat(papers3): add 0079 wasm console scaffold`

### What I did

- created `0079-papers3-wamr-assemblyscript-console/`
- added top-level project files:
  - `CMakeLists.txt`
  - `README.md`
  - `sdkconfig.defaults`
  - `partitions.csv`
  - `dependencies.lock` after the first successful component-manager resolution
- reused the donor PaperS3 component stack with `EXTRA_COMPONENT_DIRS "../../M5PaperS3-UserDemo/components"`
- added `main/idf_component.yml` so the project can pull upstream `bytecodealliance/wasm-micro-runtime`
- set the baseline to interpreter-first WAMR with USB Serial/JTAG console defaults in `sdkconfig.defaults`
- added `main/app_main.cpp` to initialize the PaperS3 display and start the console
- added `main/console_repl.h` and `main/console_repl.cpp` to start `esp_console`
- added `main/wasm_command.h` and `main/wasm_command.cpp` with `examples`, `list`, `info`, `run`, and `status`
- built the project with `idf.py` after correcting the shell environment so it actually used `ESP-IDF 5.3.4`

### Why

- the design guide assumes a new numbered firmware project rather than bolting WAMR onto an unrelated existing app
- USB Serial/JTAG was required by the repo instructions and is the safest default for an interactive PaperS3 console
- before writing a runtime service, I needed proof that the project could resolve WAMR, compile against the donor M5 components, and link successfully on `esp32s3`

### What worked

- the donor PaperS3 component reuse model worked immediately once `EXTRA_COMPONENT_DIRS` pointed at `M5PaperS3-UserDemo/components`
- upstream WAMR resolved cleanly through `idf_component.yml` and generated a reproducible `dependencies.lock`
- the console skeleton built cleanly enough to establish a good checkpoint for the next task
- the successful build confirms that the repo can support a `0079` PaperS3 WAMR app without vendoring the runtime manually

### What didn't work

- the first build attempt inherited the wrong ESP-IDF shell state
- command:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C 0079-papers3-wamr-assemblyscript-console set-target esp32s3 build
```

- issue:
  - the shell still had `IDF_PYTHON_ENV_PATH` and `IDF_PATH` from another ESP-IDF installation, so this was not a trustworthy `5.3.4` build context
- corrected command:

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C 0079-papers3-wamr-assemblyscript-console set-target esp32s3 build
```

- there are still compile warnings to address later:
  - upstream WAMR emits several warnings during its own build
  - `main/wasm_command.cpp` currently uses a partial `esp_console_cmd_t` initializer that triggers missing-field warnings

### What I learned

- WAMR’s ESP-IDF integration is clean enough that `idf_component.yml` is the right first approach here
- `ESP-IDF 5.3.4` environment drift is easy to miss when multiple IDF shells are used on the same machine, so the diary should record the `unset IDF_PYTHON_ENV_PATH IDF_PATH` guard explicitly
- the right first command namespace is a thin `wasm` family, not separate one-off console commands for every guest action

### What was tricky to build

- the real risk in this step was not code complexity but environment ambiguity
- the build can appear to work while still using the wrong shell-exported IDF state, which would make later debugging noisy and misleading
- I treated that as a setup bug and normalized the build command before considering the milestone complete

### What warrants a second pair of eyes

- whether `main/idf_component.yml` should stay on the upstream `main` branch or be pinned to a specific commit once the runtime service stabilizes
- whether the initial partition table should reserve space for future embedded wasm assets beyond the current single 4 MB factory app layout
- whether we want to eliminate the `esp_console_cmd_t` initializer warnings now or wait until the command implementation gets refactored around a real runtime service

### What should be done in the future

- commit this scaffold and console checkpoint
- implement `wasm_runtime_service.*`
- replace placeholder `wasm status`, `wasm info`, and `wasm run` behavior with runtime-backed output

### Code review instructions

- start with `0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults`
- then read `0079-papers3-wamr-assemblyscript-console/main/console_repl.cpp` and `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp`
- check `0079-papers3-wamr-assemblyscript-console/main/idf_component.yml` to confirm how WAMR is being pulled in
- validate with:

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C 0079-papers3-wamr-assemblyscript-console set-target esp32s3 build
```

### Technical details

New project files:

- `0079-papers3-wamr-assemblyscript-console/CMakeLists.txt`
- `0079-papers3-wamr-assemblyscript-console/README.md`
- `0079-papers3-wamr-assemblyscript-console/sdkconfig.defaults`
- `0079-papers3-wamr-assemblyscript-console/partitions.csv`
- `0079-papers3-wamr-assemblyscript-console/dependencies.lock`
- `0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt`
- `0079-papers3-wamr-assemblyscript-console/main/idf_component.yml`
- `0079-papers3-wamr-assemblyscript-console/main/app_main.cpp`
- `0079-papers3-wamr-assemblyscript-console/main/console_repl.h`
- `0079-papers3-wamr-assemblyscript-console/main/console_repl.cpp`
- `0079-papers3-wamr-assemblyscript-console/main/wasm_command.h`
- `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp`

Build result:

- target: `esp32s3`
- app: `papers3_wamr_assemblyscript_console.bin`
- binary size: `0x70a40`
- reported free space in the 4 MB app partition: about `89%`

## Step 4: Add a real WAMR runtime service and make `wasm status` tell the truth

After the scaffold commit, the next missing piece was runtime state. The first version of `wasm status` only reported a placeholder string, which was enough to prove the console layout but not enough to prove that WAMR was actually initialized inside the PaperS3 firmware. I added a small runtime service layer so startup now initializes WAMR once, keeps a structured status snapshot, and exposes that snapshot through the console.

This was still deliberately below the full "run a module" threshold. I did not add loading or instantiation of real guest binaries in this step. The purpose was to create a solid runtime seam for later milestones: one place that owns WAMR init, one status structure the command layer can query, and one diary entry that records the first actual integration errors instead of only planning notes.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Continue the next task in order by integrating WAMR as a real runtime dependency, then record the work and commit it as its own checkpoint.

**Inferred user intent:** Turn the firmware from a console-shaped scaffold into something that measurably hosts a WebAssembly runtime, without yet taking on the full guest-module execution path.

**Commit (code):** `35bffdb` - `feat(papers3): add wamr runtime service`

### What I did

- added `0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.h`
- added `0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp`
- updated `0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt` to compile the new service and depend on `bytecodealliance__wasm-micro-runtime`
- updated `0079-papers3-wamr-assemblyscript-console/main/app_main.cpp` to initialize WAMR before starting the console REPL
- updated `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp` so:
  - `wasm status` prints actual runtime state
  - `wasm info <name>` reports whether the runtime is ready
  - `wasm run <name>` now fails honestly if the runtime is not ready, and otherwise states that module embedding is the remaining blocker
- replaced the partial `esp_console_cmd_t` initializer with explicit field assignment to remove the earlier missing-field warning from our own code
- rebuilt the firmware successfully with:

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C 0079-papers3-wamr-assemblyscript-console build
```

### Why

- Task 3 needed a real runtime boundary rather than leaving all WAMR logic smeared through `app_main.cpp` and the console command handler
- the console command family needs runtime-owned state to report version, allocator choice, and mode support coherently
- getting this seam in place now makes Task 5 and Task 6 much easier, because module loading can attach to the existing service instead of reinventing startup state

### What worked

- `wasm_runtime_full_init()` worked cleanly with `Alloc_With_System_Allocator` and interpreter mode on the local build
- WAMR version discovery and running-mode support queries worked once the runtime was initialized
- the new status output gives a concrete checkpoint: startup now creates a runtime and the console can inspect it
- the second build completed successfully after the integration fixes, with the app binary at `0x79f70` and about `88%` free space remaining in the 4 MB partition

### What didn't work

- the first build after adding `wasm_runtime_service.cpp` failed
- command:

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C 0079-papers3-wamr-assemblyscript-console build
```

- exact failure themes:
  - `esp_get_free_heap_size` and `esp_get_minimum_free_heap_size` were not declared because I had included `esp_heap_caps.h` instead of `esp_system.h`
  - `CONFIG_WAMR_ENABLE_AOT` was not declared because the symbol is absent in this disabled configuration rather than defined to `0`
  - several `%u` format strings failed under `-Werror=format` because `uint32_t` is `unsigned long` on this toolchain
- representative errors:

```text
error: 'esp_get_free_heap_size' was not declared in this scope
error: 'CONFIG_WAMR_ENABLE_AOT' was not declared in this scope
error: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'long unsigned int' [-Werror=format=]
```

- fixes applied:
  - included `esp_system.h`
  - guarded the AOT config read with `#if defined(CONFIG_WAMR_ENABLE_AOT)`
  - switched version and heap formatting to `PRIu32`
  - stopped pretending that "AOT support" was a running mode query and instead tied it to the build configuration for this milestone

### What I learned

- the WAMR integration itself was straightforward; the sharp edges came from ESP-IDF build strictness and config-symbol behavior
- on this toolchain, using `PRIu32` consistently is the safer choice than assuming `%u` matches `uint32_t`
- for early milestones, WAMR runtime readiness is a separate concern from module availability, and the command UX should say that plainly

### What was tricky to build

- the subtle trap was that the runtime service looked conceptually small, but it crossed three boundaries at once:
  - ESP-IDF startup state
  - WAMR C APIs from C++
  - strict embedded build flags
- that combination makes small assumptions expensive; a missing header or wrong format string becomes a full build stop because the project treats warnings seriously
- the fix was to treat the service like production firmware code immediately instead of assuming it could stay "temporary" until later tasks

### What warrants a second pair of eyes

- whether `Alloc_With_System_Allocator` is the right long-term choice or whether we should move to a dedicated pool once real module loading starts
- whether runtime initialization should stay eager in `app_main()` or become lazy the first time the user runs a `wasm` command
- whether the current example metadata should stay inside `wasm_command.cpp` until Task 5, or move earlier into a dedicated registry abstraction

### What should be done in the future

- commit this runtime-service checkpoint
- add the host-side AssemblyScript build pipeline
- add a real module registry so `wasm info` and `wasm run` can point at embedded artifacts instead of placeholder names

### Code review instructions

- start with `0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp`
- then inspect `0079-papers3-wamr-assemblyscript-console/main/app_main.cpp` and `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp`
- confirm that `main/CMakeLists.txt` now depends on `bytecodealliance__wasm-micro-runtime`
- validate with:

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C 0079-papers3-wamr-assemblyscript-console build
```

### Technical details

Runtime status now reports:

- init attempt state
- WAMR version
- requested running mode
- configured interpreter/AOT build flags
- supported interpreter / fast-JIT / LLVM-JIT modes
- ESP heap snapshot
- WAMR allocator mode
- runtime allocator heap metrics when available

Files changed in this step:

- `0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt`
- `0079-papers3-wamr-assemblyscript-console/main/app_main.cpp`
- `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp`
- `0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.h`
- `0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp`

## Related

- `../index.md`
- `../tasks.md`
- `../changelog.md`
- `../design-doc/01-papers3-wamr-assemblyscript-analysis-design-and-implementation-guide.md`
