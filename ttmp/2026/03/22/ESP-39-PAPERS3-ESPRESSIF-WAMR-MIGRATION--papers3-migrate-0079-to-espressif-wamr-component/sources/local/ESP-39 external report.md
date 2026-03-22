---
Title: Imported external report - ESP-39 Espressif WAMR on PaperS3
Ticket: ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION
Status: active
Topics:
    - debugging
    - esp-idf
    - firmware
    - papers3
    - research
    - wasm
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Imported external research report about Espressif WAMR migration risks, PaperS3 display-layer hypotheses, and recommended next debugging actions.
LastUpdated: 2026-03-22T20:00:00-04:00
WhatFor: Preserves the external web-research findings used to guide follow-on PaperS3 debugging.
WhenToUse: Read when comparing local hardware findings against external WAMR, ESP-IDF, and M5GFX evidence.
---

# External Research Report: Espressif WAMR on PaperS3
**Ticket:** ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION  
**Prepared:** 2026-03-22  
**Basis:** uploaded repo inspection + external web research

## 1. Executive Summary

- The recovered `WASM_MEM_DUAL_BUS_MIRROR=0` override looks defensible for your current interpreter-only PaperS3 path. The strongest upstream evidence says Espressif/BA enabled the ESP32-S3 dual-bus mirror path specifically so AOT code can live in PSRAM while SRAM remains available for Wi-Fi/BLE and peripheral drivers. I did not find equally strong evidence that interpreter-only workloads require it.
- The recovered `os_self_thread()` override is compensating for a real upstream assumption. The official ESP-IDF docs say `pthread_self()` asserts when called from a FreeRTOS task that is **not** a pthread, and WAMR’s ESP-IDF platform file still uses `pthread_self()` with an in-code comment that `xTaskCreate` is not enough.
- The official WAMR ESP-IDF example still launches WAMR from `app_main()` by creating a pthread first. I did not find an official `esp_console`-driven example or an official example of queued host-side effects replayed after the Wasm call returns.
- I did **not** find external evidence that “WAMR on ESP32-S3 is generally broken.” The public evidence is narrower: ESP32-S3 support exists, interpreter mode is supported, and the main upstream ESP32-S3-specific memory work was around AOT-on-PSRAM support.
- The strongest external signal for the remaining `hello-frame` panic is now in the PaperS3 display layer, not in generic WAMR bring-up. ESP-IDF documents the exact panic class you are seeing when code/data in flash or PSRAM are touched while SPI1 operations have caches disabled.
- A recent open M5GFX issue for **M5Paper S3** describes two concrete PSRAM corruption bugs in `Panel_EPD.cpp`, including a rotation/update-rectangle bug that can make full-screen calls walk off the PSRAM-backed EPD buffer. That is close enough to your `fillScreen` / `Panel_EPD` crash path that it should be treated as a first-class lead.
- Upstream WAMR has advanced to `WAMR-2.4.4`, but the Espressif component registry still shows `2.4.0~1` as the latest packaged component. I did not find release-note evidence that 2.4.1–2.4.4 contain ESP-IDF fixes that obviously solve your current PaperS3 failure, and the 2.4.4 tag still has the same `pthread_self()` and ESP32-S3 dual-bus-mirror logic.

### Top 3 recommended next actions

1. Move WAMR entry and preflush replay onto a dedicated **pthread worker**, not the `esp_console` task, and call `wasm_runtime_init_thread_env()` / `wasm_runtime_destroy_thread_env()` if you use a host-created thread.
2. Diff your vendored `Panel_EPD.cpp` against the February 2026 M5GFX PaperS3 issue and locally patch/instrument the two reported PSRAM-corruption cases before doing more speculative WAMR surgery.
3. Treat “cache-disabled panic during display replay” as an **IRAM/DRAM/PSRAM placement** problem until disproven: minimize PSRAM-backed buffers and flash-resident callbacks reachable during the EPD update path.

## 2. System Context

Your local architecture is already narrower than a generic “WAMR on ESP32-S3” report:

```text
embedded AssemblyScript .wasm
    -> WAMR interpreter
    -> launched from esp_console handler
    -> guest calls host imports
    -> host queues PaperS3 canvas ops
    -> host preflush replays queue to M5GFX / Panel_EPD
```

From the uploaded repo state, the important facts are:

- the migrated Espressif component in your repo is already **patched** back to `WASM_MEM_DUAL_BUS_MIRROR=0`
- the migrated ESP-IDF thread shim in your repo is already **patched** to use a FreeRTOS-task identity path instead of `pthread_self()`
- the remaining crash boundary is in the **post-guest preflush display replay**, not during basic runtime init or trivial Wasm execution

That means the remaining question is not “can WAMR run?” It is “what upstream assumptions are still misaligned with this PaperS3 execution model, and how much of the remaining crash now belongs to the display path?”

## 3. Findings by Research Thread

### Thread 1 — Official expectations for `WASM_MEM_DUAL_BUS_MIRROR`

#### What I found

The strongest upstream evidence points to a very specific motivation: **ESP32-S3 AOT execution from PSRAM**.

In both the current mainline and the `WAMR-2.4.4` tag, the ESP-IDF platform CMake file says that when ESP32-S3 PSRAM support is enabled, “it had better to put AOT into PSRAM” so SRAM can remain available for Wi-Fi/BLE and peripheral drivers, and then it enables `WASM_MEM_DUAL_BUS_MIRROR=1`. The feature was introduced by the upstream change titled **“ESP-IDF platform supports to load AOT to PSRAM and run it (#2385)”**. The paired memory-mapping code changes executable mappings to PSRAM-backed allocations, uses the IBUS/DBUS mirror offset, and adds cache writeback / I-cache disable-enable logic in `os_dcache_flush()`.  
**Interpretation:** the clearest public rationale is **AOT/XIP-style executable placement**, not interpreter-only semantics.

#### What this means for your migration

For your current configuration — interpreter mode, precompiled `.wasm`, no evidence of needing AOT-on-PSRAM execution — forcing `WASM_MEM_DUAL_BUS_MIRROR=0` looks more like a **reasonable configuration override** than a random hack. It is still a local override, because upstream does not ship an interpreter-only conditional for it, but the upstream comments and the implementation do not support the idea that interpreter-only ESP32-S3 workloads must always run with the mirror enabled.

I did **not** find a public upstream discussion explicitly saying “disable it for interpreter-only ESP32-S3,” so confidence is moderate rather than absolute. But I also did not find evidence that the mirror is the intended default for every ESP32-S3 mode equally. The code and commit history still tie it most strongly to executable/AOT placement in PSRAM.

#### Confidence
**Moderate to high** on “this was introduced primarily for AOT-on-PSRAM.”  
**Moderate** on “keeping it off is the right long-term interpreter-only setting,” because that part is still an inference.

### Thread 2 — Official threading expectations for `os_self_thread()`

#### What I found

This one is much clearer.

- ESP-IDF 5.3.4 documents that pthreads are wrappers over FreeRTOS tasks, and specifically states that `pthread_self()` will assert if it is called from a FreeRTOS task that is **not** a pthread.
- WAMR’s ESP-IDF thread shim still implements `os_self_thread()` as `pthread_self()` and includes an in-file comment stating that this is only allowed if the current execution context is “a thread” and that `xTaskCreate` is not enough.
- The official WAMR ESP-IDF example (`product-mini/platforms/esp-idf/main/main.c`) still enters WAMR by creating a pthread from `app_main()` and then running the WAMR workload there.
- A WAMR maintainer answer in August 2024 says host-created threads should call `wasm_runtime_init_thread_env()` at the beginning and `wasm_runtime_destroy_thread_env()` at the end if the thread was created by the host rather than by the runtime itself.

#### What this means for your migration

Your local `xTaskGetCurrentTaskHandle()`-based recovery is a **reasonable tactical fix** for running WAMR from the `esp_console` task, because the stock implementation is simply not safe in that context.

However, it is not the most upstream-aligned design. The most upstream-compatible design is:

1. create a dedicated pthread worker for WAMR entry  
2. initialize/destroy the WAMR thread environment if the thread is host-created  
3. keep WAMR execution and any WAMR-owned thread bookkeeping inside that pthread context

That does not prove the remaining display panic is a threading bug. It does mean your current patched thread shim is compensating for an upstream assumption rather than fixing a one-off typo.

#### Confidence
**High.** This is one of the strongest external findings in the whole report.

### Thread 3 — Cache-disabled panics in ESP32-S3 display / PSRAM / EPD paths

#### What I found

ESP-IDF’s SPI Master documentation describes the exact failure class you are hitting. In the section on SPI1 use, Espressif says that when the driver operates on SPI1, code and data used by the current task must be in internal memory; flash and PSRAM are behind the same cache domain; and touching external memory during the relevant acquire windows can trigger **`Cache disabled but cached memory region accessed`**.

That does **not** prove your PaperS3 driver is definitely using SPI1 in the same exact way at the point of failure, but it is a close architectural match to your decoded panic: a display path that ends up in a cache-sensitive EPD transfer routine and then faults when something flash/PSRAM-backed is touched.

The second major finding is the February 2026 M5GFX issue against **M5Paper S3**:

- one reported bug is an odd-width overrun in `Panel_EPD::task_update()` that can corrupt PSRAM heap metadata
- the other is a rotation/update-rectangle bug where logical full-screen coordinates can exceed the physical 960×540 PSRAM backing buffer when the app sees the display as 540×960 under rotation
- the issue explicitly says calls like `display.display(0, 0, display.width(), display.height())` can enqueue an update whose height overruns `_step_framebuf`

That is highly relevant because your current failing boundary is a `fillScreen`-adjacent path ending in `Panel_EPD` during PaperS3 replay. A full-screen clear/update under rotation is exactly the shape of operation that the M5GFX bug report calls out.

#### Plausible root causes ranked

1. **PaperS3 / M5GFX EPD bug or PSRAM corruption in `Panel_EPD.cpp`**  
   The recent issue is too close to ignore. If your vendored M5GFX state contains equivalent logic, it could explain why the display path is the point where failure becomes visible.

2. **Display replay touches flash/PSRAM-backed code/data during a cache-disabled EPD transfer window**  
   This is strongly consistent with the panic class documented by ESP-IDF.

3. **WAMR execution perturbs task-local/runtime state enough that a fragile EPD path becomes reproducibly unsafe afterward**  
   This fits your local evidence that even `return-42` can poison later replay, but the external evidence for the *mechanism* here is weaker.

4. **USB Serial/JTAG console-specific interaction**  
   I did not find strong external evidence for this. It remains possible as a context factor because your entrypoint is `esp_console`, but it is not currently supported by strong public evidence.

#### Confidence
**Moderate to high** on “the remaining bug now looks display/memory/cache-adjacent.”  
**Moderate** on the specific root cause without a diff against your exact M5GFX version.

### Thread 4 — WAMR + ESP-IDF + interpreter-only integration patterns

#### What I found

The public examples are simpler than your current architecture.

- The Espressif component registry currently lists a single ESP-IDF example for the packaged component.
- The official ESP-IDF example pattern is effectively: initialize WAMR, create a pthread, load/instantiate, run, destroy.
- Public upstream guidance around thread environments assumes a dedicated runtime thread or a host-created thread that explicitly initializes WAMR thread state.
- I did not find an official pattern matching: `esp_console` entry -> inline execution on the console task -> queue host display effects -> replay those effects after the Wasm call returns.

#### What this means

Your current architecture is not obviously “wrong,” but it is clearly more specialized than the public happy-path examples. That matters because example coverage is not validating your highest-risk assumptions:

- non-pthread task entry
- console-task entry
- delayed replay of host side effects
- PaperS3 EPD flush after Wasm execution

So the absence of an upstream example here is not proof of a bug, but it is a warning that you are off the documented path and should prefer designs that move you back toward official expectations when possible.

#### Confidence
**Moderate.** The examples/public material are sparse enough that this is more about what is *not* being demonstrated upstream.

### Thread 5 — Upstream history and bug signals

#### What I found

The historical cluster that matters most is 2023–2024:

- **Bring up WAMR on esp32-s3 device (#2348)**
- **ESP-IDF platform supports to load AOT to PSRAM and run it (#2385)**

That is the point where the dual-bus mirror logic becomes central in the ESP-IDF platform layer.

Between `WAMR-2.4.0` and current upstream `WAMR-2.4.4`, I did not find release-note items that obviously target your exact PaperS3 problem. The 2.4.4 release is primarily security/interpreter-fast-interpreter fixes unrelated to ESP-IDF display/cache behavior. More importantly, the 2.4.4 tag still contains:

- the same ESP32-S3 PSRAM -> `WASM_MEM_DUAL_BUS_MIRROR=1` logic
- the same `os_self_thread()` -> `pthread_self()` assumption
- the same `product-mini` pthread entry model

So “try a newer WAMR” is not baseless, but it is also not strongly justified as the *first* move if the goal is to solve the current PaperS3 replay panic.

#### Recommendation on version movement

- **Do not** treat “upgrade to newer WAMR” as the primary fix hypothesis for the remaining panic.
- **Do** consider newer upstream WAMR if you want:
  - security fixes from 2.4.4
  - a cleaner baseline for future cherry-picks
  - confirmation that no hidden ESP-IDF change exists beyond the registry package
- **Do not expect** 2.4.4 alone to remove the need for either:
  - your thread-context change, or
  - interpreter-only reconsideration of dual-bus mirror

#### Confidence
**High** on “current upstream still preserves the two same assumptions that you had to patch.”  
**Moderate** on “no later hidden fix exists,” because release notes and tag inspection are good but not omniscient.

## 4. Source Table

| Source | Type | Why it matters | Primary / Secondary |
|---|---|---|---|
| Espressif component registry `espressif/wasm-micro-runtime 2.4.0~1` | Official component registry | Confirms packaged component status, example count, supported packaging context | Primary |
| WAMR `shared_platform.cmake` (main and 2.4.4 tag) | Upstream source | Shows the exact ESP32-S3 PSRAM -> `WASM_MEM_DUAL_BUS_MIRROR=1` rule and its AOT-oriented comment | Primary |
| WAMR commit `ESP-IDF platform supports to load AOT to PSRAM and run it (#2385)` | Upstream commit history | Establishes why the mirror logic was added | Primary |
| WAMR `espidf_memmap.c` (main and 2.4.4 tag) | Upstream source | Shows that mirror mode changes mmap allocation behavior and cache flush logic, reinforcing the AOT/executable-memory interpretation | Primary |
| WAMR `espidf_thread.c` (main and 2.4.4 tag) | Upstream source | Shows `os_self_thread()` still returns `pthread_self()` with an explicit warning that `xTaskCreate` is insufficient | Primary |
| WAMR `product-mini/platforms/esp-idf/main/main.c` (2.4.4 tag) | Upstream example | Shows the official ESP-IDF integration pattern still runs WAMR inside a pthread | Primary |
| ESP-IDF 5.3.4 pthread docs | Official documentation | States `pthread_self()` asserts on non-pthread FreeRTOS tasks | Primary |
| ESP-IDF SPI Master docs | Official documentation | Describes cache-disabled exceptions when flash/PSRAM are touched during SPI1 windows | Primary |
| WAMR discussion #3697 | Upstream maintainer answer | Gives the clearest public guidance on thread-env handling for host-created threads | Primary-ish (maintainer guidance) |
| M5GFX issue #181 | Upstream issue tracker | Recent, concrete PaperS3-specific `Panel_EPD.cpp` PSRAM corruption findings very near your crash path | Secondary but high-signal |
| WAMR releases / release notes | Upstream release metadata | Shows current upstream version and absence of obvious ESP-IDF fixes for this specific issue after 2.4.0 | Primary |

## 5. Recommended Next Experiments

### Experiment 1 — Run WAMR and preflush replay on a dedicated pthread worker

**Hypothesis:** the remaining failure depends in part on executing from the `esp_console` task, which is outside the upstream threading model.  
**Likely files:** `main/wasm_module_runner.cpp`, `main/wasm_runtime_service.cpp`, `managed_components/.../espidf_thread.c`  
**Exact change:** create a worker pthread from the console command handler, call `wasm_runtime_init_thread_env()` at worker start, run the Wasm load/instantiate/execute/preflush sequence entirely in that worker, then call `wasm_runtime_destroy_thread_env()` before exit.  
**Confirming result:** `run-preflush hello-frame` becomes stable or fails differently.  
**Disconfirming result:** same `Panel_EPD` cache-disabled panic with no material behavioral shift.  
**Cost:** medium  
**Priority:** 1

### Experiment 2 — Move only the replay/flush stage onto a dedicated worker task

**Hypothesis:** Wasm execution itself is not the only variable; the display replay is sensitive to the task context that performs the flush.  
**Likely files:** `main/wasm_host_api.cpp`, `main/papers3_canvas.cpp`, `main/wasm_module_runner.cpp`  
**Exact change:** keep the guest execution path unchanged, but hand off `FlushWasmHostFrame()` to a dedicated worker with a narrow, instrumented queue boundary.  
**Confirming result:** replay-only context switch reduces or removes the panic, implying a display-context sensitivity.  
**Disconfirming result:** same panic in `Panel_EPD`, which pushes suspicion back toward data corruption or memory placement.  
**Cost:** medium  
**Priority:** 2

### Experiment 3 — Audit and patch `Panel_EPD.cpp` against M5GFX issue #181

**Hypothesis:** the remaining panic is downstream of a PaperS3-specific PSRAM corruption or bounds bug in the EPD driver.  
**Likely files:** `main/papers3_canvas.cpp`, vendored `M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`, vendored `LGFXBase.cpp`  
**Exact change:** compare your vendored driver to the two bug descriptions from issue #181; add guards or local fixes for the odd-byte-width overrun and the rotation/update-rectangle mismatch; add temporary bounds/assert logging around full-screen clear/update paths.  
**Confirming result:** the crash disappears, becomes a heap-integrity warning earlier, or moves to a different boundary with useful diagnostics.  
**Disconfirming result:** no effect on the `hello-frame` preflush panic.  
**Cost:** medium to high  
**Priority:** 3

### Experiment 4 — Force replay-critical code/data off PSRAM and out of flash-backed call paths

**Hypothesis:** the replay path is touching external memory during a cache-disabled display window.  
**Likely files:** `main/papers3_canvas.cpp`, `main/wasm_host_api.cpp`, linker placement files / M5GFX callbacks  
**Exact change:** place replay-critical helpers in IRAM where practical, move small replay-state objects to DRAM, avoid PSRAM-backed temporary buffers during screen-clear/update, and eliminate flash-resident callback or lookup-table dependencies reachable inside the hot replay path.  
**Confirming result:** panic frequency drops or vanishes when replay path memory placement is tightened.  
**Disconfirming result:** same panic regardless of placement hardening.  
**Cost:** medium to high  
**Priority:** 4

### Experiment 5 — Formalize `WASM_MEM_DUAL_BUS_MIRROR` as an explicit mode matrix, not an accidental patch

**Hypothesis:** your current interpreter-only success is tied to the mirror override, and future regressions will recur unless this becomes a documented build mode.  
**Likely files:** `managed_components/.../shared_platform.cmake`, `main/wasm_runtime_service.cpp`, project docs  
**Exact change:** make the mirror behavior selectable by an explicit project option; run a small matrix: interpreter-only + mirror off/on, minimal demos + PaperS3 replay, with identical instrumentation.  
**Confirming result:** mirror-on reintroduces early instantiate/mmap instability while mirror-off stays required for interpreter mode.  
**Disconfirming result:** mirror setting no longer matters once another variable is controlled.  
**Cost:** low to medium  
**Priority:** 5

## 6. Bottom Line

The public evidence supports a split conclusion:

- Your two migration recoveries are grounded in real upstream assumptions:
  - dual-bus mirror is primarily about AOT-on-PSRAM on ESP32-S3
  - `os_self_thread()` assumes pthread context
- The remaining failure no longer looks like a generic WAMR migration failure.
- The best-supported next line of attack is a combination of:
  - move execution back toward the upstream pthread model
  - treat PaperS3 `Panel_EPD` as an active suspect
  - harden memory placement around replay until the cache-disabled panic is either removed or explained

That is the smallest externally justified next-step set I can support from current public evidence.
