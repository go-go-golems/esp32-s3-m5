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

## Step 3: Phase 2 — s3paper_storage extracted, fault battery green

`app_storage.cpp` moved wholesale into `components/s3paper_storage` (namespace `s3paper_storage`), with the two firmware couplings inverted: the display-before-SD-mount constraint is now an injected `pre_mount` hook, and the demo book is a configured seed (`StorageConfig{seed_path, seed_text, seed_len}`). 0112 keeps its `reader::` names through a thin shim (`main/app_storage.{h,cpp}`, ~60 lines: ConfigureOnce + using-declarations + SdSnapshot adapter). New in the component: `DebugCorruptStateFile`/`DebugReloadState` fault-injection hooks, exposed as `sd fault <kind> <mode>` / `sd reload` console commands — these also close the long-open ESP-50 fault-sim item and will serve 0114's Phase 9 hardening.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Phase 2: extract persistence into a reusable component and prove loader recovery on hardware.

**Inferred user intent:** 0114 gets crash-safe persistence for free; 0112 stays green.

**Commit (code):** f018182 — "ESP-51 Phase 2: extract s3paper_storage component with fault-injection hooks"

### What I did
- Component: `include/s3paper_storage/storage.h` (API sans app_events types: own `StorageStats`, `StorageConfig`), `src/storage.cpp` (git-mv'd history), CMakeLists (REQUIRES s3paper_core fatfs sdmmc esp_driver_sdspi).
- 0112 shim + EXTRA_COMPONENT_DIRS entry + main REQUIRES s3paper_storage.
- Console: `sd fault <positions|bookmarks|catalog|settings|lastbook> <flip|trunc|del>` and `sd reload` (owner op args 4 and 10+kind*3+mode).
- Device evidence (transcripts `p2-storage-component-boot1.log`, `p2-fault-battery.log`, `p2-heal.log`):
  - Boot scan through the component: `cached=3 hashed=0` in 16 ms, 2 positions loaded, boot restore intact.
  - flip(positions) -> "positions file invalid (len=792); ignoring", reload=CorruptData, fresh state, no crash.
  - trunc(catalog) -> "catalog file invalid (len=2572)", next scan hashes everything (0 cached, 3 hashed, 85 ms) and rewrites the catalog (catalog_writes=1).
  - del(settings) -> "primary settings missing; using backup", reload settings=Ok (.bak fallback proven).
  - Heal: page turns re-dirty positions, coalesced flush wrote them (position_writes=1), steady-state scan cached again (10 ms).

### Why
- The plan's P2 gate: component API on s3paper types only, injected constraints, device-proven recovery.

### What worked
- Move + shim compiled first try; every fault mode recovered exactly as the loaders promise.

### What didn't work
- Console client has no --settle-after flag (invented one, got argparse error); split the heal check into two invocations with a sleep instead.

### What was tricky to build
- Loader semantics distinction worth knowing: .bak fallback only triggers when the PRIMARY IS MISSING (fopen fails). A corrupt primary is ignored-with-log (fresh state), NOT recovered from .bak — by design, since a corrupt primary usually means a mid-write cut and the bak swap happens before rename. The fault battery exercises both paths deliberately (flip vs del).

### What warrants a second pair of eyes
- `DebugCorruptStateFile` ships in the component (not #ifdef'd out). It requires console access, and 0114 wants it for Phase 9; flagging in case someone objects to fault hooks in production builds.

### What should be done in the future
- Phase 3: extract s3paper_runtime the same way.

### Code review instructions
- `components/s3paper_storage/include/s3paper_storage/storage.h` (API), `src/storage.cpp` diff vs old `app_storage.cpp` (git follows the move), `0112/main/app_storage.{h,cpp}` shim.
- Validate: `sd fault positions flip` then `sd reload` on hardware; transcripts in this ticket's scripts/output/.

## Step 4: Phase 3 — s3paper_runtime extracted; 0112 fully on shared components

The present pipeline is now the shared `s3paper_runtime` component: frame storage, fake+M5 backends, refresh planner, font registration, the retained WidgetArena/PageRouter, render-state diffing, region table, and both present entry points (`PresentPage` / `PresentPageUpdate`, invariant comments carried over verbatim). 0112's `app_display.{h,cpp}` keeps only its console fixtures (primitive scene, typography page, soak step) plus using-declaration re-exports; `app_ui.{h,cpp}` keeps only the hello/status widget fixtures and the fixture region tick, now expressed AGAINST the component API instead of shared internals.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Phase 3: extract the present pipeline; 0112 must keep all behaviors including diff updates and trace equivalence.

**Inferred user intent:** 0114 renders through the identical hardware-proven pipeline.

**Commit (code):** c0f9eb7 — "ESP-51 Phase 3: extract s3paper_runtime (present pipeline); 0112 keeps fixtures + shims"

### What I did
- `components/s3paper_runtime/{include/s3paper_runtime/runtime.h, src/runtime.cpp}`: merged app_display core + app_ui core under namespace `s3paper_runtime` with a `RuntimeConfig` (viewport, capacities, planner turn budget, default-font toggle). Fonts register from the s3paper_core embeds inside the component.
- New API surface the fixture layer needed: `FindRegion(id)` (region-spec lookup) and the existing `PresentCount()` contract replaces the old direct `s_fixture_active=false` coupling — the status fixture now detects "someone else presented" exactly like the JS layer does.
- 0112 shims: `app_ui.h` inline-wraps Ui* names onto the component; `app_display.h` re-exports the frame hooks; fixtures stay app-side.
- Device gate (transcripts p3-runtime-smoke-a.log, p3-runtime-blitz2.log): boot restore + page turn OK, `js trace` EQUAL (1083 bytes both), `widget status` region live and ticking `update present: 1 rect(s), damage 460x34 at 40,180` every 2 s, blitz clock exactly one `460x86 at 40,336` rect per second — all logged under the new `runtime:` tag.

### Why
- P3 of the plan; the two update-mode invariants had to move as code+comments, not folklore.

### What worked
- Everything after one missing-include fix. The PresentCount-based fixture decoupling worked first try.

### What didn't work
- First build: `error: 'kFontUi' is not a member of 's3paper'` — new app_ui.cpp lost the transitive `s3paper/text.h` include when app_ui.h stopped including widget headers directly. One-line fix.
- First validation run used `widget 2` (usage is `widget status`) and the wrong blitz start tap (270,260 = label area; the working recipe from ESP-50 diff-final3 is 270,150). Lesson re-learned: read the old transcript BEFORE tapping, coordinates are typography-dependent.

### What was tricky to build
- The fixture region tick previously reached into pipeline internals (s_last_slots/s_diff). Re-expressing it as SetText + `PresentPageUpdate(own_slots,...)` is behaviorally equivalent BUT changes the fallback: the old code clip-fell-back to whole-page within a TextRegion present; the new path falls back to a full TextPage present on rect overflow. For a single clock region this path is unreachable; noting it for review.

### What warrants a second pair of eyes
- `RuntimeInit` uses function-local statics sized by config at FIRST call; a second init with a different config silently keeps the first (documented as idempotent, same as before, but now config exists).
- app_js.cpp still calls reader::Ui* through the shim — fine, but 0114 will use s3paper_runtime:: directly.

### What should be done in the future
- Phase 4: 0114 skeleton on all four components.

### Code review instructions
- Diff `components/s3paper_runtime/src/runtime.cpp` against old `0112/main/{app_display.cpp,app_ui.cpp}` (logic verbatim; only naming/config changed).
- Validate: `js trace` (EQUAL), `widget status` then watch 2 s ticks, blitz recipe `js pulp; js tap 270 330; js tap 270 150`.
