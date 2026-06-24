---
Title: Investigation diary
Ticket: ESP32-P4-NATIVE-QUICKJS
Status: active
Topics:
    - esp32p4
    - quickjs
    - javascript
    - firmware
    - microquickjs
    - esp32-p4
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0100-esp32-p4-quickjs-wasm/wasm-src/quickjs/quickjs.h
      Note: Full QuickJS API reference used in the guide
    - Path: components/mqjs_service/mqjs_service.cpp
      Note: Evidence source for service model
    - Path: ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/design/01-native-quickjs-on-esp32-p4-analysis-design-and-implementation-guide.md
      Note: Primary guide written in Step 1
ExternalSources: []
Summary: Chronological diary for the native/raw QuickJS ESP32-P4 firmware ticket.
LastUpdated: 2026-06-23T23:23:20.157487803-04:00
WhatFor: Use to resume the ESP32-P4 native QuickJS design and implementation work.
WhenToUse: Read before implementing 0101 or updating the design guide.
---


# Diary

## Goal

Capture the investigation and design work for a native ESP32-P4 QuickJS firmware that compiles QuickJS directly into ESP-IDF, avoiding the Wasm/WAMR runtime layer used by project 0100.

## Step 1: Create the native QuickJS ticket and write the intern implementation guide

Created a new ticket for the native/raw QuickJS firmware direction. The design assumes the ESP32-P4 has enough resources to run full upstream QuickJS directly and that the existing MicroQuickJS service already provides the right architectural shape: one runtime owner task, a queue-based eval/job API, deadlines, output capture, reset, and status.

The main deliverable is an intern-facing design and implementation guide. It explains why native QuickJS should be faster and simpler than QuickJS-through-Wasm, maps the relevant existing code, proposes components and APIs, includes pseudocode and diagrams, and gives a phased implementation/testing plan.

### Prompt Context

**User prompt (verbatim):** "can we make a new ticket to do a "raw" quickjs firmware, i.e. compile quickjs striaght to the esp32p4 instead of going through wasm. that shoud b emuch faster and probably also easier to build. We already have a service using microquickjs, but the esp32p4 should have enough memory and reosurces that quickjs should be no problem"

**User follow-up (verbatim):** "Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a docmgr ticket for native full QuickJS on ESP32-P4, write a comprehensive intern guide grounded in local code evidence, and upload the guide bundle to reMarkable.

**Inferred user intent:** Turn the successful but slow QuickJS-WASM experiment into a practical next implementation path that should be easier, faster, and more maintainable on ESP32-P4 hardware.

### What I did

- Created ticket `ESP32-P4-NATIVE-QUICKJS` with topics `esp32p4`, `quickjs`, `javascript`, `firmware`, `microquickjs`, and `esp32-p4`.
- Added a primary design doc: `design/01-native-quickjs-on-esp32-p4-analysis-design-and-implementation-guide.md`.
- Added this diary: `reference/01-investigation-diary.md`.
- Gathered evidence from:
  - `components/mqjs_service/include/mqjs_service.h` and `components/mqjs_service/mqjs_service.cpp` for the existing MicroQuickJS service API and owner-task model.
  - `components/mqjs_service/mqjs_vm.cpp` for deadline/interrupt/output-capture patterns.
  - `0067-esp-c3-led-matrix-http/main/mqjs/js_runtime_bridge.cpp` for application-level JS bootstrap and service use.
  - `0100-esp32-p4-quickjs-wasm/wasm-src/build-quickjs-wasm.sh` for the minimal full QuickJS source set and the decision to exclude `quickjs-libc.c`.
  - `0100-esp32-p4-quickjs-wasm/wasm-src/wasm_main.c` for minimal `print`/`millis`/`gpio_write` bindings.
  - `0100-esp32-p4-quickjs-wasm/main/wasm_runner.cpp` for the owner-thread/eval-queue pattern after the WAMR crash fix.
  - `0099-esp32-p4-picocalc-display-keyboard/sdkconfig.defaults` and `main/app_main.c` for ESP32-P4 UART0/PSRAM baseline.
- Wrote the guide with executive summary, current-state analysis, gap analysis, proposed architecture, Mermaid diagrams, API sketches, pseudocode, decision records, phased implementation plan, test strategy, risks, open questions, file reference map, and intern checklist.
- Wrote an actionable task list for implementation phases.

### Why

- The Wasm/WAMR path is proven but slow for compute loops and more complex than necessary for a memory-rich ESP32-P4.
- The repository already contains a useful JavaScript service pattern via MicroQuickJS; the native full QuickJS ticket should reuse that architecture rather than inventing a new runtime ownership model.

### What worked

- The local codebase contains enough prior art to write the guide without speculative architecture. The strongest pattern is the `mqjs_service` queue/task model.
- The full QuickJS source and API references are already available from the 0100 host/Wasm work.

### What didn't work

- No implementation was attempted in this step. This was intentionally a design/research ticket setup and deliverable pass.

### What I learned

- Native full QuickJS should not copy MicroQuickJS's arena API; it needs a runtime/context service wrapper around `JS_NewRuntime`, `JS_SetMemoryLimit`, `JS_SetMaxStackSize`, `JS_SetInterruptHandler`, `JS_NewContext`, and `JS_Eval`.
- The owner-task model remains correct even without WAMR. The reason changes from "avoid WAMR's pthread thread-manager assertion" to "serialize mutable QuickJS context access".

### What was tricky to build

- The guide needed to distinguish three JavaScript engines/paths clearly: MicroQuickJS (`mquickjs`), full QuickJS compiled to Wasm and executed by WAMR (`0100`), and full QuickJS compiled natively into ESP-IDF (this ticket). Keeping those boundaries explicit is necessary so an intern does not mix APIs accidentally.

### What warrants a second pair of eyes

- The proposed decision to create a separate `components/qjs_service` instead of extending `components/mqjs_service` should be reviewed before implementation.
- The proposed initial allocator strategy (default QuickJS allocator plus `JS_SetMemoryLimit`, custom `JS_NewRuntime2` allocator later only if needed) should be validated during first hardware tests.

### What should be done in the future

- Implement Phase 1: `components/quickjs_native` plus `0101-esp32-p4-native-quickjs` minimal smoke firmware.
- Measure native QuickJS startup/eval/loop speed and compare to the 0100 QuickJS-WASM baseline.
- Upload the design bundle to reMarkable and record the destination.

### Code review instructions

- Start with `design/01-native-quickjs-on-esp32-p4-analysis-design-and-implementation-guide.md`.
- Then read `components/mqjs_service/include/mqjs_service.h`, `components/mqjs_service/mqjs_service.cpp`, `0100-esp32-p4-quickjs-wasm/wasm-src/wasm_main.c`, and `0100-esp32-p4-quickjs-wasm/wasm-src/build-quickjs-wasm.sh`.

### Technical details

- New ticket path: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide`.
- Proposed firmware path: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0101-esp32-p4-native-quickjs`.
- Proposed reusable components: `components/quickjs_native` and `components/qjs_service`.

## Step 2: Expand the ticket task list into implementation-grade phases

Replaced the placeholder task list with a detailed, ordered implementation checklist. The tasks now start with native QuickJS vendoring, move through a minimal 0101 smoke firmware, then add the reusable full-QuickJS service, console commands, hardware validation, benchmarking, documentation, reMarkable upload, and optional product features.

This gives the intern a clear work queue and gives us a commit boundary before touching source code.

### Prompt Context

**User prompt (verbatim):** "Createa detailed list of tasks in the ticket, and then work on them one by one, committing at appropriate intervals keeping a detailed diary."

**Assistant interpretation:** Turn the design into an actionable task checklist, then proceed through the implementation incrementally with diary entries and focused commits.

**Inferred user intent:** Avoid a large unstructured coding session; make the native QuickJS firmware work reviewable and resumable.

### What I did

- Rewrote `tasks.md` with task IDs T0.1 through T7.6.
- Marked ticket setup, guide writing, file relations, and task expansion as complete.
- Broke implementation into phases:
  - Phase 1: vendor and compile native QuickJS.
  - Phase 2: create `0101-esp32-p4-native-quickjs` minimal smoke firmware.
  - Phase 3: implement reusable `components/qjs_service`.
  - Phase 4: add console commands.
  - Phase 5: hardware validation and benchmark comparison.
  - Phase 6: documentation, doctor, reMarkable upload, handoff.
  - Optional Phase 7: PicoCalc/display/timer/product features.

### Why

- The native QuickJS work touches third-party source vendoring, ESP-IDF build plumbing, firmware runtime ownership, console UX, and hardware validation. A detailed task list reduces the chance of mixing these into one unreviewable change.

### What worked

- The task list now has clear commit-sized units. T1.1/T1.2 are source/component plumbing; T2.1-T2.4 are firmware skeleton/build; hardware flashing is separated from build work.

### What didn't work

- No implementation was attempted in this step.

### What I learned

- The first source-code checkpoint should be `components/quickjs_native` plus `0101` build integration, not the whole `qjs_service`. This gives early feedback on native QuickJS compiler/linker issues.

### What was tricky to build

- The task list has to keep documentation/upload work visible without delaying implementation. I kept upload in Phase 6 so the guide can be uploaded after the initial docs are validated, while implementation can proceed now.

### What warrants a second pair of eyes

- Whether to keep product features in optional Phase 7 or pull timers into the first service milestone.

### What should be done in the future

- Start T1.1: create `components/quickjs_native` from the known 0100 QuickJS source set.

### Code review instructions

- Review `tasks.md` for ordering and granularity before relying on it as the implementation checklist.

### Technical details

- Task file: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/tasks.md`.

## Step 3: Vendor native QuickJS, add 0101 smoke firmware, and get an ESP32-P4 build

Implemented the first buildable native QuickJS checkpoint. Added a new `components/quickjs_native` component with the upstream QuickJS core engine sources, created `0101-esp32-p4-native-quickjs`, and wrote a minimal boot-time smoke app that creates a native `JSRuntime`/`JSContext`, installs `print` and `millis`, evaluates `print(1+2)`, runs a small 10k loop bench, and frees the runtime.

This step proves the most important feasibility question: full upstream QuickJS can compile directly into ESP-IDF for `esp32p4` without WAMR. The firmware has not been flashed yet in this step, but `idf.py build` now passes.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Start executing the detailed task list one checkpoint at a time, committing after a buildable source milestone.

**Inferred user intent:** Move from design to real firmware while preserving clear review and continuation points.

### What I did

- Created `components/quickjs_native/`.
- Copied the minimal full QuickJS core source set from `0100-esp32-p4-quickjs-wasm/wasm-src/quickjs/`:
  - `quickjs.c`, `quickjs.h`, `quickjs-atom.h`, `quickjs-opcode.h`
  - `cutils.c`, `cutils.h`
  - `dtoa.c`, `dtoa.h`
  - `libregexp.c`, `libregexp.h`, `libregexp-opcode.h`
  - `libunicode.c`, `libunicode.h`, `libunicode-table.h`, `unicode_gen_def.h`
  - `list.h`, `VERSION`, `LICENSE`
- Added `components/quickjs_native/CMakeLists.txt` with the minimal source list, `CONFIG_VERSION`, local warning options, and a forced ESP-IDF compatibility include.
- Added `components/quickjs_native/quickjs_espidf_compat.h` to declare `malloc_usable_size()`.
- Added `components/quickjs_native/README.md` documenting source, included files, excluded `quickjs-libc.c`, and ESP-IDF compatibility notes.
- Added a minimal `0101-esp32-p4-native-quickjs/` project with P4 UART0/PSRAM defaults, a 4 MB app partition, `README.md`, and `main/app_main.cpp`.
- Ran `source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py set-target esp32p4 && idf.py build`.

### Why

- Before building `qjs_service`, we needed to prove that full QuickJS can compile and link natively in ESP-IDF for ESP32-P4.
- A tiny boot-time smoke firmware gives a fast compile/flash target for source-portability issues.

### What worked

- `idf.py build` now passes for `0101-esp32-p4-native-quickjs`.
- Output binary size is `0xb4f00`; with the custom 4 MB app partition, `0x34b100` bytes (82%) remain free.
- Native full QuickJS is much smaller than the 0100 WAMR+embedded-wasm app (`0100` was about `0x1bec50`).

### What didn't work

- First build failed because the initial source copy missed generated/support headers:

```text
fatal error: libregexp-opcode.h: No such file or directory
fatal error: list.h: No such file or directory
fatal error: libunicode-table.h: No such file or directory
```

- Second build failed on ESP-IDF portability/warnings:

```text
implicit declaration of function 'malloc_usable_size'
error: 'struct tm' has no member named 'tm_gmtoff'
error: passing argument ... from incompatible pointer type [-Wincompatible-pointer-types]
```

### What I learned

- The minimal source list in the design guide was incomplete for native vendoring: generated headers such as `libregexp-opcode.h`, `libunicode-table.h`, and support header `list.h` must be included.
- ESP-IDF/newlib does not expose `tm_gmtoff`, so native QuickJS needs a local portability patch for timezone offset calculation.
- ESP-IDF's `-Werror=all` turns upstream QuickJS pointer-type warnings into build failures on this target. Keeping `-Wno-error=incompatible-pointer-types` local to `quickjs_native` is an acceptable third-party-source boundary.

### What was tricky to build

- I avoided broad source edits. The only QuickJS source patch was changing the timezone fallback condition to use the portable `gmtime`/`localtime`/`mktime` path when `ESP_PLATFORM` is defined. The `malloc_usable_size` fix is a forced include shim, not an edit inside QuickJS.

### What warrants a second pair of eyes

- The `quickjs.c` timezone patch should be reviewed as a local vendor patch and possibly carried as a small patch file rather than an inline modified source in future update tooling.
- The incompatible-pointer warnings are still emitted; they are only no longer fatal. Decide later whether to patch them cleanly or keep warning suppression local to the third-party component.

### What should be done in the future

- Flash `0101` to the ESP32-P4 and verify the boot-time native eval output.
- If hardware smoke passes, commit the measurement in the diary and compare native `sum10k` against the 0100 Wasm baseline.

### Code review instructions

- Review `components/quickjs_native/CMakeLists.txt`, `components/quickjs_native/quickjs_espidf_compat.h`, and the single `quickjs.c` timezone conditional patch.
- Review `0101-esp32-p4-native-quickjs/main/app_main.cpp` for correct QuickJS value/context/runtime cleanup.
- Validate with `cd 0101-esp32-p4-native-quickjs && source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build`.

### Technical details

- Build command: `idf.py build` under ESP-IDF 5.4.2.
- Build result: `Project build complete`; binary size `0xb4f00`; app partition size `0x400000`.
- Completed tasks: T1.1, T1.2, T1.3, T1.4, T2.1, T2.2, T2.3, T2.4.
