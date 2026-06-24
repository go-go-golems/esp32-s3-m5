# Tasks

This task list is intentionally implementation-grade. Work it top to bottom. Commit after each checkpoint that leaves the tree buildable or leaves the ticket docs materially clearer.

## Done

- [x] **T0.1 — Create ticket.** Create docmgr ticket `ESP32-P4-NATIVE-QUICKJS` with topics `esp32p4`, `quickjs`, `javascript`, `firmware`, `microquickjs`, `esp32-p4`.
- [x] **T0.2 — Create design doc.** Add `design/01-native-quickjs-on-esp32-p4-analysis-design-and-implementation-guide.md`.
- [x] **T0.3 — Create diary.** Add `reference/01-investigation-diary.md`.
- [x] **T0.4 — Gather evidence.** Inspect local prior art: `components/mqjs_service`, `0067` MicroQuickJS integration, `0100` QuickJS-WASM firmware, and `0099` ESP32-P4 baseline.
- [x] **T0.5 — Write intern guide.** Write the initial analysis/design/implementation guide with diagrams, pseudocode, APIs, file references, risks, phases, and checklist.
- [x] **T0.6 — Relate evidence files.** Relate key source files to the design doc and diary with `docmgr doc relate`.
- [x] **T0.7 — Expand task list.** Replace the placeholder task file with this detailed implementation-grade checklist.

## Phase 1 — Vendor and compile native QuickJS

- [x] **T1.1 — Create `components/quickjs_native`.**
  - Copy the minimal full QuickJS source set from the known 0100 upstream checkout.
  - Include at least: `quickjs.c`, `quickjs.h`, `cutils.c`, `cutils.h`, `dtoa.c`, `dtoa.h`, `libregexp.c`, `libregexp.h`, `libunicode.c`, `libunicode.h`, `quickjs-atom.h`, `quickjs-opcode.h`, `VERSION`, and license/copyright material if present.
  - Do not copy build artifacts or the whole ignored upstream tree.

- [x] **T1.2 — Add `components/quickjs_native/CMakeLists.txt`.**
  - Register the minimal source set.
  - Define `CONFIG_VERSION` from the vendored `VERSION` or a fixed string.
  - Suppress third-party warning noise only on this component.
  - Keep `quickjs-libc.c` excluded for milestone 1.

- [x] **T1.3 — Add `components/quickjs_native/README.md`.**
  - Record upstream QuickJS version, local source path used for vendoring, license note, update procedure, and the source files intentionally excluded.

- [x] **T1.4 — Build-test the component through a minimal firmware.**
  - Component-only build is not enough in ESP-IDF; create `0101` skeleton first if needed.
  - Resolve compile/link errors without changing upstream QuickJS source unless absolutely necessary.

## Phase 2 — Create native ESP32-P4 firmware skeleton (`0101`)

- [x] **T2.1 — Create `0101-esp32-p4-native-quickjs/`.**
  - Add top-level `CMakeLists.txt`, `README.md`, `sdkconfig.defaults`, optional `partitions.csv`, and `main/`.
  - Base target/console/PSRAM defaults on 0099/0100 P4 firmware.

- [x] **T2.2 — Add minimal `main/CMakeLists.txt`.**
  - Depend on `console`, `quickjs_native`, and ESP-IDF components needed for smoke logging.

- [x] **T2.3 — Add minimal native QuickJS smoke `app_main.cpp`.**
  - Create `JSRuntime` and `JSContext`.
  - Set `JS_SetMemoryLimit` and `JS_SetMaxStackSize`.
  - Register a C `print` function.
  - Evaluate `print(1+2)` at boot.
  - Free `JSValue`, `JSContext`, and `JSRuntime` correctly.

- [x] **T2.4 — Build `0101` for `esp32p4`.**
  - Use ESP-IDF 5.4.2.
  - Commit once the firmware builds.

- [x] **T2.5 — Flash first smoke to hardware.**
  - Use `/dev/ttyACM0` single-owner rules.
  - Confirm boot log shows native QuickJS eval result `3`.

## Phase 3 — Implement reusable full QuickJS service (`components/qjs_service`)

- [x] **T3.1 — Create `components/qjs_service/include/qjs_service.h`.**
  - Mirror `mqjs_service` concepts but use full QuickJS terms.
  - Define config, eval result, status, job callback, start/stop/eval/run/post/reset/status/free APIs.

- [x] **T3.2 — Implement service task and queue.**
  - Single owner task owns `JSRuntime*` and `JSContext*`.
  - Other tasks submit eval/job/reset/status messages.
  - Use internal-capability queues/semaphores where appropriate.

- [x] **T3.3 — Implement runtime lifecycle.**
  - `JS_NewRuntime`, `JS_SetMemoryLimit`, `JS_SetMaxStackSize`, `JS_SetInterruptHandler`, `JS_SetCanBlock`, `JS_NewContext`.
  - Install `print`, `millis`, `gc`, and optional `heap` globals.
  - Clean up `JSContext` and `JSRuntime` on reset/stop.

- [x] **T3.4 — Implement eval result formatting.**
  - Capture output from `print`.
  - Convert returned values to strings when useful.
  - Convert exceptions via `JS_GetException` + `JS_ToCString`.
  - Always free `JSValue` and C strings.

- [x] **T3.5 — Implement deadlines.**
  - Store absolute `deadline_us` per eval/job.
  - Interrupt handler returns true after deadline.
  - Report timeout distinctly from JS exception.

- [x] **T3.6 — Implement reset/status.**
  - `js reset` should rebuild runtime/context.
  - Status should report eval count, last eval ms, memory limit, ESP heap state, and QuickJS memory usage if available.

## Phase 4 — Replace `0101` smoke with console-driven service

- [x] **T4.1 — Add `js status`.**
- [x] **T4.2 — Add `js eval <source>`.**
- [x] **T4.3 — Add `js reset`.**
- [x] **T4.4 — Add `js gc`.**
- [x] **T4.5 — Add `js bench`.**
- [x] **T4.6 — Update `0101/README.md` with build/flash/monitor commands and expected output.**

## Phase 5 — Hardware validation and benchmark comparison

- [x] **T5.1 — Flash service firmware to ESP32-P4.**
- [x] **T5.2 — Smoke eval.** Confirm `js eval "print(1+2)"` prints `3`.
- [x] **T5.3 — Exception eval.** Confirm `throw new Error('boom')` reports `Error: boom`.
- [x] **T5.4 — Timeout eval.** Confirm `while(true){}` stops by deadline.
- [x] **T5.5 — Reset eval.** Define a global, reset, confirm global disappears.
- [x] **T5.6 — Memory high-water.** Run repeated eval loop and record heap/QuickJS memory before/after.
- [x] **T5.7 — Benchmark.** Measure startup, tiny eval, 10k loop, 100k loop, recursion, allocation.
- [x] **T5.8 — Compare against 0100.** Update design doc with native vs Wasm numbers.

## Phase 6 — Documentation, upload, and handoff

- [x] **T6.1 — Keep diary after each implementation checkpoint.**
- [x] **T6.2 — Update design doc with implementation outcomes and deviations.**
- [x] **T6.3 — Update changelog after each committed phase.**
- [x] **T6.4 — Run `docmgr doctor --ticket ESP32-P4-NATIVE-QUICKJS --stale-after 30`.**
- [x] **T6.5 — Upload ticket bundle to reMarkable.**
- [x] **T6.6 — Final handoff summary.** Include ticket path, commits, validation status, reMarkable path, and open risks.

## Optional Phase 7 — Product features

- [ ] **T7.1 — Add PicoCalc display bindings from `0099`.**
- [ ] **T7.2 — Add keyboard input bindings from `0099`.**
- [ ] **T7.3 — Add 0067-style timers (`setTimeout`, `clearTimeout`, `every`).**
- [ ] **T7.4 — Add embedded JS examples with `EMBED_TXTFILES`.**
- [ ] **T7.5 — Evaluate PSRAM-first custom allocator using `JS_NewRuntime2`.**
- [ ] **T7.6 — Evaluate bytecode precompilation with `qjsc` if startup becomes important.**
