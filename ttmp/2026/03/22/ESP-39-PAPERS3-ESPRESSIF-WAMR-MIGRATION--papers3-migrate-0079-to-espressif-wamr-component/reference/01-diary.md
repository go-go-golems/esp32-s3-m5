---
Title: Diary
Ticket: ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/dependencies.lock
      Note: Resolved dependency record used to confirm the migration result
    - Path: 0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt
      Note: |-
        Main component alias wiring that must be updated during the migration
        Adds the pthread dependency required for the worker-thread execution experiment
    - Path: 0079-papers3-wamr-assemblyscript-console/main/idf_component.yml
      Note: Dependency manifest being migrated from upstream WAMR to Espressif's package
    - Path: 0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp
      Note: Contains PaperS3 screen clear and frame lifecycle code hit by the decoded backtraces
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp
      Note: Adds the worker-thread console commands used for the A/B hardware experiment
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: Contains queued host-command flush logic that leads into the crashing PaperS3 display path
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: Contains the preflush execution path and exec probes used in the new hardware tests
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp
      Note: Adds clear-only and frame-no-clear controls that narrowed the PaperS3 crash beyond screenClear
    - Path: ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/serial_probe_sequence.py
      Note: New same-boot probe helper used to expose post-WAMR contamination on PaperS3
ExternalSources: []
Summary: Step-by-step diary for the WAMR dependency migration in `0079`.
LastUpdated: 2026-03-22T21:59:00-04:00
WhatFor: Record the migration sequence, build results, and any resolver or alias issues encountered while switching to Espressif's WAMR package.
WhenToUse: Read this before continuing the migration or reviewing how the dependency swap was validated.
---



# Diary

## Step 1: Inspect the current dependency layout

Before editing anything, I checked how `0079` currently pulls in WAMR.

What I found:

- the project does not use a top-level `idf_component.yml`
- the dependency is declared in [main/idf_component.yml](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/idf_component.yml)
- the current package is `bytecodealliance/wasm-micro-runtime`
- it is pulled from git and pinned to `version: main`
- the app component depends on `bytecodealliance__wasm-micro-runtime` in [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)
- the resolved lockfile confirms the upstream git source in [dependencies.lock](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/dependencies.lock)

Why this mattered:

- the migration surface is small
- the main risk is not code churn but package identity and alias correctness

## Step 2: Create the migration ticket and implementation guide

I created `ESP-39` to keep this work distinct from the replay-isolation ticket.

That separation matters because the success condition here is:

- migrate the dependency
- get a build

It is not:

- prove the runtime bug is fixed

That distinction should keep the implementation honest.

## Step 3: Perform the dependency swap

I then made the smallest possible code/build change in `0079`.

Files changed:

- [main/idf_component.yml](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/idf_component.yml)
- [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt)

What changed:

- replaced `bytecodealliance/wasm-micro-runtime` with `espressif/wasm-micro-runtime`
- pinned the dependency to `2.4.0~1`
- replaced the CMake alias `bytecodealliance__wasm-micro-runtime` with `espressif__wasm-micro-runtime`

Why the change was intentionally narrow:

- if the build broke immediately, the cause would most likely be aliasing, manifest syntax, or Kconfig surface mismatch
- if the build succeeded, we would know the package swap itself was mechanically valid before touching runtime logic

## Step 4: Reconfigure and build

I validated the migration with:

- `unset IDF_PYTHON_ENV_PATH IDF_PATH && source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null && idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console reconfigure build`

Important observations from the build output:

- Component Manager detected the manifest change and re-solved dependencies
- it updated [dependencies.lock](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/dependencies.lock)
- it resolved `espressif/wasm-micro-runtime (2.4.0~1)`
- the active component alias in the build graph was `espressif__wasm-micro-runtime`
- the WAMR config symbols consumed by `sdkconfig.defaults` and `wasm_runtime_service.cpp` still compiled cleanly
- the full firmware build completed successfully

Build result:

- success

## Step 5: Check the resolved artifact state

After the successful build, I checked the lockfile and managed component directories.

What changed cleanly:

- [dependencies.lock](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/dependencies.lock) now names `espressif/wasm-micro-runtime`
- the resolved version is `2.4.0~1`
- the source is the Espressif Component Registry service, not the upstream git repository

What remained slightly messy:

- both of these local generated directories exist:
  - `managed_components/bytecodealliance__wasm-micro-runtime`
  - `managed_components/espressif__wasm-micro-runtime`

Interpretation:

- the old upstream directory is a stale local cache artifact
- the actual successful build used the Espressif component alias and lockfile entry
- this is cleanup debt, not evidence that the migration failed

## Step 6: First hardware smoke test after migration

After the build passed, I moved to the first on-device comparison step. The intent was deliberately modest:

- flash the migrated firmware
- confirm it boots cleanly
- run the lowest-risk Wasm probe first
- compare the first failure boundary against the previous upstream integration

This is important because `ESP-39` is not merely "the build compiles." The whole value of switching packages is to observe whether the runtime boundary changes.

### What happened during device bring-up

The first hardware attempt was partially noisy because the device had briefly disappeared from USB enumeration and an old `idf_monitor` process was still attached to `/dev/ttyACM0`. That is a useful reminder in itself:

- on this setup, USB Serial/JTAG state and leftover monitor processes can create misleading interaction failures
- a "serial write timeout" from the monitor is not automatically a firmware regression

Once the stale monitor process was cleared and the device reappeared as:

- `/dev/ttyACM0`
- `USB JTAG/serial debug unit`

the migrated firmware flashed and booted normally.

### Boot observations

The important boot-time lines were:

- `Initializing WAMR runtime (allocator=pool, mode=interp)`
- `WAMR ready (version=2.4.0, interp=yes, aot=no, fast-jit=no, llvm-jit=no)`
- `Registered 6 host symbols for module 'host'`
- `paper>`

This matters because it proves the migration did not merely compile. The Espressif package successfully:

- linked
- initialized at runtime
- exposed the same basic runtime status surface
- registered the host API

So the failure moved deeper into the lifecycle.

## Step 7: Run the lowest-risk Wasm probe and capture the new failure boundary

The first intentional runtime probe was:

- `wasm run-preflush return-42`

This is the right first probe because `return-42` is the smallest execution path available:

- no display imports
- no drawing commands
- no replay complexity
- no dependency on `hello-frame`

If this module fails, then the bug is earlier than the application-level guest logic.

### Result

The firmware panicked immediately.

The new panic class was:

- `Guru Meditation Error`
- `Cache disabled but cached memory region accessed`
- `Write back error occurred while dcache tries to write back to flash`

The critical part of the stack trace is that it is no longer centered on replay or PaperS3 drawing. Instead it goes through WAMR memory setup:

- `memset in ROM`
- `os_mmap` at [espidf_memmap.c:76](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c#L76)
- `wasm_mremap_linear_memory`
- `wasm_mmap_linear_memory`
- `wasm_allocate_linear_memory`
- `memory_instantiate`
- `wasm_instantiate`
- `wasm_runtime_instantiate_internal`
- `wasm_runtime_instantiate`
- [wasm_module_runner.cpp:138](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp#L138)

## Step 8: Write a focused `Panel_EPD` analysis guide

After narrowing the live failure to the PaperS3 display path, I wrote a dedicated design/analysis guide for `Panel_EPD`.

Why I did this:

- the decoded crash line alone is too shallow a story
- `Panel_EPD` is not just one drawing function, it is a buffered and asynchronous display subsystem
- future debugging will be wasteful if the next person does not understand `_buf`, dirty-rect tracking, `display(...)`, cache writeback, and `task_update(...)`

What the new guide covers:

- the call path from `wasm_command.cpp` to `wasm_module_runner.cpp` to `wasm_host_api.cpp` to `papers3_canvas.cpp` to `M5.Display` to `Panel_EPD`
- the immediate draw path into `_buf`
- the deferred update path through `display(...)` and the background update task
- the strongest current interference mechanisms
- a strict separation of proven facts from hypotheses
- recommended next experiments for the PaperS3-specific debugging phase

Files studied while writing it:

- [main/papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
- [main/wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
- [main/wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- [Panel_EPD.hpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp)
- [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)

Additional upstream context included:

- the local `M5GFX` checkout is at `0.2.15`
- there is a newer PaperS3 refresh commit `fd824ee...` in upstream `M5GFX`
- an older PSRAM/cache-related PaperS3 fix `c899961...` is already present in our local version

The point of this step was not to change code. It was to compress the system model and the investigation model into one document so the next debugging slice can start from understanding instead of guesswork.

### Why this is important

This is the single most important learning from the first Espressif hardware test:

- the failure boundary changed

Under the previous upstream integration, the project had already reached a later stage:

- simple Wasm execution could succeed
- replay-related failures happened later, after runtime execution

Under the Espressif package, the system now fails earlier:

- instantiation of even a trivial Wasm module crashes during linear-memory mapping

That means the package swap did not preserve the previous failure boundary. It created a new one.

### What this rules out

This result rules out several lazy explanations:

- it is not specific to `hello-frame`
- it is not specific to display imports
- it is not specific to post-run replay
- it is not specific to WAMR teardown

The crash happens before all of those things.

### Current best interpretation

The Espressif package appears to exercise a different or differently configured ESP-IDF memory-mapping path during Wasm instantiation.

The most suspicious file is:

- [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)

At the crash site, `os_mmap()` allocates a buffer and immediately zeroes it with `memset(...)`. Since the panic lands there, the problem is now likely to be one of:

- an executable-memory allocation mode that is invalid on this board/configuration
- a dual-bus mirror or address-offset configuration mismatch
- a cache-sensitive memory capability combination that differs from the previous package

### Why this is a useful result even though it failed

This is exactly what a good A/B experiment should do:

- change one variable
- observe whether the boundary moves

The boundary moved from:

- "later replay after WAMR execution"

to:

- "instantiation-time WAMR linear-memory mapping"

That is a much cleaner debugging target than the old mixed runtime-plus-display failure.

## What I learned about the setup

There are two non-obvious lessons from this slice that matter for future debugging on this machine:

### Lesson 1: Serial interaction problems can come from stale monitor ownership

The host had a leftover `idf_monitor` process attached to `/dev/ttyACM0`. That created confusing behavior:

- the firmware booted
- the prompt appeared
- command injection through the old monitor path timed out

That was not the same thing as a firmware crash. Clearing the stale monitor process was necessary before interpreting runtime behavior.

### Lesson 2: Package migration can change the failure boundary even when the public runtime status looks identical

From the console, both integrations looked superficially similar:

- interpreter mode
- runtime initialized
- host symbols registered

But the real behavior diverged much earlier internally. That is a good reminder that:

- matching status banners do not guarantee matching runtime internals
- the real signal often appears at instantiate/load time, not in the status command

## Next debugging direction

The next correct step is not to revisit the display code. The next correct step is:

- inspect the Espressif memmap path and its active config
- compare it against the previous upstream integration
- look specifically at the conditions around `os_mmap()` in [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)

That is where the migration now teaches us the most.

## Step 8: Save the debugging helpers into the ticket scripts folder

At this point the debugging workflow had started to accumulate small ad hoc shell and Python snippets:

- USB enumeration checks
- monitor startup commands
- raw serial send/capture experiments

The user explicitly asked that these stop living only in transient command history and instead be saved under the ticket.

I added:

- `scripts/check_usb_console.sh`
- `scripts/run_idf_monitor.sh`
- `scripts/serial_send_and_capture.py`

Why this matters:

- the serial-monitoring setup on this machine is slightly tricky
- stale `idf_monitor` ownership and terminal-control probing (`ESC[6n`) are both real sources of confusion
- a future debugging session should not have to rediscover the exact helper commands by reading terminal scrollback

This is not just process hygiene. It is part of the technical learning:

- the setup itself affects the debugging outcome
- serial transport behavior is part of the system under investigation

The rule going forward in this ticket is:

- if a helper is used more than once, keep it in `scripts/`
- if a helper affects how runtime output is interpreted, mention it in the diary

## Step 9: Recover the PaperS3-specific platform patches and switch to a scripted probe loop

At this point I had enough contradictory evidence that I stopped treating the Espressif component as a black box. The right question was no longer "does the official component work?" but "what project-specific platform assumptions did the old integration already patch that the stock migration just reintroduced?"

This step was productive because it converted a vague migration failure into a concrete delta list. By diffing the old and new managed components and then rerunning the same Wasm probes with a reproducible script, I was able to show that the migrated runtime can in fact work on PaperS3 if those two platform-level assumptions are restored.

### Prompt Context

**User prompt (verbatim):** "continue investigating now"

**Assistant interpretation:** Continue debugging the migrated Espressif WAMR integration on real hardware instead of stopping at the first crash boundary.

**Inferred user intent:** Find out whether the migration is fundamentally broken or whether specific local integration patches need to be carried forward.

### What I did

- Compared the old and new WAMR ESP-IDF platform files and found the first critical migration delta:
  - old file: [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)
  - new file: [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)
- Verified from `compile_commands.json` that the stock Espressif component was building with `-DWASM_MEM_DUAL_BUS_MIRROR=1` on ESP32-S3.
- Realized the earlier "prefer internal RAM" patch inside [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c) was dead code under that setting, because the active branch was the dual-bus path.
- Restored the old project-specific PaperS3 behavior in the Espressif component by changing [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake) to force `WASM_MEM_DUAL_BUS_MIRROR=0`.
- Rebuilt and flashed, then reran `wasm run-preflush return-42`.
- Observed that the failure boundary moved forward again: the early `os_mmap()` panic disappeared, but execution now aborted at `pthread_self()`.
- Compared the old and new ESP-IDF thread shims and found the second critical migration delta:
  - old file: [espidf_thread.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c)
  - new file: [espidf_thread.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c)
- Restored the old PaperS3-specific `os_self_thread()` behavior in [espidf_thread.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c) so WAMR uses `xTaskGetCurrentTaskHandle()` instead of `pthread_self()` when running from `esp_console`.
- Replaced the tmux-heavy test loop with a repeatable script in the ticket:
  - [flash_and_probe_wasm.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/flash_and_probe_wasm.sh)
- Reused and kept the serial helper in the ticket:
  - [serial_send_and_capture.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/serial_send_and_capture.py)
- Used the scripted probe loop to run three real hardware checks:
  - `wasm run-preflush return-42`
  - `wasm run-preflush log-only`
  - `wasm run-preflush hello-frame`

### Why

- The old integration had already been locally adapted for PaperS3. A package swap without carrying forward those adaptations is not a neutral baseline.
- The tmux/manual monitor loop was consuming time and making it harder to tell whether a failure came from firmware or from host tooling state.
- A scripted flash-and-probe loop is easier to repeat, easier to log in the diary, and easier to reuse later.

### What worked

- Restoring `WASM_MEM_DUAL_BUS_MIRROR=0` removed the early instantiation crash in `os_mmap()`.
- Restoring the FreeRTOS-task-based `os_self_thread()` removed the `pthread_self()` assertion when running Wasm from `esp_console`.
- The scripted probe path worked reliably enough to capture complete results without depending on tmux pane state.
- After those two platform patches were restored:
  - `wasm run-preflush return-42` succeeded and returned `42`
  - `wasm run-preflush log-only` succeeded and logged `guest_log tag=9 value=42`
- This means the migrated Espressif runtime can now:
  - load a module
  - instantiate it
  - create an exec env
  - execute guest code
  - cross a simple host import boundary

### What didn't work

- The stock Espressif component failed on PaperS3 before guest execution because it re-enabled dual-bus mirroring under ESP32-S3 PSRAM support.
- The stock Espressif thread shim failed under `esp_console` because it assumes the current execution context is a pthread and calls:
  - `pthread_self()`
- `wasm run-preflush hello-frame` still panicked after the guest returned, with:
  - `Cache disabled but cached memory region accessed`
  - `Write back error occurred while dcache tries to write back to flash`
- After address decoding, the active `hello-frame` failure path is:
  - [Panel_EPD.cpp:371](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp#L371)
  - [LGFXBase.cpp:203](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp#L203)
  - [papers3_canvas.cpp:135](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp#L135)
  - [wasm_host_api.cpp:222](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp#L222)
  - [wasm_module_runner.cpp:190](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp#L190)

### What I learned

- The migration did not introduce a totally new failure story. It temporarily reintroduced two older platform mismatches that the previous PaperS3 integration had already solved locally.
- The key compile-time clue was the active define:
  - `-DWASM_MEM_DUAL_BUS_MIRROR=1`
- The user-visible moral is:
  - the stock Espressif component is not unusable on PaperS3
  - but it is not a drop-in replacement for this interpreter-only console path without carrying forward the existing platform patches
- Once those two patches are restored, the migration lands back on the same high-level boundary as before:
  - simple Wasm works
  - simple host logging works
  - the remaining crash is in the PaperS3 display preflush path

### What was tricky to build

- The first tricky part was a misleading dead patch. I initially focused on the non-mirror branch in `espidf_memmap.c`, but the build was actually compiled with `WASM_MEM_DUAL_BUS_MIRROR=1`, so that code path was not active. The symptom was that the source looked "patched" while the runtime behavior did not change. The fix was to stop guessing from the source and inspect the actual compile definitions.
- The second tricky part was transport/tooling contamination. `idf_monitor`, tmux, raw pyserial, and USB Serial/JTAG resets all interact in ways that can make a good firmware look bad. The scripted flash/probe wrapper reduced that confusion by enforcing the same order every time:
  - kill stale monitor holders
  - build and flash
  - wait for boot
  - answer the console `ESC[6n` probe
  - send one command
  - capture output

### What warrants a second pair of eyes

- Whether forcing `WASM_MEM_DUAL_BUS_MIRROR=0` is the right long-term migration strategy or only the right interpreter-mode PaperS3 strategy.
- Whether `os_self_thread()` should stay task-handle-based only for this project, or whether the Espressif component should expose a cleaner configuration switch for console-driven non-pthread execution.
- Whether the remaining `hello-frame` crash is still entirely display-layer specific or whether some post-call cache state from WAMR remains a contributing factor.

### What should be done in the future

- Preserve these two restored platform patches explicitly in the migration notes:
  - `WASM_MEM_DUAL_BUS_MIRROR=0`
  - `os_self_thread()` using `xTaskGetCurrentTaskHandle()`
- Keep using the scripted probe loop for all future hardware comparisons in this ticket.
- Decode every raw hardware backtrace into file/line form before drawing conclusions.
- Continue from the remaining `hello-frame` preflush crash rather than reopening the already-resolved instantiation and pthread paths.

### Code review instructions

- Start with the two restored platform files:
  - [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)
  - [espidf_thread.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c)
- Then review the new scripted probe entrypoint:
  - [flash_and_probe_wasm.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/flash_and_probe_wasm.sh)
- Validate with:
  - `scripts/flash_and_probe_wasm.sh /dev/ttyACM0 'wasm run-preflush return-42'`
  - `scripts/flash_and_probe_wasm.sh /dev/ttyACM0 'wasm run-preflush log-only'`
  - `scripts/flash_and_probe_wasm.sh /dev/ttyACM0 'wasm run-preflush hello-frame'`

### Technical details

- Commands that surfaced the migration deltas:
  - `diff -u .../bytecodealliance__/shared_platform.cmake .../espressif__/shared_platform.cmake`
  - `diff -u .../bytecodealliance__/espidf_thread.c .../espressif__/espidf_thread.c`
  - `rg -n "WASM_MEM_DUAL_BUS_MIRROR" build/compile_commands.json build/build.ninja ...`
- Scripted probe results:
  - `return-42`: success, `return_value=42`
  - `log-only`: success, `guest_log tag=9 value=42`, `return_value=42`
  - `hello-frame`: panic during [FlushWasmHostFrame](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp#L222) -> [PaperCanvasScreenClear](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp#L135)

## Step 10: Prepare a web-research handoff brief

Once the migration baseline was recovered, the next bottleneck stopped being raw firmware bring-up and became uncertainty. At that point, a good external research report could save time, but only if the researcher gets a precise brief rather than a vague “look into WAMR on ESP32-S3.”

I wrote a dedicated research handoff document for an in-house web researcher at:

- [02-web-research-brief-for-espressif-wamr-on-papers3.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/design/02-web-research-brief-for-espressif-wamr-on-papers3.md)

### Prompt Context

**User prompt (verbatim):** "make a detailed research guide for our in house web research expert. that way they can hopefully make a detailed report and we can maybe figure things out"

**Assistant interpretation:** Create a structured, detailed brief inside the ticket that tells a web-focused researcher exactly what to investigate, what evidence we already have locally, and what a useful report should contain.

**Inferred user intent:** Reduce future debugging time by giving a research specialist enough context to find upstream evidence, known issues, or design guidance without repeating the firmware team’s local work.

### What I did

- Added a new design doc focused on external research handoff.
- Framed the current state as a narrowed problem, not a generic “WAMR is broken” story.
- Documented:
  - the system architecture
  - the exact local findings already established
  - the recovered migration deltas
  - the current remaining failure boundary
  - five concrete research threads
  - source-priority rules
  - suggested search queries
  - a recommended report structure
- Linked the new brief from [index.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/index.md).

### Why

- A researcher can only help if they know what is already proven locally.
- Without a precise brief, they are likely to spend time rediscovering the already-resolved instantiation and pthread regressions.
- The real value of web research here is ranking next experiments and identifying upstream expectations or known issues.

### What worked

- The brief now gives a researcher enough context to investigate:
  - `WASM_MEM_DUAL_BUS_MIRROR`
  - `os_self_thread()` expectations under ESP-IDF
  - cache-disabled panics in display/preflush paths
  - public WAMR + ESP-IDF integration patterns
  - upstream issue history

### What didn't work

- N/A. This was documentation work, not a runtime experiment.

### What I learned

- The more the runtime behavior gets narrowed locally, the more valuable a tightly scoped research brief becomes.
- A good handoff document should not just ask questions. It should explicitly state what is already known, what phase the system now fails in, and what sources count as strong evidence.

### What was tricky to build

- The tricky part was keeping the brief useful to a web researcher without turning it into a firmware-only internal memo. The solution was to split it into:
  - system context
  - established local findings
  - targeted research threads
  - required report structure

### What warrants a second pair of eyes

- Whether the research brief is scoped correctly for the specific person or team who will use it.
- Whether we should later add a shorter executive-summary version for non-technical stakeholders.

### What should be done in the future

- Once the external report exists, link it back into `ESP-39` and compare each conclusion against the local findings in Step 9.

### Code review instructions

- Read the new brief at [02-web-research-brief-for-espressif-wamr-on-papers3.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/design/02-web-research-brief-for-espressif-wamr-on-papers3.md).
- Verify that it points to the correct local files and current failure boundary.

### Technical details

- The brief intentionally treats the current `hello-frame` panic as the active live bug and the earlier instantiation/pthread failures as resolved migration regressions.

## Step 11: Prove the remaining PaperS3 failure is same-boot WAMR contamination

Once the PaperS3 board was reattached, I stopped relying on one-command-per-boot probes and reran the baseline from scratch. That mattered because the older failure theory had quietly mixed two different cases together: "does replay work on a clean boot?" and "does replay still work after a successful Wasm execution in the same boot?" Those are not the same question.

This step produced the most useful narrowing result since the AtomS3R cross-check. A clean-boot `wasm replay hello-frame` now succeeds on PaperS3, but a later replay in the same boot can still be broken by an earlier Wasm run that never touched the PaperS3 drawing API. That moves the live theory away from "the replay queue is wrong" and toward "successful WAMR execution leaves behind state that makes later PaperS3 display work unsafe on this board."

### Prompt Context

**User prompt (verbatim):** "you figure it out, it''s arttached, run your test"

**Assistant interpretation:** Resume the real-hardware PaperS3 debugging loop immediately, choose the right on-device probes without more back-and-forth, and use the attached board to narrow the remaining failure.

**Inferred user intent:** Get a decisive new hardware result instead of more speculation, and keep the investigation trail detailed enough to learn from later.

**Commit (code):** 42205d7 — "debug(ticket): add same-boot serial probe helper"

### What I did

- Reconfirmed that the attached USB Serial/JTAG device was the reattached PaperS3 and that the active firmware still contained the two recovered Espressif WAMR platform fixes:
  - `WASM_MEM_DUAL_BUS_MIRROR=0`
  - `os_self_thread()` using `xTaskGetCurrentTaskHandle()`
- Reused the existing ticket-local flash/probe wrapper:
  - [flash_and_probe_wasm.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/flash_and_probe_wasm.sh)
- Reused the existing single-command serial helper:
  - [serial_send_and_capture.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/serial_send_and_capture.py)
- Ran clean-boot single-command probes and captured the actual current state:
  - `wasm status`
  - `wasm run-preflush return-42`
  - `wasm run-preflush log-only`
  - `wasm replay hello-frame`
  - `wasm run-preflush hello-frame`
- Decoded the new `hello-frame` crash against the active ELF and confirmed it still lands in:
  - [Panel_EPD.cpp:371](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp#L371)
  - [papers3_canvas.cpp:135](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp#L135)
  - [wasm_host_api.cpp:222](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp#L222)
  - [wasm_module_runner.cpp:190](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp#L190)
- Noticed a test-harness blind spot: the single-command serial helper resets the board every run, so it cannot answer whether a successful Wasm execution contaminates later commands in the same boot.
- Added a new multi-command helper under the ticket:
  - [serial_probe_sequence.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/serial_probe_sequence.py)
- Used the new helper to run two same-boot sequence probes:
  - `wasm run-preflush return-42` then `wasm replay hello-frame`
  - `wasm run-preflush log-only` then `wasm replay hello-frame`

### Why

- Clean-boot success and same-boot contamination are different hypotheses and require different tools.
- The single-command helper was good for baseline checks but structurally incapable of proving whether WAMR execution was poisoning later display work.
- If a trivial Wasm module that does not draw can still break a later PaperS3 replay in the same boot, the remaining bug is not “the guest queue is malformed.”

### What worked

- `wasm status` on clean boot reported:
  - `runtime=ready`
  - `version=2.4.0`
  - `host_api=ready`
  - `host_api.canvas.width=960`
  - `host_api.canvas.height=540`
- `wasm run-preflush return-42` succeeded on PaperS3 in the current migrated build.
- `wasm run-preflush log-only` succeeded on PaperS3 in the current migrated build.
- `wasm replay hello-frame` also succeeded on a clean boot.
- The new [serial_probe_sequence.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/serial_probe_sequence.py) helper worked as intended and let me prove a same-boot interaction rather than rebooting the board between commands.
- The decisive same-boot result is:
  - `wasm run-preflush return-42` succeeds
  - a later `wasm replay hello-frame` in the same boot crashes with `Cache disabled but cached memory region accessed`
- A second same-boot sequence with `log-only` instead of `return-42` also fails before the later replay can complete, which means the contamination does not require PaperS3 drawing imports.

### What didn't work

- My first attempt at rerunning probes reused the same serial port from two concurrent host processes and produced a tooling-side error:
  - `serial.serialutil.SerialException: device reports readiness to read but returned no data (device disconnected or multiple access on port?)`
- `wasm run-preflush hello-frame` still fails on a clean boot after the `before-preflush` probe point, with:
  - `Guru Meditation Error: Core / panic'ed (Cache disabled but cached memory region accessed).`
  - `Write back error occurred while dcache tries to write back to flash`
- In the clean-boot `hello-frame` case, the decoded crash path still lands in the PaperS3 display clear path:
  - [Panel_EPD.cpp:371](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp#L371)
  - [wasm_module_runner.cpp:190](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp#L190)
- In the `log-only` then replay sequence, the backtrace that was visible after the later command was shorter and less directly helpful:
  - `0x4200c686: find_command_by_name`
  - `0x4200ca37: esp_console_run`
  - because the panic note explicitly warns that the printed backtrace may not indicate the original cache-invalid access site

### What I learned

- The current PaperS3 state is subtler than the older summary “hello-frame crashes”:
  - clean boot replay works
  - direct `hello-frame` after Wasm still crashes
  - successful non-drawing Wasm can poison a later replay in the same boot
- That means the remaining PaperS3 bug is no longer well described as a bad replay queue or a Wasm guest drawing bug.
- The better current model is:
  - successful WAMR execution on PaperS3 changes some runtime/cache/task/display interaction state
  - later display work on this board becomes unsafe
  - AtomS3R remains the counterexample showing that this is not generic ESP32-S3 + WAMR behavior

### What was tricky to build

- The hardest part was a false sense of confidence from reboot-based probes. Rebooting between commands can make a poisoned same-boot system look healthy because the next test starts from a fresh runtime and fresh board state. The symptom was “replay succeeds” in one test and “hello-frame still crashes” in another, without an obvious bridge between them.
- The solution was to stop treating the serial helper as fixed infrastructure and instead extend the tooling. The new helper keeps one USB Serial/JTAG session open across multiple commands, waits for the `paper>` prompt after each step, answers the ANSI cursor query, and only then advances to the next command. That made the same-boot contamination visible.
- Another tricky edge was serial ownership. Running two pyserial capture helpers against the same USB Serial/JTAG device at once produced a misleading transport-level failure before the firmware could tell us anything. From this point on, these probes need to stay strictly sequential.

### What warrants a second pair of eyes

- Whether the next experiment should move WAMR execution onto a dedicated worker thread or `pthread` so PaperS3 replay never runs after WAMR on the `esp_console` task.
- Whether PaperS3 EPD driver state should be explicitly reinitialized after each Wasm run before any later display operation.
- Whether there is an ESP-IDF cache-management or memory-barrier expectation in the PaperS3 display path that AtomS3R never exercises.

### What should be done in the future

- Keep `ESP-39` open for the next slice, because the active bug is now explicitly about post-migration WAMR execution side effects on later PaperS3 behavior.
- Try the worker-thread or `pthread` execution model next, then rerun the same same-boot sequence probes.
- If that does not move the boundary, instrument or isolate the PaperS3 display path immediately before `PaperCanvasScreenClear()`.

### Code review instructions

- Review the new same-boot helper first:
  - [serial_probe_sequence.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/scripts/serial_probe_sequence.py)
- Then review the runtime/display boundary where the clean-boot `hello-frame` failure still lands:
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  - [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
  - [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
  - [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)
- Validate with these exact sequence probes:
  - `scripts/serial_probe_sequence.py --port /dev/ttyACM0 --command 'wasm run-preflush return-42' --command 'wasm replay hello-frame'`
  - `scripts/serial_probe_sequence.py --port /dev/ttyACM0 --command 'wasm run-preflush log-only' --command 'wasm replay hello-frame'`
  - `scripts/serial_send_and_capture.py --port /dev/ttyACM0 --command 'wasm replay hello-frame'`
  - `scripts/serial_send_and_capture.py --port /dev/ttyACM0 --command 'wasm run-preflush hello-frame'`

### Technical details

- Clean-boot results from this step:
  - `wasm replay hello-frame` -> success
  - `wasm run-preflush return-42` -> success, `return_value=42`
  - `wasm run-preflush log-only` -> success, `guest_log tag=9 value=42`
  - `wasm run-preflush hello-frame` -> panic in the PaperS3 display clear path
- Same-boot sequence results from this step:
  - `return-42` then replay -> replay crashes in the same `Panel_EPD` clear path
  - `log-only` then replay -> later replay attempt also fails with the same cache-disabled panic class
- Representative commands:
  - `/.../serial_send_and_capture.py --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00' --command 'wasm run-preflush hello-frame'`
  - `/.../serial_probe_sequence.py --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00' --command 'wasm run-preflush return-42' --command 'wasm replay hello-frame'`
  - `/.../serial_probe_sequence.py --port '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:16:17:DC-if00' --command 'wasm run-preflush log-only' --command 'wasm replay hello-frame'`

## Step 12: Falsify the worker-thread fix and show the post-WAMR PaperS3 crash is broader than `screenClear`

The next fix idea to test was straightforward: if the console task context is the problem, move WAMR execution onto a dedicated worker `pthread` and leave the replay path alone. That is a good experiment because it changes one major variable while preserving the same guest programs, the same host queue, and the same PaperS3 display code.

The result was negative in the useful sense. Running Wasm on a worker thread did not stop the later PaperS3 crash, and the reduced replay controls showed that the post-WAMR failure is not limited to `screenClear()`. After a successful worker-thread `return-42`, both a `clear-only` replay and a `frame-no-clear` replay still crash inside `Panel_EPD::writeFillRectPreclipped`. That sharply reduces the value of spending more time on the “wrong task context” theory.

### Prompt Context

**User prompt (verbatim):** "go ahead,"

**Assistant interpretation:** Continue immediately with the next concrete debugging experiment instead of stopping at the previous report.

**Inferred user intent:** Keep pushing the PaperS3 runtime/display investigation forward until a hypothesis is falsified or a new one becomes clearly stronger.

**Commit (code):** e91eaaf — "debug(papers3): add worker and reduced replay probes"

### What I did

- Added a worker-thread execution mode to the Wasm runner in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- Implemented the worker path as a dedicated host-created `pthread` with:
  - explicit `pthread_attr_setstacksize(...)`
  - `wasm_runtime_init_thread_env()`
  - `wasm_runtime_destroy_thread_env()`
- Added new console commands in [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp):
  - `wasm run-worker <name>`
  - `wasm run-preflush-worker <name>`
- Added `pthread` to [main/CMakeLists.txt](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt) so the new worker path links cleanly under ESP-IDF.
- Extended the replay-control harness in [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp) with:
  - `clear-only`
  - `frame-no-clear`
- Rebuilt and flashed the updated firmware to the attached PaperS3.
- Ran these hardware probes:
  - clean boot `wasm run-preflush-worker return-42`
  - same boot `wasm run-preflush-worker return-42` then `wasm replay hello-frame`
  - clean boot `wasm run-preflush-worker hello-frame`
  - same boot `wasm run-preflush-worker return-42` then `wasm replay clear-only`
  - same boot `wasm run-preflush-worker return-42` then `wasm replay frame-no-clear`
- Decoded the resulting backtraces against the new ELF.

### Why

- The worker-thread idea was the most defensible next experiment after Step 11 because it directly tested the upstream-style threading recommendation from the external report.
- The reduced replay controls were necessary because “it crashes in replay” was still too broad. I needed to know whether the failure after WAMR was only tied to full-screen clear operations or whether it affected other PaperS3 draw primitives too.

### What worked

- The new worker-thread path built and flashed cleanly.
- `wasm run-preflush-worker return-42` succeeded on hardware and clearly ran on a different task handle than the console task:
  - the `exec_probe.task` value changed to the worker-thread task
  - `execution_context=worker-thread` printed in the command result
- The new reduced replay examples were accepted by the firmware and executed far enough to trigger useful decoded backtraces.
- Address decoding on the new firmware stayed precise enough to separate the two reduced replay cases:
  - `clear-only` crashes via [PaperCanvasScreenClear](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp#L135)
  - `frame-no-clear` crashes via [PaperCanvasDrawRect](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp#L146)

### What didn't work

- The worker-thread experiment did **not** fix the main issue:
  - `wasm run-preflush-worker return-42` succeeded
  - a later `wasm replay hello-frame` in the same boot still crashed with `Cache disabled but cached memory region accessed`
- A clean-boot `wasm run-preflush-worker hello-frame` still crashed during the PaperS3 preflush path.
- The reduced replay controls ruled out a narrow `screenClear`-only theory:
  - `wasm replay clear-only` crashes after worker-thread `return-42`
  - `wasm replay frame-no-clear` also crashes after worker-thread `return-42`
- The decoded reduced-replay crash sites are:
  - `clear-only`
    - [Panel_EPD.cpp:370](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp#L370)
    - [wasm_replay_control.cpp:103](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L103)
  - `frame-no-clear`
    - [Panel_EPD.cpp:370](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp#L370)
    - [papers3_canvas.cpp:146](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp#L146)
    - [wasm_replay_control.cpp:103](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L103)

### What I learned

- The problem is not well explained by “WAMR ran on the wrong task.” Even when guest execution moves to a dedicated `pthread`, the later PaperS3 drawing path still becomes unsafe.
- The problem is also not well explained by “only full-screen clear is bad after WAMR.” A reduced replay that avoids `screenClear()` and starts with `drawRect()` still crashes in the same `Panel_EPD` machinery.
- The new better model is:
  - successful WAMR execution poisons some PaperS3 display-layer assumption
  - the poisoned state affects more than one draw primitive
  - the common downstream choke point is still `Panel_EPD::writeFillRectPreclipped(...)`

### What was tricky to build

- The worker-thread experiment had to be structured as an A/B path rather than a replacement. If I had replaced the inline path entirely, a failure would have been harder to interpret because I would lose the known baseline. Keeping both inline and worker commands in the same firmware made the comparison clean.
- The reduced replay controls were also important because the original `hello-frame` replay combined several PaperS3 operations. Without splitting them up, it would still be tempting to overfit to `fillScreen()` or to the full scene sequence. The separate `clear-only` and `frame-no-clear` probes made it obvious that multiple PaperS3 drawing operations fail after WAMR.

### What warrants a second pair of eyes

- Whether the next debugging slice should shift fully into the vendored M5GFX `Panel_EPD.cpp` implementation rather than continue around the edges from the WAMR side.
- Whether PaperS3 display buffer placement or cache-invalidation assumptions need explicit instrumentation around `startWrite()`, `fillScreen()`, `drawRect()`, and `endWrite()`.
- Whether a direct comparison against the latest upstream M5GFX PaperS3 fixes should now outrank any additional WAMR-side experiment.

### What should be done in the future

- Treat the worker-thread hypothesis as tested and currently falsified for this bug.
- Focus the next slice on the PaperS3 display driver and buffer handling, especially `Panel_EPD.cpp`.
- Compare the local vendored M5GFX PaperS3 EPD path against any newer upstream fixes or issue reports before inventing more runtime-side mitigations.

### Code review instructions

- Start with the new worker-thread execution path:
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Then review the reduced replay controls:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Validate with:
  - `wasm run-preflush-worker return-42`
  - `wasm run-preflush-worker return-42` then `wasm replay hello-frame`
  - `wasm run-preflush-worker return-42` then `wasm replay clear-only`
  - `wasm run-preflush-worker return-42` then `wasm replay frame-no-clear`

### Technical details

- Worker-thread direct result:
  - `wasm run-preflush-worker return-42` -> success
  - `execution_context=worker-thread`
  - `exec_probe.task` differs from the console task handle
- Worker-thread falsification result:
  - `wasm run-preflush-worker return-42` then `wasm replay hello-frame` -> still crashes
- Reduced replay results after successful worker-thread `return-42`:
  - `clear-only` -> crash in `Panel_EPD::writeFillRectPreclipped` via `PaperCanvasScreenClear`
  - `frame-no-clear` -> crash in `Panel_EPD::writeFillRectPreclipped` via `PaperCanvasDrawRect`
- Representative commands:
  - `/.../serial_probe_sequence.py --command 'wasm run-preflush-worker return-42' --command 'wasm replay hello-frame'`
  - `/.../serial_probe_sequence.py --command 'wasm run-preflush-worker return-42' --command 'wasm replay clear-only'`
  - `/.../serial_probe_sequence.py --command 'wasm run-preflush-worker return-42' --command 'wasm replay frame-no-clear'`
