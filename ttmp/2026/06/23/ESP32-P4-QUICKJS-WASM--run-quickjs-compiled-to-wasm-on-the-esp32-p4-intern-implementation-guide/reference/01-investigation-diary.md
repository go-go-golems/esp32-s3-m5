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
