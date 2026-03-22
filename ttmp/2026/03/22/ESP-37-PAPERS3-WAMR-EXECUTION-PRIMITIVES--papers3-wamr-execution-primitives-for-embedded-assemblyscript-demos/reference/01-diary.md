---
Title: Diary
Ticket: ESP-37-PAPERS3-WAMR-EXECUTION-PRIMITIVES
Status: active
Topics:
    - papers3
    - wasm
    - assemblyscript
    - wamr
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp
      Note: Diary step 2 covers the first PaperS3 canvas wrapper
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp
      Note: |-
        Entry point for the placeholder execution path that this ticket replaces
        Diary tracks replacement of the placeholder console execution path
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: Diary step 2 covers the host NativeSymbol registration layer
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: Diary step 2 covers the module execution lifecycle and cleanup
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: |-
        Existing runtime init path used as the host-API registration anchor
        Diary tracks runtime and host-API integration decisions
    - Path: 0079-papers3-wamr-assemblyscript-console/wasm-src/shared/host.ts
      Note: |-
        Guest import declarations that drive the host ABI design
        Diary tracks guest ABI decisions against the actual import declarations
ExternalSources: []
Summary: Detailed implementation diary for the PaperS3 WAMR execution-primitives work.
LastUpdated: 2026-03-22T11:19:54.294375557-04:00
WhatFor: Record the implementation sequence, design tradeoffs, failures, and validation steps for the `0079` WAMR execution path.
WhenToUse: Read this when continuing implementation, reviewing changes, or reconstructing why a specific host-ABI or runner decision was made.
---



# Diary

## Goal

This diary captures the execution-primitives half of the `0079` PaperS3 WAMR project. Its job is to make continuation and review easy: every meaningful implementation slice should explain what changed, why it was done that way, what failed, what remains risky, and how to validate the result.

## Step 1: Create the Execution-Primitives Ticket and Guide

The first step in this ticket was to separate "getting WAMR to exist in firmware" from "making a real guest program run." That boundary matters. The first ticket already proved the runtime can initialize and that embedded `.wasm` assets can be discovered. This second ticket isolates the next engineering problem: define the host ABI, implement the drawing primitives, wire the module-execution lifecycle, and verify that `wasm run` works on actual hardware.

I created a dedicated ticket, wrote a new design guide aimed at a new intern, and turned the work into explicit tasks that can be completed one slice at a time. The guiding principle is to keep the first runnable system small and inspectable rather than prematurely abstracting it into a generic sandbox framework.

### Prompt Context

**User prompt (verbatim):**

> Create a second ticket about implemening the execution primitives required for your program, create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and detailed. Store in the ticket and the nupload to remarkable.
>
> Then add detailed list of tasks and work on them one by one, commititng as you go and keeping a etaile diary

**Assistant interpretation:** Create a new docmgr ticket dedicated to the `0079` execution layer, write an intern-facing design guide, upload it to reMarkable, define detailed tasks, and then implement those tasks with incremental commits and diary updates.

**Inferred user intent:** Split the Wasm runner work into a focused, reviewable project with enough documentation that another engineer can continue it safely.

### What I did

- Created the new ticket workspace `ESP-37-PAPERS3-WAMR-EXECUTION-PRIMITIVES`.
- Added a detailed design-guide document and this diary document.
- Added a task list that breaks the work into canvas, host API, runner, console wiring, and hardware validation.
- Anchored the ticket to the current `0079` files that define the runtime, registry, and guest import contract.

### Why

- The first ticket was about architecture and project bring-up.
- This ticket needs to be implementation-focused and should not dilute its scope with project-scaffold history that is already documented elsewhere.

### What worked

- The existing `0079` codebase gives this ticket a clean seam to target: `wasm_command.cpp` still contains a placeholder `run` path, while `wasm_runtime_service.cpp` and `wasm_module_registry.cpp` already provide the runtime and module inventory needed by the future runner.

### What didn't work

- N/A

### What I learned

- The best structure for this ticket is not "generic Wasm runtime research." It is "replace one placeholder command path with a concrete, traceable runner and host ABI."

### What was tricky to build

- The tricky part at this stage was scoping, not coding. The repo now has two closely related tickets on the same project. The second ticket had to complement the first one without copying it wholesale or losing the causal chain between them.

### What warrants a second pair of eyes

- Whether the current task breakdown is the right level of granularity for future diary/commit hygiene.
- Whether any additional runtime-safety instrumentation should be treated as part of the first execution slice rather than a follow-on hardening slice.

### What should be done in the future

- Implement Task 1 and update this diary immediately after the first code commit.

### Code review instructions

- Start with the new design guide and task list in this ticket.
- Compare the ticket scope against the current placeholder behavior in `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp`.

### Technical details

- Current guest imports are declared in `0079-papers3-wamr-assemblyscript-console/wasm-src/shared/host.ts`.
- Current runtime entry point is `0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp`.
- Current embedded demo registry is `0079-papers3-wamr-assemblyscript-console/main/wasm_module_registry.cpp`.

## Step 2: Add the First Execution-Primitives Slice in `0079`

This step turned the design into working firmware code. I added three missing runtime components: a small PaperS3 canvas wrapper around `M5.Display`, a WAMR host-API registration layer that satisfies the imports declared in `host.ts`, and a module runner that loads an embedded wasm binary, instantiates it, looks up the export, executes it, and cleans up. I also replaced the `wasm run` placeholder so the console now attempts a real execution path.

The important design choice in this step was to keep everything narrow and explicit. The canvas wrapper only supports the operations already required by the guest programs. The host ABI stays integer-only. The module runner creates fresh WAMR objects per invocation and reports failure stage and message instead of hiding all failure causes behind one generic "run failed" string.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Start implementing the execution-primitives tasks in order, commit the work incrementally, and record the details in the diary.

**Inferred user intent:** Make `0079` actually runnable while preserving enough engineering traceability that someone else can review or continue the work safely.

**Commit (code):** `1d6ebf2` — "feat(papers3): add wamr execution primitives"

### What I did

- Added `0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.h` and `0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp`.
- Added `0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.h` and `0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp`.
- Added `0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h` and `0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp`.
- Updated `0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt` to compile the new files.
- Updated `0079-papers3-wamr-assemblyscript-console/main/app_main.cpp` to initialize the canvas and register the host API after WAMR startup.
- Updated `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp` to call the new runner and include host-API status in `wasm status`.
- Built the firmware with:
  - `source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build`

### Why

- The guest-side imports already existed, so the host needed a real import module.
- The display path needed one central place to clamp coordinates, manage frame writes, and map present modes.
- The console command needed a structured runner so failures can be debugged by stage: load, instantiate, lookup, exec-env, or execute.

### What worked

- The first code slice compiled cleanly after one local correction pass.
- The new runner keeps cleanup in one place and returns user-visible execution fields such as `loaded`, `instantiated`, `executed`, and `return_value`.
- The host API registration model is simple: a static `"host"` module that matches the AssemblyScript import declarations exactly.

### What didn't work

- The first build failed in `wasm_module_runner.cpp`.
- Exact errors from the failed build:
  - `'WASM_I32_VAL' was not declared in this scope; did you mean 'WASM_I32'?`
  - `jump to label 'cleanup' ... crosses initialization of 'const uint32_t result_count'`
  - `expected ')' before 'PRId32'`
- Root causes:
  - I used a macro path that was not available in the current C++ compilation context.
  - I declared `param_count` and `result_count` after code paths that jump to `cleanup`.
  - I used `PRId32` before adding the correct `<cinttypes>` include.
- Fixes:
  - replaced the result macro with explicit `wasm_val_t` field assignment
  - moved `param_count` and `result_count` declarations earlier
  - added `<cinttypes>`

### What I learned

- The WAMR C API is straightforward to embed from C++, but the sharp edges are mostly C++ control-flow and initialization rules rather than WAMR itself.
- `wasm_runtime_call_wasm_a(...)` is a practical first choice for capturing the `i32` return value from `run()` without inventing an argument-string layer.

### What was tricky to build

- The subtle part was deciding how much display policy to put into the first canvas wrapper. The guest API has a `present(mode)` call, but PaperS3 update mode really belongs to the frame transaction. For the first slice I used a pragmatic compromise: begin lazily, draw immediately, and treat `present(mode)` as the transaction-finalization hook. That is enough to move forward, but it is worth re-checking on hardware.

### What warrants a second pair of eyes

- Whether the current present-mode handling is the best PaperS3 behavior for repeated runs.
- Whether `wasm_runtime_call_wasm_a(...)` should remain the long-term invocation path or eventually be replaced with an explicit signature/type inspection layer.
- Whether the current stack/heap defaults (`16 KiB` / `32 KiB`) are the right first values for all bundled demos.

### What should be done in the future

- Flash the firmware and verify `wasm run hello-frame` on hardware.
- Record whether repeated runs remain stable and whether the present-mode behavior needs refinement.

### Code review instructions

- Start in `0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp` to see the old placeholder replaced with the runner.
- Then read `0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp` for the lifecycle order.
- Then read `0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp` and `0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp` together to verify that the guest import names and runtime display behavior line up.
- Validate with:
  - `source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build`

### Technical details

- New host imports registered:
  - `host_log_i32`
  - `host_delay_ms`
  - `host_screen_clear`
  - `host_draw_rect`
  - `host_fill_rect`
  - `host_present`
- New first-run memory defaults in the runner:
  - guest stack `16 * 1024`
  - guest heap `32 * 1024`
- New execution result fields:
  - `loaded`
  - `instantiated`
  - `export_found`
  - `exec_env`
  - `executed`
  - `return_value`
  - `error_stage`
  - `error_message`

## Step 3: Hardware Debugging Cycle for the First Real `wasm run`

This debugging cycle started as soon as the first execution-primitives slice was flashed to the PaperS3 and `wasm run hello-frame` was exercised on real hardware. The console-oriented bring-up commands worked immediately: `wasm status`, `wasm list`, and `wasm info hello-frame` all returned sensible values. The failure only appeared once the firmware attempted real module execution and the guest imported a drawing primitive.

The most important lesson from this cycle is that "the runtime initialized" and "the module can execute host work safely on-device" are very different milestones. Multiple distinct bugs showed up one after another, and each one only became visible after the previous one was removed.

### Prompt Context

**User prompt (verbatim):** "Then add detailed list of tasks and work on them one by one, commititng as you go and keeping a etaile diary"

**Assistant interpretation:** Continue the task sequence on hardware, document every debugging step, and keep enough detail that someone else can reconstruct the causal chain later.

**Inferred user intent:** Treat failed hardware validation as part of the deliverable rather than as invisible scratch work.

### Cycle 3.1: First hardware flash confirmed bring-up and exposed an instantiate-time cache panic

The first successful flash/monitor pass showed that the console and runtime came up correctly on-device. The PaperS3 booted, `esp_console` started over USB Serial/JTAG, and `wasm status` reported a ready interpreter runtime. That proved the basic project wiring was sound.

The first real failure happened on `wasm run hello-frame`. At that point WAMR crashed during instantiate/load with a cache-disabled panic in its ESP-IDF memory-mapping path. The visible failure path involved `os_mmap` in the vendored WAMR ESP-IDF platform layer.

#### What I did

- Flashed the firmware through `idf.py -p /dev/ttyACM0 flash monitor` inside tmux.
- Verified:
  - `wasm status`
  - `wasm list`
  - `wasm info hello-frame`
- Ran:
  - `wasm run hello-frame`

#### What happened

- Boot and console bring-up succeeded.
- Metadata commands succeeded.
- Module execution crashed before a usable render result.

#### Hypothesis

- WAMR was allocating or mapping memory in a way that was unsafe for the PaperS3 ESP32-S3 memory/cache arrangement during instantiate.

#### Code changes made during this cycle

- Changed the runtime allocator in [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp) from system allocator mode to pooled runtime mode backed by a dedicated buffer.
- Patched vendored WAMR ESP-IDF platform files so the interpreter-only PaperS3 path would not rely on the dual-bus PSRAM mirror path:
  - [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)
  - [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)

#### Result

- The instantiate-time cache panic was removed.
- A second bug became visible immediately after: WAMR asserted inside `pthread_self()` because `esp_console` is not running on a pthread-created task.

### Cycle 3.2: Fixing the `pthread_self()` assertion with a worker thread created a new display-context crash

Once the memory-mapping crash was gone, the next failure was a hard assertion:

- `assert failed: pthread_self pthread.c:564 (false && "Failed to find current thread ID!")`

The call path showed WAMR trying to set native thread identity during `wasm_runtime_call_wasm_a(...)`. My first response was pragmatic: move guest execution into a pthread-owned worker thread so WAMR would be running in the environment it appeared to expect.

That fixed the `pthread_self()` assertion, but it introduced a new, clearer bug: once the guest invoked `host_screen_clear`, the display call itself crashed with a cache-disabled panic inside M5GFX:

- `Panel_EPD::writeFillRectPreclipped(...)`
- via `PaperCanvasScreenClear(...)`
- via `HostScreenClear(...)`

This showed that moving the whole Wasm execution path into a pthread worker was not a safe solution for the PaperS3 display driver context.

#### What I did

- Updated [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp) so `RunEmbeddedWasmModule(...)` created a pthread worker and joined it.
- Rebuilt and reflashed.
- Re-ran `wasm run hello-frame`.

#### What happened

- The earlier `pthread_self()` assertion disappeared.
- A new panic appeared during the first render primitive:
  - `HostScreenClear(...)`
  - `PaperCanvasScreenClear(...)`
  - `M5.Display.fillScreen(...)`
  - `Panel_EPD::writeFillRectPreclipped(...)`

#### Hypothesis

- WAMR itself was no longer the direct problem at that point.
- The real failure was that M5 PaperS3 display IO was happening from the wrong execution context when invoked from the pthread worker.

#### What I learned

- "Make WAMR happy" and "make hardware IO happy" are separate constraints.
- A worker thread can solve the runtime-thread-identity problem while still violating hardware/display expectations.

### Cycle 3.3: Moving back to the console task fixed the thread assertion, but direct display imports still panic inside the WAMR-native call path

After the worker-thread experiment failed, I reverted it. Instead of pushing the full Wasm execution into a pthread, I patched vendored WAMR so `os_self_thread()` uses the current FreeRTOS task handle on PaperS3:

- [espidf_thread.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c)

That change successfully removed the `pthread_self()` assertion while keeping `wasm run` on the `esp_console` task. However, the next `hello-frame` run still crashed at the same render primitive. The backtrace now clearly showed the path from Wasm interpreter to native host import to display IO:

- `wasm_runtime_call_wasm_a(...)`
- `wasm_runtime_invoke_native(...)`
- `HostScreenClear(...)`
- `PaperCanvasScreenClear(...)`
- `LGFXBase::fillScreen(...)`
- `Panel_EPD::writeFillRectPreclipped(...)`

That result matters because it changes the design direction. The bug is no longer "WAMR cannot run on the console task." The bug is "calling PaperS3 display operations directly from inside a WAMR native import still trips a cache-disabled panic."

#### What I did

- Reverted the pthread-worker execution path in [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp).
- Patched [espidf_thread.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c) so `os_self_thread()` returns the current FreeRTOS task handle for this console-driven execution path.
- Rebuilt and reflashed.
- Re-ran `wasm run hello-frame`.

#### What happened

- The `pthread_self()` assertion was gone.
- The render-time cache panic remained.

#### Updated hypothesis

- The host ABI should not drive `M5.Display` directly from within the WAMR native-import call stack.
- A safer first milestone is to make host imports record drawing commands into a small render queue and then replay those commands after the Wasm call returns to the runner.

### Cycle 3.4: Current debugging step in progress

The current code change in progress is a host-render queue in [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp). The guest-facing imports stay the same, but they no longer touch `M5.Display` directly. Instead they append `clear`, `drawRect`, `fillRect`, and `present` commands into a bounded in-memory queue. The runner will flush that queue only after the Wasm invocation returns.

This is the most promising current direction because it preserves the AssemblyScript ABI while moving real hardware IO out of the interpreter/native-import stack.

#### Current expected validation for the next cycle

- Rebuild firmware.
- Flash to PaperS3.
- Run:
  - `wasm status`
  - `wasm run hello-frame`
- If `hello-frame` succeeds:
  - check `last_run` status
  - repeat the run
  - test `checkerboard` and `bars`
- If it still fails:
  - capture the new backtrace
  - record whether the failure now happens during replay rather than during native import

### Commands used during the debugging cycle

- `source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build`
- `tmux new-session -d -s papers3-0079-exec 'zsh -lic "... idf.py -C ... -p /dev/ttyACM0 flash monitor; exec zsh -li"'`
- `tmux capture-pane -pt papers3-0079-exec:0 -S -220`
- `tmux send-keys -t papers3-0079-exec:0 'wasm status' Enter`
- `tmux send-keys -t papers3-0079-exec:0 'wasm list' Enter`
- `tmux send-keys -t papers3-0079-exec:0 'wasm info hello-frame' Enter`
- `tmux send-keys -t papers3-0079-exec:0 'wasm run hello-frame' Enter`
- `lsof -t /dev/ttyACM0 | xargs -r kill`

### Review guidance for this debugging cycle

- Start with the most recent backtrace and confirm where the crash now happens.
- Compare the three failure phases in order:
  - instantiate-time WAMR memory mapping panic
  - `pthread_self()` assertion
  - render-time panic in `HostScreenClear(...)`
- Check whether each code change removed exactly one problem or accidentally masked two.
- Verify that the next patch keeps the guest ABI stable while changing only the host-side execution strategy.

### Cycle 3.5: Queueing display commands proved the crash is not specific to one import callback

The next debugging step was to stop touching the display directly from host import callbacks. I changed the host API so `host_screen_clear`, `host_draw_rect`, `host_fill_rect`, and `host_present` no longer called `PaperCanvas...` immediately. Instead, they appended bounded commands into a host-side queue, and the runner flushed that queue after the Wasm call.

That changed the failure again in a useful way. The first queued-render build no longer crashed in `HostScreenClear(...)`. Instead it crashed in `HostLogI32(...)` because logging was still calling `ESP_LOGI(...)` from inside a native import callback. That was a strong signal that the real rule was broader than "don’t touch the display in a native import." The safer rule was "don’t do any complex host side effect from inside a native import on this PaperS3 path."

#### What I did

- Added a bounded host-command queue in [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp).
- Updated the runner in [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp) so it reset the queue before execution and flushed it after the Wasm call.
- Rebuilt, reflashed, and re-ran `wasm run hello-frame`.

#### What happened

- The render callback itself stopped being the first crash site.
- The new failure site became:
  - `HostLogI32(...)`
  - `esp_log_write(...)`
  - `vprintf(...)`

#### Updated hypothesis

- The remaining unsafe behavior is not limited to PaperS3 display IO.
- The current environment appears hostile to any nontrivial host-side effect performed directly inside the WAMR native-import callback path.

### Cycle 3.6: Queueing `log` and `delay` moved the failure to queued replay, but replay still panics

After the logging backtrace, I widened the queue so it covered every host side effect currently used by the demo path:

- `host_log_i32`
- `host_delay_ms`
- `host_screen_clear`
- `host_draw_rect`
- `host_fill_rect`
- `host_present`

This achieved another important reduction. The next backtrace no longer showed the crash occurring inside a native import callback at all. It happened during `FlushWasmHostFrame(...)`, after the Wasm call had returned. That proved the queueing strategy successfully moved side effects out of the WAMR native-import path.

I then tested one more refinement: move the queue replay to after full WAMR teardown, so the queued commands would execute only after `exec_env`, `module_inst`, and `wasm_module` were destroyed. That still did not eliminate the panic. The crash remains in `PaperCanvasScreenClear(...)` during queued replay.

#### What I did

- Expanded the host queue in [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp) to include log and delay commands.
- Moved the queued replay in [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp) so it occurs after WAMR object teardown.
- Rebuilt and reflashed after each change.
- Re-ran `wasm run hello-frame` on hardware after each iteration.

#### What happened

- The crash moved from:
  - direct native import
to:
  - queued replay after Wasm returned
and finally to:
  - queued replay after WAMR teardown
- The current visible failure path is:
  - `FlushWasmHostFrame(...)`
  - `PaperCanvasScreenClear(...)`
  - `LGFXBase::fillScreen(...)`
  - `Panel_EPD::writeFillRectPreclipped(...)`

#### What I learned

- The queueing work was still useful even though it did not solve the final crash.
- It removed two whole classes of uncertainty:
  - the panic is no longer caused by direct logging inside a host import
  - the panic is no longer caused by direct display access inside a host import
- The remaining blocker is now specifically associated with replaying the queued PaperS3 display operations after a WAMR run.

#### Current blocker statement

The `hello-frame` demo still panics on real hardware after Wasm execution completes. The current best understanding is:

- WAMR can initialize.
- Embedded modules can load and execute far enough to emit queued host commands.
- Host side effects can be deferred until after execution and even after WAMR teardown.
- The PaperS3 display path still trips a cache-disabled panic when the queued frame is replayed after the Wasm run.

#### Best next experiments

- Add a non-Wasm control path in firmware that replays the same queued command sequence without invoking WAMR, to separate "display driver issue" from "WAMR left the system in a bad state."
- Try a minimal replay primitive first:
  - skip `fillScreen`
  - replay only one rectangle or one `present`
- Check whether any explicit cache or memory-barrier restoration is required after interpreter execution on this ESP32-S3 path.
- Consider whether the M5 PaperS3 display driver requires running from a different task context than the console REPL task, independent of WAMR.
