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
    - Path: 0079-papers3-wamr-assemblyscript-console/main/idf_component.yml
      Note: Dependency manifest being migrated from upstream WAMR to Espressif's package
    - Path: 0079-papers3-wamr-assemblyscript-console/main/CMakeLists.txt
      Note: Main component alias wiring that must be updated during the migration
    - Path: 0079-papers3-wamr-assemblyscript-console/dependencies.lock
      Note: Resolved dependency record used to confirm the migration result
ExternalSources: []
Summary: Step-by-step diary for the WAMR dependency migration in `0079`.
LastUpdated: 2026-03-22T19:52:00-04:00
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
