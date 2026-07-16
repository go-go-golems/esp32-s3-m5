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
