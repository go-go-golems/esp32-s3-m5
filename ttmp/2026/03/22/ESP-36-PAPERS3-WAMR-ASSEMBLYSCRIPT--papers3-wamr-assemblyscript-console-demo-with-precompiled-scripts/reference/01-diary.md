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
Summary: Chronological diary for the ticket creation and architecture investigation that produced the PaperS3 WAMR AssemblyScript design guide.
LastUpdated: 2026-03-22T10:22:38.277717198-04:00
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

## Related

- `../index.md`
- `../tasks.md`
- `../changelog.md`
- `../design-doc/01-papers3-wamr-assemblyscript-analysis-design-and-implementation-guide.md`
