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

## Step 4: Flash 0101 and verify native QuickJS on the ESP32-P4

Flashed the new 0101 firmware to the same ESP32-P4 hardware used for the 0100 WAMR bring-up. The boot-time smoke passed: native QuickJS created a runtime/context, installed the first globals, evaluated `print(1+2)`, and ran the 10k loop benchmark without crashing.

This is the first on-device proof that the native/raw QuickJS path works on ESP32-P4 and removes the WAMR startup/runtime overhead from the first milestone.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Continue executing the task list after the buildable checkpoint by validating the smoke firmware on real hardware.

**Inferred user intent:** Replace paper design claims with device evidence and keep a precise validation trail for interns and future agents.

### What I did

- Ensured stale tmux monitor sessions were killed before using `/dev/ttyACM0`.
- Checked there were no port holders before flash with `lsof /dev/ttyACM0`.
- Ran `idf.py -p /dev/ttyACM0 flash` from `0101-esp32-p4-native-quickjs`.
- Started `idf.py -p /dev/ttyACM0 monitor` inside tmux session `qjs0101`, captured the boot output, and killed the tmux session afterward to free the serial port.
- Checked off T2.5 in `tasks.md`.

### Why

- A successful compile is not enough for this firmware path. Native QuickJS needed to prove it can initialize and execute JavaScript under the real ESP32-P4 memory/PSRAM/runtime conditions.
- The serial single-owner discipline avoids the false failures seen during earlier ESP32-S3/ESP32-P4 monitor work.

### What worked

- Flash succeeded with `idf.py` using `/dev/ttyACM0`.
- The board booted ESP-IDF v5.4.2 on chip revision v1.3.
- PSRAM was detected and tested successfully: 32 MB at 200 MHz.
- Native QuickJS initialized in `6 ms`.
- `print(1+2)` printed `3` and returned `native eval ok`.
- The 10k loop smoke printed `sum10k=11,s=49995000`, where `11` is the measured JavaScript-side elapsed milliseconds for the loop in the current print formatting.
- Heap snapshots were stable:
  - before: `internal=594247 8bit=34145995 psram=33551748`
  - after: `internal=594071 8bit=34145819 psram=33551748`

### What didn't work

- Running `idf.py monitor` directly through the non-interactive harness failed because IDF monitor requires a TTY:

```text
Error: Monitor requires standard input to be attached to TTY. Try using a different terminal.
idf_monitor failed with exit code 1
```

- The workaround was to run the monitor inside tmux, capture the pane, and kill the tmux session afterward.

### What I learned

- The native engine startup is dramatically faster than the 0100 QuickJS-Wasm path's `qjs_init` time (~6 ms native versus ~2.7 s WAMR/QuickJS-Wasm startup in the earlier measurement).
- The first native smoke app does not need a pthread owner yet because all work runs synchronously in `app_main`; the owner-task design becomes necessary when adding console/runtime reuse in T3/T4.

### What was tricky to build

- The monitor capture needed an interactive terminal. Using tmux preserved the user's requested `idf.py monitor` workflow while still allowing scripted capture. The session was explicitly killed after capture so `/dev/ttyACM0` did not remain held.

### What warrants a second pair of eyes

- The smoke `print` function currently concatenates arguments without separators, which makes benchmark output compact but ambiguous. Before productizing the console service, decide whether firmware `print` should match QuickJS CLI spacing behavior more closely.
- The app version showed `55eb024-dirty` because the broader repository had unrelated dirty state; do not interpret that as a firmware source change after the commit unless the focused diff says so.

### What should be done in the future

- Build the reusable `qjs_service` owner-task layer and move this one-shot runtime into that service.
- Add a stable `js bench` command later so native and WAMR timings are measured with the same scripts and formatting.

### Code review instructions

- Start from `0101-esp32-p4-native-quickjs/main/app_main.cpp` and verify cleanup paths for `JS_FreeValue`, `JS_FreeContext`, and `JS_FreeRuntime`.
- Reproduce with `cd 0101-esp32-p4-native-quickjs && source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py -p /dev/ttyACM0 flash`, then monitor in tmux.

### Technical details

- Flash command: `idf.py -p /dev/ttyACM0 flash`.
- Monitor command used inside tmux: `idf.py -p /dev/ttyACM0 monitor`.
- Verified output:

```text
I (1530) 0101_qjs: 0101 ESP32-P4 native QuickJS smoke (ticket ESP32-P4-NATIVE-QUICKJS)
I (1540) 0101_qjs: heap before: internal=594247 8bit=34145995 psram=33551748
I (1560) 0101_qjs: native QuickJS ready in 6 ms
I (1560) 0101_qjs: native eval: print(1+2)
3
I (1560) 0101_qjs: native eval ok
I (1560) 0101_qjs: native bench: sum10k
sum10k=11,s=49995000
I (1580) 0101_qjs: heap after: internal=594071 8bit=34145819 psram=33551748
I (1580) main_task: Returned from app_main()
```

## Step 5: Add reusable `qjs_service` and verify service-backed native QuickJS on hardware

Replaced the one-shot boot runtime with a reusable full-QuickJS service component. The new service owns one `JSRuntime*` and `JSContext*` on a FreeRTOS owner task, accepts eval/job/reset/status messages through an internal queue, captures `print()` output, formats exceptions, exposes status counters, and supports a deadline interrupt handler.

After building the service-backed 0101 firmware, I flashed it to the ESP32-P4 and verified that the owner-task model works on device. The firmware initializes the service, evaluates a smoke expression, runs a small loop benchmark, formats a thrown exception, and resets the runtime successfully.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Continue executing the implementation checklist by turning the minimal native runtime into the reusable service layer requested by the design.

**Inferred user intent:** Produce a maintainable firmware architecture that interns can extend with console commands and board bindings instead of leaving QuickJS embedded directly in `app_main`.

### What I did

- Added `components/qjs_service/include/qjs_service.h`.
- Added `components/qjs_service/qjs_service.cpp`.
- Added `components/qjs_service/CMakeLists.txt`.
- Added `components/qjs_service` to `0101-esp32-p4-native-quickjs/CMakeLists.txt` `EXTRA_COMPONENT_DIRS`.
- Rewrote `0101-esp32-p4-native-quickjs/main/app_main.cpp` to start `qjs_service`, call `qjs_service_eval`, call `qjs_service_get_status`, call `qjs_service_reset`, and stop the service.
- Updated `0101-esp32-p4-native-quickjs/README.md` with service-smoke build/flash/monitor expectations.
- Ran `idf.py build` successfully.
- Flashed with `idf.py -p /dev/ttyACM0 flash`.
- Captured `idf.py -p /dev/ttyACM0 monitor` output inside tmux and killed the tmux session afterward.

### Why

- Full QuickJS mutable state should be serialized behind one owner task before adding console commands. This mirrors the working 0100 WAMR owner-thread lesson but uses normal FreeRTOS task ownership instead of pthread/WAMR execution requirements.
- A reusable service component keeps future display, keyboard, GPIO, timer, and console code from directly manipulating `JSRuntime*` and `JSContext*`.

### What worked

- `idf.py build` passes for the service-backed firmware.
- Service firmware binary size is `0xb84d0`; the 4 MB app partition still has 82% free.
- Hardware flash succeeded on `/dev/ttyACM0`.
- Runtime init through the service task completed in `6 ms`.
- `print(1+2)` produced captured output `3` with `ok=1` and `timed_out=0`.
- `sum10k` completed in `14 ms` and printed `sum10k=14,s=49995000`.
- `throw new Error('boom')` returned `ok=0` and formatted `Error: boom`.
- `qjs_service_reset` returned `ESP_OK` and status showed `resets=1`.

### What didn't work

- First service build failed because the 0101 project-level `EXTRA_COMPONENT_DIRS` only exposed `components/quickjs_native`:

```text
Failed to resolve component 'qjs_service' required by component 'main': unknown name.
```

- Fix: add `"${CMAKE_CURRENT_LIST_DIR}/../components/qjs_service"` to `0101-esp32-p4-native-quickjs/CMakeLists.txt`.

### What I learned

- The owner-task service model works cleanly with native QuickJS; unlike WAMR, it does not need a pthread-specific runtime call context.
- `JS_ComputeMemoryUsage` is cheap enough for status snapshots in the smoke path.
- Native service startup remains around `6 ms`, so the service abstraction does not materially change startup compared with the one-shot smoke.

### What was tricky to build

- Capturing `print()` output required using `JS_SetContextOpaque(ctx, service)` and a service-local `std::string* capture` pointer. The print callback appends to the capture buffer during `qjs_service_eval`; if no capture is active, it falls back to stdout.
- Stop/reset ownership is important. Runtime destruction happens on the owner task for reset/stop so `JSContext` and `JSRuntime` are not freed concurrently with an eval.
- Deadline support is implemented with `JS_SetInterruptHandler`, but the infinite-loop timeout behavior has not yet been hardware-validated. That remains a Phase 5 validation item.

### What warrants a second pair of eyes

- Review the service stop path: it sends a stop message, the owner task destroys QuickJS state and deletes itself, then the caller deletes the queue and service object.
- Review result ownership: `qjs_eval_result_t.output` and `.error` are heap-allocated and must always be freed with `qjs_eval_result_free`.
- Review `qjs_service_post`: asynchronous jobs are heap-owned and cannot safely reference stack-owned user data unless the caller guarantees lifetime.

### What should be done in the future

- Add console commands (`js status`, `js eval`, `js reset`, `js gc`, `js bench`) on top of the service.
- Hardware-test timeout behavior with `while(true){}`.
- Add repeated-eval stress and memory high-water measurements.

### Code review instructions

- Start with `components/qjs_service/include/qjs_service.h` to understand the public API contract.
- Then review `components/qjs_service/qjs_service.cpp`, especially message handling, runtime creation/destruction, eval result formatting, print capture, and interrupt deadlines.
- Finally review `0101-esp32-p4-native-quickjs/main/app_main.cpp` for the temporary service smoke harness.
- Validate with `cd 0101-esp32-p4-native-quickjs && source /home/manuel/esp/esp-idf-5.4.2/export.sh && idf.py build` and then `idf.py -p /dev/ttyACM0 flash` plus tmux monitor capture.

### Technical details

- Build result: `0101-esp32-p4-native-quickjs.bin binary size 0xb84d0 bytes. Smallest app partition is 0x400000 bytes. 0x347b30 bytes (82%) free.`
- Verified service output:

```text
I (1555) qjs_service: task start name=qjs0101 prio=8 core=0
I (1565) qjs_service: runtime init status=ESP_OK elapsed=6 ms
I (1565) 0101_qjs: status after-start: ready=1 busy=0 evals=0 resets=0 last=0ms qjs_used=49760 qjs_malloc=360 atoms=518 heap8=33980391 psram=33551748
I (1575) 0101_qjs: eval boot-smoke: print(1+2)
I (1585) 0101_qjs: eval boot-smoke result: ok=1 timed_out=0 elapsed=2ms
3
I (1615) 0101_qjs: eval sum10k result: ok=1 timed_out=0 elapsed=14ms
sum10k=14,s=49995000
I (1615) 0101_qjs: eval exception result: ok=0 timed_out=0 elapsed=0ms
E (1625) 0101_qjs: eval exception exception: Error: boom
I (1645) 0101_qjs: reset: ESP_OK
I (1645) 0101_qjs: status after-reset: ready=1 busy=0 evals=3 resets=1 last=0ms qjs_used=49760 qjs_malloc=360 atoms=518 heap8=33980391 psram=33551748
```

- Completed tasks: T3.1, T3.2, T3.3, T3.4, T3.5, T3.6, T5.1, T5.2, T5.3.

## Step 6: Add interactive `js` console commands and validate timeout/bench behavior

Converted 0101 from a boot-only service smoke into an interactive UART0 console firmware. The firmware now starts `qjs_service`, registers ESP console commands, and leaves a persistent `0101>` prompt for `js status`, `js eval`, `js reset`, `js gc`, and `js bench`.

This step also found and fixed two device-only correctness issues. The first `js bench` implementation reused top-level `let` names across evals and failed with redeclaration errors. After wrapping benchmarks in IIFEs, recursive `fib(20)` exposed that the 12 KiB QuickJS owner-task stack was too small, so the owner stack was raised to 32 KiB. The console eval timeout was also reduced to 1000 ms so infinite loops interrupt before the task watchdog complains.

### Prompt Context

**User prompt (verbatim):** (same as Step 2)

**Assistant interpretation:** Continue through the task list by replacing smoke-only validation with the requested interactive console UX.

**Inferred user intent:** Make the native QuickJS firmware usable from the serial console and validate the intern-facing workflow on real hardware.

### What I did

- Added `0101-esp32-p4-native-quickjs/main/js_command.h`.
- Added `0101-esp32-p4-native-quickjs/main/js_command.cpp`.
- Rewrote `0101-esp32-p4-native-quickjs/main/app_main.cpp` to start `qjs_service`, register console commands, and start the UART REPL.
- Updated `0101-esp32-p4-native-quickjs/main/CMakeLists.txt` to build `js_command.cpp` and require `console`.
- Updated `0101-esp32-p4-native-quickjs/README.md` with command list, build/flash/monitor instructions, expected output, and validation notes.
- Updated the design guide benchmark table with measured native 0101 values.
- Built with `idf.py build`, flashed with `idf.py -p /dev/ttyACM0 flash`, and validated commands through `idf.py monitor` in tmux.

### Why

- The implementation guide's service architecture is only useful to interns once it has a concrete operator interface.
- Console commands provide the fastest validation loop for eval, reset, exception handling, timeout handling, and performance checks.

### What worked

- `idf.py build` passes; final console binary size is `0xc84f0`, leaving about 80% of the 4 MB app partition free.
- `js status` reports service and heap state.
- `js eval "print(1+2)"` prints `3`.
- `js eval "throw new Error('boom')"` reports `Error: boom`.
- `js reset` returns `ESP_OK`.
- `js gc` completes successfully.
- `js bench` now runs 10k loop, 100k loop, and `fib(20)`.
- `js eval "while(true){}"` interrupts at about 1000 ms with `timed_out=1` and `InternalError: interrupted`.

### What didn't work

- First `js bench` attempt failed because benchmark snippets used top-level lexical declarations:

```text
[bench-100k] ok=0 timed_out=0 elapsed=0ms
error: SyntaxError: redeclaration of 't'
[bench-fib20] ok=0 timed_out=0 elapsed=0ms
error: SyntaxError: redeclaration of 't'
```

- Fix: wrap each benchmark snippet in an IIFE.
- After that fix, `fib(20)` crashed with a FreeRTOS stack protection fault when the qjs owner task stack was 12 KiB:

```text
Guru Meditation Error: Core  0 panic'ed (Stack protection fault).
Detected in task "qjs0101" at 0x40020e30
JS_CallInternal ... quickjs.c:17749
Stack bounds: 0x4ff21044 - 0x4ff24040
```

- Fix: raise `cfg.task_stack_words` in `app_main.cpp` from `12288` to `32768`.
- A 5000 ms console eval timeout allowed task watchdog warnings during `while(true){}` before QuickJS returned:

```text
E (...) task_wdt: Task watchdog got triggered.
CPU 0: qjs0101
```

- Fix: reduce console default eval timeout to `1000` ms.

### What I learned

- QuickJS's recursive execution uses enough native C stack that the owner task stack must be sized for intended workloads, independently of `JS_SetMaxStackSize`.
- Top-level lexical declarations persist across evals in the same context; benchmarks and console examples should use IIFEs or `var` if repeated execution is expected.
- QuickJS's interrupt handler works for infinite loops on ESP32-P4, but the timeout must be shorter than the FreeRTOS task watchdog threshold unless the service is enhanced to feed/yield safely.

### What was tricky to build

- The console command parser joins remaining argv tokens to support both quoted and unquoted source forms. This mirrors the 0100 command UX while keeping source length bounded to 2048 bytes.
- `js bench` must avoid polluting the global lexical environment. Wrapping each benchmark in `(()=>{ ... })()` fixed repeated invocation without resetting the context.
- Timeout validation had to distinguish a successful QuickJS interrupt from a system-level task watchdog warning. Reducing the timeout to 1000 ms produced clean `timed_out=1` behavior.

### What warrants a second pair of eyes

- Review whether `js eval` should return shell error status for JavaScript exceptions. It currently returns non-zero, which makes failures visible but prints ESP console's `Command returned non-zero error code` line.
- Review whether 32 KiB is the right default owner-task stack or whether the service config should document stack sizing by workload more explicitly.
- Review if console timeout should become an argument, for example `js eval --timeout 5000 <source>`.

### What should be done in the future

- Add repeated-eval stress and memory high-water tests.
- Add reset-specific validation that defines a global, resets, and confirms the global disappears.
- Add allocation benchmark coverage if T5.7 is to be marked fully complete.

### Code review instructions

- Review `0101-esp32-p4-native-quickjs/main/js_command.cpp` for command parsing, result printing, benchmark snippets, and timeout behavior.
- Review `0101-esp32-p4-native-quickjs/main/app_main.cpp` for service stack size and REPL startup.
- Validate with `idf.py build`, `idf.py -p /dev/ttyACM0 flash`, then tmux monitor commands: `js status`, `js eval "print(1+2)"`, `js reset`, `js gc`, `js bench`, and `js eval "while(true){}"`.

### Technical details

- Final command validation output:

```text
0101>  js status
ready=1 busy=0 evals=0 resets=0 last_eval_ms=0
limits: memory=2097152 stack=65536
quickjs: used=49760 malloc=360 atoms=518
esp_heap: internal=398191 8bit=33947971 psram=33549780
0101>  js eval "print(1+2)"
[console-eval] ok=1 timed_out=0 elapsed=2ms
3
0101>  js eval "throw new Error('boom')"
[console-eval] ok=0 timed_out=0 elapsed=0ms
error: Error: boom
0101>  js reset
reset: ESP_OK
0101>  js gc
[gc] ok=1 timed_out=0 elapsed=1ms
0101>  js bench
[bench-10k] ok=1 timed_out=0 elapsed=13ms
sum10k=11,s=49995000
[bench-100k] ok=1 timed_out=0 elapsed=133ms
sum100k=133,s=4999950000
[bench-fib20] ok=1 timed_out=0 elapsed=32ms
fib20=6765,ms=31
0101>  js eval "while(true){}"
[console-eval] ok=0 timed_out=1 elapsed=1000ms
error: InternalError: interrupted
```

- Completed tasks: T4.1, T4.2, T4.3, T4.4, T4.5, T4.6, T5.4.
