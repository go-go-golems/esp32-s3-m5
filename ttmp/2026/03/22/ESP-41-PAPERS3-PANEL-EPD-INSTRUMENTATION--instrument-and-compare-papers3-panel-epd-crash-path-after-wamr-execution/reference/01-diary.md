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
