# Tasks

## In Progress

- [x] Task 1: Create a focused PaperS3 `Panel_EPD` debugging slice that starts from the existing `ESP-39` baseline instead of reopening generic WAMR questions
- [x] Task 1.1: Document the exact crash choke point and the immediate code paths on both the app side and the M5GFX side
- [x] Task 1.2: Decide the smallest useful instrumentation set for `Panel_EPD.cpp`
- [x] Task 1.3: Add reusable ticket-local probe notes and keep the diary current

## Planned

- [ ] Task 2: Instrument the PaperS3 EPD write path around `writeFillRectPreclipped(...)`
- [ ] Task 2.1: Add bounded diagnostics for `_buf`, row stride math, rect bounds, and display mode at the first draw after reset
- [ ] Task 2.2: Make the diagnostics safe enough that they do not themselves swamp the crash path
- [ ] Task 2.3: Rebuild `0079` with the instrumented M5GFX nested repo
- [ ] Task 2.4: Flash the attached PaperS3 and rerun the smallest useful probes

- [ ] Task 3: Compare the local `Panel_EPD.cpp` against newer upstream changes that plausibly affect PaperS3 refresh/write behavior
- [ ] Task 3.1: Recheck the local `M5GFX` history around `Panel_EPD.cpp`
- [ ] Task 3.2: Identify whether `fd824ee` or any later PaperS3-specific delta is safe to A/B locally
- [ ] Task 3.3: If justified, apply the smallest upstream-aligned patch and rerun the same probes

- [ ] Task 4: Decide whether the evidence points to buffer/caching assumptions, update-queue state, or a broader EPD backend issue
- [ ] Task 4.1: Record what instrumentation proved and what it falsified
- [ ] Task 4.2: Commit the code slice and then the ticket diary/task/changelog slice
