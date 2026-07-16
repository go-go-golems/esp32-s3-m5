---
Title: Implementation Diary
Ticket: ESP-51-PULP-OS-V2
Status: active
Topics:
    - papers3
    - eink
    - esp32s3
    - microquickjs
    - architecture
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-07-16T14:18:16.491689955-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Chronological record of implementing ESP-51: extracting the proven s3paper components out of 0112, then building the 0114 PULP OS v2 firmware with the native builder JS API. Companion to the intern guide (design-doc/01); records what was done, what failed verbatim, and what to watch.

## Step 1: Orientation — environment proven, 0112 baseline green

Phase 0 of the plan: prove the environment end to end before touching anything. The intern guide and the ESP-50 diary are the working references; the host suite, a clean 0112 build, and a live-device console transcript are the evidence that the starting point is healthy.

### Prompt Context

**User prompt (verbatim):** "Implement docmgr ticket ESP-51-PULP-OS-V2"

**Assistant interpretation:** Execute the ESP-51 plan (10 phases, 65 tasks) that was handed off: extract components, re-point 0112, build 0114 with the v2 builder API, rewrite the PULP apps.

**Inferred user intent:** Turn the handoff package into a working PULP OS v2 firmware, following the plan's phase gates and evidence discipline.

### What I did
- Re-read the intern guide (design-doc/01) end to end.
- Host suite: `cd 0112-papers3-reader-primitives/components/s3paper_core/tests/host && make run` -> `PASS: 37989 checks, 0 failures`.
- Built 0112 with pinned IDF 5.3.4 -> success, `papers3_reader_primitives.bin` = 1008K (baseline for Phase 1 size comparison).
- Console smoke against the running device (no reflash needed; it runs the tip build): `status`, `js status`, `library` via the ESP-50 console client. Transcript: `scripts/output/p0-baseline-smoke.log`.
- Created this diary.

### Why
- Phase 1 moves components; without a green baseline any regression would be unattributable.

### What worked
- Everything: 37,989 host checks, clean build, device up 3.1 h with `js init=1 screen_active=1 evals=395 exceptions=0`, 3-book library scan, PULP postcard screen presenting (hits=30 fingerprint).

### What didn't work
- N/A (baseline run).

### What I learned
- Device had been sitting on the postcard screen since the prior session and the per-second dispatch counter kept climbing without a single exception — good soak datapoint for the v1 runtime.

### What should be done in the future
- N/A

### Code review instructions
- Evidence transcript: `scripts/output/p0-baseline-smoke.log` in this ticket.

### Technical details
- Baseline binary size 1008K; host suite 37,989 checks; console fingerprints: postcard hits=30, `js present ... full=1` after CleanFull.

## Step 2: Phase 1 — s3paper_core and s3paper_m5 promoted to repo components/

The two pure/proven components moved out of 0112 into the repo-level `components/` directory with `git mv` (history preserved), and 0112 was re-pointed with explicit `EXTRA_COMPONENT_DIRS` entries plus a trimmed `set(COMPONENTS main esp_psram)`. The engine copy (`0112/components/mquickjs`) deliberately stays per-firmware: its atom header is generated from that firmware's stdlib.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Phase 1 of the plan: extract shared components without regressing 0112.

**Inferred user intent:** Shared foundation for the 0114 firmware.

**Commit (code):** 9d80478 — "ESP-51 Phase 1: promote s3paper_core + s3paper_m5 to repo components/, re-point 0112"

### What I did
- `git mv 0112-papers3-reader-primitives/components/{s3paper_core,s3paper_m5} components/`.
- Rewrote `0112/CMakeLists.txt`: EXTRA_COMPONENT_DIRS pointing at the two specific component dirs (never the whole shared components/ tree), `set(COMPONENTS main esp_psram)` with a comment explaining the named-esp_psram gotcha.
- Clean rebuild (`rm -rf build sdkconfig`): binary 0xfa9b0 (~1002 KiB, matches 1008K baseline). `CONFIG_SPIRAM=y` confirmed re-seeded.
- Host suite from the new location: `components/s3paper_core/tests/host` -> 37,989 green (relative paths inside the component were unaffected by the move).
- Flashed; smoke transcript `scripts/output/p1-relocated-components-smoke.log`: boot restore `resumed=1` into Alice at saved offset, `reader next/prev` TextPage present, `library show` with 4 row regions and 3 books.

### Why
- 0114 must consume these components without copying; 0112 stays the regression guard.

### What worked
- Everything on the first build. The explicit-dirs approach avoided the 0113 failure mode (whole-directory EXTRA_COMPONENT_DIRS pulling unresolved components).

### What didn't work
- N/A — the two known gotchas (esp_psram naming, sdkconfig re-seed) were designed around up front.

### What was tricky to build
- Only the CMake trim: `set(COMPONENTS main esp_psram)` prunes the build graph, so anything not reachable from main's REQUIRES disappears, including Kconfig-only components. esp_psram is named explicitly for that reason.

### What warrants a second pair of eyes
- Any other project referencing these components by the old path (none found; 0113 has its own copies).

### What should be done in the future
- Phase 2/3 extractions build on this layout.

### Code review instructions
- `0112-papers3-reader-primitives/CMakeLists.txt` (the whole change surface besides the move).
- Validate: host suite from `components/s3paper_core/tests/host`; `idf.py build` in 0112.
