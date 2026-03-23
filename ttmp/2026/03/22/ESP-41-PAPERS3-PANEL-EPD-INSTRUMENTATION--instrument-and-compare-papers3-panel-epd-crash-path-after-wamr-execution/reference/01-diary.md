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
RelatedFiles: []
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
  - `fillrect xs=0 ys=0 xe=959 ye=539 w=540 h=960 ... last=460589 len=259200`
- Successful fresh-boot `frame-no-clear` replay log:
  - `display req=(0,0 0x0) upd=(16,16 928x508) range=(16,16)-(943,523) mode=3 ...`
- Standing process lesson:
  - one `/dev/tty*` device
  - one flash/probe/monitor owner at a time

## Step 3: Reconfirm the actual contamination behavior with cleaner probes and pivot from “does it crash?” to “where exactly does it crash?”

The next slice cleaned up the uncertainty from the earlier truncated multi-command run. I reran the smallest probes one at a time using the new ticket-local script and the single-owner serial rule.

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
