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

**User prompt (verbatim):** "Create a second ticket about implemening the execution primitives required for your program, create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file\n  references.\n  It should be very clear and detailed. Store in the ticket and the nupload to remarkable.\n\nThen add detailed list of tasks and work on them one by one, commititng as you go and keeping a etaile diary"

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
