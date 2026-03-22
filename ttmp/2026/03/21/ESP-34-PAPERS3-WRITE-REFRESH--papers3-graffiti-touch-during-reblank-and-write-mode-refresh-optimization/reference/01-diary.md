---
Title: Diary
Ticket: ESP-34-PAPERS3-WRITE-REFRESH
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5gfx
    - m5stack
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.cpp
      Note: Write-mode redraw routing changed in commit 30a54df
    - Path: 0077-papers3-alphabet-graffiti/main/alphabet_app.h
      Note: Added write-mode localized redraw flags and helpers in commit 30a54df
    - Path: ttmp/2026/03/21/ESP-34-PAPERS3-WRITE-REFRESH--papers3-graffiti-touch-during-reblank-and-write-mode-refresh-optimization/tasks.md
      Note: Task-by-task tracking for the redraw-first follow-up
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T22:29:15.268056066-04:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Capture the redraw-first follow-up work for `0077-papers3-alphabet-graffiti`, including what changed, why it changed, what was verified, and what should be reviewed before moving on to touch-path changes.

## Step 1: Reduce Write-Mode Full Redraws

The immediate goal in this step was to reduce the amount of whole-screen e-paper work performed during normal `WRITE`-mode usage. The earlier analysis already showed that touch polling still runs in the main loop, but routine handwriting input was still ending in `QueueFullRender()`, which makes the device spend more time waiting on slow panel work than the write workflow actually needs.

The implementation in this step did not try to solve touch capture outright. It narrowed scope to the redraw path only: keep full redraws for mode changes and training-heavy UI, but let normal write-mode edits refresh just the output-buffer region and clear the canvas locally. That gives a cleaner baseline before any touch-specific concurrency or sampling changes are considered.

### Prompt Context

**User prompt (verbatim):** "fix redraw first. We'll tackle touch next. Add tasks to the ticket, then do task by task  and commit and diary"

**Assistant interpretation:** Prioritize redraw-efficiency work in the new `ESP-34` ticket, leave touch-threading for later, implement the first redraw optimization in `0077`, and record the work step-by-step with commits and diary entries.

**Inferred user intent:** Make the current PaperS3 handwriting UI feel more responsive by removing unnecessary full-screen e-paper updates before attempting riskier touch-path changes.

**Commit (code):** `30a54df` — `feat(papers3): reduce write-mode full redraws`

### What I did
- inspected the current write-mode redraw path in `0077-papers3-alphabet-graffiti/main/alphabet_app.cpp`
- confirmed that `FinishStroke()`, `AddSpace()`, `BackspaceText()`, and `ClearText()` still fed routine write interactions into `QueueFullRender()`
- added localized redraw bookkeeping in `0077-papers3-alphabet-graffiti/main/alphabet_app.h`
- added `QueueWriteTextBarRender()` and `RenderWriteTextBufferBar()` in `0077-papers3-alphabet-graffiti/main/alphabet_app.cpp`
- changed write-mode text editing actions to refresh only the write output/status bar instead of requesting a whole-screen redraw
- changed write-mode stroke completion to queue the text-bar refresh and a local canvas reset, then return without calling `QueueFullRender()`
- kept the existing full redraw path for training mode and mode/layout changes
- rebuilt the app with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py build
```

### Why
- full-screen e-paper redraws are the most expensive visual operation in this app
- routine handwriting input usually changes only two things: the canvas state and the text/status strip
- reducing redraw scope is the lowest-risk performance improvement because it does not require touching `M5Unified` threading assumptions

### What worked
- the new write-mode redraw path compiled without further structural changes
- `idf.py build` completed successfully against ESP-IDF `5.3.4`
- the diff stayed tightly scoped to `alphabet_app.cpp` and `alphabet_app.h`
- training mode behavior was not entangled with the first optimization step

### What didn't work
- nothing failed at compile time in this step
- no physical hardware test was run yet, so there is still no proof in this diary step that the perceived touch interruption is reduced on device

### What I learned
- the main source of redraw waste was not live stroke drawing; it was the post-stroke and text-edit actions that still escalated into `QueueFullRender()`
- the existing code already had enough structure to introduce a localized write-mode refresh without splitting the whole runtime class
- a small `epd_text` redraw over the output buffer is a much safer first optimization than trying to make touch polling concurrent

### What was tricky to build
- the main constraint was preserving the new UI layout while changing only redraw scope. The write-mode output strip still needs crisp text, which argues for keeping `epd_text` for that region, but the rest of the screen should not be repainted just because one character was appended.
- another sharp edge was `ClearStroke()`. In training mode it still needs a full UI redraw because the training-side informational panels depend on current gesture state. In write mode that is unnecessary, so the implementation had to branch by mode rather than replacing `QueueFullRender()` globally.

### What warrants a second pair of eyes
- whether `RenderWriteTextBufferBar()` should remain `epd_text` or switch to a faster mode if ghosting remains acceptable
- whether clearing the write canvas locally without re-drawing the placeholder prompt is the desired UX
- whether any other write-mode controls still indirectly trigger whole-screen refreshes more often than necessary

### What should be done in the future
- run this firmware on physical PaperS3 hardware and compare perceived responsiveness before and after commit `30a54df`
- if touch still feels blocked during reblank, investigate capture timing separately without assuming `M5Unified` is thread-safe across tasks

### Code review instructions
- start in `0077-papers3-alphabet-graffiti/main/alphabet_app.cpp`
- inspect `FinishStroke()`, `ClearStroke()`, `AddSpace()`, `BackspaceText()`, `ClearText()`, `QueueWriteTextBarRender()`, `ProcessPendingDisplayWork()`, and `RenderWriteTextBufferBar()`
- inspect `0077-papers3-alphabet-graffiti/main/alphabet_app.h` for the new localized redraw flag and helper declarations
- validate with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py build
```

### Technical details
- code changed:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti/main/alphabet_app.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti/main/alphabet_app.h`
- related ticket docs:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/21/ESP-34-PAPERS3-WRITE-REFRESH--papers3-graffiti-touch-during-reblank-and-write-mode-refresh-optimization/analysis/01-touch-during-reblank-and-write-mode-refresh-analysis.md`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/03/21/ESP-34-PAPERS3-WRITE-REFRESH--papers3-graffiti-touch-during-reblank-and-write-mode-refresh-optimization/tasks.md`
