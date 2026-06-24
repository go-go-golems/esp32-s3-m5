---
Title: ""
Ticket: ""
Status: ""
Topics: []
DocType: ""
Intent: ""
Owners: []
RelatedFiles:
    - Path: 0100-esp32-p4-quickjs-wasm/wasm-src/wasm_main.c
      Note: Reactor wrapper (qjs_init/qjs_eval) produced during design Step 4
    - Path: ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/sources/05-wamr-export-native-api.md
      Note: Native symbol signature legend (i/I/f/F/*/~/$/r) cited in diary Step 3
    - Path: ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/sources/10-vercel-quickjs-wasi-readme.md
      Note: QuickJS-as-WASM prior art that validated the approach
ExternalSources: []
Summary: ""
LastUpdated: 0001-01-01T00:00:00Z
WhatFor: ""
WhenToUse: ""
---


# Investigation Diary

## Goal

Capture the research, analysis, and design journey for running **QuickJS compiled to
WebAssembly** on the **ESP32-P4**, and produce an intern-ready analysis/design/implementation
guide stored in this ticket and uploaded to reMarkable. The eventual firmware lands in workspace
directory `0100-esp32-p4-quickjs-wasm`.

## Step 1: Orient in the workspace and load the relevant skills

I started by mapping the workspace and confirming which tools and prior art were available.
The workspace root is `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`,
which already contains two directly relevant families of projects: ESP32-S3 WAMR firmware
(`0079-papers3-wamr-assemblyscript-console`, `0082-papers3-wamr-allocator-control`) and ESP32-P4
bring-up firmware (`0097`, `0098`, `0099-esp32-p4-picocalc-display-keyboard`). The highest firmware
directory is `0099`, so the next available slot is `0100`.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to run quickjs compiled to wasm on the esp32p4 . Run research, download the interesting doucments to the sources folder of the ticket using defuddle, and analyze them. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable. Keep a diary as you work. The firmware will be in the next avail directory number in this workspace"

**Assistant interpretation:** Create a docmgr ticket for running QuickJS-as-WASM on the ESP32-P4, research the topic, harvest primary sources into the ticket's `sources/` with defuddle/curl, analyze them, write a thorough intern-oriented analysis/design/implementation guide (prose, bullets, pseudocode, diagrams, API + file references), keep a diary, and upload the bundle to reMarkable. The firmware goes in the next free workspace directory (`0100`).

**Inferred user intent:** A reusable, self-contained knowledge package that lets a new intern understand the entire stack (QuickJS, WebAssembly/WASI, WAMR, ESP32-P4) and implement firmware `0100` without rediscovering the architecture. Evidence-based, not hand-wavy.

### What I did

- Listed the workspace and confirmed `0099` is the highest firmware dir → `0100` is next.
- Checked `docmgr status` (175 tickets, 579 docs) and the vocabulary (already has `quickjs`, `microquickjs`, `wasm`, `assemblyscript`, `javascript`, `esp-idf`, `firmware`).
- Loaded pinned skills (`diary`, `docmgr`, `ticket-research-docmgr-remarkable`) and the `defuddle` + `remarkable-upload` skills.
- Verified `defuddle 0.18.1` and `remarquee` are installed.
- Inspected prior-art project structure: WAMR embedding source (`wasm_runtime_service.cpp`, `wasm_module_runner.cpp`, `wasm_host_api.cpp`) and the ESP32-P4 `sdkconfig.defaults`/`CMakeLists.txt` from `0099`.

### Why

An evidence-based guide must be anchored to real local prior art so the intern copies working
patterns rather than inventing new ones. The WAMR `0079`/`0082` projects are the closest analog:
they already embed `espressif/wasm-micro-runtime`, register native host symbols, and run wasm
modules from `EMBED_FILES`. The ESP32-P4 `0099` project fixes the target's console, memory, and
toolchain conventions.

### What worked

- The local `espressif__wasm-micro-runtime` component's Kconfig was still present under
  `0079/.../managed_components/`, giving the exact `CONFIG_WAMR_*` symbol names.
- The local WAMR host-API source gave a concrete, working `NativeSymbol` registration example.
- Vocabulary already covered the JS/WASM topics, so only `esp32p4` had to be added.

### What didn't work

- `defuddle parse` against `raw.githubusercontent.com` URLs returns 0 bytes (raw text/plain is
  not HTML). Fix: switched those specific docs to `curl -sL` and kept `defuddle` for HTML pages.
- The ESP32-P4 product page (`espressif.com/.../esp32-p4`) is JavaScript-rendered; defuddle
  extracted only 5 lines. Fix: used the ESP-IDF `memory-types.html` page and the datasheet facts
  from search instead.

### What I learned

- The repo has a "Micro QuickJS" lineage: the `microquickjs` vocab topic and ticket
  `ESP-30-M5DIAL-MQJS-LAIN-DSL` already explored embedding QuickJS natively on an ESP32. This new
  ticket is the *WASM-sandboxed* variant on the more capable P4.
- ESP32-P4 has no native USB Serial/JTAG console (unlike ESP32-S3); the PicoCalc/Waveshare P4
  board uses an external CH343 USB-UART bridge on UART0 (GPIO37/38). The S3 AGENTS.md console
  guidance does not transfer directly.

### What was tricky to build

- Distinguishing the two host boundaries in the design: (1) WAMR ↔ QuickJS-wasm (WASI + custom
  `env` imports) and (2) QuickJS ↔ user JavaScript (JS globals like `print`/`gpio` defined in C
  inside the wasm). The intern must understand that user JS never calls WAMR directly; it calls
  C inside the wasm, which calls WAMR imports.

### What warrants a second pair of eyes

- Whether to build QuickJS as a WASI *command* (uses `_start`, needs WAMR WASI FS) or a
  *reactor/library* module (exports `qjs_eval`, no FS). The design recommends the reactor form;
  review that the chosen `--no-entry --export=` link flags actually produce a module WAMR loads.

### What should be done in the future

- Validate the wasi-sdk QuickJS build on the host PC (Step 4 future work) before flashing.
- Add an AOT path (`wamrc --target=riscv32`) as a perf optimization once interp baseline works.

### Code review instructions

- Start with the design doc `design/01-...md`, section "Architecture".
- Cross-check WAMR config symbols against `0079/.../managed_components/espressif__wasm-micro-runtime/build-scripts/esp-idf/wamr/Kconfig`.

### Technical details

- Workspace firmware slot: `0100-esp32-p4-quickjs-wasm`.
- ESP-IDF version used by local P4 projects: `5.4.2` (`source ~/esp/esp-idf-5.4.2/export.sh`).

## Step 2: Create the ticket and add the `esp32p4` vocabulary topic

Created the docmgr ticket `ESP32-P4-QUICKJS-WASM` and added the missing `esp32p4` topic so
`docmgr doctor` stays clean.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Provision the ticket workspace (index/tasks/changelog/design/reference/playbooks/scripts/sources/various/archive) and register vocabulary.

### What I did

- `docmgr vocab add --category topics --slug esp32p4 --description "ESP32-P4 microcontroller ..."`.
- `docmgr ticket create-ticket --ticket ESP32-P4-QUICKJS-WASM --title "Run QuickJS compiled to WASM on the ESP32-P4 (intern implementation guide)" --topics esp32p4,quickjs,wasm,esp-idf,firmware,javascript,console`.
- Ticket path: `ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide`.

### Why

A dedicated ticket gives the intern one root for design, diary, sources, and tasks, and matches
the repo's established `NAME--slug` June-2026 convention.

### What worked

- Ticket creation produced the full directory scaffold including `sources/` and `scripts/`.

### What didn't work

- Nothing; one-shot success.

### What I learned

- The repo now uses descriptive `NAME--slug` ticket IDs (June 2026) rather than the older
  `ESP-NN-NAME--slug` scheme.

### What was tricky to build

- N/A (bookkeeping step).

### What warrants a second pair of eyes

- N/A.

### What should be done in the future

- N/A.

### Code review instructions

- Run `docmgr ticket list --ticket ESP32-P4-QUICKJS-WASM` to confirm structure.

### Technical details

- Ticket topics: `esp32p4,quickjs,wasm,esp-idf,firmware,javascript,console`.

## Step 3: Research and harvest primary sources into `sources/`

Ran Kagi searches and downloaded 14 primary sources into the ticket `sources/` folder using a mix
of `defuddle` (HTML pages) and `curl` (raw GitHub markdown / header). Then read the key technical
sources to extract precise API references and config symbols.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Gather authoritative, quotable references for QuickJS, WebAssembly/WASI, WAMR embedding, wasi-sdk, and the ESP32-P4, then mine them for exact APIs and config.

### What I did

- Defuddled: WAMR GitHub README, QuickJS Bellard home, WAMR dev site, vercel-labs/quickjs-wasi
  README, nick.zoic.org "WASM on ESP32 with WAMR" tutorial, ESP32-P4 memory-types, WAMR tutorial,
  wamr-app-framework README.
- curl'd raw: WAMR `export_native_api.md`, `embed_wamr.md`, `build_wamr.md`, wasi-sdk `README.md`,
  and the WAMR `wasm_export.h` header (84 KB — the authoritative C API).
- Read local prior art: `0079/main/wasm_runtime_service.cpp`, `wasm_module_runner.cpp`,
  `wasm_host_api.cpp`, `Kconfig.projbuild`, `idf_component.yml`, `CMakeLists.txt`; and the
  espressif WAMR component `Kconfig`.

### Why

The guide must cite real APIs (`wasm_runtime_load`, `wasm_runtime_instantiate`,
`wasm_runtime_register_natives`, native-symbol signature strings, `CONFIG_WAMR_*`) and real
QuickJS APIs (`JS_NewRuntime`, `JS_Eval`, `JS_NewCFunction`) — not paraphrases.

### What worked

- The WAMR `embed_wamr.md` gives the complete load→instantiate→lookup→call lifecycle and the
  buffer-passing API (`wasm_runtime_module_dup_data`), which is exactly how the host will hand
  JS source into the QuickJS wasm instance.
- The `export_native_api.md` gives the signature legend (`i I f F r * ~ $`) and the
  `wasm_exec_env_t`-first calling convention.
- The local `espressif__wasm-micro-runtime` Kconfig gives the exact ESP-IDF config symbols.

### What didn't work

- (Same raw-github + JS-page limitations noted in Step 1; worked around with curl + alternate pages.)

### What I learned

- `vercel-labs/quickjs-wasi` proves the approach: QuickJS-ng compiled to `wasm32-wasi`, exposing
  `evalCode` + host functions, snapshottable. Our design is the embedded-C equivalent of that.
- QuickJS is ES2025, ~a few C files, no deps, refcounting + cycle GC, MIT; official repo
  `github.com/bellard/quickjs`. Bellard also ships `mquickjs` (Micro QuickJS) for bare MCUs.
- ESP32-P4: dual-core RISC-V @ 400 MHz (HP) + LP @ 40 MHz, 768 KB HP SRAM (L2MEM), 32 KB LP SRAM,
  8 KB SPM, 32 MB stacked PSRAM (ESP32-P4NRW32), 16 MB flash, 55 GPIO, MIPI-CSI/DSI, USB 2.0 OTG.
  Memory is IRAM/DRAM (internal) vs IROM/DROM (flash via MMU cache) vs PSRAM (external heap).

### What was tricky to build

- Reconciling the "run QuickJS as wasm" mental model with the local 0079 "run AssemblyScript as
  wasm" pattern: same WAMR host, different guest. The guest (QuickJS) is itself an engine, so there
  are two layers of guest code (the QuickJS C engine, and the user JS it evaluates).

### What warrants a second pair of eyes

- The exact wasi-sdk link flags for a reactor QuickJS module (no `_start`, export `qjs_eval`).
  The design proposes flags; they must be validated on the host build step.

### What should be done in the future

- Capture the exact QuickJS C API signatures from `quickjs.h` in the sources folder (not done;
  relied on documented APIs). A future step should `curl` `quickjs.h` into `sources/`.

### Code review instructions

- Browse `sources/01..14-*.md` and `sources/09-wamr-wasm-export-header.h`.

### Technical details

- Sources directory: `ttmp/2026/06/23/ESP32-P4-QUICKJS-WASM--run-quickjs-compiled-to-wasm-on-the-esp32-p4-intern-implementation-guide/sources/`.
- 14 files; largest are `09-wamr-wasm-export-header.h` (84 KB) and `07-wamr-build-options.md` (39 KB).

## Step 4: Author the analysis/design/implementation guide

Wrote the primary deliverable `design/01-quickjs-wasm-esp32p4-analysis-design-and-implementation-guide.md`
targeting a new intern: prose + bullets + ASCII diagrams + pseudocode + API references + absolute
file references + a phased implementation plan + build/flash/verify + risks/decisions.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Synthesize all evidence into one navigable, intern-grade technical document.

### What I did

- Drafted sections: Executive Summary; System Architecture (hardware + JS-in-WASM-in-WAMR stack);
  Background Concepts (QuickJS, WASM/WASI, WAMR, ESP32-P4); Prior Art & Gap Analysis; Proposed
  Design (build pipeline, firmware embedding, two host boundaries, console REPL); API References
  (WAMR native API + signatures, QuickJS C API); Pseudocode (`wasm_main.c`, host registration,
  console command, eval flow); Diagrams (call flow, memory layout, build pipeline); File References;
  Phased Plan; Build/Flash/Verify; Testing & Validation; Risks/Alternatives/Open Questions; Decision
  Records; References.
- Anchored every major claim to a source file path or local prior-art file path.

### Why

The intern must reach a working firmware without re-deriving the architecture; the doc is the
single source of truth that the diary, tasks, and changelog point back to.

### What worked

- Reusing the local 0079 embedding pattern made the host-API pseudocode concrete and copy-pasteable.

### What didn't work

- (None so far; pending validation of the wasi-sdk build flags — see Step 3 future work.)

### What I learned

- The cleanest teaching model is "two host boundaries": WAMR↔wasm (WASI + `env` imports) and
  QuickJS↔user JS (C-defined globals). Stating this explicitly prevents the most common confusion.

### What was tricky to build

- Keeping the guide both exhaustive and navigable: sectioned with diagrams up front, deep API
  reference in the middle, concrete commands at the end.

### What warrants a second pair of eyes

- Memory sizing: the proposed WAMR pool (e.g. 1–2 MB in PSRAM) and QuickJS guest heap (e.g. 256 KB)
  are engineering estimates; validate against actual `wasm_runtime_get_mem_alloc_info` high-water.
- AOT feasibility: whether `wamrc --target=riscv32` produces a loadable P4 AOT module for QuickJS.

### What should be done in the future

- After the intern builds firmware `0100`, record measured numbers (eval latency, heap high-water,
  binary size) back into this guide's "Validation" section.

### Code review instructions

- Read `design/01-...md` end to end; cross-check the WAMR API calls against
  `sources/09-wamr-wasm-export-header.h` and the QuickJS calls against QuickJS docs.

### Technical details

- Design doc path: `design/01-quickjs-wasm-esp32p4-analysis-design-and-implementation-guide.md`.

## Step 5: Scaffold firmware `0100` and finalize ticket bookkeeping

Created a minimal firmware scaffold in workspace directory `0100-esp32-p4-quickjs-wasm` (README,
CMakeLists, sdkconfig.defaults, main/ stub) so the design's file references resolve to real paths,
then related files, updated changelog/tasks, ran `docmgr doctor`, and uploaded to reMarkable.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Make the workspace ready for the intern and close out the ticket with validation + reMarkable delivery.

### What I did

- Created `0100-esp32-p4-quickjs-wasm/{CMakeLists.txt, sdkconfig.defaults, partitions.csv, README.md, main/{CMakeLists.txt, app_main.c}}` matching the `0099` P4 conventions.
- `docmgr doc relate` for the design doc, diary, and key prior-art/firmware files (absolute paths).
- `docmgr changelog update` and `docmgr task` updates.
- `docmgr doctor --ticket ESP32-P4-QUICKJS-WASM --stale-after 30`.
- `remarquee upload bundle` (dry-run then real) to `/ai/2026/06/23/ESP32-P4-QUICKJS-WASM`.

### Why

The user asked for the firmware to occupy the next available directory and for the bundle to reach
reMarkable; bookkeeping keeps the ticket self-consistent for future sessions.

### What worked

- `docmgr doctor --ticket ESP32-P4-QUICKJS-WASM --stale-after 30` → **All checks passed** after adding YAML frontmatter to the 13 harvested `.md` sources (the `.h` source is not scanned).
- `remarquee upload bundle` (dry-run then real) → `OK: uploaded ESP32-P4 QuickJS-WASM Intern Implementation Guide.pdf -> /ai/2026/06/23/ESP32-P4-QUICKJS-WASM`.
- Firmware scaffold `0100-esp32-p4-quickjs-wasm` is buildable as a console-only stub (`main/CMakeLists.txt` keeps WAMR + `EMBED quickjs.wasm` commented until the intern adds the wasm).

### What didn't work

- Raw `defuddle`/`curl` source files lack YAML frontmatter, so the first `doctor` run reported 13 `invalid_frontmatter` errors. Fixed by prepending frontmatter (title/doc_type/ticket/topics/status=active/source_type=harvested) to each `.md` source.

### What I learned

- The `sources/` dir is created empty by `docmgr ticket create-ticket`; populating it with raw scraped `.md` requires frontmatter for the files to count as valid docmgr docs (and become searchable via `docmgr doc search`).

### What was tricky to build

- Keeping the scaffold minimal but buildable: it must `set-target esp32p4` and include the WAMR
  component dependency without yet compiling QuickJS (the intern does the wasm build).

### What warrants a second pair of eyes

- The scaffold's `idf_component.yml` WAMR dependency version should match `0079` (`2.4.0~1`).

### What should be done in the future

- The intern fills in `main/`, the `wasm-src/` QuickJS build, and the host API.

### Code review instructions

- `cd 0100-esp32-p4-quickjs-wasm && idf.py set-target esp32p4 && idf.py build` (after the intern
  adds the wasm asset and host API).

### Technical details

- Firmware path: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0100-esp32-p4-quickjs-wasm`.
- reMarkable remote dir: `/ai/2026/06/23/ESP32-P4-QUICKJS-WASM`.

## Step 6: Phase 0 — host-side QuickJS→WASM build + WAMR smoke test (in progress)

User asked to implement Phase 0 of the design: build `quickjs.wasm` on the host, verify imports/exports, and smoke-test it with a WAMR host before any device work. The user also said we will run this on the same device as the PicoCalc firmware (`0099`), so the firmware target/console/PSRAM config already matches `0099`'s `sdkconfig.defaults`.

### Prompt Context

**User prompt (verbatim):** "ok, implement phase 0. we will be runnning thi son the same device as the picocalc firmware" — then later: "let me know when you are ready to test on the device and i'll connect it. keep a diary as you work, and commit at appropriate intervals." and "remembber to keep a frequent diary as you work to keep track of all you do, so that we can write a great report once we are done"

**Assistant interpretation:** Build quickjs.wasm with wasi-sdk on the host PC, verify it, run it under a WAMR host test (the exact code path the firmware will use), and only then tell the user to connect the device. Keep the diary current and commit in logical chunks.

**Inferred user intent:** Prove the JS-in-WASM-in-WAMR stack works end to end on the host before flashing, so the device session is short and high-signal.

### What I did

- **Environment probe:** no `wasi-sdk`, no `wasm-ld`/`lld`, no `wabt`, no wasm runtime on the machine; no passwordless sudo (so everything must be userspace). System clang 18 has the `wasm32` target but lacks the WASI sysroot. `cmake`/`ninja`/`gcc` present. **WAMR is vendored locally** in `0079/managed_components/bytecodealliance__wasm-micro-runtime` (with a `product-mini/platforms/linux` host build) — so I can build a faithful `libvmlib` host test from it.
- **Installed wasi-sdk-33** (userspace, `~/tools/wasi-sdk-33.0-x86_64-linux`): clang 22.1.0 (`wasm32-unknown-wasip1`), `wasm-ld`, `wasi-sysroot`. 185 MB tarball.
- **Vendored QuickJS** (`git clone --depth 1 bellard/quickjs` → version `2026-06-04`) into `0100/wasm-src/quickjs`.
- **Built `quickjs.wasm`** as a reactor (exports `qjs_init`/`qjs_eval`; imports `env.host_print/host_millis/host_gpio_write` + `wasi_snapshot_preview1.*`).
- **Wrote a dependency-free Python wasm section parser** (`wasm_inspect.py`) to replace `wabt`/`wasm-objdump` (no sudo to install wabt). Verified imports/exports exactly match the design.
- **Built a WAMR host test** (`wasm-src/host-test/{CMakeLists.txt,host_test.c}`) that builds `libvmlib` (interp + WASI + ref-types) from the vendored WAMR and links a host program that registers the `env` natives, loads `quickjs.wasm`, and calls `qjs_init` + `qjs_eval`.
- **Iterated on build errors** (all captured verbatim below).

### Build errors hit and fixed (verbatim chain)

1. `malloc_usable_size` undeclared (quickjs.c) → wasi-libc's `dlmalloc` *defines* it but doesn't *declare* it in a header QuickJS includes. Fix: `wasm_shim.h` (force-included via `-include`) that forward-declares `size_t malloc_usable_size(void *);` — the definition comes from libc.a.
2. `CONFIG_VERSION` undefined (quickjs.c fprintf) → Fix: `-DCONFIG_VERSION="$(cat quickjs/VERSION)"` (= `2026-06-04`), matching the upstream Makefile.
3. setjmp/longjmp `#error` in wasi-libc `setjmp.h` (dtoa.c includes it but **never uses it** — verified no `setjmp(`/`longjmp(`/`jmp_buf` call sites) → first tried `-D__wasm_exception_handling__` (silences the `#error`) but later switched to a **stub `wasm_overrides/setjmp.h`** via `-I` to avoid pulling the EH proposal at all.
4. **duplicate symbol `malloc_usable_size`** → my first shim *defined* it; dlmalloc already does. Fix: drop the definition, keep only the declaration.
5. **WAMR load failure:** `"The module uses reference types feature which is disabled in the runtime"` (clang 22 emits reference-types table ops) → Fix: `set(WAMR_BUILD_REF_TYPES 1)` in the host-test CMake (and on device: `CONFIG_WAMR_ENABLE_REF_TYPES=y`). After this, `qjs_init` ran successfully.
6. **WAMR `module_dup_data` failure:** `"app heap is corrupted ... please add -Wl,--export=malloc -Wl,--export=free"` → Fix: export `malloc`/`free` so WAMR can allocate in the guest heap.
7. `host_print` contract mismatch (wasm declared `(s,len)`, host signature `($)` was 1-param) → Fix: made `host_print(const char *s)` single-param (WAMR `$` gives a NUL-terminated copy), updated both call sites.

### Where it stands (the current blocker)

- `qjs_init` succeeds (prints `[qjs_init done]`), so the module loads, instantiates, and QuickJS creates its runtime/context + registers `print`.
- `qjs_eval` runs to completion (WAMR `call_wasm` returns true — **no trap**) but returns `-1`: `JS_Eval` throws a **message-less exception** for *every* input, including `1+2`.
- **Native sanity check (gcc, same `qjs_init`/`qjs_eval`/`js_print` sources) works perfectly:** `print(1+2)`→`3`, `for(let i=0;i<3;i++) print(i)`→`012`. So the QuickJS API usage is correct; the failure is **wasm-specific**.
- Verified the source string arrives intact in wasm memory (`[qjs_eval enter src=print(1+2)]`), memory is plentiful (8 MB initial linear memory, dlmalloc from `__heap_base`=166048), and WAMR verbose log shows **no error/trap/grow failure** — `Memory instantiate success`.
- A message-less throw rules out QuickJS's `JS_ThrowOutOfMemory` ("out of memory") and `JS_ThrowStackOverflow` ("stack overflow") — both carry messages.
- Key representation difference found: `quickjs.h` does `#ifndef JS_PTR64 #define JS_NAN_BOXING`. So **wasm32 (32-bit pointers) uses NaN-boxing (`uint64_t` JSValue) while native 64-bit uses a struct JSValue** — a real divergence to investigate.

### What worked

- The whole host toolchain installs and builds userspace; the wasm imports/exports match the design exactly; the WAMR host test (built from the vendored runtime) loads/instantiates/`qjs_init` correctly.
- The native build of the identical wrapper proves the QuickJS embedding is correct.

### What didn't work

- `JS_Eval` throws message-less in the wasm build only. `-O3 -flto` → `-O2` did not help; bumping initial memory 1 MB → 8 MB did not help; the stub-setjmp swap did not help.

### What I learned

- clang 22 (wasi-sdk-33) emits reference-types by default → WAMR must enable `WAMR_BUILD_REF_TYPES` (and the device `CONFIG_WAMR_ENABLE_REF_TYPES=y`); my original design had it `=n` — **needs updating**.
- WAMR requires wasi-sdk modules to `--export=malloc --export=free` for guest-heap allocation (module_dup_data).
- The `__wasm_exception_handling__` define is a risky hack (it lies to wasi-libc that EH is compiled in); the stub `setjmp.h` is the principled fix.
- Native-vs-wasm isolation (build the same wrapper with gcc) was the highest-signal debug step — it pinned the issue to the wasm build, not the embedding logic.

### What was tricky to build

- Driving `JS_Eval` to a message-less exception with zero WAMR-side error is unusual: the classic culprits (trap, OOM, stack overflow) are all ruled out by the absence of a trap and the presence of messages on those paths. The remaining suspects are wasm-specific: NaN-boxed JSValue handling under WAMR's interp, or a clang-22-vs-WAMR-2.4.0 feature gap (the vendored WAMR is ~early 2024; clang 22 is 2025/2026).

### What warrants a second pair of eyes

- Whether WAMR 2.4.0 (espressif component `2.4.0~1`) correctly runs this clang-22 wasm; if a newer WAMR host works, the device component may need bumping too.
- The NaN-boxing `JSValue` (uint64) round-trip through WAMR's fast interpreter.

### What should be done in the future

- Capture the actual exception value/tag (need a small int-to-string helper in `wasm_main.c`) to identify the message-less throw precisely.
- Try `-O0`/`-O1` to rule out an optimizer miscompile; try a newer WAMR host.
- Once the host smoke test prints `3`, copy `quickjs.wasm` into `main/`, wire Phase 1 firmware, build for `esp32p4`, then ask the user to connect the device.

### Code review instructions

- `cd 0100-esp32-p4-quickjs-wasm/wasm-src && ./build-quickjs-wasm.sh && python3 wasm_inspect.py ../wasm-build/quickjs.wasm`
- `cd 0100-esp32-p4-quickjs-wasm/wasm-src/host-test && cmake -B build && cmake --build build && ./build/host_test ../../wasm-build/quickjs.wasm "print(1+2)"`
- Native comparison: `gcc -O2 -DCONFIG_VERSION=\"native-test\" -I quickjs quickjs/{quickjs,cutils,dtoa,libregexp,libunicode}.c wasm_main.c native_test.c -lm -lpthread -o /tmp/qjs_native && /tmp/qjs_native "print(1+2)"`

### Technical details

- wasi-sdk: `/home/manuel/tools/wasi-sdk-33.0-x86_64-linux` (clang 22.1.0, target `wasm32-wasip1`).
- WAMR source (vendored): `0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime`.
- quickjs.wasm: `0100-esp32-p4-quickjs-wasm/wasm-build/quickjs.wasm` (~1.4 MB, -O2).
- WAMR config needed: `WAMR_BUILD_INTERP=1`, `WAMR_BUILD_FAST_INTERP=1`, `WAMR_BUILD_LIBC_BUILTIN=1`, `WAMR_BUILD_LIBC_WASI=1`, `WAMR_BUILD_REF_TYPES=1`, `WAMR_BUILD_AOT=0`.

## Step 7: Phase 0 success — root cause was QuickJS's C-stack-overflow check

Found the blocker and fixed it. Phase 0 host smoke test now passes end to end.

### Prompt Context

**User prompt (verbatim):** (see Step 6) — plus "Once you are confident you reached a good stopping point after this first success, stop and write a detailed project report for the obsidian vault ..."

**Assistant interpretation:** Reach a clean Phase 0 stopping point (wasm builds + host smoke test prints `3`), record the root cause, commit, then write the Obsidian deep-dive report.

### What I did

- Added a `host_print_int` helper and an exception dumper to `wasm_main.c` to read the actual exception (type/name/message) instead of the unhelpful "message-less" view.
- Result: `JS_Eval` threw `SyntaxError: stack overflow` for every input, including `1+2`.
- Cross-referenced `quickjs.c`: the parser calls `js_parse_error(s, "stack overflow")` (lines 22837, 23484) when `js_check_stack_overflow(rt)` returns true. That check reads the real C stack pointer via `__builtin_frame_address(0)` and compares it to `rt->stack_limit = rt->stack_top - rt->stack_size`.
- Root cause: under WAMR's interpreter the host C stack pointer does not track wasm/js call recursion. `rt->stack_top` is captured once at `JS_NewRuntime`; the default limit then false-trips during the first parse, and the false trip also made `JS_ToCString` fail (its `toString` re-entered the check), which is why the exception first appeared message-less.
- Fix: `JS_SetMaxStackSize(rt, 0)` in `qjs_init` → `update_stack_limit` sets `rt->stack_limit = 0` ("no limit"), so `sp < 0` is always false. Wasm stack bounds are enforced by WAMR instead.
- Cleaned up `wasm_main.c` (removed diagnostics; `JS_ToCString` now works, so the exception handler prints the message directly) and set the host-test log level back to `WARNING`.
- Copied `quickjs.wasm` (1.2 MB, -O2) into `main/` for the future firmware embed.

### What worked

Clean smoke test output:
```
print(1+2)            -> 3               (returned 0)
print(6*7)            -> 42              (returned 0)
for(let i=0;i<3;i++) print(i)   -> 0/1/2  (returned 0)
let s="hi"; print(s+" wasm")  -> hi wasm (returned 0)
throw new Error('boom')        -> Error: boom  (returned -1, exception printed)
```
The JS-in-WASM-in-WAMR stack works on the host: user JS is parsed and executed by a QuickJS engine that itself runs as a wasm module under WAMR, and `print`/`millis`/`gpio_write` cross both host boundaries correctly.

### What didn't work

- (Resolved.) The native-vs-wasm isolation in Step 6 was the turning point: it pinned the failure to the wasm build and ruled out the QuickJS embedding logic.

### What I learned

- A message-less QuickJS exception often means a second, hidden exception (here: `toString` re-entering the same false stack-overflow check). Reading the Error's `.name`/`.message` properties directly (string property lookups, no `toString`) bypasses that and reveals the real error.
- `JS_SetMaxStackSize(rt, 0)` is the correct setting whenever QuickJS is run under a wasm interpreter rather than as native code: the C-stack recursion limit it models does not exist in that execution model.
- clang 22 (wasi-sdk-33) emits reference types by default; WAMR builds need `WAMR_BUILD_REF_TYPES=1` (and the ESP-IDF component needs `CONFIG_WAMR_ENABLE_REF_TYPES=y`).

### What was tricky to build

- The exception first presented as "no message", which is the least informative failure mode. The chain `false stack-overflow -> JS_Eval throws SyntaxError -> JS_ToCString re-trips -> returns NULL` is non-obvious; only dumping `.name`/`.message` exposed it.

### What warrants a second pair of eyes

- Whether disabling QuickJS's stack check (stack_size=0) leaves the device vulnerable to unbounded JS recursion. On WAMR this surfaces as a wasm stack trap (call_wasm returns false) rather than a clean JS exception — acceptable for bounded scripts, worth a `js reset`/watchdog on the device.
- Memory sizing for the device (2 MB WAMR pool in PSRAM, 256 KB QuickJS JS heap) is still an estimate; validate with profiling once on hardware.

### What should be done in the future

- Phase 1: wire the WAMR host API into `0100/main/` (port from `0079`), embed `quickjs.wasm` via `EMBED_FILES`, implement `js eval`/`js status`, build for `esp32p4`, then ask the user to connect the device.
- Update the design doc's `sdkconfig.defaults` to set `CONFIG_WAMR_ENABLE_REF_TYPES=y` (it was `=n`).
- Record measured device numbers (eval latency, heap high-water, binary size) back into the design doc's Validation section.

### Code review instructions

- `cd 0100-esp32-p4-quickjs-wasm/wasm-src && ./build-quickjs-wasm.sh && python3 wasm_inspect.py ../wasm-build/quickjs.wasm`
- `cd 0100-esp32-p4-quickjs-wasm/wasm-src/host-test && cmake -B build && cmake --build build -j && ./build/host_test ../../wasm-build/quickjs.wasm "print(1+2)"`

### Technical details

- Fix commit: one line, `JS_SetMaxStackSize(rt, 0);` in `qjs_init`.
- quickjs.wasm: 1.2 MB (-O2, wasm32-wasip1, reactor, exports qjs_init/qjs_eval/malloc/free).

## Step 8: Phase 0 wrap-up — tasks checked, Obsidian report published, Phase 1 starting

Closed out Phase 0: checked off the Phase 0 tasks in `tasks.md`, committed the build infra and diary, and wrote + pushed the Obsidian deep-dive report. Now moving to Phase 1 (wire the WAMR host API into firmware `0100`, embed `quickjs.wasm`, build for `esp32p4`).

### Prompt Context

**User prompt (verbatim):** "check off necessary tasks once you are done, and then continue with the next phase. don't forget to write a frequent detailed diary as you work"

**Assistant interpretation:** Mark Phase 0 complete in the ticket task list, finish the pending vault commit/push, then begin Phase 1 firmware wiring with frequent diary updates.

### What I did

- Committed and pushed the Obsidian report to the go-go-parc vault: `ARTICLE - QuickJS Wasm on WAMR - Running a JS Engine Inside a Wasm Sandbox.md` at `Projects/2026/06/23/` (commit `20eb64c`, pushed to `origin/main`). Staged only the note file, not incidental workspace changes.
- Updated `tasks.md`: Phase 0 items moved to Done; Phase 1 items are the active checklist.
- This diary entry.

### What worked

- Vault push succeeded: `b701288..20eb64c main -> main`.

### What should be done in the future

- Fix the design doc + scaffold `sdkconfig.defaults`: set `CONFIG_WAMR_ENABLE_REF_TYPES=y` (the design had `=n`; Phase 0 proved clang 22 emits reference types).

### Technical details

- Vault note path: `/home/manuel/code/wesen/go-go-golems/go-go-parc/Projects/2026/06/23/ARTICLE - QuickJS Wasm on WAMR - Running a JS Engine Inside a Wasm Sandbox.md`.
- Firmware commit history: `4626781` (Phase 0 build) → `7b59511` (diary 6-7) → (this step's tasks/diary commit).

## Step 9: Phase 1 — wire WAMR host API into firmware 0100; builds for esp32p4

Implemented the firmware host side and got it to compile and link for the ESP32-P4. The app is not yet flashed (that needs the user to connect the device).

### Prompt Context

**User prompt (verbatim):** (see Step 8) — continue Phase 1 with frequent diary.

### What I did

- Fixed `sdkconfig.defaults`: set `CONFIG_WAMR_ENABLE_REF_TYPES=y` (was `=n`; Phase 0 proved clang 22 emits reference types), and enabled the custom partition table (`CONFIG_PARTITION_TABLE_CUSTOM=y` + `CUSTOM_FILENAME` + `FILENAME` = `partitions.csv`) because the app is ~1.8 MB and overflows the default 1 MB factory partition.
- Wrote the firmware host sources in `0100/main/`, modelled on the proven Phase-0 `host_test.c` and `0099`'s console setup: `quickjs_embed.h` (EMBED blob symbols), `wasm_runtime_service.{h,cpp}` (WAMR init with a 16 MB PSRAM pool), `wasm_host_api.{h,cpp}` (the `env` natives `host_print`/`host_millis`/`host_gpio_write` with signatures `($)`/`()i`/`(ii)`), `wasm_runner.{h,cpp}` (load embedded `quickjs.wasm`, instantiate once, `qjs_init`, `qjs_eval` via `module_dup_data`), `js_command.{h,cpp}` (`js eval <source>` joins args; `js status`), and `app_main.cpp` (UART0 console + init + register).
- Updated `main/CMakeLists.txt` to `EMBED_FILES quickjs.wasm` and `REQUIRES espressif__wasm-micro-runtime console pthread` (+ `PRIV_REQUIRES esp_driver_gpio esp_timer esp_psram heap`).
- Moved the component manifest from the project root to `main/idf_component.yml` (the component manager reads the main component's manifest there; the root copy was ignored).
- Built with `source ~/esp/esp-idf-5.4.2/export.sh && idf.py set-target esp32p4 && idf.py build`.

### Build errors hit and fixed

1. `Failed to resolve component 'espressif__wasm-micro-runtime': unknown name` → the manifest was at the project root; ESP-IDF's component manager reads `main/idf_component.yml`. Moved it there; `idf.py reconfigure` then fetched `espressif__wasm-micro-runtime`.
2. `'printf' was not declared` in `wasm_runner.cpp` → added `#include <cstdio>`.
3. `esp_console_cmd_t` missing-field-initializers warnings → zero-initialised the struct (`esp_console_cmd_t cmd = {};` then assign fields).
4. `app partition is too small ... size 0x1be410 (1.77 MB) ... factory size 0x100000 (1 MB)` → the custom `partitions.csv` (4 MB factory) was not applied. Root cause: `sdkconfig.defaults` changes are ignored once `sdkconfig` exists; `idf.py fullclean` does **not** delete `sdkconfig`. Fix: `rm -f sdkconfig` (regenerate from defaults) + set `CONFIG_PARTITION_TABLE_CUSTOM=y`/`CUSTOM_FILENAME`/`FILENAME` (matching `0098`'s pattern). After that, `CONFIG_PARTITION_TABLE_CUSTOM=y` and the build passed.

### What worked

- `idf.py build` → `Project build complete. To flash`. App binary: 1.8 MB (`build/0100-esp32-p4-quickjs-wasm.bin`), fits the 4 MB factory partition. `quickjs.wasm` (1.2 MB) is embedded via `EMBED_FILES` and resolved to `_binary_quickjs_wasm_start`/`_end` symbols.

### What didn't work

- (Resolved, see above.) The `sdkconfig.defaults`-not-applied trap cost two rebuild cycles; this is the single most important idf.py gotcha to document for future agents.

### What I learned

- ESP-IDF component dependencies for the main component go in `main/idf_component.yml`, not a project-root `idf_component.yml`.
- `sdkconfig.defaults` only seeds options absent from the generated `sdkconfig`; to force a defaults change (e.g. partition table), delete `sdkconfig` (or `rm sdkconfig` then build). `idf.py fullclean` removes `build/` and `managed_components/` but keeps `sdkconfig`.
- The embedded QuickJS+WAMR app is ~1.8 MB, so every QuickJS-WAMR firmware needs a custom partition table with a ≥2 MB factory.

### What warrants a second pair of eyes

- Runtime behaviour on hardware is unverified: the 16 MB PSRAM pool, 32 KB guest stack, and 512 KB guest heap are estimates from the host test. `qjs_init` and `js eval` must be confirmed on the P4.
- `host_gpio_write` calls `gpio_set_level` without configuring the pin; Phase 2 must add `gpio_config` before JS can touch GPIOs.

### What should be done in the future

- Flash to the PicoCalc P4 board (user connects device), run `js eval "print(1+2)"`, expect `3`; run `js status`.
- Then Phase 2 (REPL, `js reset`, real GPIO/millis wiring, `js bench`).

### Code review instructions

- `cd 0100-esp32-p4-quickjs-wasm && source ~/esp/esp-idf-5.4.2/export.sh && idf.py build`
- Inspect `main/wasm_runner.cpp` (eval flow) and `main/wasm_host_api.cpp` (signatures) against the Phase-0 `host_test.c`.

### Technical details

- Build: `~/esp/esp-idf-5.4.2` (5.4.2); target `esp32p4`; console UART0 (CH343 bridge); PSRAM hex 200 MHz; WAMR pool 16 MB in PSRAM.
- App binary: 1.8 MB; partition: custom `partitions.csv` (factory 4 MB).

## Step 10: Document the idf.py build/dev-env rules for future agents

Captured the `idf.py` gotchas from Steps 9 (and earlier) so future coding agents do not rediscover them, as the user requested.

### Prompt Context

**User prompt (verbatim):** "as you've been fighting with idf.py a little bit i think it would be wise to add some concise instructions on building / rebuilding / setting up the dev environment properly in the AGENTS.md file so that future coding agents get the right instructions. Feel free to point to existing doc files in docs/ or maybe create one of your own (see the textbook writing guidelines for more explanatory text and the glazed help entry guidelines on writing as well)."

**Assistant interpretation:** Add concise build/rebuild/dev-env rules to `AGENTS.md`, backed by a detailed playbook in `docs/`, written in textbook prose with operational (glazed-style) sections.

### What I did

- Wrote `docs/01-playbook-esp-idf-build-and-dev-environment.md` (docmgr `DocType: playbook` frontmatter, matching the existing `docs/` playbooks). Covers: the multiple installed IDF versions and which projects use which, sourcing the toolchain, `set-target`, the `main/idf_component.yml` rule, how `sdkconfig.defaults` is applied (the `rm sdkconfig` trap; `fullclean` keeps `sdkconfig`), custom partition tables, target-specific console (S3 USB Serial/JTAG vs P4 UART0), build/flash/monitor, serial port patterns, what is/isn't committed, a troubleshooting table of the exact failures hit, and a See Also.
- Added a concise `## Build and dev environment (concise rules)` section at the top of `AGENTS.md` that points to the playbook and states the 7 critical rules. Clarified the existing console section as ESP32-S3-only with an explicit ESP32-P4 exception (UART0/CH343), since the original text implied USB Serial/JTAG universally.

### What worked

- The playbook and AGENTS.md cross-reference each other; the troubleshooting table records the verbatim errors from this ticket (component-not-resolving, defaults-not-applied, partition-too-small, printf/`<cstdio>`, `esp_console_cmd_t` initializers, WAMR reference-types, malloc/free export).

### What should be done in the future

- Keep the playbook's IDF-version table current as new projects/versions are added.

### Technical details

- Playbook: `docs/01-playbook-esp-idf-build-and-dev-environment.md`. AGENTS.md: top section + clarified console heading.

## Step 11: First device session — two crashes found (flash store-fault fixed; pthread_self assert open)

Flashed firmware 0100 to the PicoCalc ESP32-P4 (CH343 bridge `/dev/ttyACM0`, auto-reset works, chip confirmed ESP32-P4). WAMR init + native registration succeed on hardware, but two embedding crashes surfaced that the host did not. Wrote a full post-mortem (`design/02-phase1-device-bringup-post-mortem.md`) for the intern.

### Prompt Context

**User prompt (verbatim):** "it's connected, can you see it?" → (flashed + monitored via tmux) → "Ok, it's getting late. Write a full post-mortem / report on where you are at now, all you did when porting to the esp32, etc... Use a textbook writing style, address the fundamentals and necessary knowledge so that our new intern can continue the debugging tomorrow. Once that is done, write a detailed project report for the obsidian vault ... Commit and push the obsidian vault when done."

**Assistant interpretation:** Run the device session with idf.py/tmux, capture the result, then write a textbook post-mortem (ticket) and an Obsidian deep-dive report, and push the vault.

### What I did

- Used `idf.py -p /dev/ttyACM0 flash` + a tmux monitor (`tmux send-keys`/`capture-pane`) as the single serial owner. Confirmed 32 MB hex PSRAM @ 200 MHz, CPU 360 MHz, app boots.
- **Crash A (diagnosed + fixed + verified):** `Store access fault` (MCAUSE 0x7) in WAMR `b_memmove_s` (`bh_common.c:116`) writing to `0x400809b9`, which is inside the flash-mapped `quickjs.wasm` segment (segment 0 `0x40070020`–`0x401b066c`). WAMR's loader writes to the module buffer; `EMBED_FILES` puts it in read-only flash. Fix in `wasm_runner.cpp`: copy the blob to PSRAM (`heap_caps_malloc MALLOC_CAP_SPIRAM`) before `wasm_runtime_load`. After the fix, load succeeds (log: `copied quickjs.wasm (1231348 bytes) to writable buffer 0x49000aa8`).
- **Crash B (diagnosed, open):** `assert failed: pthread_self` at `wasm_exec_env_set_thread_info` ← `os_self_thread` ← `wasm_runtime_call_wasm` (our `qjs_init` call from `app_main`). WAMR thread tracking calls `pthread_self`, which asserts because `app_main`'s `main_task` is a FreeRTOS task, not a pthread. Not yet fixed.
- Wrote `design/02-phase1-device-bringup-post-mortem.md` (fundamentals, both crashes with addresses/backtraces, fix directions, resume commands).

### What worked

- `idf.py flash` + tmux monitor is a clean single-owner way to drive the console and capture output; the CH343 auto-reset works (no manual reset needed).
- Crash A's PSRAM-copy fix is verified on hardware (load completes). This is the same fix that distinguishes the host (malloc'd buffer) from the device (flash).

### What didn't work

- JavaScript is not yet evaluated on the device: Crash B fires at the `qjs_init` `call_wasm` boundary, before the engine runs. (My earlier read that `qjs_init` was "slow" was wrong — the panic fires immediately at `call_wasm` entry; the device was in a boot loop.)

### What I learned

- `EMBED_FILES` symbols point into read-only flash; WAMR's loader writes to the buffer it is given → the module must be copied to writable memory before `wasm_runtime_load`. This is a general WAMR-on-ESP-IDF rule, not QuickJS-specific.
- WAMR's `wasm_exec_env_set_thread_info` uses `pthread_self` on ESP-IDF; `wasm_runtime_call_wasm` must be called from a pthread, not a bare FreeRTOS task. `0079` avoids this somehow — reading its runner is the fastest path to the fix.

### What warrants a second pair of eyes

- Crash B fix direction (pthread vs lazy-init-from-console-task vs a WAMR config). Confirm whether `0079`'s calling task is a pthread.
- Whether disabling `CONFIG_WAMR_ENABLE_LIB_PTHREAD` or another WAMR Kconfig removes the `os_self_thread`→`pthread_self` path.

### What should be done in the future

- Resolve Crash B (see post-mortem §5). Then `js eval "print(1+2)"` → expect `3`.
- Record the device's measured `qjs_init` and `js eval` latency.

### Code review instructions

- `design/02-phase1-device-bringup-post-mortem.md` for the full analysis.
- `0100/main/wasm_runner.cpp` (Crash A fix + Crash B call site).

### Technical details

- Device: `/dev/ttyACM0` = `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B61091051-if00`; chip ESP32-P4 rev v1.3; ESP-IDF v5.4.2.
- Crash A MCAUSE=0x7 (store access fault), MTVAL=0x400809b9. Crash B: `assert failed: pthread_self` in `wasm_exec_env_set_thread_info`.

## Step 12: Take over Crash B and compare 0100 against the working 0079 WAMR pattern

Started the second device-debugging session by preserving the current failure model instead of immediately editing code. The goal of this step was to identify the exact difference between 0100 and the known-working 0079 WAMR project, then choose a fix that can be committed in 0100 without relying on generated `managed_components` edits.

The useful finding is that 0079 contains two relevant protections: it executes wasm work inside a pthread worker and its managed WAMR copy has a local ESP-IDF `os_self_thread()` patch that returns `xTaskGetCurrentTaskHandle()` instead of `pthread_self()`. 0100 is building the registry `espressif__wasm-micro-runtime` component with `WASM_ENABLE_THREAD_MGR=1` and the unpatched ESP-IDF thread shim, so the next implementation should route all `wasm_runtime_call_wasm` calls through a real pthread owned by the QuickJS session.

### Prompt Context

**User prompt (verbatim):** "Your little brother has been fighting with this crash, let's step in and figure out if we can find this bug and figure out how to make it work."

**User follow-up (verbatim):** "make sure to commit at appropriate intervals, and to keep a diary as you work."

**Assistant interpretation:** Resume the ticket, diagnose Crash B rather than just reporting it, make a real fix, document each meaningful step in the diary, and commit focused changes as checkpoints.

**Inferred user intent:** Get the ESP32-P4 firmware past the WAMR `pthread_self` assertion and toward an actual `js eval` result on hardware while preserving a continuation trail for future work.

### What I did

- Read the tail of `reference/01-investigation-diary.md` to re-enter the ticket context.
- Grepped 0079 and 0100 for `wasm_runtime_call_wasm`, `pthread`, `wasm_runtime_init_thread_env`, and console/task creation.
- Read 0079's `RunEmbeddedWasmModuleOnWorkerThread()` flow, which creates a pthread, calls `wasm_runtime_init_thread_env()` inside the worker, performs the wasm call there, then destroys the thread env.
- Read 0100's current `app_main.cpp`, `wasm_runner.cpp`, `wasm_runner.h`, and `js_command.cpp`. 0100 currently calls `wasm_runner_init()` from `app_main` and `wasm_runner_eval()` from the console command; both routes can hit `wasm_runtime_call_wasm` from non-pthread FreeRTOS tasks.
- Inspected the active 0100 build and confirmed it compiles `managed_components/espressif__wasm-micro-runtime` with `WASM_ENABLE_THREAD_MGR=1`, `WASM_ENABLE_LIB_PTHREAD=1`, and `WASM_ENABLE_SHARED_MEMORY=1`.
- Read the 0079 WAMR `core/shared/platform/esp-idf/espidf_thread.c` and found a local patch: `os_self_thread()` returns `xTaskGetCurrentTaskHandle()` with an explicit comment explaining the same `pthread_self()` assert.

### Why

- The first post-mortem hypothesized that WAMR calls need a pthread, but 0079 offered concrete evidence. Comparing with 0079 avoids a speculative fix.
- Editing 0100's generated `managed_components` would be brittle because the component manager can regenerate it. A 0100-owned runner-thread design is commit-safe and keeps QuickJS's runtime context single-thread-owned.

### What worked

- The 0079 comparison immediately explained why the prior assumption was incomplete: 0079 is not merely “console task works”; it has a pthread-worker pattern and a WAMR platform shim patch.
- The 0100 build logs contain repeated backtraces proving Crash B occurs at `managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c:40`, matching the source-level finding.

### What didn't work

- A one-line Kconfig change is not yet justified. The active compile command shows thread manager support is enabled; disabling only wasm lib-pthread may not remove the native thread-tracking path.

### What I learned

- The most durable app-level fix is a long-lived QuickJS/WAMR owner pthread. It should load, instantiate, run `qjs_init`, and service `qjs_eval` requests on the same thread. Console commands should submit work to that thread and wait for the result.
- The generated WAMR component has a possible platform-level fix (`os_self_thread` → FreeRTOS task handle), but that belongs either upstream, in a managed-component patch workflow, or in a deliberately vendored component, not as an untracked local edit.

### What was tricky to build

- The key distinction is not only “pthread vs no pthread”; it is also ownership of a QuickJS context. QuickJS is not meant to be called concurrently, so a per-call pthread without serialization could introduce a different bug. A long-lived owner thread plus queue is the safer structure.

### What warrants a second pair of eyes

- Whether to eventually vendor or patch the WAMR ESP-IDF platform shim like 0079 does, versus keeping the application-level owner-thread design only.
- Whether WAMR's `WASM_ENABLE_SHARED_MEMORY=1` is unintentionally enabled by Kconfig despite `CONFIG_WAMR_ENABLE_SHARED_MEMORY=n` in defaults; this is not the immediate crash, but it is visible in the compile command.

### What should be done in the future

- Implement the long-lived owner pthread in `wasm_runner.cpp` and keep all `wasm_runtime_call_wasm` calls on that thread.
- Build, flash, and run `js eval "print(1+2)"` on `/dev/ttyACM0` with one tmux monitor owner.

### Code review instructions

- Start with the diff from 0100's `main/wasm_runner.cpp`; verify that `wasm_runtime_call_wasm` is no longer reachable directly from `app_main` or `js_command.cpp`.
- Compare against 0079's `RunEmbeddedWasmModuleOnWorkerThread()` and `RunEmbeddedWasmModuleWorkerEntry()`.

### Technical details

- 0079 worker reference: `0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp` lines around `RunEmbeddedWasmModuleWorkerEntry()` and `RunEmbeddedWasmModuleOnWorkerThread()`.
- 0079 WAMR shim reference: `0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c`, where `os_self_thread()` returns `xTaskGetCurrentTaskHandle()`.
- 0100 active component: `0100-esp32-p4-quickjs-wasm/managed_components/espressif__wasm-micro-runtime`, compile command contains `-DWASM_ENABLE_THREAD_MGR=1`.

## Step 13: Implement a long-lived QuickJS/WAMR owner pthread and build it

Implemented the app-level fix selected in Step 12. Instead of calling WAMR from `app_main` and from the console command task, 0100 now starts one pthread that owns the QuickJS/WAMR session for the firmware lifetime. That thread initialises WAMR's thread environment, loads the copied wasm blob, instantiates the module, runs `qjs_init`, and then services eval requests from a FreeRTOS queue.

This is a deliberate single-owner design: it fixes the `pthread_self` assertion path while also avoiding concurrent access to the QuickJS runtime. The console command remains synchronous from the user's perspective, but it no longer calls `wasm_runtime_call_wasm` directly.

### Prompt Context

**User prompt (verbatim):** (same as Step 12)

**Assistant interpretation:** Apply the investigated fix, validate it at least through an ESP-IDF build, and commit an implementation checkpoint before moving to hardware.

**Inferred user intent:** Move from crash diagnosis to a concrete firmware change while keeping the ticket diary and git history reviewable.

### What I did

- Rewrote `0100-esp32-p4-quickjs-wasm/main/wasm_runner.cpp` around a long-lived pthread owner.
- Added a `WorkerThreadEntry()` that calls `wasm_runtime_init_thread_env()`, performs the old load/instantiate/`qjs_init` sequence, signals `app_main` through a binary semaphore, then waits for eval requests.
- Added a FreeRTOS queue of `EvalRequest *` values. `wasm_runner_eval()` allocates a one-shot completion semaphore on the caller side, queues the request, waits, then returns the result.
- Kept the existing PSRAM wasm-copy fix from Step 11.
- Updated `wasm_runner.h` comments to state the new ownership model.
- Built with `source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build`.

### Why

- WAMR's `wasm_runtime_call_wasm` path needs a valid pthread identity under the current ESP-IDF WAMR component build.
- QuickJS should not be called concurrently from arbitrary tasks. A single owner thread makes the rule explicit.

### What worked

- `idf.py build` passes after the runner-thread rewrite.
- App size remains within the custom 4 MB factory partition: `0x1bec50` bytes, with `0x2413b0` bytes free.

### What didn't work

- No hardware validation yet in this step. The next step is flash/monitor/probe on `/dev/ttyACM0`.

### What I learned

- The cleanest fix is not to make every caller a pthread; it is to remove all direct WAMR calls from those callers. The runner now acts as a serialized RPC boundary into the QuickJS/WAMR session.

### What was tricky to build

- The request queue passes pointers to stack-owned `EvalRequest` structures. This is safe only because `wasm_runner_eval()` blocks until the worker signals completion; future async eval would need heap-owned requests or a request pool.
- The worker thread is detached and intentionally long-lived. Cleanup is not implemented because the firmware owns one QuickJS session for process lifetime.

### What warrants a second pair of eyes

- The worker stack is currently `64 * 1024` bytes. Confirm on hardware that it is enough for WAMR call boundaries plus host native callbacks; the wasm guest stack is separate (`32 * 1024`).
- `pthread_detach()` return value is not currently checked; if ESP-IDF exposes an error here, it should be logged.

### What should be done in the future

- Flash and check whether the `pthread_self` assertion is gone.
- If `qjs_init` succeeds, run `js eval "print(1+2)"` and confirm `3`.
- If the firmware boots but `qjs_init` is slow, measure the latency before changing architecture.

### Code review instructions

- Review `wasm_runner.cpp` top-down: globals, `InitSessionOnWorkerThread()`, `EvalOnWorkerThread()`, `WorkerThreadEntry()`, `wasm_runner_init()`, and `wasm_runner_eval()`.
- Verify that `app_main.cpp` and `js_command.cpp` still call only `wasm_runner_init()` / `wasm_runner_eval()` and never call WAMR directly.

### Technical details

- Build command: `cd 0100-esp32-p4-quickjs-wasm && source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build`.
- Build output: `Project build complete`; binary size `0x1bec50`; factory partition size `0x400000`.

