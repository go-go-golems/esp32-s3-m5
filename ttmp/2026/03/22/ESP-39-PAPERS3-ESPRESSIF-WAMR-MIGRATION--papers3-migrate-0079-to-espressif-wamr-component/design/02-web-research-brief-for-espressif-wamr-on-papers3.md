---
Title: Web research brief for Espressif WAMR on PaperS3
Ticket: ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - research
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp
      Note: |-
        PaperS3 canvas wrapper used during host-frame replay and implicated in the remaining crash
        PaperS3 canvas wrapper involved in the remaining crash
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: |-
        Host command queue and preflush path where the remaining `hello-frame` crash surfaces
        Current remaining preflush failure boundary
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: Main execution path for `wasm run-preflush ...`, including the current success/failure boundary instrumentation
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp
      Note: Runtime initialization/config context, including interpreter-mode and pool allocator setup
    - Path: 0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c
      Note: |-
        Espressif platform-layer file where the PaperS3 migration had to restore the FreeRTOS-task-based `os_self_thread()` behavior
        Recovered console-safe thread identity behavior for non-pthread execution
    - Path: 0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake
      Note: |-
        Espressif platform-layer file where the PaperS3 migration had to restore `WASM_MEM_DUAL_BUS_MIRROR=0`
        Recovered PaperS3-specific dual-bus configuration that the researcher should understand
    - Path: ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/reference/01-diary.md
      Note: Chronological local debugging record that the researcher should treat as the primary local evidence log
ExternalSources:
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0~1
    - https://components.espressif.com/components/espressif/wasm-micro-runtime/versions/2.4.0/examples/esp-idf
    - https://github.com/espressif/wasm-micro-runtime
    - https://github.com/bytecodealliance/wasm-micro-runtime
    - https://docs.espressif.com/projects/esp-idf/en/v5.3.4/
Summary: Detailed research handoff for a web-focused investigator who needs to study Espressif WAMR, ESP-IDF, and ESP32-S3/PaperS3-related cache and threading behavior and produce a useful external report.
LastUpdated: 2026-03-22T20:30:00-04:00
WhatFor: Give an in-house web researcher enough technical context, exact questions, source priorities, and evidence requirements to produce a useful report without redoing the firmware debugging from scratch.
WhenToUse: Read this before doing external web research about Espressif WAMR behavior on ESP32-S3, PaperS3, USB Serial/JTAG console tasks, cache-disabled panics, or dual-bus mirror configuration.
---


# Web Research Brief For Espressif WAMR On PaperS3

## Goal

This document is a handoff brief for a web research specialist. The goal is not to write firmware directly. The goal is to gather strong external evidence that helps the firmware team answer this question:

- Which parts of our current PaperS3 WAMR behavior are expected, known, supported, or already discussed upstream, and which parts look project-specific?

The researcher should assume the local team already did substantial hands-on debugging. This brief exists to prevent low-value duplication and to focus online research on the highest-signal unknowns.

The output we want from the researcher is:

- a detailed external report
- based on primary sources wherever possible
- tied back to our exact local failure boundaries
- with concrete recommendations for what to try next

## Executive Summary

The project is:

- board: M5Stack PaperS3 / ESP32-S3 with e-ink display
- SDK: `ESP-IDF 5.3.4`
- runtime: WAMR interpreter mode
- app model: precompiled AssemblyScript `.wasm` programs embedded in firmware and launched from `esp_console`

We migrated from:

- upstream package: `bytecodealliance/wasm-micro-runtime`

to:

- official package: `espressif/wasm-micro-runtime` `2.4.0~1`

The stock Espressif component initially failed earlier than the older local integration, but the local team found two migration-specific regressions:

1. The stock Espressif build re-enabled `WASM_MEM_DUAL_BUS_MIRROR=1` on ESP32-S3.
2. The stock Espressif thread shim used `pthread_self()` in a context where we are executing from `esp_console`, not from a pthread-created task.

After restoring the project’s older PaperS3-specific behavior:

- `return-42` succeeds
- `log-only` succeeds
- `hello-frame` still crashes during the PaperS3 display preflush path

So the current research target is narrower than “WAMR on ESP32-S3 is broken.”

It is closer to:

- what does Espressif/WAMR officially expect regarding `WASM_MEM_DUAL_BUS_MIRROR` on ESP32-S3 interpreter-only configurations?
- what is the intended threading model for `os_self_thread()` under ESP-IDF when WAMR is called from a normal FreeRTOS task?
- are there known cache or display-driver hazards when replaying queued display commands after WAMR execution on ESP32-S3 / PaperS3 / M5GFX EPD?

## What The System Is

The high-level architecture is:

```text
AssemblyScript source
    ->
precompiled .wasm demo modules
    ->
embedded into firmware image
    ->
loaded by WAMR at runtime
    ->
run from esp_console command handler
    ->
guest calls host imports
    ->
host queues PaperS3 canvas commands
    ->
host flushes command queue to M5GFX / Panel_EPD
```

The important local project is:

- [0079-papers3-wamr-assemblyscript-console](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console)

Important implementation files:

- [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
- [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
- [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)

Important migrated component files:

- [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)
- [espidf_thread.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_thread.c)

Relevant local documentation:

- [migration guide](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/design/01-espressif-wamr-migration-guide.md)
- [diary](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION--papers3-migrate-0079-to-espressif-wamr-component/reference/01-diary.md)

## What We Already Know Locally

Treat the following statements as established local evidence unless external sources contradict them strongly.

### Established Findings

- The firmware builds and boots with Espressif WAMR `2.4.0`.
- WAMR initializes in interpreter mode on `ESP-IDF 5.3.4`.
- The host API registers successfully.
- The stock Espressif component, unpatched, crashed earlier than the older local integration.
- The earlier stock-Espressif crash boundary was inside `os_mmap()` / `wasm_runtime_instantiate()`.
- Restoring `WASM_MEM_DUAL_BUS_MIRROR=0` removed that instantiation-time crash.
- Restoring a FreeRTOS-task-based `os_self_thread()` removed the `pthread_self()` assertion under `esp_console`.
- After those two restorations:
  - `wasm run-preflush return-42` succeeds
  - `wasm run-preflush log-only` succeeds
  - `wasm run-preflush hello-frame` still crashes during preflush/replay

### Current Remaining Crash Boundary

After address decoding, the current failing `hello-frame` path is:

```text
Wasm guest returns successfully
    ->
FlushWasmHostFrame()
    ->
PaperCanvasScreenClear()
    ->
M5GFX / Panel_EPD fill path
    ->
Cache-disabled panic
```

Decoded file/line trail:

- [wasm_module_runner.cpp:190](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp#L190)
- [wasm_host_api.cpp:222](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp#L222)
- [papers3_canvas.cpp:135](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp#L135)
- [LGFXBase.cpp:203](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp#L203)
- [Panel_EPD.cpp:371](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp#L371)

### Probe Matrix Already Run

The following on-device probes have already been executed:

| Probe | Result | Interpretation |
|---|---|---|
| `wasm run-preflush return-42` | success | trivial guest execution works |
| `wasm run-preflush log-only` | success | simple guest-to-host import works |
| `wasm run-preflush hello-frame` | panic | failure returns only when replaying real PaperS3 canvas commands |

This matters because it narrows the web research problem. The report should not waste effort arguing that “WAMR cannot run at all on ESP32-S3,” because local evidence already disproves that.

## What We Need The Web Researcher To Investigate

The research should be organized into five major threads.

## Research Thread 1: Official Expectations For `WASM_MEM_DUAL_BUS_MIRROR`

This is the highest-value thread because it directly explains one recovered regression.

### Questions

- Why does Espressif’s ESP-IDF platform layer automatically enable `WASM_MEM_DUAL_BUS_MIRROR=1` on `CONFIG_ESP32S3_SPIRAM_SUPPORT`?
- Is that recommendation meant mainly for:
  - AOT
  - JIT
  - executable mappings
  - interpreter mode too?
- Are there official caveats for interpreter-only workloads on ESP32-S3 with e-ink/display-heavy apps?
- Are there known issues with dual-bus mirroring plus PSRAM plus cache-disabled sections on ESP32-S3?
- Is there any upstream discussion where users intentionally disable dual-bus mirror for interpreter mode?

### Source Priority

- Espressif WAMR source tree
- Espressif issues / PRs / discussions
- Bytecode Alliance WAMR issues / PRs / discussions
- release notes or migration notes

### Desired Report Output

- a short explanation of what dual-bus mirror is for on ESP32-S3
- whether it is required for our use case
- whether forcing it off is a hack, a normal configuration, or a known workaround

## Research Thread 2: Official Threading Expectations For `os_self_thread()`

This is the second recovered regression and may reveal whether our current fix is fragile or normal.

### Questions

- In Espressif’s WAMR ESP-IDF integration, when is `pthread_self()` actually safe?
- Does Espressif assume WAMR entry always happens from a pthread-created task?
- Are there examples of WAMR being called from:
  - `esp_console`
  - plain FreeRTOS tasks
  - event-loop callbacks
  - main application tasks
- Is there any upstream note about `pthread_self()` asserting on non-pthread ESP-IDF tasks?
- Are there better supported alternatives than `xTaskGetCurrentTaskHandle()` for this context?

### Source Priority

- Espressif WAMR examples
- Espressif ESP-IDF + pthread docs
- WAMR platform-layer code and issues
- any product-mini or example applications referenced by comments or docs

### Desired Report Output

- the intended threading model
- whether our local `xTaskGetCurrentTaskHandle()` shim is reasonable
- any better upstream-compatible design if one exists

## Research Thread 3: Cache-Disabled Panics In ESP32-S3 Display / PSRAM / EPD Paths

This is the current live bug after the migration baseline was repaired.

### Questions

- Are there known ESP32-S3 cache-disabled panics when calling display or e-ink drivers after code that touched PSRAM or memory-mapped regions?
- Are there known M5GFX / M5PaperS3 / EPD-specific issues around:
  - `fillScreen`
  - `fillRect`
  - panel update paths
  - DMA/internal-RAM expectations
- Are there known constraints about what memory must be internal RAM versus PSRAM for these display paths?
- Are there known interactions between USB Serial/JTAG console tasks and display update code on ESP32-S3?
- Do any Espressif docs describe when cached memory access becomes invalid during writeback or flash cache transitions in application code?

### Source Priority

- ESP-IDF docs on cache-disabled regions, PSRAM, and IRAM safety
- M5GFX / M5Unified issues and discussions
- M5Stack PaperS3 examples and issue trackers
- Espressif forum or GitHub issues involving “Cache disabled but cached memory region accessed”

### Desired Report Output

- a shortlist of plausible root causes for the remaining `hello-frame` crash
- any external examples with the same or very similar panic signature
- recommended experiments ranked by confidence

## Research Thread 4: WAMR + ESP-IDF + Interpreter-Only Integration Patterns

This thread is about learning how other people structure similar systems.

### Questions

- Are there public examples of interpreter-only WAMR on ESP32-S3 using host imports and no AOT?
- Are there public examples that embed `.wasm` assets and call them from a CLI or console task?
- Are there examples that queue host-side effects and replay them after the Wasm call returns?
- Do examples generally run Wasm:
  - inline on the caller’s task
  - on a dedicated worker task
  - on a pthread-created task

### Desired Report Output

- architecture patterns used by other projects
- anything that matches or conflicts with our current execution model

## Research Thread 5: Upstream History And Bug Signals

This thread is about finding whether we are rediscovering known edges.

### Questions

- Are there upstream issues, commits, or PRs involving:
  - `espidf_memmap.c`
  - `espidf_thread.c`
  - `WASM_MEM_DUAL_BUS_MIRROR`
  - ESP32-S3
  - PSRAM
  - cache-disabled panics
  - `pthread_self`
- Are there commits after `2.4.0~1` that look relevant to our problem?
- Did Espressif diverge from upstream in ways that matter for ESP32-S3?

### Desired Report Output

- a timeline of relevant upstream fixes or discussions
- whether trying a newer component version seems justified
- whether there are candidate cherry-picks or config changes worth testing

## Search Strategy

Use primary sources first. The strongest evidence sources are:

- official Espressif component pages
- official ESP-IDF docs
- Espressif WAMR GitHub repo
- Bytecode Alliance WAMR GitHub repo
- M5GFX / M5Unified / M5Stack repos and issue trackers

Use secondary sources only if they add real value:

- forum threads
- blog posts
- Stack Overflow

If a secondary source makes a claim, try to trace it back to primary code, docs, or issue threads.

### Suggested Search Queries

- `site:github.com/espressif/wasm-micro-runtime WASM_MEM_DUAL_BUS_MIRROR esp32s3`
- `site:github.com/bytecodealliance/wasm-micro-runtime espidf_thread pthread_self esp-idf`
- `site:github.com/espressif/wasm-micro-runtime espidf_memmap cache disabled`
- `site:docs.espressif.com ESP32-S3 "Cache disabled but cached memory region accessed"`
- `site:github.com/m5stack/M5GFX Panel_EPD cache disabled`
- `site:github.com/m5stack/M5Unified PaperS3 cache disabled`
- `site:github.com "Write back error occurred while dcache tries to write back to flash" esp32s3`
- `site:github.com/espressif/wasm-micro-runtime "os_self_thread" pthread_self`
- `site:github.com/espressif/wasm-micro-runtime "esp_console"`

## What A Good Final Report Should Look Like

We do not want a vague “I searched some things” memo. We want a report with the following structure.

## Section 1: Executive Summary

- 5 to 10 bullet points
- what is already established locally
- what external evidence strongly supports
- top 3 recommended next actions

## Section 2: System Context

- brief explanation of PaperS3 + ESP32-S3 + WAMR + `esp_console`
- enough context that a new engineer can follow the rest

## Section 3: Findings By Research Thread

For each thread above:

- what was investigated
- what was found
- what sources support it
- confidence level
- whether it changes our next action

## Section 4: Source Table

For every important source:

- link
- source type
- short summary
- why it matters
- whether it is primary or secondary

## Section 5: Recommended Next Experiments

Each experiment should have:

- hypothesis
- exact local files likely involved
- expected confirming result
- expected disconfirming result
- cost/risk estimate

Example format:

```text
Experiment: Run preflush canvas replay from a dedicated worker task
Hypothesis: The remaining panic depends on the esp_console task context, not only on the display path
Files: wasm_module_runner.cpp, wasm_host_api.cpp, papers3_canvas.cpp
If true: worker-task replay reduces or eliminates the panic
If false: same panic remains in Panel_EPD path
Cost: medium
```

## Constraints For The Researcher

- Do not assume the earliest failure is still the current failure.
- Do not recommend generic “update everything” unless you can tie that recommendation to specific source evidence.
- Do not conflate:
  - stock Espressif behavior
  - our patched migration state
  - the older upstream local integration
- Keep the runtime phases separate:
  - load
  - instantiate
  - create exec env
  - execute guest
  - flush queued host frame
  - teardown

This distinction is critical because our local results differ by phase.

## Why This Research Matters

Without good external research, the firmware team can keep chasing local symptoms forever. With good external research, we can answer higher-quality questions:

- are we fighting a known platform assumption?
- are we carrying the right local patches?
- are we now back to the old display-only failure for a known reason?
- should we stay on Espressif WAMR with local platform overrides, or is there a better-supported integration model?

The best outcome is not “prove one side right.” The best outcome is:

- reduce uncertainty
- separate upstream expectations from local hacks
- give the firmware team a smaller, better-ranked next-step list
