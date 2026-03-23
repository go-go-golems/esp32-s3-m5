---
Title: Diary
Ticket: ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - display
    - m5gfx
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp
      Note: Exposed the new replay probe commands in the console
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp
      Note: Added explicit cache-sync and cache-line-aligned PSRAM replay probes
    - Path: ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/espressif_wamr_espidf_memmap_local_debug_patch.diff
      Note: Tracked patch artifact for the ignored WAMR memmap instrumentation
    - Path: ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/probe_wamr_psram_cache.sh
      Note: Ticket-local flash-and-probe wrapper for the PSRAM contamination sequence
    - Path: ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/serial_probe_sequence.py
      Note: Ticket-local single-boot serial probe helper
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-22T22:42:55.213541061-04:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

<!-- What is the purpose of this reference document? -->

## Context

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
---
Title: Diary
Ticket: ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
    - display
    - m5gfx
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp
      Note: PaperS3 frame bridge into M5GFX EPD drawing
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Main crash choke point and instrumentation target
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp
      Note: Declares the background update task, buffers, and queue state used by the backend
ExternalSources: []
Summary: Step-by-step diary for the PaperS3 Panel_EPD instrumentation and upstream-diff debugging slice.
LastUpdated: 2026-03-22T22:38:00-04:00
WhatFor: Record the exact driver-level debugging steps used to narrow the remaining PaperS3 display crash after successful WAMR execution.
WhenToUse: Read before continuing the PaperS3 display-path investigation or reviewing why Panel_EPD became the primary focus.
---

# Diary

## Goal

Capture the focused PaperS3 driver debugging slice that starts after the WAMR migration and headless baseline work, and narrows the remaining failure around `M5GFX` `Panel_EPD`.

## Step 1: Create the Panel_EPD instrumentation ticket and pin down the first driver-level target

This new slice exists because the broader runtime question is no longer the most useful one. The headless PaperS3 build already proved that simple Wasm execution can succeed on the board when our own display path is removed, and AtomS3R already proved that the recovered WAMR integration can drive a non-EPD display path. That leaves the PaperS3-specific EPD backend as the highest-value target.

Before patching anything, I reopened the exact choke point and the bridge into it. The key detail is that the crash line is not in the deferred update task first; it is in the direct nibble-write loop of `Panel_EPD::writeFillRectPreclipped(...)`, writing into `_buf`, and `_buf` is allocated from PSRAM. That makes the immediate debugging question more concrete: is the first post-WAMR draw writing to a bad PSRAM framebuffer region, or is some broader EPD backend state already corrupt before that write?

### Prompt Context

**User prompt (verbatim):** "go ahead."

**Assistant interpretation:** Create the next ticket slice around `Panel_EPD`, add tasks and diary scaffolding, and start the driver-focused debugging work instead of staying at the architecture-summary level.

**Inferred user intent:** Move from broad hypothesis discussion to concrete PaperS3 driver investigation, with a clean ticket history and detailed debugging notes.

### What I did

- Created ticket `ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION`.
- Added:
  - [tasks.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/tasks.md)
  - [01-panel-epd-instrumentation-and-upstream-diff-plan.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/design/01-panel-epd-instrumentation-and-upstream-diff-plan.md)
  - this diary
- Re-read the exact crash region in:
  - [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)
  - [Panel_EPD.hpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp)
  - [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
- Confirmed from source that:
  - `writeFillRectPreclipped(...)` writes directly into `_buf`
  - `_buf` is allocated with `heap_caps_aligned_alloc(..., MALLOC_CAP_SPIRAM)`
  - the row base is computed as `&_buf[y * ((_cfg.panel_width + 1) >> 1)]`
- Rechecked local upstream history for `Panel_EPD.cpp` to keep the next patch grounded in real version movement rather than vague memory.

### Why

- The new ticket isolates the PaperS3 driver slice from the broader WAMR migration history.
- Re-reading the exact code first matters because the next patch should answer a specific question about the framebuffer write path, not produce generic noise.
- The PSRAM allocation detail is especially important because it matches the general shape of the crashes we have been chasing on PaperS3.

### What worked

- The local source confirms that the immediate crash choke point is not abstract anymore; it is a direct write into the PSRAM-backed EPD framebuffer.
- The local `M5GFX` history still shows the PaperS3-specific line of development, including:
  - `c899961` `fix PSRAM cache write back for Tab5 + PaperS3`
  - `031dbe2` `tweak for PaperS3 refresh rising version 0.2.15`
- The design plan and task list are now explicit enough that the next code slice can be reviewed against a concrete hypothesis.

### What didn't work

- N/A so far in this ticket slice. This step was ticket creation plus source grounding, not yet a live code patch.

### What I learned

- The most actionable first instrumentation target is the direct draw path, not the deferred update task.
- The framebuffer is explicitly in PSRAM, which keeps memory/cache assumptions near the top of the suspect list.
- The next code change should capture a small number of facts at first entry into `writeFillRectPreclipped(...)`:
  - pointer
  - bounds
  - stride math
  - mode/state

### What was tricky to build

- The main sharp edge here is scope control. It would be easy to treat `Panel_EPD` as a black box and start scattering logs through the whole driver, but that would make the result harder to interpret. The safer starting point is the shortest path between the current app bridge and the first crashing write into `_buf`.

### What warrants a second pair of eyes

- Whether the first patch should stay pure instrumentation or include a small upstream-aligned A/B change in the same slice.
- Whether any diagnostic added to the write path risks perturbing timing or cache state enough to move the bug rather than illuminate it.

### What should be done in the future

- Add the first bounded `Panel_EPD` instrumentation patch.
- Rebuild `0079`.
- Re-run the smallest PaperS3 probes before adding any broader upstream delta.

### Code review instructions

- Start with the design plan:
  - [01-panel-epd-instrumentation-and-upstream-diff-plan.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/design/01-panel-epd-instrumentation-and-upstream-diff-plan.md)
- Then review the current choke-point sources:
  - [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)
  - [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)

### Technical details

- Current direct write choke point:
  - `auto buf = &_buf[y * ((_cfg.panel_width + 1) >> 1)];`
  - followed by nibble writes into `buf[idx]`
- Current framebuffer allocation:
  - `_buf = (uint8_t *)heap_caps_aligned_alloc(16, (panel_w * panel_h) / 2, MALLOC_CAP_SPIRAM);`
- Relevant local history lines:
  - `031dbe2` `tweak for PaperS3 refresh rising version 0.2.15`
  - `c899961` `fix PSRAM cache write back for Tab5 + PaperS3`

## Step 2: Add bounded Panel_EPD instrumentation, learn that startup already violates framebuffer bounds, and record the serial-ownership mistake

This step produced two useful outcomes and one avoidable process error.

The first useful outcome came from the first instrumented boot. I added bounded logging around `Panel_EPD::init_intenal()`, `Panel_EPD::writeFillRectPreclipped(...)`, and `Panel_EPD::display(...)` in the nested `M5GFX` repo. On the very first fresh boot, before any Wasm command was sent, the PaperS3 startup path already called `writeFillRectPreclipped(...)` with geometry that does not fit the framebuffer math we expect for a `960x540` panel. The logs showed fullscreen-looking calls with `xe=959`, `ye=539`, but with `w=540` and `h=960`, which drove the computed last byte offset past the end of the `259200`-byte PSRAM framebuffer.

The second useful outcome came from rerunning smaller replay probes after adding those logs. A fresh-boot `wasm replay clear-only` succeeded. Then a fresh-boot `wasm replay frame-no-clear` also succeeded. That means the mere presence of the new driver instrumentation did not reproduce the old crash, and it also means that “any replay draw after boot crashes immediately in `Panel_EPD`” is too broad. The remaining hypothesis space is now narrower and more conditional.

The avoidable process error was mine: I briefly launched two flash/probe jobs in parallel against the same `/dev/ttyACM0` device. That is invalid for this setup and could have polluted the evidence, so I killed both jobs, wrote down the mistake here, and added a standing repo instruction to `AGENTS.md` so future work treats each serial device as single-owner during flash/probe work.

### Prompt Context

**User prompt (verbatim):** "ok"

**Assistant interpretation:** Continue the next PaperS3 probe step after creating the `M5GFX` branch and initial instrumentation commit.

**Inferred user intent:** Run the new instrumented firmware on real PaperS3 hardware, get concrete results, and keep the debugging trail accurate enough to trust later conclusions.

### What I did

- Patched the nested `M5GFX` checkout on branch `codex/papers3-panel-epd-instrumentation` to add bounded logs in:
  - `Panel_EPD::init_intenal()`
  - `Panel_EPD::writeFillRectPreclipped(...)`
  - `Panel_EPD::display(...)`
- Added the reusable probe helper:
  - [flash_and_probe_panel_epd.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/flash_and_probe_panel_epd.sh)
- Flashed the attached PaperS3 and ran:
  - `wasm replay clear-only`
  - `wasm replay frame-no-clear`
- Started a same-boot contamination probe sequence:
  - `wasm run-preflush return-42`
  - `wasm replay clear-only`
- Accidentally launched two flash/probe jobs in parallel against `/dev/ttyACM0`, stopped both, and reran sequentially.
- Added a persistent repo-level instruction in:
  - [AGENTS.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/AGENTS.md)
  explaining that a serial device must be treated as single-owner during flash/probe work.

### Why

- The bounded driver logs were needed to answer whether the first bad state shows up at driver entry, during PSRAM framebuffer math, or only later in asynchronous EPD work.
- The fresh-boot replay probes were the smallest way to separate “driver path is inherently broken” from “something about prior execution contaminates later display work.”
- Writing down the serial-ownership mistake matters because the failure mode is subtle. Port contention can look like a real firmware regression if it is not recorded.

### What worked

- The instrumentation printed the framebuffer pointer, PSRAM placement, stride math, and dirty-range metadata without preventing a normal boot.
- The first fresh-boot `wasm replay clear-only` probe succeeded.
- The first fresh-boot `wasm replay frame-no-clear` probe also succeeded and showed:
  - `display req=(0,0 0x0) upd=(16,16 928x508) range=(16,16)-(943,523) mode=3 ...`
  - `control_execution=success`
- The new logs showed that the driver is already seeing out-of-range write geometry during the startup splash path before any Wasm command is run.
- The serial-ownership rule is now captured in the repo instruction file instead of living only in temporary debugging context.

### What didn't work

- My first attempt to speed things up by running two flash/probe jobs in parallel against the same serial device was wrong. Each process could interfere with the other’s access to `/dev/ttyACM0`, so any result from that attempt would have been suspect.
- The multi-command contamination probe that included `frame-no-clear` truncated the most important part of the serial output, so it had to be rerun in narrower pieces.

### What I learned

- The PaperS3 startup display path itself is already capable of producing obviously out-of-range framebuffer math in `writeFillRectPreclipped(...)`.
- A fresh-boot `clear-only` replay can succeed.
- A fresh-boot `frame-no-clear` replay can also succeed.
- Therefore the old crash cannot currently be explained as “any direct post-boot call into `Panel_EPD` dies.”
- Serial access discipline is not just convenience here; it is part of experimental correctness.

### What was tricky to build

- The hardest part was separating genuine firmware behavior from tooling artifacts. The driver logs were giving useful hints at the same time that the probe workflow itself could be invalidated by two concurrent serial owners. That is exactly the kind of debugging trap that wastes time later if it is not written down immediately.

### What warrants a second pair of eyes

- Whether the startup splash’s `w=540`, `h=960` geometry reflects a rotation/preclipped contract mismatch or a genuinely wrong caller path.
- Whether the contamination bug we are still probing depends on a prior successful WAMR call, a prior display mode change, or some combination of both.

### What should be done in the future

- Finish the clean same-boot contamination probe:
  - `wasm run-preflush return-42`
  - followed by `wasm replay clear-only`
- If contamination still reproduces, compare the `Panel_EPD` logs before and after the successful WAMR call.
- Trace the startup splash caller path to understand why fullscreen-style calls are entering `writeFillRectPreclipped(...)` with swapped-looking geometry.

### Code review instructions

- Review the driver instrumentation in:
  - [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)
- Review the new probe helper in:
  - [flash_and_probe_panel_epd.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/flash_and_probe_panel_epd.sh)
- Review the new standing serial rule in:
  - [AGENTS.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/AGENTS.md)

### Technical details

- First fresh-boot init log:
  - `init panel=960x540 memory=960x540 buf=0x3c17ddc0 ext=1 step=0x3c080ba0 ext_step=1 ...`
- Repeated startup out-of-range logs before console:

## Step 9: Add runtime-pool telemetry, rerun the strict no-module-load control, and correct the allocator model

This step tightened two important pieces of the story.

The first was experimental. I rebuilt `0079` with new app-side telemetry around the WAMR pool allocator and then reran two PaperS3 sequences on real hardware: a strict control that avoids module load and instantiate entirely, and the known `instantiate-bare-keepalive` repro. The control still succeeded, while the instantiate path still poisoned a later persistent PSRAM write. That preserves the current failure boundary and keeps the repro tied to module instantiation rather than to the shared replay helper itself.

The second was conceptual. I had been discussing “WAMR allocations” mostly in terms of ESP-IDF heap behavior, but the project is not using the system allocator for most runtime objects. [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp) initializes WAMR with `Alloc_With_Pool`, and the `512 KiB` pool buffer is itself allocated once from `MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT` at app startup. That means many of the module-owned structures we care about never show up as fresh ESP-IDF heap allocations during instantiate; they are carved from the already-allocated WAMR pool.

### Prompt Context

**User prompt (verbatim):** "let's try that as scenario as well and write it up in the tasks. Do you know which allocations the current WAMR path has been doing? is there some freertos utils to monitor that and also maybe see if there is some memory corruption when allocating?"

**Assistant interpretation:** Add the no-module-load/no-instantiate PaperS3 control to the active ticket, then gather the smallest useful allocator telemetry that can explain what WAMR is actually allocating during instantiate.

**Inferred user intent:** Replace hand-wavy “maybe it allocates something weird” discussion with explicit evidence, and preserve the reasoning so a later reader can understand why some telemetry tools are more relevant than others.

### What I did

- Extended [wasm_runtime_service.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.h) and [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp) so `wasm status` now prints:
  - `wamr.pool_buffer`
  - `wamr.pool_buffer_external`
  - `wamr.pool_size`
- Extended [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp) so `runtime_mem.*` snapshots now also print:
  - `runtime_mem.wamr_pool_info`
  - `runtime_mem.wamr_pool_total`
  - `runtime_mem.wamr_pool_free`
  - `runtime_mem.wamr_pool_highmark`
- Rebuilt and flashed the attached PaperS3 with `ESP-IDF 5.3.4`.
- Ran the strict control sequence with no module load/instantiate:
  - `wasm replay psram-persistent-init`
  - `wasm replay psram-persistent-touch-sync`
- Ran the instantiate repro sequence with the new telemetry:
  - `wasm status`
  - `wasm replay psram-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch-sync`
- Re-checked the tracked WAMR source snapshots under [wamr-local-debug-snapshots](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots/01-readme.md) against the live ignored vendor files and confirmed they still match.
- Added and ran [check_wamr_snapshot_sync.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/check_wamr_snapshot_sync.sh) so that snapshot-validation step is repeatable instead of living only in shell history.

### Why

- The strict control matters because it corrects a sloppy shorthand. `0079` still initializes the WAMR runtime at app startup, so this is not literally “no WAMR at all.” What it *does* prove is that runtime initialization by itself is not enough to trigger the persistent-PSRAM crash. The dangerous boundary is later, at module load/instantiate.
- The pool telemetry matters because the runtime is using `Alloc_With_Pool`, so generic heap statistics alone were never going to explain most module-owned allocations cleanly.
- Rechecking the tracked WAMR snapshots matters because the vendor source lives under `managed_components/`, which is ignored by the main repo. If the snapshots drifted from the live files, the debugging history would become hard to reproduce later.

### What worked

- The telemetry build compiled cleanly after fixing the `printf` width issue in [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp).
- The strict no-module-load/no-instantiate control still succeeded on real PaperS3 hardware:
  - `persistent_psram_probe.touch_checksum=0x47effc1c`
  - `control_execution=success`
- `wasm status` exposed the runtime pool clearly:
  - `wamr.pool_buffer=0x3c1bd244`
  - `wamr.pool_buffer_external=yes`
  - `wamr.pool_size=524288`
  - `wamr.heap_total=524096`
  - `wamr.heap_free=524024`
  - `wamr.heap_highmark=72`
- The instantiate repro still behaved the same way, but the pool deltas made the allocator picture much clearer:
  - before load: `runtime_mem.wamr_pool_free=524024`
  - after load: `runtime_mem.wamr_pool_free=523608`
  - after instantiate: `runtime_mem.wamr_pool_free=522968`
  - at the same time, linear memory still mapped into internal RAM at `0x3fcb9aa0`
- The tracked WAMR source snapshots still matched the live ignored vendor files exactly, so the current WAMR-side instrumentation remains reproducible from tracked artifacts.

### What didn't work

- The instantiate repro still crashed in the same place:
  - [TouchPersistentPsramProbe(...)](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L261)
  - called from [TouchPersistentPsramProbeWithCacheSync(...)](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L360)
- So the new telemetry improved the model, but it did not move the crash boundary.

### What I learned

- The current “strict control” proves something precise:
  - PaperS3 can still allocate and touch the persistent PSRAM buffer when the runtime is initialized but no Wasm module is loaded or instantiated
  - therefore the contamination boundary still sits at or after module instantiate, not at runtime startup
- The project’s WAMR allocator model is now much clearer:
  - the large runtime pool buffer lives in external RAM
  - module-owned metadata is likely coming from that pool
  - linear memory is still a separate internal-RAM allocation path
- That means plain ESP-IDF heap tracing is not the first tool I should reach for when the question is “what did instantiate allocate inside WAMR?” The pool metrics are a better first-line signal for most runtime-owned objects in this project.

### What was tricky to build

- The main conceptual trap was conflating “WAMR touched the heap” with “ESP-IDF heap telemetry should show every important WAMR allocation.” That is only true when the runtime uses the system allocator. In this project it does not, so the instrumentation had to follow the runtime’s actual allocator boundary rather than the intuitive one.

### What warrants a second pair of eyes

- Whether a truly WAMR-disabled PaperS3 probe firmware is still worth building now that the no-module-load/no-instantiate control already succeeds.
- Whether the next best instrument is:
  - heap tracing for the outer pool-buffer and linear-memory allocations
  - or a smaller PaperS3 control firmware that reduces the surrounding app state further

### What should be done in the future

- Decide whether the next probe should be a truly WAMR-disabled PaperS3 control firmware or a smaller “console + instantiate + PSRAM touch” firmware.
- If the goal is allocator provenance rather than control-path isolation, add heap tracing or hooks only for the outer allocations that still bypass the WAMR pool.

### Code review instructions

- Review the runtime-pool telemetry in:
  - [wasm_runtime_service.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.h)
  - [wasm_runtime_service.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_runtime_service.cpp)
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- Review the updated task framing in:
  - [tasks.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/tasks.md)
- Review the tracked WAMR snapshot artifacts in:
  - [01-readme.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots/01-readme.md)

### Technical details

- Build command:

```bash
unset IDF_PYTHON_ENV_PATH IDF_PATH
source /home/manuel/esp/esp-idf-5.3.4/export.sh >/dev/null
idf.py -C 0079-papers3-wamr-assemblyscript-console build
```

- Strict control probe:

```bash
python ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/serial_probe_sequence.py \
  --port /dev/ttyACM0 \
  --command 'wasm replay psram-persistent-init' \
  --command 'wasm replay psram-persistent-touch-sync'
```

- Instantiate repro with pool telemetry:

```bash
python ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/serial_probe_sequence.py \
  --port /dev/ttyACM0 \
  --command 'wasm status' \
  --command 'wasm replay psram-persistent-init' \
  --command 'wasm instantiate-bare-keepalive return-42' \
  --command 'wasm replay psram-persistent-touch-sync'
```

- Most useful allocator facts from the run:
  - runtime pool buffer: `0x3c1bd244`, external RAM, `524288` bytes
  - pool free before load: `524024`
  - pool free after load: `523608`
  - pool free after instantiate: `522968`
  - linear memory: `0x3fcb9aa0`, `32768` bytes, internal RAM

## Step 10: Write the dedicated WAMR allocator and SPIRAM guide so later debugging does not regress into the wrong model

This step was documentation-heavy, but it closes a real debugging risk. The live conversation had reached a point where "WAMR allocates in SPIRAM, so maybe it conflicts with the heap manager" sounded plausible enough to steer later work in the wrong direction. That is exactly the kind of idea that needs a durable write-up, not just a quick terminal answer.

I created a dedicated design document, [02-wamr-allocator-and-spiram-analysis-for-papers3.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/design/02-wamr-allocator-and-spiram-analysis-for-papers3.md), and wrote it as an intern-facing guide instead of a narrow debugging note. The guide explains the three WAMR allocator modes, the current `Alloc_With_Pool` configuration in `0079`, the default EMS allocator inside the pool, the separate `os_mmap()` path used for linear memory, and the project-local `WASM_MEM_DUAL_BUS_MIRROR=0` override. It also explicitly answers the question that triggered the guide: the current evidence does not point to a naive "WAMR and ESP-IDF are both managing the same SPIRAM objects" conflict.

I also related the key code files directly to that design doc with `docmgr doc relate`, updated the ticket index so the guide is easy to discover, and kept the wording aligned with the latest traced hardware results. That matters because the allocator story has changed over time; this doc needed to reflect the current state, not the earlier rougher hypothesis.
  - `fillrect xs=0 ys=0 xe=959 ye=539 w=540 h=960 ... last=460589 len=259200`
- Successful fresh-boot `frame-no-clear` replay log:
  - `display req=(0,0 0x0) upd=(16,16 928x508) range=(16,16)-(943,523) mode=3 ...`
- Standing process lesson:
  - one `/dev/tty*` device
  - one flash/probe/monitor owner at a time

## Step 3: Reconfirm the actual contamination behavior with cleaner probes and pivot from “does it crash?” to “where exactly does it crash?”

The next slice cleaned up the uncertainty from the earlier truncated multi-command run. I reran the smallest probes one at a time using the new ticket-local script and the single-owner serial rule.

## Step 4: Prove that WAMR cleanup is not the primary trigger by keeping the instantiated module alive

At this point the most important unresolved branch was cleanup. The outer probes had already shown that `wasm_runtime_instantiate(...)` plus the normal cleanup path was enough to poison later PSRAM writes, but that still left one major ambiguity: was the bad state created during instantiate, or during deinstantiate/unload cleanup?

To answer that, I added an explicit keepalive lifecycle mode in the app-side runner. The new command, `wasm instantiate-bare-keepalive <name>`, stops immediately after successful `wasm_runtime_instantiate(...)`, reports success, and intentionally keeps the `wasm_module_t` and `wasm_module_inst_t` alive in process-global state instead of deinstantiating and unloading them. Any later non-keepalive Wasm run first releases that leaked state so the firmware does not accumulate abandoned modules forever.

The first implementation attempt introduced a small but real mistake: I expanded the existing `instantiate-*` command selection in [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp) with a nested ternary, and the expression became error-prone enough that I had to stop and simplify it into a named `invocation_mode` local. That fix matters because this slice is partly about experimental hygiene. When the firmware is already debugging-sensitive, even small command-surface mistakes need to be written down instead of mentally erased.

### Prompt Context

**User prompt (verbatim):** "ok, let's go. we can also put back the atoms3r later on"

**Assistant interpretation:** Continue the active PaperS3 investigation now, keep the diary and commits current, and use the already-attached PaperS3 rather than broadening back out to AtomS3R yet.

**Inferred user intent:** Push the next discriminating experiment immediately and preserve the reasoning trail so the result is useful later.

### What I did

- Added `WasmInvocationMode::InstantiateBareKeepAlive` in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
- Added leaked-state handling in:
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  with:
  - `g_leaked_wasm_module`
  - `g_leaked_module_inst`
  - `ReleaseLeakedWasmState()`
- Added the new command surface in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Rebuilt and flashed the attached PaperS3.
- Ran the same-boot sequence:
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-scratch`

### Why

- This is the cleanest way to isolate instantiate from cleanup without patching WAMR internals further.
- If the crash disappeared in keepalive mode, cleanup would become the prime suspect.
- If the crash survived, instantiate itself would become the active boundary.

### What worked

- The firmware built and flashed cleanly after the command-surface fix.
- The keepalive mode correctly reported:
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=no`
  - `exec_env=no`
  - `executed=no`
  - `invocation_mode=instantiate-bare-keepalive`
- The WAMR memory logs remained visible, still showing an internal-RAM linear-memory block.

### What didn't work

- The keepalive run did **not** prevent the later PSRAM crash.
- Same-boot `wasm instantiate-bare-keepalive return-42` followed by `wasm replay psram-scratch` still panicked with `Cache disabled but cached memory region accessed`.

### What I learned

- Cleanup is not the primary trigger.
- The active boundary is now even tighter: successful module instantiation alone is already sufficient to poison later PSRAM writes on PaperS3.
- This justifies spending the next slice on memory-locality controls rather than more runner-level lifecycle splitting.

### What was tricky to build

- The only meaningful value in this experiment came from keeping it genuinely minimal. If I had changed cleanup semantics and the replay controls at the same time, the result would have been harder to trust.

### What warrants a second pair of eyes

- Whether the leaked module state should be explicitly exposed in status output later, if this mode stays around as a durable debugging tool.
- Whether WAMR init/instantiate code has any hidden board-specific assumptions that are visible on PaperS3 but not AtomS3R.

### What should be done in the future

- Compare post-instantiate writes into internal RAM and PSRAM directly.
- Specifically test a PSRAM buffer allocated **before** WAMR instantiation, so allocator corruption and broader PSRAM/cache poisoning can be separated.

### Technical details

- Key same-boot sequence:
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-scratch`
- Key conclusion:
  - instantiate alone is sufficient for the repro

## Step 5: Use RAM-locality controls to show that internal RAM survives while both new and preallocated PSRAM writes fail

Once cleanup was falsified, the best next question was whether the surviving bug was really allocator metadata corruption or something broader and more physical about PSRAM/cache state on PaperS3. The easiest way to separate those is not another WAMR lifecycle split. It is a set of controlled post-instantiate write probes with different memory locality.

I added four replay controls in [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp):

- `internal-scratch`
- `psram-persistent-init`
- `psram-persistent-touch`
- `psram-persistent-free`

The design is deliberate:

- `internal-scratch` allocates a bounded internal-RAM buffer, writes a deterministic byte pattern, and prints a checksum.
- `psram-persistent-init` allocates a PSRAM buffer and keeps it alive across commands.
- `psram-persistent-touch` writes into that same already-allocated PSRAM buffer later in the same boot.

The first attempt at `internal-scratch` taught an immediate lesson. I initially reused the full display-sized `259200`-byte scratch size, but that was unrealistic for the internal heap and just failed allocation. That was not a useful control, so I cut the internal probe down to `32 KiB`. That made it a real discriminating test instead of a predictable allocator failure.

### What I did

- Extended non-display replay commands in:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Updated command help and headless command gating in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Rebuilt and flashed the attached PaperS3.
- Ran:
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay internal-scratch`
- Then ran a fresh same-boot persistent-PSRAM sequence:
  - `wasm replay psram-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch`
- Decoded the persistent-touch crash against the exact ELF.

### Why

- If internal RAM writes also failed after instantiate, the bug would look like a much broader memory/cpu-state failure.
- If only newly allocated PSRAM failed, heap metadata corruption would stay near the top.
- If a PSRAM buffer allocated **before** instantiate also failed, then “bad allocator metadata for later allocations” would no longer explain the behavior well.

### What worked

- After resizing it to `32 KiB`, `internal-scratch` became a valid control and succeeded after instantiate:
  - `internal_probe.external=no`
  - `internal_probe.bytes=32768`
  - stable checksum
  - `control_execution=success`
- `psram-persistent-init` succeeded and confirmed the persistent buffer was external RAM.
- The persistent-touch crash decoded cleanly to:
  - [wasm_replay_control.cpp:179](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L179)
  inside `TouchPersistentPsramProbe(...)`

### What didn't work

- `psram-persistent-touch` still crashed after instantiate-bare-keepalive, even though that PSRAM buffer had been allocated before WAMR instantiation.
- So the surviving bug is not well explained by allocator metadata for *new* PSRAM allocations alone.

### What I learned

- Internal RAM remains writable after WAMR instantiation on PaperS3.
- WAMR’s own logged linear-memory block is still in internal RAM, not PSRAM.
- Newly allocated PSRAM writes fail after instantiate.
- Preallocated PSRAM writes also fail after instantiate.

That combination is the strongest evidence so far for a PaperS3-specific PSRAM/cache poisoning problem rather than a simple heap-allocation bug.

### What was tricky to build

- The first internal control was bogus because it over-assumed the size of a reasonable internal allocation on this board. That failure was worth keeping in the diary because it shows the difference between “a probe failed because the bug is real” and “a probe failed because the probe itself was poorly sized.”

### What warrants a second pair of eyes

- Whether the next move should be WAMR-side cache/state instrumentation or a lower-level ESP-IDF/PSRAM state probe around instantiate.
- Whether PaperS3 board init is putting the PSRAM/cache subsystem into a mode that AtomS3R does not use, even before any WAMR call.

### What should be done in the future

- Inspect the active WAMR instantiate path for operations that could perturb PSRAM/cache state without directly allocating guest memory in PSRAM.
- Consider a minimal board-level PSRAM/cache control project if the next round inside WAMR does not expose the state transition.

### Technical details

- Successful internal control after instantiate:
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay internal-scratch`
- Failing persistent PSRAM control after instantiate:
  - `wasm replay psram-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch`
- Crash decode:
  - [wasm_replay_control.cpp:179](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L179)
  - [wasm_replay_control.cpp:276](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L276)
  - [wasm_command.cpp:181](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp#L181)

## Step 6: Probe cache-enabled and heap-integrity state directly, then compare the PaperS3 and AtomS3R sdkconfig slices

Once the preallocated-PSRAM probe still crashed, the natural next suspicion was “maybe the external cache is still effectively off, or maybe the SPIRAM heap is already corrupt but we only see it when we touch a user buffer.” Rather than keep inferring that from panics, I added direct state probes around the instantiate boundary and around the replay controls.

The result is useful because it is a negative result, not just more noise. Right before the failing `psram-persistent-touch` write, the firmware still reports `flash_cache_enabled=yes`, `internal_heap_ok=yes`, and `spiram_heap_ok=yes`. That does not prove everything about the cache subsystem is healthy, but it does rule out two simpler stories: “WAMR left flash/extmem cache globally disabled” and “the SPIRAM heap is already obviously corrupt before the first user write.”

### Prompt Context

**User prompt (verbatim):** "go at it. keep git commits and specific branches for all the stuff thirdparty that you modify as you investigate, so that we cna properly track them down and identify them later on. Continue keeping a detailed diary."

**Assistant interpretation:** Continue the active debugging slice, preserve traceability for third-party changes, and keep the ticket diary detailed enough to reconstruct the investigation.

**Inferred user intent:** Narrow the remaining PaperS3-specific failure without losing the provenance of code changes or the reasoning trail.

### What I did

- Added direct state probes in:
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Those probes log:
  - `spi_flash_cache_enabled()`
  - `heap_caps_check_integrity(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT, false)`
  - `heap_caps_check_integrity(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, false)`
  - internal and SPIRAM free-space snapshots
- Rebuilt and flashed the attached PaperS3.
- Reran the same-boot persistent PSRAM sequence:
  - `wasm replay psram-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch`
- Compared the relevant `sdkconfig` slices for:
  - [0079 sdkconfig](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/sdkconfig)
  - [0081 sdkconfig](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/sdkconfig)

### Why

- If `spi_flash_cache_enabled()` flipped to `no` after instantiate, that would be a much cleaner explanation than the current indirect crash signal.
- If `heap_caps_check_integrity()` failed for SPIRAM before the explicit write, the remaining bug would look more like allocator corruption than cache-path poisoning.
- Comparing `sdkconfig` against the working AtomS3R control is the cheapest way to rule out a boring project-config mismatch before digging deeper into board bring-up or cache internals.

### What worked

- The new state probes compiled and ran cleanly on PaperS3.
- The failing sequence now shows a clear pre-crash state:
  - `replay_mem.flash_cache_enabled=yes`
  - `replay_mem.internal_heap_ok=yes`
  - `replay_mem.spiram_heap_ok=yes`
- The instantiate boundary also stays nominal:
  - `runtime_mem.flash_cache_enabled=yes`
  - `runtime_mem.internal_heap_ok=yes`
  - `runtime_mem.spiram_heap_ok=yes`
- The `sdkconfig` comparison showed no meaningful PSRAM/cache-mode mismatch between the failing PaperS3 project and the working AtomS3R control project.

### What didn't work

- The new probes did **not** reveal a simple stuck-disable state or obvious heap corruption before the crash.
- `psram-persistent-touch` still crashes immediately after those probes report a healthy-looking state.

### What I learned

- The remaining bug is subtler than “external cache is off” as reported by `spi_flash_cache_enabled()`.
- The remaining bug is also subtler than “SPIRAM heap metadata is already obviously broken” as reported by `heap_caps_check_integrity()`.
- The PaperS3 vs AtomS3R difference is not explained by the obvious PSRAM/cache `sdkconfig` knobs:
  - both use octal PSRAM
  - both run PSRAM at `40 MHz`
  - both keep `32 KiB` data cache and `32 B` data-cache lines
  - both disable XIP/fetch/rodata from PSRAM

### What was tricky to build

- These probes are useful only if they run before the crash and do not themselves create the crash. That meant keeping them bounded and lightweight even though they touch system state and heap APIs in a debugging-sensitive firmware.
- The WAMR component under `managed_components/` is not a separately branched nested repo in this workspace. So unlike the nested `M5GFX` repo, those third-party edits have to stay traceable through focused commits in the main repo rather than through a dedicated vendor branch. I wrote that down explicitly here because it affects how future reviewers should interpret the provenance of WAMR-side patches.

### What warrants a second pair of eyes

- Whether the next best discriminator is a lower-level cache/msync probe on the PSRAM buffer, or a build that removes app-owned PaperS3/M5 bring-up entirely.
- Whether the one remaining config difference that did show up in the quick comparison, flash size (`16 MB` on PaperS3 vs `8 MB` on AtomS3R), is worth any dedicated follow-up at all.

### What should be done in the future

- Build a PaperS3 control image that skips app-owned M5 display bring-up entirely and rerun the same WAMR + PSRAM probes there.
- If the crash survives that control build, continue with lower-level PSRAM/cache probes such as `esp_cache_msync(...)` or a dedicated minimal PaperS3 WAMR probe project.

### Code review instructions

- Start with:
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Then review the project-control comparison targets:
  - [0079 sdkconfig](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/sdkconfig)
  - [0081 sdkconfig](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0081-atoms3r-wamr-probe-console/sdkconfig)
- To reproduce the key sequence:
  - `wasm replay psram-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch`

### Technical details

- Key pre-crash state on the failing PaperS3 run:
  - `replay_mem.flash_cache_enabled=yes`
  - `replay_mem.internal_heap_ok=yes`
  - `replay_mem.spiram_heap_ok=yes`
- Key post-instantiate state:
  - `runtime_mem.flash_cache_enabled=yes`
  - `runtime_mem.internal_heap_ok=yes`
  - `runtime_mem.spiram_heap_ok=yes`
- `sdkconfig` control result:
  - no meaningful PSRAM/cache-mode mismatch between `0079` and `0081`
  - notable difference only in flash size (`16 MB` vs `8 MB`)

Two of those reruns changed the shape of the problem in a helpful way. First, a fresh-boot `wasm replay frame-no-clear` succeeded. That means the current crash is not “any non-clear draw reaches `Panel_EPD` and dies.” Second, a same-boot sequence of `wasm run-preflush return-42` followed by `wasm replay clear-only` still crashed immediately after the successful Wasm call. That kept the contamination hypothesis alive while also falsifying the broader idea that the PaperS3 replay path is simply always broken after boot.

That combination matters because it gives us a cleaner experimental split:

- fresh boot, no prior Wasm:
  - `clear-only` succeeds
  - `frame-no-clear` succeeds
- same boot, after successful non-drawing Wasm:
  - `clear-only` crashes

So the next useful action is no longer another high-level architecture argument. It is to decode the current crash against the exact `0079` ELF and identify whether the first post-WAMR failure is in the direct framebuffer write path, the cache writeback path, or a mode/queue transition inside the EPD backend.

### Prompt Context

**User prompt (verbatim):** "where are we at in our investigation ?"

**Assistant interpretation:** Summarize the current state accurately after the new probes and use that summary to choose the next concrete debugging step.

**Inferred user intent:** Understand what is already proven, what is still open, and what the next non-hand-wavy action should be.

### What I did

- Reran fresh-boot `wasm replay frame-no-clear` using the instrumented firmware and the ticket-local probe script.
- Reran the same-boot contamination sequence:
  - `wasm run-preflush return-42`
  - `wasm replay clear-only`
- Verified again that:
  - `return-42` succeeds
  - `frame-no-clear` succeeds on a fresh boot
  - `clear-only` succeeds on a fresh boot
  - `clear-only` crashes after successful non-drawing Wasm execution in the same boot
- Updated the task list so the next active work is:
  - decode the crash addresses against the current ELF
  - add one tighter first-entry post-WAMR replay probe
  - compare the fresh-boot-success and contaminated-crash paths

### Why

- The fresh-boot vs same-boot distinction is now the most informative variable in the investigation.
- It is no longer enough to say “the crash is in Panel_EPD somewhere.” We need exact symbol-to-line mapping for the current build so the next patch is attached to real code addresses.

### What worked

- The sequential reruns produced clean, interpretable evidence instead of the ambiguous earlier truncated run.
- `frame-no-clear` succeeding was especially valuable because it removed one overly broad theory from the table.
- The same-boot contamination sequence still reproduces, so the main bug remains live and testable.

### What didn't work

- The contamination crash output still does not include the extra `lgfx_epd_dbg` line I want immediately before the failing operation, so the current trace still needs source-level address decoding.

### What I learned

- The bug is more conditional than it first looked.
- The PaperS3 replay path is capable of working on a fresh boot even with the suspicious startup splash behavior still present.
- A successful non-drawing Wasm call is enough to push the later `clear-only` replay into the bad state.

### What was tricky to build

- The hard part here was resisting the temptation to over-generalize from one success or one crash. The new evidence only became trustworthy after the probes were reduced to clean fresh-boot and same-boot cases.

### What warrants a second pair of eyes

- Whether the post-WAMR crash is more likely to be in `Panel_EPD::display(...)` cache handling than in `writeFillRectPreclipped(...)`, given that fresh-boot replay commands can succeed.
- Whether the startup splash’s out-of-range geometry is a red herring, a separate bug, or an important clue about mode/rotation state.

### What should be done in the future

- Decode the contamination crash addresses against the exact `papers3_wamr_assemblyscript_console.elf`.
- Add one narrower probe at the first `Panel_EPD` entry after `return-42`.
- Compare those logs with the corresponding fresh-boot-success path.

### Code review instructions

- Review the updated active tasks in:
  - [tasks.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/tasks.md)
- Review the current driver instrumentation in:
  - [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)

### Technical details

- Fresh-boot `frame-no-clear` success:
  - `control_example=frame-no-clear`
  - `queued_commands=8`
  - `control_execution=success`
- Same-boot contamination repro:
  - `wasm run-preflush return-42`
  - `return_value=42`
  - followed by `wasm replay clear-only`
  - then `Cache disabled but cached memory region accessed`

## Step 4: Decode the exact crash, then prove that the success path and crash path reach `Panel_EPD` with the same logged inputs

This step is the first one that materially changes the level of confidence in the remaining theory.

First, I decoded the current contamination crash against the exact built ELF instead of continuing to reason from raw PC values. That reconfirmed the same high-level path:

- `Panel_EPD::writeFillRectPreclipped(...)`
- `LGFXBase::fillScreen(...)`
- `PaperCanvasScreenClear(...)`
- `FlushWasmHostFrame(...)`
- `RunWasmReplayControlExample(...)`

Second, I added tighter app-side instrumentation in the `0079` project:

- `papers3_canvas.cpp`
  - logs frame lifecycle around `BeginFrameIfNeeded`, `PaperCanvasResetFrame`, `PaperCanvasPresent`
  - logs `M5.Display.getEpdMode()`
- `wasm_host_api.cpp`
  - logs each flushed host command with its index and parameters

That already showed an important detail: the post-WAMR crash happens after the replay control path has successfully:

- reset the app-side frame state
- started a new frame
- switched `epd_mode` from `1` to `3`
- begun the `screen-clear` operation

But startup still consumed most of the driver log budget before the replay phase, so I added a tiny debug hook in the nested `M5GFX` repo:

- `debugResetPanelEpdLogBudgets(fillrect_budget, display_budget)`

Then I called that hook at the start of `RunWasmReplayControlExample(...)` so every replay run gets a fresh driver log budget.

That finally gave the clean comparison we needed:

- fresh boot `wasm replay clear-only`:
  - succeeds
  - logs `fillrect xs=0 ys=0 xe=959 ye=539 w=960 h=540 raw=255 mode=3 ... last=259199 len=259200 busy=0`
- same boot `wasm run-preflush return-42` then `wasm replay clear-only`:
  - crashes
  - logs the *same* `fillrect` parameters immediately before the panic:
    - `xs=0 ys=0 xe=959 ye=539`
    - `w=960 h=540`
    - `mode=3`
    - `buf=0x3c17ddc0`
    - `last=259199 len=259200`
    - `busy=0`

That is the key result of the current slice. The first concrete divergence is **not** in the logged app-side frame lifecycle, and it is **not** in obviously wrong `Panel_EPD` rectangle math for the crashing replay call. The corruption must exist below the fields we are currently observing, or in a lower-level cache/PSRAM/driver interaction that leaves these high-level parameters unchanged.

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Keep pushing the active `ESP-41` debugging slice forward instead of stopping at the last intermediate result.

**Inferred user intent:** Do the next real debugging work, preserve it in the ticket, and reduce the ambiguity around the PaperS3 contamination bug.

### What I did

- Decoded the current contamination crash with `xtensa-esp32s3-elf-addr2line` against:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/build/papers3_wamr_assemblyscript_console.elf`
- Added app-side instrumentation in:
  - [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
  - [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
- Added a driver-side replay-budget reset hook in:
  - [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)
- Called that hook from:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Reran:
  - same-boot contamination repro:
    - `wasm run-preflush return-42`
    - `wasm replay clear-only`
  - matching fresh-boot control:
    - `wasm replay clear-only`

### Why

- Address decoding was needed to keep the next code changes tied to exact lines rather than to a remembered stack pattern.
- The app-side logs were needed because the current hypothesis had become too vague: “somewhere below WAMR execution, PaperS3 is poisoned.” We needed to know whether the app-side frame lifecycle already diverged before the crash.
- The driver-side replay-budget reset was needed because startup noise was still masking the first driver call that actually mattered.

### What worked

- The crash was mapped cleanly back to `Panel_EPD::writeFillRectPreclipped(...)`.
- The app-side replay logs showed that the crashing path has a normal-looking command sequence:
  - `screen-clear`
  - `begin-frame-enter`
  - `begin-frame-started`
- The replay-budget reset finally exposed the first post-WAMR crashing `fillrect` call.
- The fresh-boot success path and post-WAMR crash path now have a real side-by-side comparison at the driver entry point.

### What didn't work

- Increasing the raw `Panel_EPD` budgets alone was not enough. Startup display activity still consumed too much of the budget before replay began.
- The currently logged fields still do not show the root cause. They only prove that the obvious geometry/state candidates are *not* where the first divergence appears.

### What I learned

- The crash path is more subtle than “bad rectangle” or “wrong mode.”
- The post-WAMR crashing replay and the fresh-boot successful replay both reach `Panel_EPD` with the same visible parameters.
- That makes the remaining likely suspects lower-level than the current probes:
  - cache state
  - PSRAM/cache coherency
  - hidden driver/task state
  - memory corruption that does not change the top-level call parameters

### What was tricky to build

- The main challenge was getting the instrumentation budget to line up with the lifecycle of the board. On PaperS3, startup itself exercises the display path heavily, so a normal “log the first N calls” approach can completely miss the phase we actually care about unless it can be reset.

### What warrants a second pair of eyes

- Whether the next slice should move downward into cache/PSRAM-oriented instrumentation rather than adding more app-side logs.
- Whether the startup splash out-of-range issue and the post-WAMR crash are two separate bugs or a shared sign that `Panel_EPD` state assumptions are fragile.

### What should be done in the future

- Decode the latest post-WAMR crash addresses against the newest ELF after the reset-hook patch, just to keep the line references exact for this build.
- Add one lower-level probe around the actual byte-write loop or around cache maintenance, not just around replay command dispatch.
- Consider checking whether the PSRAM framebuffer contents or cache-writeback behavior differ before and after a successful `return-42` run.

### Code review instructions

- Review the app-side probe additions in:
  - [papers3_canvas.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp)
  - [wasm_host_api.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp)
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Review the driver-side budget reset hook in:
  - [Panel_EPD.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp)

### Technical details

- Exact crash mapping for the contamination repro:
  - `Panel_EPD::writeFillRectPreclipped(...)` at line 431 on the earlier decoded build
  - via `PaperCanvasScreenClear(...)` at line 135 in `papers3_canvas.cpp`
- Fresh-boot success `fillrect` log:
  - `fillrect xs=0 ys=0 xe=959 ye=539 w=960 h=540 raw=255 mode=3 ... last=259199 len=259200 busy=0`
- Post-WAMR crash `fillrect` log:
  - `fillrect xs=0 ys=0 xe=959 ye=539 w=960 h=540 raw=255 mode=3 ... last=259199 len=259200 busy=0`
- App-side replay state before both cases:
  - `canvas_init=1`
  - `frame_active` transitions `0 -> 1`
  - `present_mode=1`
  - `epd_mode` transitions `1 -> 3`

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Keep pushing the active `ESP-41` slice forward, but stop spending cycles only inside `Panel_EPD` if a sharper control experiment can split the problem faster.

**Inferred user intent:** Move the investigation from "the crash happens in `Panel_EPD`" to "what class of state is actually poisoned after a successful Wasm run on PaperS3?"

### What I did

- Added a non-display control path in:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- The new control example is:
  - `psram-scratch`
- That control:
  - allocates a PSRAM-backed scratch buffer with `heap_caps_aligned_alloc(...)`
  - verifies that the pointer lives in external RAM with `esp_ptr_external_ram(...)`
  - performs a nested nibble-write loop that mirrors the byte-write shape used by the EPD clear path
  - prints a simple checksum so we can tell whether a fresh-boot run completed normally
- Rebuilt:
  - `idf.py -C /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console build`
- Probed the attached PaperS3 in two modes:
  - same boot:
    - `wasm run-preflush return-42`
    - `wasm replay psram-scratch`
  - fresh boot:
    - `wasm replay psram-scratch`

### Why

- Up to this point, the strongest visible choke point was still `Panel_EPD::writeFillRectPreclipped(...)`, but that did not prove the bug was specific to `M5GFX` or to the e-ink backend.
- A PSRAM-only control path is a better split:
  - if it succeeds after `return-42`, the contamination remains display-specific
  - if it crashes after `return-42`, then the active problem is broader than `Panel_EPD`

### What worked

- The fresh-boot control succeeded cleanly:
  - `control_example=psram-scratch`
  - `queued_commands=0`
  - `control_execution=success`
  - `psram_probe.buffer=0x3c23d430`
  - `psram_probe.external=yes`
  - `psram_probe.bytes=259200`
  - `psram_probe.checksum=0x2fed4bd5`
- That proves the control path itself is valid and that plain PSRAM scratch writes are not inherently broken on PaperS3.

### What didn't work

- The same-boot sequence:
  - `wasm run-preflush return-42`
  - `wasm replay psram-scratch`
  crashed with the same class of panic:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
- The panic occurred before the control path could print a checksum, which means the scratch-write loop is enough to reproduce the surviving contamination even without entering `Panel_EPD` or touching `M5GFX`.

### What I learned

- This is the most important narrowing result of the current ticket so far.
- The surviving PaperS3 bug is broader than the e-ink backend.
- `Panel_EPD` is still a visible crash choke point for one family of repros, but it is no longer the best top-level explanation.
- The leading theory should now shift to:
  - post-WAMR PSRAM/cache poisoning on PaperS3
  - or a broader PaperS3 memory/cache state corruption that makes later PSRAM writes unsafe

### What was tricky to build

- The control needed to be similar enough to the EPD clear path to be meaningful, but not so similar that it quietly dragged `M5GFX` back into the picture.
- The easiest mistake here would have been a "PSRAM control" that still called the display stack indirectly. The implemented path stays inside `wasm_replay_control.cpp`, heap allocation, and raw buffer writes.

### What warrants a second pair of eyes

- The WAMR ESP-IDF memory/cache path in:
  - [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)
- The runtime/platform config in:
  - [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)
- Any PaperS3-specific board configuration that changes how PSRAM/cache access behaves differently from the AtomS3R control board

### What should be done in the future

- Decode the new `psram-scratch` same-boot crash against the exact current ELF so the next slice stays anchored to current addresses.
- Inspect the WAMR ESP-IDF platform code for cache or memory operations that run after a successful Wasm call and could leave later external-RAM accesses in a bad state.
- Only after that decide whether more `Panel_EPD` instrumentation is still the highest-value next move.

### Code review instructions

- Review the new control path in:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Read it against the previous `clear-only` and `frame-no-clear` helpers so the distinction stays clear:
  - the new probe allocates PSRAM and writes it directly
  - it does not queue display commands
  - it does not depend on `M5GFX`

### Technical details

- Fresh-boot success command:
  - `wasm replay psram-scratch`
- Fresh-boot success output:
  - `psram_probe.buffer=0x3c23d430`
  - `psram_probe.external=yes`
  - `psram_probe.bytes=259200`
  - `psram_probe.checksum=0x2fed4bd5`
  - `control_example=psram-scratch`
  - `queued_commands=0`
  - `control_execution=success`
- Same-boot contamination command sequence:
  - `wasm run-preflush return-42`
  - `wasm replay psram-scratch`
- Same-boot contamination result:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
  - panic before checksum output from the scratch probe

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Keep pushing the PaperS3 split further and test whether display initialization is actually required for the post-WAMR PSRAM contamination.

**Inferred user intent:** Do the next controlled experiment, preserve the result in the ticket, and keep reducing the live theory instead of staying attached to the older `Panel_EPD` hypothesis.

### What I did

- Inspected the active WAMR ESP-IDF platform files:
  - [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)
  - [shared_platform.cmake](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/shared_platform.cmake)
- Rechecked the PaperS3 and AtomS3R PSRAM configs and noted that AtomS3R succeeds with its own simpler display path and non-PSRAM canvas config, while PaperS3 still starts with the full PSRAM/display environment.
- Used the existing headless PaperS3 build path from `ESP-39` to run a stricter control:
  - headless build uses `sdkconfig.defaults;sdkconfig.headless`
  - `host_api.display=disabled`
  - only 2 host symbols are registered
- The first headless probe exposed a small local bug:
  - `wasm replay psram-scratch` was still rejected by `wasm_command.cpp` with `display host API is disabled in this build`
  - even though `RunWasmReplayControlExample(...)` already allowed `psram-scratch` without display support
- Fixed that command gate in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Rebuilt and reflashed the headless PaperS3 variant.
- Reran the same-boot sequence on the headless build:
  - `wasm run-preflush return-42`
  - `wasm replay psram-scratch`
- Decoded the resulting headless crash against:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/build-headless/papers3_wamr_assemblyscript_console.elf`

### Why

- The earlier `psram-scratch` result already proved the bug is broader than `Panel_EPD`, but the headful PaperS3 build still performs board/display initialization during boot.
- The headless build is the right next split:
  - if the crash disappears there, display initialization is part of the contamination precondition
  - if the crash remains there, the active problem is broader than both `Panel_EPD` and display initialization

### What worked

- The headless build booted correctly and clearly showed the reduced host surface:
  - `0079_host_api: Registered 2 host symbols for module 'host'`
- `wasm run-preflush return-42` still succeeded in the headless build.
- After the command-gate fix, `wasm replay psram-scratch` really ran in the headless build rather than being rejected early.
- The headless same-boot crash decoded cleanly to:
  - [wasm_replay_control.cpp:88](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L88)
  - inside `RunPsramScratchProbe(...)`
  - at the initial `std::memset(buffer, 0, kScratchBufferBytes);`

### What didn't work

- The headless build did **not** eliminate the contamination.
- Same-boot on headless PaperS3:
  - `wasm run-preflush return-42`
  - `wasm replay psram-scratch`
  still crashes with:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
- So display initialization is **not** required for the live repro.

### What I learned

- This is another major narrowing step.
- The active bug is no longer well described as:
  - `Panel_EPD` specific
  - `M5GFX` specific
  - display initialization specific
- The current best description is:
  - successful WAMR execution on PaperS3 poisons later PSRAM access in the same boot
  - and the earliest proven crashing operation in the new control path is a plain `memset(...)` over a PSRAM buffer
- That means the next slice should spend more time in:
  - WAMR/platform memory handling
  - PaperS3 board-level PSRAM/cache assumptions
  than in higher-level display code

### What was tricky to build

- The headless control path was nearly correct already, but the CLI gate in `wasm_command.cpp` still assumed that all `wasm replay ...` examples required the display API.
- That would have produced a false negative if left unfixed, because the headless build would have looked "safe" only because the control was blocked before execution.

### What warrants a second pair of eyes

- Whether anything in the PaperS3 board setup outside our host API path can leave PSRAM/cache behavior meaningfully different from AtomS3R even before any explicit display replay.
- Whether the WAMR runtime cleanup path needs additional platform-specific normalization after a successful Wasm call on PaperS3.

### What should be done in the future

- Inspect the WAMR/platform path with the new headless result in mind and look specifically for PaperS3-relevant external-memory side effects.
- Compare PaperS3 and AtomS3R runtime/board setup for any board-level PSRAM/cache differences that survive when the display host API is compiled out.
- Only return to `Panel_EPD`-focused work if a later experiment shows a second EPD-specific problem layered on top of the broader PSRAM contamination.

### Code review instructions

- Review the headless replay-gate fix in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Review the headless-compatible control path in:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
- Cross-check the decoded headless panic against the exact ELF used in the probe, not the normal build ELF.

### Technical details

- Headless build boot evidence:
  - `0079_host_api: Registered 2 host symbols for module 'host'`
- Headless same-boot command sequence:
  - `wasm run-preflush return-42`
  - `wasm replay psram-scratch`
- Headless same-boot result:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
- Decoded headless crash addresses:
  - `0x4200bfd1` -> `RunPsramScratchProbe(...)` at `wasm_replay_control.cpp:88`
  - `0x4200c16f` -> `RunWasmReplayControlExample(...)` at `wasm_replay_control.cpp:150`
  - `0x4200b20f` -> `CmdWasm(...)` at `wasm_command.cpp:128`
- The crashing source line is:
  - `std::memset(buffer, 0, kScratchBufferBytes);`

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Keep reducing the WAMR-side search space. After the headless PSRAM result, the next high-value split is lifecycle: does the contamination require actual guest execution, or is instantiate/cleanup already enough?

**Inferred user intent:** Do another concrete experiment, not just more reading, and preserve the result in the diary with enough detail to reconstruct why the theory changed.

### What I did

- Added a new invocation mode to the runner in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- The new mode is:
  - `WasmInvocationMode::InstantiateOnly`
- In that mode, the runner still:
  - loads the embedded module
  - instantiates it
  - looks up the target export
  - creates the exec env
  - tears everything back down
- But it does **not**:
  - call `wasm_runtime_call_wasm(...)`
  - call `wasm_runtime_call_wasm_a(...)`
  - flush any host frame
- Added a matching console command in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
  - command: `wasm instantiate-only <name>`
- Rebuilt and reflashed the headless PaperS3 build.
- Ran this same-boot sequence:
  - `wasm instantiate-only return-42`
  - `wasm replay psram-scratch`
- Decoded the new crash against the headless ELF.

### Why

- The earlier headless `return-42 -> psram-scratch` crash still left one important ambiguity:
  - maybe the contamination happens only after actual guest bytecode execution
- `instantiate-only` is the smallest useful next split because it preserves the runtime bring-up/teardown path while removing guest execution and host-import execution entirely.

### What worked

- `wasm instantiate-only return-42` succeeded cleanly in the headless build.
- The result printed the new mode explicitly:
  - `invocation_mode=instantiate-only`
  - `execution=success`
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=yes`
  - `exec_env=yes`
  - `executed=no`
- That proves the lifecycle probe is actually exercising the intended boundary.

### What didn't work

- The next command in the same boot:
  - `wasm replay psram-scratch`
  still crashed with:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
- So `call_wasm` is **not** required for the contamination.

### What I learned

- This is a major shift in the theory.
- The active bug is now best described as:
  - WAMR instantiate/teardown on PaperS3 is already sufficient to poison later PSRAM writes in the same boot
- That means the next likely investigation areas are even narrower:
  - WAMR linear-memory allocation / instantiate path
  - exec-env creation / destruction
  - module deinstantiate / unload cleanup
- It is now much harder to blame:
  - guest bytecode behavior
  - host imports
  - display code
  - or even guest execution itself

### What was tricky to build

- The main trap here was making an "instantiate-only" mode that still exercised a meaningful portion of the runtime lifecycle instead of becoming a fake success path.
- Stopping only after `exec_env` creation is a better cut than stopping after module load, because it keeps more of the real runtime setup/cleanup in play while still excluding actual execution.

### What warrants a second pair of eyes

- The WAMR instantiate/deinstantiate path in:
  - [wasm_memory.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c)
  - [wasm_runtime_common.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_runtime_common.c)
  - [wasm_exec_env.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_exec_env.c)
- Any PaperS3 board initialization that changes PSRAM/cache invariants before we ever enter `wasm replay psram-scratch`

### What should be done in the future

- Inspect the instantiate and cleanup path in WAMR now that guest execution has been removed from the causal chain.
- Compare the same instantiate-only probe on AtomS3R if needed, just to confirm that this narrow lifecycle step remains PaperS3-specific.
- If we need one more code experiment locally, add an even earlier probe that stops after `wasm_runtime_instantiate(...)` but before `wasm_runtime_create_exec_env(...)`.

### Code review instructions

- Review the new invocation-mode plumbing in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- Review the user-facing command surface in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Confirm that `instantiate-only` does not call the guest and does not flush host work.

### Technical details

- Headless same-boot command sequence:
  - `wasm instantiate-only return-42`
  - `wasm replay psram-scratch`
- Successful instantiate-only output:
  - `invocation_mode=instantiate-only`
  - `execution=success`
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=yes`
  - `exec_env=yes`
  - `executed=no`
- Same-boot crash result:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
- Decoded crash addresses:
  - `0x4200c0b5` -> `RunPsramScratchProbe(...)` at `wasm_replay_control.cpp:88`
  - `0x4200c253` -> `RunWasmReplayControlExample(...)` at `wasm_replay_control.cpp:150`
  - `0x4200b29c` -> `CmdWasm(...)` at `wasm_command.cpp:154`

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Keep narrowing the lifecycle boundary while the result remains cheap to test locally. After `instantiate-only` still reproduced the bug, the next clean split is whether `wasm_runtime_create_exec_env(...)` is required.

**Inferred user intent:** Continue with another concrete experiment, keep the diary detailed, and stop only when the lifecycle boundary is significantly tighter than before.

### What I did

- Extended the invocation-mode enum again in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
- Added a new mode in:
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  - `WasmInvocationMode::InstantiateNoExecEnv`
- In that mode, the runner still:
  - loads the module
  - instantiates it
  - looks up the export
  - validates the signature
- But it does **not**:
  - create an exec env
  - call guest code
  - flush host work
- Added the matching console command in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
  - command: `wasm instantiate-no-execenv <name>`
- Rebuilt and reflashed the headless PaperS3 build.
- Ran this same-boot sequence:
  - `wasm instantiate-no-execenv return-42`
  - `wasm replay psram-scratch`
- Decoded the new crash against the headless ELF.

### Why

- The previous `instantiate-only` result proved that guest execution was not required, but it still left one remaining runner-side setup step in play:
  - `wasm_runtime_create_exec_env(...)`
- This mode isolates that step cleanly without reaching into WAMR internals yet.

### What worked

- `wasm instantiate-no-execenv return-42` succeeded cleanly.
- The output clearly showed the intended boundary:
  - `invocation_mode=instantiate-no-execenv`
  - `execution=success`
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=yes`
  - `exec_env=no`
  - `executed=no`
- That confirms the probe did not accidentally drift into the exec-env path.

### What didn't work

- The next command in the same boot:
  - `wasm replay psram-scratch`
  still crashed with:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`

### What I learned

- `wasm_runtime_create_exec_env(...)` is not required for the contamination either.
- The active boundary is now at or before:
  - module instantiation
  - export lookup/signature checks
  - deinstantiate/unload cleanup after those steps
- This sharply reduces the value of staying in our higher-level runner logic.
- The next serious debugging value is now likely in direct WAMR internals, especially:
  - `wasm_runtime_instantiate(...)`
  - `wasm_runtime_deinstantiate(...)`
  - linear-memory setup/teardown in `wasm_memory.c`

### What was tricky to build

- The important implementation detail was to stop *after* lookup/signature validation but *before* exec-env creation. Stopping too early would have made the mode less comparable to the previous probes.
- I kept the command surface explicit so the serial logs show exactly which lifecycle boundary was tested.

### What warrants a second pair of eyes

- Whether one more local split before export lookup is worth it, or whether the better use of time is now code reading and targeted instrumentation in WAMR internals.
- Whether export lookup itself is too trivial to matter compared to instantiate/deinstantiate and therefore not worth another probe mode.

### What should be done in the future

- Inspect WAMR instantiate/deinstantiate internals directly now that exec-env creation is ruled out as a required step.
- If we still need another experimental split, the next one should likely stop immediately after `wasm_runtime_instantiate(...)` and before export lookup.

### Code review instructions

- Review the new invocation mode plumbing in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
- Review the new console surface in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Confirm that `instantiate-no-execenv` reports `exec_env=no` and `executed=no`.

### Technical details

- Headless same-boot command sequence:
  - `wasm instantiate-no-execenv return-42`
  - `wasm replay psram-scratch`
- Successful pre-exec-env output:
  - `invocation_mode=instantiate-no-execenv`
  - `execution=success`
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=yes`
  - `exec_env=no`
  - `executed=no`
- Same-boot crash result:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
- Decoded crash addresses:
  - `0x4200c111` -> `RunPsramScratchProbe(...)` at `wasm_replay_control.cpp:88`
  - `0x4200c2af` -> `RunWasmReplayControlExample(...)` at `wasm_replay_control.cpp:150`
  - `0x4200b2e4` -> `CmdWasm(...)` at `wasm_command.cpp:159`

### Prompt Context

**User prompt (verbatim):** "continue"

**Assistant interpretation:** Take the last cheap runner-side split before crossing into WAMR internals. After `instantiate-no-execenv` still reproduced the bug, test whether export lookup itself is required.

**Inferred user intent:** Keep pushing until the runner-level search space is basically exhausted, while preserving enough detail in the diary to show why we stopped adding probe modes after this.

### What I did

- Added a final runner-side invocation mode in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  - `WasmInvocationMode::InstantiateBare`
- In this mode, the runner:
  - loads the module
  - calls `wasm_runtime_instantiate(...)`
  - then immediately tears down
- It does **not**:
  - look up the export
  - validate the signature
  - create an exec env
  - execute guest code
  - flush host work
- Added the matching console command in:
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
  - command: `wasm instantiate-bare <name>`
- Rebuilt and reflashed the headless PaperS3 build.
- Ran this same-boot sequence:
  - `wasm instantiate-bare return-42`
  - `wasm replay psram-scratch`
- Decoded the new crash against the headless ELF.

### Why

- `instantiate-no-execenv` had already ruled out guest execution and exec-env creation.
- The only obviously cheaper runner-side boundary left was export lookup/signature validation.
- If `instantiate-bare` still reproduced the bug, we could stop adding local probe modes with confidence and move into WAMR internals directly.

### What worked

- `wasm instantiate-bare return-42` succeeded and clearly showed the intended boundary:
  - `invocation_mode=instantiate-bare`
  - `execution=success`
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=no`
  - `exec_env=no`
  - `executed=no`
- That confirms the probe stopped exactly where intended.

### What didn't work

- The next command in the same boot:
  - `wasm replay psram-scratch`
  still crashed with:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`

### What I learned

- Export lookup is not required for the contamination either.
- The required boundary is now effectively:
  - `wasm_runtime_instantiate(...)`
  - followed by the matching cleanup path
- That makes the next move much clearer:
  - stop adding runner-side modes
  - inspect or instrument WAMR instantiate/deinstantiate internals directly

### What was tricky to build

- The only subtle point here was keeping the mode meaningful while still making it the cheapest remaining runner-side split.
- Stopping immediately after instantiate gives the tightest local boundary we can get without patching WAMR itself.

### What warrants a second pair of eyes

- Whether the next instrumented target should be:
  - `memory_instantiate(...)` / `wasm_allocate_linear_memory(...)`
  - `memories_deinstantiate(...)` / `wasm_deallocate_linear_memory(...)`
  - or both together
- Whether there is any plausible value left in adding a `load-only` mode, or whether that would now be mostly noise

### What should be done in the future

- Add direct WAMR-side instrumentation around:
  - `wasm_runtime_instantiate(...)`
  - `memory_instantiate(...)`
  - `memories_deinstantiate(...)`
  - `wasm_deallocate_linear_memory(...)`
- Focus first on allocation/free and any cache-relevant behavior around linear memory, because that is the remaining shared path across all successful runner-side poison modes.

### Code review instructions

- Review the final runner-side mode additions in:
  - [wasm_module_runner.h](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.h)
  - [wasm_module_runner.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp)
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Confirm that `instantiate-bare` reports:
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=no`
  - `exec_env=no`
  - `executed=no`

### Technical details

- Headless same-boot command sequence:
  - `wasm instantiate-bare return-42`
  - `wasm replay psram-scratch`
- Successful instantiate-bare output:
  - `invocation_mode=instantiate-bare`
  - `execution=success`
  - `loaded=yes`
  - `instantiated=yes`
  - `export_found=no`
  - `exec_env=no`
  - `executed=no`
- Same-boot crash result:
  - `Guru Meditation Error: Cache disabled but cached memory region accessed`
- Decoded crash addresses:
  - `0x4200c15d` -> `RunPsramScratchProbe(...)` at `wasm_replay_control.cpp:88`
  - `0x4200c2fb` -> `RunWasmReplayControlExample(...)` at `wasm_replay_control.cpp:150`
  - `0x4200b314` -> `CmdWasm(...)` at `wasm_command.cpp:164`

### Prompt Context

**User prompt (verbatim):** "Ok, let's go. we can also put back the atoms3r later on"

**Assistant interpretation:** Continue into direct WAMR instrumentation on PaperS3 now, and treat the AtomS3R as a later control board if the new internal logs warrant a cross-check.

**Inferred user intent:** Keep momentum on the PaperS3 root cause first, while preserving the option of bringing back AtomS3R only if the new findings need a comparative sanity check.

### What I did

- Added direct WAMR-side memory logs in:
  - [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)
  - [wasm_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c)
- Instrumented:
  - `os_mmap(...)`
  - `os_munmap(...)`
  - `memory_instantiate(...)`
  - `memories_deinstantiate(...)`
- Rebuilt and reflashed the headless PaperS3 build.
- Reran the same-boot sequence:
  - `wasm instantiate-bare return-42`
  - `wasm replay psram-scratch`

### Why

- The runner-side probes had already narrowed the required boundary to essentially `wasm_runtime_instantiate(...)` plus cleanup.
- The most important remaining uncertainty was where the relevant WAMR memory lived:
  - if it was in PSRAM, the contamination could still be a very direct PSRAM misuse
  - if it was in internal RAM, then the surviving theory would have to become broader

### What worked

- The new logs printed exactly the allocation lifecycle we needed.
- On PaperS3 headless `instantiate-bare`, WAMR reported:
  - `wamr_memmap.stage=mmap-data`
  - `wamr_memmap.ptr=0x3fcad470`
  - `wamr_memmap.size=32768`
  - `wamr_memmap.external=no`
  - `wamr_memmap.mem_caps=0x804`
- Then it reported:
  - `wamr_mem.stage=instantiate-memory`
  - `memory_data=0x3fcad470`
  - `memory_data_size=32768`
  - `heap_offset=0`
  - `heap_size=32768`
- And on cleanup:
  - `wamr_mem.stage=destroy-heap`
  - `wamr_mem.stage=deallocate-linear-memory`
  - `wamr_memmap.stage=munmap`
  - `external=no`

### What didn't work

- Even after seeing that the WAMR linear-memory block was internal, not PSRAM, the same-boot `psram-scratch` probe still crashed immediately afterward.
- So the new logs did not identify the root cause directly, but they ruled out one important class of explanations.

### What I learned

- This is a strong negative result in a useful sense.
- The remaining bug is **not** well explained by:
  - WAMR simply allocating guest linear memory in PSRAM and leaving it in a bad state
- Instead, the stronger current theory is:
  - WAMR instantiate/deinstantiate on PaperS3 is corrupting broader allocator or cache-related state
  - and the later PSRAM write is where that corruption becomes visible
- In other words, the crash site is PSRAM, but the cause may now be allocator metadata or cache-coherency state touched by internal-RAM WAMR allocations and frees.

### What was tricky to build

- The important part was keeping the logging bounded, because startup and repeated probes can easily drown the signal on a board like this.
- I kept the logs focused on:
  - pointer
  - external/internal classification
  - size
  - lifecycle stage
  which is enough for this round without turning the runtime into a log flood.

### What warrants a second pair of eyes

- Whether `mem_allocator_destroy(...)` or the surrounding free path could be corrupting shared heap metadata in a way that later PSRAM allocations/writes expose.
- Whether the PaperS3 board configuration changes the allocator/cache environment enough that an internal allocation/free bug shows up later only on PSRAM accesses.

### What should be done in the future

- Inspect the allocator path behind:
  - `runtime_malloc(...)`
  - `mem_allocator_create_with_struct_and_pool(...)`
  - `mem_allocator_destroy(...)`
  - `wasm_runtime_free(...)`
- Compare the same WAMR-side logs on AtomS3R later if we need to verify that this exact internal-allocation lifecycle is harmless there.
- If we need another code experiment on PaperS3 first, add a temporary mode that instantiates and intentionally skips deinstantiate/unload so we can split instantiate-time poisoning from cleanup-time poisoning.

### Code review instructions

- Review the new WAMR-side logs in:
  - [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)
  - [wasm_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c)
- Focus on whether the logged allocation/free sequence matches what the current PaperS3 build should be doing.

### Technical details

- WAMR allocation log:
  - `wamr_memmap.stage=mmap-data`
  - `wamr_memmap.ptr=0x3fcad470`
  - `wamr_memmap.size=32768`
  - `wamr_memmap.external=no`
  - `wamr_memmap.mem_caps=0x804`
- WAMR instantiate log:
  - `wamr_mem.stage=instantiate-memory`
  - `wamr_mem.index=0`
  - `wamr_mem.memory_data=0x3fcad470`
  - `wamr_mem.memory_data_size=32768`
  - `wamr_mem.num_bytes_per_page=32768`
  - `wamr_mem.init_pages=1`
  - `wamr_mem.max_pages=1`
  - `wamr_mem.heap_offset=0`
  - `wamr_mem.heap_size=32768`
- WAMR cleanup logs:
  - `wamr_mem.stage=destroy-heap`
  - `wamr_mem.stage=deallocate-linear-memory`
  - `wamr_memmap.stage=munmap`
  - `wamr_memmap.external=no`
- Crash afterward:
  - same-boot `wasm replay psram-scratch`
  - still dies in `RunPsramScratchProbe(...)` at the initial `memset(...)`

## Step 7: Test explicit cache sync and cache-line alignment on the poisoned PSRAM buffer, then preserve the ignored WAMR patch separately

Once the PaperS3 failure had been narrowed to “WAMR instantiate poisons later PSRAM writes,” the next obvious question was whether we could repair or at least reclassify that poisoned state with the ESP-IDF cache API itself. I added one probe that explicitly calls `esp_cache_msync(...)` on the already-poisoned persistent PSRAM buffer, and a second probe that removes the remaining alignment variable by allocating a separate `32 B`-aligned persistent PSRAM buffer before the same instantiate sequence.

Those two probes were worth doing because they close several tempting explanations quickly. The first one proved that a successful `esp_cache_msync(...)` call does **not** repair the PaperS3 failure. The second one proved that even a cache-line-aligned PSRAM buffer, plus a successful `DIR_M2C` pre-touch sync, still dies on the first CPU write after `wasm instantiate-bare-keepalive return-42`. That makes the surviving bug look even less like a simple alignment mistake or an omitted explicit cache-sync call.

### Prompt Context

**User prompt (verbatim):** "comparing to the atoms3r sdkconfig could also be instructive, potentially"

**Assistant interpretation:** Use the AtomS3R control where it helps, but continue the PaperS3-specific investigation and check whether configuration differences or lower-level cache-state probes separate the two boards.

**Inferred user intent:** Keep narrowing the PaperS3-only failure with the smallest discriminating experiments available, instead of staying at the level of broad board-to-board guesses.

### What I did

- Added explicit cache-sync replay controls in:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- New control commands:
  - `wasm replay psram-persistent-touch-sync`
  - `wasm replay psram-cacheline-persistent-init`
  - `wasm replay psram-cacheline-persistent-touch-sync`
- Added ticket-local reusable scripts in:
  - [serial_probe_sequence.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/serial_probe_sequence.py)
  - [probe_wamr_psram_cache.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/probe_wamr_psram_cache.sh)
- Updated [flash_and_probe_panel_epd.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/flash_and_probe_panel_epd.sh) to use the local `ESP-41` probe helper instead of the older `ESP-39` script path.
- Added extra WAMR-side memmap-state logging in the ignored component file:
  - [espidf_memmap.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/shared/platform/esp-idf/espidf_memmap.c)
- Preserved that ignored WAMR-side instrumentation as a tracked patch artifact in:
  - [espressif_wamr_espidf_memmap_local_debug_patch.diff](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/espressif_wamr_espidf_memmap_local_debug_patch.diff)
- Rebuilt and flashed the attached PaperS3 repeatedly while keeping `/dev/ttyACM0` single-owned during each probe cycle.
- Ran the first same-boot sequence:
  - `wasm replay psram-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch-sync`
- Then ran the aligned-buffer same-boot sequence:
  - `wasm replay psram-cacheline-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-cacheline-persistent-touch-sync`
- Decoded the new crash PCs against the exact ELF with:
  - `xtensa-esp32s3-elf-addr2line -pfiaC -e ...`

### Why

- If `esp_cache_msync(...)` failed or crashed on the poisoned PSRAM buffer, that would directly implicate the low-level cache path rather than only the later CPU write loop.
- If `esp_cache_msync(...)` succeeded and then the later write also succeeded, that would point to a missing explicit cache maintenance step.
- If the aligned buffer survived while the old `16`-aligned buffer died, then alignment could still explain the failure.
- Preserving the WAMR-side memmap patch as a tracked artifact matters because `managed_components/` is ignored in this repo, so those local edits do **not** show up in normal git status or commits.

### What worked

- The new control commands compiled and ran on PaperS3.
- On the original persistent PSRAM buffer, after `wasm instantiate-bare-keepalive return-42`, the firmware reported:
  - `persistent_psram_probe.cache_alignment_err=ESP_OK`
  - `persistent_psram_probe.cache_alignment=32`
  - `persistent_psram_probe.sync_flags=0x7`
  - `persistent_psram_probe.sync_err=ESP_OK`
- So `esp_cache_msync(...)` itself does **not** immediately fail on the poisoned buffer.
- On the aligned persistent PSRAM buffer, after the same instantiate step, the firmware reported:
  - `aligned_persistent_psram_probe.alignment=32`
  - `aligned_persistent_psram_probe.pre_sync_err=ESP_OK`
- The extra WAMR memmap-state logs also worked and confirmed heap-integrity probes remained nominal inside the WAMR-side `os_mmap(...)` path.
- The new aligned-buffer crash decoded cleanly:
  - `0x4200d3ca` -> `TouchAlignedPersistentPsramProbeWithCacheSync(...)` at [wasm_replay_control.cpp:298](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L298)

### What didn't work

- `psram-persistent-touch-sync` still crashed on the very next CPU write even though `esp_cache_msync(...)` returned `ESP_OK`.
- `psram-cacheline-persistent-touch-sync` also still crashed on the first direct CPU write, even though:
  - the buffer was allocated with `32 B` alignment
  - the pre-touch `DIR_M2C` sync returned `ESP_OK`
- My first attempt to reflash after adding the aligned-buffer control was flawed: I wrapped the flash in an over-broad `pkill` pattern, then later noticed the device still reported the old app hash and did not recognize `psram-cacheline-persistent-init`.
  - Exact symptom:
    - `control_example=psram-cacheline-persistent-init`
    - `control_execution=failure`
    - `error_stage=lookup`
    - `error_message=unknown replay example`
  - That was a stale-firmware mistake on my side, not a real runtime result.

### What I learned

- A successful explicit cache-sync call is **not** sufficient to repair the poisoned PaperS3 PSRAM state after WAMR instantiate.
- A cache-line-aligned PSRAM buffer is **also not** sufficient to repair the failure.
- The surviving crash still occurs at the first direct CPU write into PSRAM after the instantiate boundary.
- The WAMR-side `managed_components` edits are not tracked by normal git because:
  - `.gitignore` contains `managed_components`
  - `git check-ignore -v .../espidf_memmap.c` points at that ignore rule
- That means future debugging in that area must either:
  - store patch artifacts in the ticket, or
  - move the component into a tracked local override if we need a longer-lived vendor branch

### What was tricky to build

- The most annoying operational trap in this step was firmware provenance, not C++ itself. I had one false result because the board still ran the previous binary and the missing replay command looked, at first glance, like a code-path problem.
- The ignored `managed_components` tree is also a sharp edge. It is easy to believe a WAMR-side edit is “in git” because the local file changed and the build used it, while the main repo remains completely unaware of that change. That is why I added the tracked patch artifact in the ticket scripts directory before moving on.

### What warrants a second pair of eyes

- Whether the surviving PaperS3-only failure is now best modeled as:
  - lower-level external-memory cache/MMU/tag-state corruption, or
  - a board-specific PSRAM controller/driver state problem that survives successful high-level heap/cache checks
- Whether the next best experiment is:
  - a minimal PaperS3 PSRAM writer outside the current app stack, or
  - moving this exact instantiate + PSRAM-write probe back to AtomS3R as a control with the new aligned-buffer path

### What should be done in the future

- Decide whether to build a minimal PaperS3 PSRAM probe app that excludes almost all of `0079`, so the surviving issue can be tested without the current console/demo stack.
- If we stay inside `0079`, add one more lower-level probe that inspects whether the failure can be triggered by simple PSRAM reads after instantiate, not just writes.
- If a longer WAMR-side investigation continues, move away from ad-hoc edits inside ignored `managed_components` and either vendor the component in a tracked path or keep generating tracked patch artifacts for each WAMR-side change.

### Code review instructions

- Start with the new replay controls in:
  - [wasm_replay_control.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp)
  - [wasm_command.cpp](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_command.cpp)
- Then review the tracked reproduction helpers:
  - [serial_probe_sequence.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/serial_probe_sequence.py)
  - [probe_wamr_psram_cache.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/probe_wamr_psram_cache.sh)
- Finally, inspect the tracked patch artifact for the ignored WAMR file:
  - [espressif_wamr_espidf_memmap_local_debug_patch.diff](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/espressif_wamr_espidf_memmap_local_debug_patch.diff)

### Technical details

- Same-boot explicit-cache-sync sequence:
  - `wasm replay psram-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch-sync`
- Key result before crash:
  - `persistent_psram_probe.cache_alignment=32`
  - `persistent_psram_probe.sync_flags=0x7`
  - `persistent_psram_probe.sync_err=ESP_OK`
- Crash decode for that path:
  - `0x4200d04e` -> `TouchPersistentPsramProbe(...)` at [wasm_replay_control.cpp:216](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L216)
- Same-boot aligned-buffer sequence:
  - `wasm replay psram-cacheline-persistent-init`
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-cacheline-persistent-touch-sync`
- Key aligned-buffer result before crash:
  - `aligned_persistent_psram_probe.alignment=32`
  - `aligned_persistent_psram_probe.pre_sync_err=ESP_OK`
- Crash decode for that path:
  - `0x4200d3ca` -> `TouchAlignedPersistentPsramProbeWithCacheSync(...)` at [wasm_replay_control.cpp:298](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L298)

## Step 8: Read the WAMR instantiate path properly, add ownership-transition logs, and stop before the next hardware cycle

At this point the remaining bug had become narrow enough that spraying more app-side logs would have been lazy. The smallest known repro boundary was already `wasm_runtime_instantiate(...)` plus later PSRAM access on PaperS3. Before adding any more instrumentation, I stopped and read the active instantiate/deinstantiate chain in the local WAMR sources so the next logs would sit at real ownership transitions instead of just wrapping public entrypoints.

The main code-reading pass went through the exact functions that matter for allocation and teardown:

- [wasm_runtime_common.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_runtime_common.c)
  - `wasm_runtime_instantiate_internal(...)`
  - `wasm_runtime_deinstantiate_internal(...)`
- [wasm_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c)
  - `wasm_instantiate(...)`
  - `memory_instantiate(...)`
  - `memories_instantiate(...)`
  - `memories_deinstantiate(...)`
  - `wasm_deinstantiate(...)`
- [wasm_memory.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c)
  - `wasm_allocate_linear_memory(...)`
  - `wasm_deallocate_linear_memory(...)`

That review changed the plan in a useful way. The important transitions are not “public API called” and “public API returned.” The real transitions are:

- module-instance block allocated
- memories array instantiated
- linear memory mmapped
- memory heap handle created
- module-instance teardown begins
- linear memory munmapped
- module-instance block freed

So I added bounded logs at exactly those points. The new trace additions are in the ignored local component sources:

- [wasm_runtime_common.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_runtime_common.c)
  - `wamr_rt.stage=instantiate-enter`
  - `wamr_rt.stage=instantiate-exit`
  - `wamr_rt.stage=deinstantiate-enter`
  - `wamr_rt.stage=deinstantiate-exit`
- [wasm_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c)
  - `wamr_inst.stage=enter`
  - `wamr_inst.stage=alloc-module-inst-ok`
  - `wamr_inst.stage=alloc-module-inst-failed`
  - `wamr_inst.stage=subinstantiate-ok`
  - `wamr_inst.stage=subinstantiate-failed`
  - `wamr_inst.stage=success`
  - `wamr_inst.stage=fail`
  - `wamr_deinst.stage=enter`
  - `wamr_deinst.stage=free-module-inst`
- [wasm_memory.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c)
  - `wamr_linear.stage=mmap-enter`
  - `wamr_linear.stage=mmap-exit`
  - `wamr_linear.stage=mmap-failed`
  - `wamr_linear.stage=munmap-enter`
  - `wamr_linear.stage=munmap-exit`

I also preserved the current local versions of those ignored WAMR files as tracked ticket artifacts in:

- [README.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots/README.md)
- [wasm_runtime_common.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots/wasm_runtime_common.c)
- [wasm_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots/wasm_runtime.c)
- [wasm_memory.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots/wasm_memory.c)

That is intentionally redundant with the older memmap patch artifact. The older file captures the `espidf_memmap.c` edits specifically. These new snapshots capture the wider local WAMR debug state that still would not show up in normal git status because `managed_components/` is ignored here.

In parallel with the code reading, I finally wrote down the board-level comparison that had been floating around as a hypothesis. The useful comparison is not “AtomS3R must be on newer ESP-IDF” or “maybe AtomS3R just has better PSRAM.” Both projects in this workspace are built against `ESP-IDF 5.3.4`, and the relevant cache/PSRAM `sdkconfig` slices are broadly aligned. The more interesting hardware difference is:

- AtomS3R: `ESP32-S3-PICO-1-N8R8` SiP, with flash and PSRAM packaged together
- PaperS3: `ESP32-S3R8` plus separate external flash on the board

That matters because the panic text we keep seeing on PaperS3 explicitly mentions dcache writeback trouble while writing back to flash. So the current board-level suspicion is not “PaperS3 has PSRAM and AtomS3R does not.” It is closer to “PaperS3’s flash-side external-memory topology may make it vulnerable to whatever state WAMR instantiate is disturbing.”

### Prompt Context

**User prompt (verbatim):** "ok, so let's stasrt by adding logging. stop before flashing and running, i need to attach the device"

**Assistant interpretation:** Study the active WAMR instantiate path carefully enough to add targeted logs, preserve the ignored component edits in tracked artifacts, update the ticket record, and stop before any new build/flash/probe cycle.

**Inferred user intent:** Prepare the next debugging run responsibly, with better runtime instrumentation and strong provenance, rather than immediately touching hardware.

### What I did

- Read the active instantiate/deinstantiate and linear-memory code paths in the local WAMR sources listed above.
- Added structured bounded logs in:
  - [wasm_runtime_common.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_runtime_common.c)
  - [wasm_runtime.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/interpreter/wasm_runtime.c)
  - [wasm_memory.c](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/espressif__wasm-micro-runtime/core/iwasm/common/wasm_memory.c)
- Created tracked snapshots of those ignored files in:
  - [scripts/wamr-local-debug-snapshots](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/wamr-local-debug-snapshots)
- Compared the relevant PaperS3 and AtomS3R hardware docs plus local `sdkconfig` slices and recorded the stronger board-level differentiator:
  - same `ESP-IDF 5.3.4`
  - similar cache and octal-PSRAM configuration
  - different flash packaging/topology
- Updated:
  - [tasks.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/tasks.md)
  - [changelog.md](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/changelog.md)
  - this diary

### Why

- We were already past the point where more app-side logs were the right next move.
- The smallest known repro boundary now lives inside WAMR instantiate plus later PaperS3 PSRAM access.
- Adding logs without understanding the instantiate path would have been low-signal and hard to interpret.
- Because the local WAMR sources live under ignored `managed_components/`, preserving snapshots before the next hardware run is part of the debugging work, not optional cleanup.

### What worked

- The code-reading pass clarified where ownership and teardown actually change.
- The new logs are bounded and focused on the transitions most likely to matter.
- The ticket now has tracked copies of the local ignored WAMR files, not just one memmap patch artifact.
- The board comparison rules out one weak explanation:
  - AtomS3R is not secretly a `5.4` firmware while PaperS3 is on `5.3`

### What didn't work

- I did not build, flash, or run this new trace slice yet. That was deliberate because the explicit instruction for this step was to stop before hardware activity.
- The main repo still cannot track the live `managed_components` edits directly. The snapshots solve provenance, but they do not solve the underlying “ignored vendor tree” ergonomics.

### What I learned

- The right logging boundary is narrower than I first phrased it. The interesting transitions are memory and module ownership transitions, not just top-level API entry/exit.
- The surviving board-level difference worth taking seriously is PaperS3’s external flash topology, not an imagined ESP-IDF version mismatch.
- For this project, provenance is part of debugging quality. If an ignored component changes and that state is not copied into the ticket, future analysis becomes much weaker.

### What was tricky to build

- The awkward part was not C syntax. It was making sure the ignored component edits stayed reviewable and recoverable.
- The interrupted turns also meant I had to re-check workspace state before continuing, instead of assuming every previous partial tool action had completed cleanly.

### What warrants a second pair of eyes

- Whether the new `wamr_inst` / `wamr_linear` trace points are enough, or whether the next slice should instrument even deeper inside the ESP-IDF memory-mapping helpers again.
- Whether the board-level “off-package flash topology” theory should be tested next with a minimal PaperS3 probe, or with a tighter AtomS3R/PaperS3 differential run using the new logs.

### What should be done in the future

- Rebuild `0079` with the new WAMR traces.
- Flash and run the smallest PaperS3 repro again once the device is attached:
  - `wasm instantiate-bare-keepalive return-42`
  - `wasm replay psram-persistent-touch`
- Capture the new `wamr_rt`, `wamr_inst`, and `wamr_linear` logs and compare them against the existing memmap-state trace.
- If the new logs still only show “everything looks normal,” the next slice should probably move to a minimal PaperS3 probe app or deeper ESP-IDF-level instrumentation.

## Step 9: Run the traced PaperS3 repro and confirm that instantiate still looks healthy before the PSRAM fault

Once the PaperS3 was attached again and host device access was available, I used the existing ticket-local script to do one controlled cycle instead of improvising command-by-command. The script path was:

- [probe_wamr_psram_cache.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/probe_wamr_psram_cache.sh)

and it flashed `0079` and then ran, in one boot session:

- `wasm replay psram-persistent-init`
- `wasm instantiate-bare-keepalive return-42`
- `wasm replay psram-persistent-touch-sync`

That exact sequence was important. The first command allocates and initializes the persistent PSRAM buffer before WAMR instantiate. The second command instantiates a trivial module while intentionally skipping deinstantiate/unload cleanup. The third command performs the explicit cache-sync path and then tries the first direct CPU write into the already-existing PSRAM buffer.

The new result is useful because the added WAMR trace points were finally live in the same capture as the crash. They showed:

- `wamr_rt.stage=instantiate-enter`
- `wamr_inst.stage=enter`
- `wamr_inst.stage=alloc-module-inst-ok`
- `wamr_linear.stage=mmap-enter`
- `wamr_memmap.stage=mmap-data`
- `wamr_memmap.ptr=0x3fcb9a98`
- `wamr_memmap.external=no`
- `wamr_linear.stage=mmap-exit`
- `wamr_mem.stage=instantiate-memory`
- `wamr_inst.stage=subinstantiate-ok`
- `wamr_inst.stage=success`
- `wamr_rt.stage=instantiate-exit`

So the instantiate path itself does not visibly fail or even wobble. It allocates the module instance, allocates linear memory, and that linear memory is still in internal RAM rather than PSRAM. The module remains alive because this run used the keepalive mode, and there were no deinstantiate logs before the later crash.

That last point matters. We already suspected cleanup was not required from earlier keepalive probes, but this run makes it more concrete. The traced crash happened without any `wamr_deinst.*` or `wamr_rt.stage=deinstantiate-*` output beforehand, so the surviving hazard boundary is still “successful instantiate” rather than “later cleanup.”

Immediately before the failing PSRAM write, the replay-side state still looked ordinary:

- `replay_mem.flash_cache_enabled=yes`
- `replay_mem.internal_heap_ok=yes`
- `replay_mem.spiram_heap_ok=yes`
- `persistent_psram_probe.cache_alignment=32`
- `persistent_psram_probe.sync_err=ESP_OK`

Then the board still panicked on the first direct CPU write into the persistent PSRAM buffer. I decoded the crash backtrace against the exact freshly built ELF with:

```bash
xtensa-esp32s3-elf-addr2line -pfiaC \
  -e 0079-papers3-wamr-assemblyscript-console/build/papers3_wamr_assemblyscript_console.elf \
  0x4200d183 0x4200d396 0x4200d6e3 0x4200bde0 0x4200dffb 0x4200e1f6 0x4037d2f1
```

and got:

- `TouchPersistentPsramProbe(...)` at [wasm_replay_control.cpp:261](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L261)
- `TouchPersistentPsramProbeWithCacheSync(...)` at [wasm_replay_control.cpp:360](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/main/wasm_replay_control.cpp#L360)

### Prompt Context

**User prompt (verbatim):** "continue" followed later by "I attached the papers3 btw, if you want to test." and then "you have full access now"

**Assistant interpretation:** Finish the prepared WAMR trace slice, then use the attached PaperS3 to run the smallest meaningful repro under the new instrumentation.

**Inferred user intent:** Move from preparation to one concrete hardware validation run, while keeping the investigation reproducible and documented.

### What I did

- Rebuilt `0079` against the modified local WAMR sources.
- Flashed the attached PaperS3 with:
  - [probe_wamr_psram_cache.sh](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/probe_wamr_psram_cache.sh)
- Captured the single-boot trace output from:
  - [serial_probe_sequence.py](/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/22/ESP-41-PAPERS3-PANEL-EPD-INSTRUMENTATION--instrument-and-compare-papers3-panel-epd-crash-path-after-wamr-execution/scripts/serial_probe_sequence.py)
- Decoded the crash against the freshly built ELF with `xtensa-esp32s3-elf-addr2line`.

### Why

- This was the first run where the new WAMR trace points and the known PSRAM failure lived in the same log capture.
- The keepalive mode is the sharpest way to answer whether instantiate alone still looks clean before the crash.

### What worked

- The rebuild succeeded cleanly.
- Flashing the PaperS3 succeeded.
- The scripted single-boot sequence ran and captured the new WAMR trace logs.
- The logs showed a fully successful instantiate path and reconfirmed internal-RAM linear memory allocation.
- The crash decoded cleanly back to the persistent PSRAM touch path.

### What didn't work

- The new WAMR traces did not reveal an obvious bad state inside instantiate.
- The explicit cache-sync path still did not save the later PSRAM write.

### What I learned

- Successful WAMR instantiate on PaperS3 can look completely normal in the currently visible runtime trace and still be enough to poison later PSRAM writes.
- The surviving repro is not waiting for unload/cleanup.
- The stronger current model is not “WAMR allocates bad PSRAM.” It is “successful instantiate perturbs some PaperS3-specific external-memory or flash-side state that the current runtime logs do not expose.”

### What was tricky to build

- The biggest practical difference in this run was environmental, not code. Earlier, the sandbox could not see `/dev/ttyACM0`; once full host access was restored, the same script worked normally.
- The keepalive run was especially valuable because it prevents us from over-attributing the bug to teardown paths we did not even execute.

### What warrants a second pair of eyes

- Whether the next slice should move below WAMR into ESP-IDF external-memory state or a minimal PaperS3 probe app.
- Whether PaperS3’s separate external flash topology is interacting with something in a way the AtomS3R SiP does not.

### What should be done in the future

- Either build a minimal PaperS3 probe that excludes almost all of `0079`, or add one more layer of lower-level instrumentation below the current WAMR trace points.
- If we stay in `0079`, preserve the new trace-bearing build logs or serial capture as an artifact as well, not just the source snapshots.
