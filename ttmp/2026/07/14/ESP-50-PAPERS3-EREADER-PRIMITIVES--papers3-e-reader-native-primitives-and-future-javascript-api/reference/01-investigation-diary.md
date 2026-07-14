---
Title: Investigation Diary
Ticket: ESP-50-PAPERS3-EREADER-PRIMITIVES
Status: active
Topics:
    - papers3
    - eink
    - ereader
    - esp-idf
    - esp32s3
    - m5gfx
    - microquickjs
    - architecture
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: abs:///home/manuel/workspaces/2025-12-21/echo-base-documentation/M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Driver and local patch state examined in Steps 2 and 3
    - Path: repo://0080-papers3-ereader/main/ereader_app.cpp
      Note: Existing reader examined in Step 2
    - Path: repo://0106-papers3-epd-qualification/README.md
      Note: Operator build/flash/monitor and visual qualification procedure
    - Path: repo://0106-papers3-epd-qualification/main/app_main.cpp
      Note: Standalone Phase 0 harness, boundary corpus, diagnostics, soaks, sleep/wake, and waveform comparison (commit 62b7b8e)
    - Path: repo://0107-papers3-epd-painter-control/main/app_main.cpp
      Note: |-
        Step 14 no-drive firmware entrypoint (commit f7c3e7347ebe75c9d654a9c9d92a5ae7f439dfd7)
        Step 15 bounded command implementation (commit e9f3769dc417adb1623ac0a1435b891c5f936d0f)
    - Path: repo://0107-papers3-epd-painter-control/main/fixtures/reader_page.bin
      Note: Deterministic reader fixture (commit e9f3769dc417adb1623ac0a1435b891c5f936d0f)
    - Path: repo://0107-papers3-epd-painter-control/sdkconfig.defaults
      Note: Exact tick, PSRAM, console, and partition defaults (commit f7c3e7347ebe75c9d654a9c9d92a5ae7f439dfd7)
    - Path: repo://0108-papers3-m5gfx-runtime-trace/main/epd_trace_runtime.cpp
      Note: Step 18 fixed-ring implementation (commit 2badb87)
    - Path: repo://0109-papers3-factory-v0.5-runtime-trace/main/main.cpp
      Note: Step 19 exact factory sequence and post-idle F2 dump (commit 4ab273a)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md
      Note: |-
        Step 12 pre-hardware audit and decision (commit 4c1c89c76e22768d142310b75db631132379a711)
        Step 13 independent-control experiment protocol (commit e7e4848d9544b902dcf79246fa520f039c2d74ee)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/00-research-log.md
      Note: Retroactive reproducibility trace requested by the user
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/05-add-phase-tasks.sh
      Note: Idempotent source for the detailed phase task breakdown
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/09-replay-factory-v0.5-flash.sh
      Note: Safe replay control and non-destructive check (commit 4c1c89c76e22768d142310b75db631132379a711)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/10-audit-epd-painter.py
      Note: Reproducible audit implementation (commit 4c1c89c76e22768d142310b75db631132379a711)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/14-generate-epd-control-fixtures.py
      Note: Fixture generation and identity (commit e9f3769dc417adb1623ac0a1435b891c5f936d0f)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/29-capture-synchronized-serial.py
      Note: Step 20 shared-clock serial capture and guarded raw mode (commit ec2bf1b)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/10-epd-painter-pre-hardware-audit.md
      Note: Expanded eight-blocker audit (commit e7e4848d9544b902dcf79246fa520f039c2d74ee)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/12-epd-painter-build-latest.md
      Note: Final build identity (commit f7c3e7347ebe75c9d654a9c9d92a5ae7f439dfd7)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/13-built-control-audit-latest.md
      Note: |-
        Passing no-drive binary gate (commit f7c3e7347ebe75c9d654a9c9d92a5ae7f439dfd7)
        Passing P0.16 pre-flash audit (commit e9f3769dc417adb1623ac0a1435b891c5f936d0f)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/17-p0.17-hard-white-observation.md
      Note: Chronological P0.17 HARD-white optical result
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/20-m5gfx-runtime-trace-audit-latest.md
      Note: Step 18 observer-effect evidence (commit 2badb87)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/25-factory-v0.5-trace-audit-latest.md
      Note: Step 19 19-check audit (commit 4ab273a)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/29-printalyzer-passive-reference-result.md
      Note: Step 21 passive calibrated reference result (commit 3dc771a)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/29-synchronized-serial-capture-validation.md
      Note: Step 20 validation and preserved observer reset failure (commit ec2bf1b)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/patches/11-epd-painter-pure-idf-hardening.patch
      Note: Audited source hardening (commit f7c3e7347ebe75c9d654a9c9d92a5ae7f439dfd7)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/code/epd-painter-753c521da8aef59756df07c1a4eb88f1c64c8227/src/EPD_Painter.cpp
      Note: Source that established audit blockers (commit 4c1c89c76e22768d142310b75db631132379a711)
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/2026-07-14-cell-C/01-tmux-live-transcript.txt
      Note: Cell C flash, boot, boundary, sleep/wake, status, and waveform comparison transcript
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/hardware/2026-07-14-cell-C/03-operator-observations.md
      Note: Human visual findings for washed black and ghosting
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/local/s3paper-api-design.md
      Note: Imported source recorded in Step 1
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/sources/local/s3paper-studio.jsx
      Note: Imported implementation recorded in Step 1
ExternalSources:
    - https://docs.m5stack.com/en/core/PaperS3
    - https://github.com/m5stack/M5GFX/issues/181
    - https://github.com/m5stack/M5GFX/releases/tag/0.2.25
    - https://github.com/bellard/mquickjs
Summary: Chronological record of importing the s3paper prototype, inspecting local PaperS3 firmware, researching upstream driver/toolchain state, and designing the native-first phased reader plan.
LastUpdated: 2026-07-14T16:30:00-04:00
WhatFor: Resume or review the ESP-50 research without rediscovering its evidence, failures, or decisions.
WhenToUse: Read before continuing implementation or revisiting the toolchain and MicroQuickJS conclusions.
---













# Diary

## Goal

Record how the PaperS3 e-reader primitive plan was assembled from the user's JavaScript prototype, existing local firmwares, prior display investigations, and current upstream sources. The diary also records failed assumptions and exact research commands so another engineer can continue without repeating discovery work.

## Step 1: Create the ticket and preserve the design inputs

I created a new docmgr ticket before drawing conclusions, then imported the user-authored API design and its executable studio companion. The prompt repeated the Markdown path, but the Downloads directory contained exactly two matching `s3paper*` files; preserving the JSX companion was necessary to understand what the prose design actually implemented.

The result was a ticket with a design document, diary, source directories, and the two local inputs copied verbatim. No firmware or nested dependency source was modified.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to create an eink reader using the s3paper.

I created a pretty clever JS API to do prototyping, but while I would like to impelment it in the long term with microquickjs, I think it's important first to build and test the primitives that can later be used to implement the JS layer.

Import the files ~/Downloads/s3paper-api-design.md and ~/Downloads/s3paper-api-design.md and reaed them and then create a ticket with series of phases to implement more and more of the necessary primitives for the s3paper. Also look at existing firmwares for that device that are in this project as inspiration. 

The tool chain v5.3.3 is kind of important, because it is necessary to avoid waveform eink corruption, but maybe that has been fixed by other people on the other internet (https://github.com/m5stack/M5PaperS3-UserDemo), we have docs in ../M5PaperS3-UserDemo/ and maybe here. Download any interesting resources you find with defuddle into the sources/ folder.

Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a new research/design ticket for a native-first PaperS3 reader, import and analyze the local JS design/prototype, investigate local and upstream EPD/toolchain evidence, produce phased intern-grade documentation, and deliver it to reMarkable.

**Inferred user intent:** Preserve the appealing JavaScript product API while avoiding a premature runtime port; instead, create a reliable native substrate that can later support MicroQuickJS.

**Commit (code):** N/A — documentation/research only

### What I did

- Ran `docmgr status --summary-only` and inspected existing ticket IDs.
- Created `ESP-50-PAPERS3-EREADER-PRIMITIVES`.
- Added a `design-doc` and a `reference` diary.
- Inspected `~/Downloads` and found:
  - `s3paper-api-design.md`
  - `s3paper-studio.jsx`
- Copied both to `sources/local/`.

### Why

- Ticket-first setup keeps the investigation and future phases searchable.
- The Markdown describes semantics, while the JSX reveals actual data flow, approximations, and callbacks.

### What worked

- The ticket was created cleanly with the requested topics.
- The imported files were readable and mutually explanatory.

### What didn't work

- Reading a project-root README failed because none exists:

  ```text
  ENOENT: no such file or directory, access '/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/README.md'
  ```

- The prompt named the same Markdown path twice, so a literal interpretation would have imported one file twice. I instead used the only companion `s3paper*` file and documented that interpretation.

### What I learned

- The API proposal is intentionally e-ink-opinionated: pages, regions, pagination, quiet scheduling, and refresh policy are first-class.
- The studio's architecture header explicitly proposes `builders -> tree -> layout -> draw ops -> backend`, which is the right native boundary.

### What was tricky to build

- The input ambiguity had to be resolved without inventing a missing filename. Listing matching Downloads files produced a concrete, auditable answer.

### What warrants a second pair of eyes

- Confirm that `s3paper-studio.jsx` was indeed the intended second input. It is strongly implied by the file set and content, but the original prompt repeated the Markdown name.

### What should be done in the future

- Keep local design imports immutable; update architecture conclusions in the ticket design doc rather than editing the source snapshots.

### Code review instructions

- Start with `sources/local/s3paper-api-design.md`.
- Then read the header and engine sections of `sources/local/s3paper-studio.jsx`.
- Verify hashes in `scripts/output/02-source-manifest.txt`.

### Technical details

```bash
find "$HOME/Downloads" -maxdepth 1 -type f -iname '*s3paper*' -printf '%f\n' | sort
```

## Step 2: Map the local PaperS3 implementation history

I inventoried the PaperS3 firmware lineage and read the existing native reader from storage through presentation. The local projects form a useful progression: touch drawing, gesture handling, persistent data, retained layout, a reader vertical slice, and queued guest drawing.

The important result was not “reuse 0080.” It was a sharper separation between proven primitives and prototype shortcuts. The current reader proves product flow but also exposes why a new foundation is needed.

### Prompt Context

**User prompt (verbatim):** (same as Step 1; the same prompt was submitted twice)

**Assistant interpretation:** Use existing PaperS3 projects as evidence and identify which parts are reusable primitive contracts versus demo-specific code.

**Inferred user intent:** Avoid starting from scratch while also avoiding accidental inheritance of fragile reader and display behavior.

**Commit (code):** N/A — documentation/research only

### What I did

- Inventoried `0075`, `0076`, `0077`, `0078`, `0079`, `0080`, and `0082`.
- Read the complete reader app, storage, paginator, bookmarks, layout, dirty tracker, renderer, font, console, build, and configuration files in `0080`.
- Read the factory demo's HAL, display acceptance patterns, component pins, and build configuration.
- Read prior EPD, reader-crash, WAMR/display, and loader postmortem ticket documents.
- Captured source anchors in `scripts/output/04-line-anchors.txt`.

### Why

- Major recommendations need file-backed evidence.
- Earlier failures contain hard-won constraints around PSRAM, display rotation, allocation, and guest/runtime isolation.

### What worked

- `0080` provides a complete baseline for library -> pagination -> page turn -> partial refresh -> bookmark.
- `0075` provides a simple responsive input/EPD pattern.
- `0078` provides node/layout/damage concepts.
- `0079` provides a bounded intent queue suitable as a future scripting precedent.
- The ESP-37 diary identifies the local M5GFX LUT allocation and boot-rotation failures precisely.

### What didn't work

- A broad `rg` over the entire project produced more than the tool's 50 KB output limit and was truncated. I replaced it with focused per-firmware inventories and scripted line-anchor queries.
- The existing `0080` design document describes a useful first reader but cannot be treated as a production contract: some planned constants and assumptions differ from the implemented code.

### What I learned

- `0080` paginates by character count rather than glyph metrics and stores a zero byte offset in bookmarks.
- `0080` and `0078` let console callbacks mutate app/display state outside the UI loop.
- The existing 5x7 font maps lowercase to uppercase and drops characters above its small range.
- `0079`'s queue/replay boundary is more relevant to future JavaScript than its specific WAMR runtime.

### What was tricky to build

- Prior PaperS3 tickets contain multiple overlapping hypotheses. The final ESP-37 and ESP-44 conclusions had to be distinguished from earlier, later-falsified theories.
- The local nested M5GFX repository is on a modified debugging branch, so local behavior cannot be attributed to the nominal 0.2.15 tag without inspecting Git status and diffs.

### What warrants a second pair of eyes

- Review the proposed “single UI owner task” against any future storage worker design. Storage may run elsewhere, but it must return immutable results through events.
- Review whether the local `_lut_2pixel` allocation patch remains necessary against current upstream M5GFX.

### What should be done in the future

- Use the existing firmwares as test vectors and source references, not as shared mutable dependencies.
- Add regression fixtures for every local bug promoted into an architectural invariant.

### Code review instructions

- Start at `0080-papers3-ereader/main/ereader_app.cpp:23`, then follow `OpenBook`, `ComputeTotalPages`, `LoadCurrentPage`, and `ProcessDirtyRefresh`.
- Read `0080-papers3-ereader/main/paginator.cpp:21-222`.
- Compare with `0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp:24-344`.

### Technical details

The reproducible inventory command is `scripts/01-inventory-local-evidence.sh`; its current output is `scripts/output/01-local-inventory.txt`.

## Step 3: Research current upstream PaperS3 and MicroQuickJS state

I searched current primary sources to test the assumption that ESP-IDF 5.3.3 remains uniquely safe. The result changes the implementation plan: 5.3.3 remains the factory control, but newer M5GFX releases contain explicit ESP-IDF 5.4 and PaperS3 fixes that deserve a controlled hardware qualification rather than automatic rejection.

I also read current MicroQuickJS documentation and C API examples. It is promising, but its stricter mostly-ES5 language, moving compacting GC, fixed memory arena, and unverified unstable bytecode make it a later bounded experiment, not an initial dependency.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Determine whether waveform/toolchain constraints have changed upstream and collect authoritative sources into the ticket.

**Inferred user intent:** Avoid both unsafe upgrades and needless permanent dependence on an obsolete toolchain.

**Commit (code):** N/A — documentation/research only

### What I did

- Searched for PaperS3 ESP-IDF, M5GFX waveform, heap-corruption, and driver issues.
- Downloaded official and external sources with Defuddle into `sources/web/`.
- Queried Issues 119, 152, and 181 plus release/commit history through `gh api`.
- Verified:
  - M5GFX 0.2.17 explicitly fixed PaperS3 under ESP-IDF 5.4.
  - commit `33f8ce25e969...` fixed Issue 181's two heap-corruption defects.
  - M5GFX 0.2.25 contains that commit.
- Captured M5GFX 0.2.25, M5Unified 0.2.18, and current MicroQuickJS HEAD state.
- Read external embedded-reader architectures for EPUB and SD-backed caches.

### Why

- Toolchain decisions affect panel health and correctness.
- Release notes alone are not enough; commit ancestry and issue details make the claim testable.
- The future JS design must be constrained by the actual runtime API rather than by QuickJS assumptions.

### What worked

- Defuddle produced useful primary-source Markdown for the official hardware page, Issue 181, M5GFX docs, MicroQuickJS, and reader references.
- GitHub API evidence clarified details omitted by search snippets and sparse Defuddle issue extraction.
- Upstream commit ancestry proved that the Issue 181 fix is in 0.2.25.

### What didn't work

- Defuddle emitted useful GitHub release content but returned a metadata-stage error:

  ```text
  Failed to parse URL: TypeError: Invalid URL
      at new URL (node:internal/url:818:25)
  ...
  code: 'ERR_INVALID_URL',
  input: '/m5stack/M5GFX/releases'
  ```

- Defuddle's Issue 152 capture contained only the opening report, not the maintainer discussion. The GitHub API snapshot was required for the full evidence.

### What I learned

- The “5.3.3 requirement” is partly a driver-version compatibility story. M5GFX 0.2.17 explicitly corrected the 5.4 path.
- Current M5GFX 0.2.25 includes Issue 181 safety fixes, but still allocates the EPD LUT with `MALLOC_CAP_DMA`; the local allocation concern must be tested separately.
- MicroQuickJS does not use general `malloc/free` internally and accepts a caller-owned arena, but C bindings must root moving values with `JSGCRef`.
- MicroQuickJS bytecode has no compatibility guarantee and is not verified; only trusted generated bytecode is acceptable.

### What was tricky to build

- “Fixed upstream” has several meanings. The issue remained open when queried, yet the corrective commit was merged and included in 0.2.25. The report records all three facts.
- Current web facts are date-sensitive. Scripts and snapshots were necessary so later readers know exactly what was true on 2026-07-14.

### What warrants a second pair of eyes

- Validate the proposed A-D toolchain matrix on the actual board before selecting the new firmware baseline.
- Confirm the candidate pairing of current M5GFX/M5Unified versions under each ESP-IDF version; it is a test candidate, not an accepted pin.

### What should be done in the future

- Re-run `scripts/03-query-upstream-state.sh` when implementation begins and review the diff.
- If the LUT allocation patch is still required, minimize it and open/relate an upstream issue.

### Code review instructions

- Read `sources/web/04-m5gfx-issue-181-panel-epd-heap-corruption.md`.
- Inspect `scripts/output/03-upstream-state.txt` for releases and the exact patch.
- Read `sources/web/06-mquickjs-readme.md`, especially C API memory handling and bytecode sections.

### Technical details

```bash
gh api repos/m5stack/M5GFX/releases/tags/0.2.17
gh api repos/m5stack/M5GFX/commits/33f8ce25e96903bc8d11122de81147d8a5cca39b
gh api repos/m5stack/M5GFX/compare/33f8ce25e96903bc8d11122de81147d8a5cca39b...ad9b814264d4e2000e9f30070002310bbccaffc9
```

## Step 4: Make the research process reproducible

The user asked that research work be saved retroactively in the ticket's `scripts/` directory. I converted the ad-hoc discovery into numbered scripts and checked in output snapshots, while keeping downloaded source content under `sources/` as the original prompt required.

This adds a trace from questions, through commands, to current evidence and line anchors. It also documents the ambiguous import and Defuddle failure rather than hiding them.

### Prompt Context

**User prompt (verbatim):** "save all research stuff in the scripts/ folder of the ticket. Retroactively too, so we can trace your steps."

**Assistant interpretation:** Preserve reproducible scripts and outputs for all research already performed, without moving the requested Defuddle source archive out of `sources/`.

**Inferred user intent:** Make the investigation auditable and repeatable instead of leaving only a prose conclusion.

**Commit (code):** N/A — ticket scripts and documentation only

### What I did

- Added `scripts/00-research-log.md`.
- Added numbered scripts for local inventory, source import/download, upstream API queries, and line anchors.
- Ran the non-destructive inventory/query/anchor scripts and saved outputs under `scripts/output/`.
- Added source hashes and sizes to `scripts/output/02-source-manifest.txt`.
- Added `sources/README.md` to map each capture to its provenance and purpose.

### Why

- Current web and Git state will change.
- A later intern should be able to refresh evidence and understand why a conclusion changed.

### What worked

- The scripts reproduced local state, upstream state, and line anchors without changing firmware source.
- Current snapshots total enough detail to audit issue comments and component history offline.

### What didn't work

- I did not blindly rerun all Defuddle downloads after writing the script because that would overwrite already captured sources and repeat the known GitHub metadata failure. The script encodes a safer temporary-file policy for the next run.

### What I learned

- Keeping source captures in `sources/` and research procedures in `scripts/` satisfies both provenance and the original directory request.
- Substantial command output is easier to review as saved snapshots than as truncated agent terminal output.

### What was tricky to build

- Defuddle may return non-zero after writing useful content. The fetch script must distinguish substantial extraction from an error-only file before replacing a prior snapshot.

### What warrants a second pair of eyes

- Review whether generated `scripts/output/` snapshots should remain committed long-term or be regenerated per investigation policy. For this ticket they are intentionally preserved for traceability.

### What should be done in the future

- Add each implementation-phase hardware probe script to this same numbered directory, with exact component SHAs in its output.

### Code review instructions

- Start with `scripts/00-research-log.md`.
- Review scripts in numerical order.
- Compare each script to its corresponding file under `scripts/output/`.

### Technical details

```bash
wc -lc scripts/output/*
# 1,316 lines / 150,605 bytes at the time of capture
```

## Step 5: Synthesize, validate, and deliver the design package

I turned the evidence into a 14-phase native-first implementation guide with architecture diagrams, C/C++ API sketches, pseudocode, decision records, a toolchain qualification matrix, testing strategy, risks, and an explicit future MicroQuickJS boundary. The ticket now distinguishes a useful native-reader milestone from later generic widgets and scripting work.

I validated the docmgr metadata, resolved vocabulary and frontmatter findings, ran Markdown/fence/diff checks, performed a reMarkable dry run, and uploaded a four-document bundle. No firmware build or live hardware probe was run because this ticket's requested deliverable was research and design.

### Prompt Context

**User prompt (verbatim):** (primary deliverable: see Step 1; reproducibility addition: see Step 4)

**Assistant interpretation:** Complete the intern-grade design, create actionable implementation phases, validate the ticket, and deliver the package to reMarkable.

**Inferred user intent:** Hand an unfamiliar engineer a technically credible, traceable path from existing prototypes to a native reader and eventually the fluent JavaScript API.

**Commit (code):** N/A — documentation/research only

### What I did

- Wrote `design-doc/01-papers3-e-reader-primitives-analysis-design-and-implementation-guide.md` (over 8,000 words).
- Added 14 open docmgr tasks covering Phases 0 through 13.
- Updated the index, README, source inventory, diary, and changelog.
- Related the focused design and diary to the seven/five most relevant files.
- Added vocabulary entries for `eink` and `ereader`.
- Ran frontmatter validation, `docmgr doctor`, `git diff --check`, and code-fence parity checks.
- Ran a dry-run and real `remarquee upload bundle`.

### Why

- The requested guide needed to be implementation-ready, not a high-level brainstorm.
- Docmgr and delivery validation prevent a large document from becoming an inaccessible artifact.

### What worked

- All three ticket docs passed frontmatter validation.
- Final doctor result:

  ```text
  ## Doctor Report (1 findings)

  ### ESP-50-PAPERS3-EREADER-PRIMITIVES

  - ✅ All checks passed
  ```

- The reMarkable upload reported:

  ```text
  OK: uploaded ESP-50 PaperS3 E-Reader Primitives Guide.pdf -> /ai/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES
  ```

### What didn't work

- The first doctor run correctly rejected the research log and unknown topics:

  ```text
  [ERROR] invalid_frontmatter — Failed to parse frontmatter: ... scripts/00-research-log.md frontmatter delimiters '---' not found
  [WARNING] unknown_topics — unknown topics value(s): eink (3 docs), ereader (3 docs)
  ```

  I added valid frontmatter to the research log, added both vocabulary entries, and reran doctor successfully.

- Hand-written `RelatedFiles` entries used a noncanonical one-line `Path: path:note` representation. `docmgr doc relate` normalized them and initially produced duplicates; I removed the old entries and retained only docmgr's `repo://`/`abs://` entries.

### What I learned

- M5GFX upstream evidence supports testing newer ESP-IDF versions, but the safe output is a qualification matrix, not an untested pin recommendation.
- The best JavaScript boundary is a versioned descriptor/event/patch ABI above a native draw-op and reader core, not direct M5GFX bindings.
- The native reader should precede generic retained widgets so abstractions are extracted from working behavior.

### What was tricky to build

- The guide had to reconcile three different states: upstream factory pins, a locally modified nested M5GFX checkout, and current 2026 upstream releases. The toolchain section labels controls, candidates, and accepted evidence separately.
- The guide also had to preserve the JS API's ergonomic intent while explicitly rejecting direct storage of moving JS values or callback execution inside display transactions.

### What warrants a second pair of eyes

- Review Phase 0's component matrix and visual corpus before any implementation begins.
- Review the proposed native C ABI for arena lifetimes, handle generations, and error propagation before Phase 9/11 freezes it.
- Review typography scope and stable locator design with representative real books.

### What should be done in the future

- Begin with task `la6t` (Phase 0); do not scaffold the full reader before selecting a tested driver/toolchain pin.
- Record all live-board reset/attach behavior and output in new numbered ticket scripts.

### Code review instructions

- Start with the executive summary and Sections 4, 6, 8, 14, and 15 of the primary design.
- Cross-check claims using `scripts/output/03-upstream-state.txt` and `scripts/output/04-line-anchors.txt`.
- Run:

  ```bash
  docmgr doctor --ticket ESP-50-PAPERS3-EREADER-PRIMITIVES --stale-after 30
  git diff --check -- ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api ttmp/vocabulary.yaml
  ```

### Technical details

The delivered bundle includes:

1. ticket index;
2. primary design/implementation guide;
3. investigation diary;
4. phased task list.

Remote destination:

```text
/ai/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES/ESP-50 PaperS3 E-Reader Primitives Guide.pdf
```

## Step 6: Expand every phase into resumable docmgr tasks

Before implementing Phase 0, I converted the 14 broad phase milestones into a detailed work breakdown. Each detailed task is independently understandable, has a stable `[PN.x]` key plus a docmgr task ID, and describes a concrete artifact or validation outcome.

The ticket now has 14 milestone tasks and 146 detailed tasks. The task header explains ordering, completion rules, hardware-evidence requirements, and the expected code-commit then diary-commit rhythm, so a future session can resume at the first unchecked detailed task.

### Prompt Context

**User prompt (verbatim):** "Ok, go ahead, keep a detailed diary as you work , commit at appropriate intervals, task by task."

**User follow-up (verbatim):** "Add detailed tasks for each phase (break it down) so we can keep detailed track of where you are and pick up where you left off in the future."

**Assistant interpretation:** Begin implementation, but first expand every phase into granular trackable tasks; maintain the strict diary and make focused commits at meaningful task boundaries.

**Inferred user intent:** Make this long-running firmware effort safely resumable across sessions without relying on conversational memory.

**Commit (code):** N/A — task planning and ticket bookkeeping only

### What I did

- Added `scripts/05-add-phase-tasks.sh`, an idempotent task-seeding script.
- Added detailed tasks `[P0.1]` through `[P13.10]` (with phase-specific counts).
- Added tracking conventions and a phase/count/milestone table to `tasks.md`.
- Preserved the original 14 phase tasks as exit-gate milestones.
- Confirmed `docmgr task list` reports 160 total open tasks.

### Why

- The design guide's phase prose is detailed, but prose is not checkable state.
- Stable human-readable keys make task IDs understandable in commits and diary entries.
- Separating detailed tasks from milestone tasks prevents a phase from being marked complete after only its implementation portion while evidence/docs remain unfinished.

### What worked

- The seed script added all detailed tasks in phase order.
- Its idempotence check prevents a normal rerun from duplicating exact task text.
- The resulting phase counts match the task overview table.

### What didn't work

- The first attempt to run the pre-commit scan did not execute because a shell regex contained mismatched quotes:

  ```text
  /bin/bash: -c: line 48: unexpected EOF while looking for matching `''
  ```

  The command failed before staging or committing anything. I postponed the commit until after the requested detailed-task expansion and will use a simpler secret-pattern scan.

### What I learned

- This project is large enough that “one task per phase” is not a useful continuation boundary.
- Phase 0 alone needs separate tasks for the harness, matrix configuration, diagnostics, visual scenes, Issue 181 regressions, soak, power behavior, real-board execution, pin selection, scripts, and documentation.

### What was tricky to build

- The breakdown had to be detailed without coupling future tasks to unproven implementation choices. Tasks therefore specify contracts and evidence, while leaving measured choices such as EPD alignment and typography selection to their qualification tasks.
- Hardware tasks are explicitly distinguishable from host-only tasks so future agents do not accidentally check them after a build-only result.

### What warrants a second pair of eyes

- Review the 146 detailed tasks for missing dependencies before Phase 1 begins.
- Confirm that Phase 0's four-cell matrix remains the desired scope after the first control firmware builds.

### What should be done in the future

- Check detailed tasks individually with `docmgr task check --ticket ESP-50-PAPERS3-EREADER-PRIMITIVES --id <id>` only after their acceptance evidence exists.
- Check a phase milestone only after every corresponding `[PN.x]` task is complete.

### Code review instructions

- Read the conventions and table at the top of `tasks.md`.
- Verify counts with:

  ```bash
  docmgr task list --ticket ESP-50-PAPERS3-EREADER-PRIMITIVES > /tmp/esp50-tasks.txt
  grep -c '^\\[' /tmp/esp50-tasks.txt
  for p in $(seq 0 13); do grep -c "\\[P${p}\\." /tmp/esp50-tasks.txt; done
  ```

### Technical details

Detailed task counts by phase:

```text
P0=12 P1=10 P2=11 P3=10 P4=9 P5=10 P6=11
P7=11 P8=11 P9=11 P10=9 P11=10 P12=11 P13=10
```

## Step 7: Build and begin live qualification of the standalone EPD harness

I created tutorial `0106-papers3-epd-qualification` as a deliberately standalone Phase 0 instrument, pinned clean M5 component checkouts, and built it under the currently installed matrix cells C and D. The harness now exposes deterministic static scenes, explicit waveform comparisons, the Issue 181 edge/rotation corpus, text and mixed-update soaks, heap/timing diagnostics, and display sleep/wake controls over USB Serial/JTAG.

The first Cell C hardware run established that current M5GFX no longer corrupts heap or reboots during the boundary corpus, but it also found a visual policy problem that a compile could not reveal: `epd_quality` renders nominal black as washed-out gray on this panel, whereas `epd_text` produces a deep black checkerboard. I preserved the transcript and operator report, added a four-waveform comparison fixture, and have not accepted a production waveform or toolchain pin.

### Prompt Context

**User prompt (verbatim):** "the all black is not really black, it's a weird washed out grey. I was see a bit of the same wasout effect on the grey example, and the left most bar is not really black either.

checkerboard is fine and nice deep black as far as i can tell.

epd scene text is fine but i can see a bit of ghosing of the checker pattern within the grey swatches


---

it's running the rotation corruption boundary tests right now.

Poll the tmux until done (you can use coarse sleep times, we're on the rotation 3), and then test the last few statuses, then advise, continue.

Keep a detailed diary as you work."

**Assistant interpretation:** Let the active boundary test finish without serial contention, collect its result, run sleep/wake and final diagnostics, preserve the user's visual findings, investigate the mode-dependent black/ghosting behavior, and continue refining Phase 0.

**Inferred user intent:** Qualify real PaperS3 behavior rather than mistaking a successful render or compile for a safe e-reader display policy, while leaving enough evidence for a future session to resume precisely.

**Commit (code):** `62b7b8ec19728dfe8ffc56ca659119b007363277` — "Add PaperS3 EPD qualification harness"

### What I did

- Created `0106-papers3-epd-qualification` with ESP-IDF project files, a custom partition table, USB Serial/JTAG console defaults, and committed `dependencies.lock`.
- Added exact matrix cells and clean component preparation:
  - C: ESP-IDF 5.3.4, M5GFX 0.2.25 at `ad9b814264d4e2000e9f30070002310bbccaffc9`, M5Unified 0.2.18 at `b1ffcc677014ed8bd01e5a1f240736ae654bfe12`;
  - D: ESP-IDF 5.4.2 with the same M5 revisions;
  - A/B remain configured for exact ESP-IDF 5.3.3 and fail closed because that IDF is not installed.
- Added build, flash, matrix-list, clean-component, and one-session serial qualification tools.
- Implemented boot/status diagnostics, static scenes, 1–16-pixel edge/corner updates in rotations 0–3, explicit full logical ranges, text soak, mixed full/partial soak, sleep/wake, and guarded power-off.
- Configured the monitor menu key as `Ctrl-A`; the actual reflash chord is `Ctrl-A`, then `Ctrl-F`.
- Built cells C and D successfully. Cell C produced a 16 MB-flash image with ample partition headroom.
- Flashed Cell C, polled the running boundary command at 20-second intervals, then ran `epd cycle-sleep 2000` and `epd status` through the existing tmux-owned monitor.
- Preserved the live transcript, exact build metadata, and operator findings under `sources/hardware/2026-07-14-cell-C/`.
- Inspected M5GFX 0.2.25's `Panel_EPD` source. `epd_quality`, `epd_text`, `epd_fast`, and `epd_fastest` select distinct LUTs; fast modes also quantize framebuffer writes differently.
- Added `epd waveform <mode> black|white|gray` and `epd waveform-compare`. The latter leaves simultaneous nominal-black columns rendered with QUALITY, TEXT, FAST, and FASTEST for direct operator comparison.
- Corrected reset reason 11 to report `usb` and added physical panel dimensions to status.
- Checked tasks P0.1 and P0.3–P0.7. P0.8 remains open until an explicit display-idle command is implemented and tested.

### Why

- Phase 0 exists to expose driver, waveform, geometry, and power behavior before reader code obscures the source of failures.
- The checkerboard/black discrepancy demonstrates that the same framebuffer value is not enough to predict physical output; refresh intent must explicitly select a qualified waveform.
- Capturing the serial transcript and human visual report together distinguishes machine safety from display quality.

### What worked

- The second flash completed at 115200 baud and the board autodetected as `board_M5PaperS3`.
- Boot reported one EPD display, physical/logical 960×540 geometry, 8 MiB PSRAM, and passing heap integrity.
- The boundary corpus completed all four rotations without reboot, prompt loss, heap failure, or command failure.
- Sleep/wake completed in 2003 ms and restored the text scene; final heap integrity passed.
- After 271 updates (11 full and 260 partial), final free internal heap was 305,387 bytes and free SPIRAM was 7,088,072 bytes. Average measured transaction time was 1,831 ms; maximum was 2,837 ms.
- Both ESP-IDF 5.3.4 and 5.4.2 compile the current component/API combination.
- The four-column waveform comparison compiled, reflashed, ran, and returned `command.result=pass` with heap integrity intact.

### What didn't work

- The first live flash at 460800 baud lost the USB connection at 21%:

  ```text
  Lost connection, retrying...
  serial.serialutil.SerialException: device reports readiness to read but returned no data (device disconnected or multiple access on port?)
  serial.serialutil.SerialException: Could not configure port: (5, 'Input/output error')
  ```

  No competing holder existed. Moving to another host USB port and lowering the requested baud to 115200 produced a complete flash. This attach/cable/port sensitivity remains part of the board evidence.
- The first C build failed under `-Werror` because M5GFX dimensions are `int32_t` on this toolchain while the format strings used `%d`, and because untyped `std::max(0, int32_t_value)` could not deduce one type. I replaced the formats with `PRI` macros and supplied explicit `int32_t` template arguments.
- ESP-IDF 5.3.3 is absent at `~/esp/esp-idf-5.3.3`; matrix A/B correctly stop with:

  ```text
  error: ESP-IDF 5.3.3 is not installed at /home/manuel/esp/esp-idf-5.3.3
  install the exact toolchain before building matrix cell A
  ```
- Sending `Ctrl-A` followed by lowercase `f` did not reflash:

  ```text
  --- Error: Unknown menu character 'f'
  ```

  The monitor command is `Ctrl-A`, then `Ctrl-F`; this exact distinction is now in the README.
- Visual Cell C findings are not an unconditional pass:
  - quality-mode full black is washed-out gray;
  - quality-mode grayscale has no convincing black endpoint;
  - the quality-mode text scene retained checker-pattern ghosting in gray swatches;
  - text-mode checkerboard black looked deep and clean.

### What I learned

- On this PaperS3/M5GFX combination, `epd_quality` means the grayscale-capable LUT, not “best black-and-white output.” `epd_text` is currently the stronger candidate for reader text and high-contrast fixtures.
- Ghost cleanup cannot be inferred from `startWrite`/`endWrite` success. It needs history-aware full-refresh policy plus visual acceptance.
- M5GFX's four EPD modes are materially different physical operations, so the future `PresentIntent` mapping must be measured and documented rather than named heuristically.
- Lowering the requested baud was part of the successful recovery, although esptool's compressed-write throughput means baud alone may not explain the earlier physical disconnect; the host port change is confounded with it.

### What was tricky to build

- The display driver performs asynchronous queued EPD work. Every scene and boundary update therefore has to wait before changing mode, wrap drawing in one transaction, and wait again before recording timing or checking heap. Omitting the second wait would report success while physical work was still in flight.
- The Issue 181 corpus had to exercise logical width/height after each rotation and explicitly submit the full logical range, while keeping every tiny edge rectangle in bounds. The historical bug was rotation/range arithmetic in the driver, not merely a visually malformed rectangle.
- Live work had to preserve serial single ownership. Polling used `tmux capture-pane`; commands were injected into the existing monitor pane rather than opening pyserial or a second monitor.
- Comparing waveforms is history-sensitive. The comparison fixture first establishes a common white quality baseline, then updates four non-overlapping black columns so all modes remain visible simultaneously. Even this does not eliminate panel history as a variable, so operator notes remain essential.

### What warrants a second pair of eyes

- Confirm the visual ordering and black density of the four waveform-comparison columns; automatic `pass` only proves the driver/heap path completed.
- Review whether `epd_text` should be the default clean monochrome mode and whether grayscale should be isolated to regions that genuinely require it.
- Review the boundary fixture's visual result, especially at the physical edges after rotation 3, despite the automatic pass.
- Review whether the USB disconnect was solely a connection problem or whether repeated flashing at 460800 can reproduce it on the new port.

### What should be done in the future

- Record the operator's QUALITY/TEXT/FAST/FASTEST comparison and the visual sleep/wake/boundary disposition.
- Add and test an explicit display-idle command, then complete P0.8.
- Run a short text/mixed smoke soak before committing to the full 1,000-update corpus.
- Flash and repeat the identical corpus with Cell D. Install exact ESP-IDF 5.3.3 before attempting cells A/B; do not substitute 5.3.4.
- Preserve photos under the Cell C evidence directory and do not select the accepted Phase 0 pin until all required matrix evidence is reviewed.

### Code review instructions

- Start with `0106-papers3-epd-qualification/main/app_main.cpp`, especially `DrawTransaction`, `RunBoundaryRotation`, `RunSoak`, and `DrawWaveformComparison`.
- Review `matrix/cells.tsv` and the preparation/build scripts for exact-tag and dirty-checkout enforcement.
- Inspect `sources/hardware/2026-07-14-cell-C/01-tmux-live-transcript.txt` beside `03-operator-observations.md`.
- Validate with:

  ```bash
  ./0106-papers3-epd-qualification/tools/prepare_matrix_components.sh
  ./0106-papers3-epd-qualification/tools/build_matrix_cell.sh C
  ./0106-papers3-epd-qualification/tools/build_matrix_cell.sh D
  git diff --check -- 0106-papers3-epd-qualification
  ```

### Technical details

Cell C final automatic status before the waveform-comparison reflash:

```text
boundary.rotation=0 result=pass
boundary.rotation=1 result=pass
boundary.rotation=2 result=pass
boundary.rotation=3 result=pass
display_sleep_cycle_ms=2003
heap.integrity=pass
updates.total=271
updates.full=11
updates.partial=260
updates.average_ms=1831
updates.max_ms=2837
```

The source-level explanation for mode-dependent output is anchored in the exact M5GFX checkout: `Panel_EPD.cpp` selects separate quality/text/fast/fastest LUTs, and `enum.hpp` assigns mode IDs 1–4. The evidence does not yet justify modifying those LUTs locally.

## Step 8: Interpret the four-waveform black comparison

The simultaneous four-column fixture showed that none of the partial-update waveforms produces an ideal solid black. FASTEST reaches the deepest black but leaves texture, FAST is uniform but gray, TEXT is lighter and textured, and QUALITY is both light and visibly uneven. This rules out selecting a mode solely by its enum name or nominal black value.

Because the earlier full checkerboard under TEXT looked much deeper than the partial TEXT column, I ran a controlled full-screen TEXT transition from white to black. The operator found that result very light—almost white—with slight ghosting, so TEXT strongly fails as a clean solid-black waveform in Cell C despite its good checkerboard appearance.

### Prompt Context

**User prompt (verbatim):** "faster is deepest black, altohugh it has some \"texture\". Fasst is grey but very uniform, text is light with some texture. quality has some weird gradients and is lightest. It's not great."

**User follow-up (verbatim):** "it's light, slight ghost. which idf are you using?"

**User clarification (verbatim):** "it's very light in fact, almost white"

**Assistant interpretation:** Treat “faster” as the FASTEST column and “Fasst” as FAST, preserve the visual ranking, investigate why the prior full-screen/mixed TEXT fixture looked better than the partial TEXT comparison column, and identify the exact active matrix cell/toolchain.

**Inferred user intent:** Avoid freezing an unattractive waveform policy and continue toward an e-reader-specific refresh strategy that balances density, texture, uniformity, and ghost cleanup.

**Commit (code):** N/A — no firmware change in this step; the already committed diagnostic was exercised further

### What I did

- Recorded the operator's per-column comparison in `sources/hardware/2026-07-14-cell-C/03-operator-observations.md`.
- Preserved the post-comparison status: heap integrity passed after two full and four partial comparison updates.
- Issued `epd waveform text white`, waited for completion, then issued `epd waveform text black` and waited for the REPL to return.
- Left the full-screen TEXT black result visible for operator assessment; the operator reported it was very light—almost white—with slight ghosting.
- Confirmed the active firmware is matrix Cell C: ESP-IDF 5.3.4, M5GFX 0.2.25, and M5Unified 0.2.18.

### Why

- The four-column comparison uses partial, non-overlapping updates after a QUALITY white baseline. It is excellent for relative comparison but does not reproduce the full-screen checkerboard's transition history.
- A white-to-black full-screen transition under one mode is a cleaner test of whether TEXT can reach acceptable black when it owns the complete update.

### What worked

- All comparison and controlled-transition commands returned `command.result=pass` without a reboot or heap failure.
- The comparison gave a clear relative ranking rather than the earlier ambiguous “washed out” description.

### What didn't work

- None of the four partial black columns was visually acceptable without qualification, and the controlled full-screen TEXT black was almost white with slight ghosting.
- My first attempt to queue the black command immediately after polling the white command did not reach the REPL. The poll had matched an old prompt before the new transaction was visibly complete. I resent the black command only after confirming the white command's fresh `command.result=pass`; it then completed normally.

### What I learned

- Black density depends on waveform, update extent, previous physical state, and likely spatial transition pattern—not just framebuffer color.
- FASTEST's density does not make it a clean-refresh default; its texture and no-erase fast path make it a candidate only for transient/local updates.
- FAST's uniform gray may be useful for low-latency interaction, but it is not a final reader-page appearance.
- The reader should be judged primarily with realistic white-page/black-text fixtures and controlled cleanup cycles, not only with pathological full-black fills.

### What was tricky to build

- Tmux pane polling includes old prompts and results. A completion check must identify output specific to the newly issued command, not merely any trailing prompt, before sending the next transaction.
- EPD comparisons are stateful experiments. Updating four strips sequentially means each strip has a different waveform but shares the same earlier physical baseline; a full-screen follow-up is needed before attributing the differences entirely to partial geometry.

### What warrants a second pair of eyes

- Review why full-screen TEXT black remains light/ghosted while the TEXT checkerboard's black cells looked deep; likely transition pattern and neighboring white pixels materially affect perceived density.
- Review whether the comparison should be repeated in reversed mode order to detect order or accumulated-history bias.

### What should be done in the future

- Treat Cell C's full-screen TEXT black as a visual failure and preserve it in the matrix evidence.
- Add a realistic reader-page waveform fixture and test page-turn plus cleanup sequences.
- Repeat the same scenes under Cell D (ESP-IDF 5.4.2) before blaming or accepting M5GFX's waveform behavior.
- Do not select FASTEST as the reader default merely because it produced the darkest partial column.

### Code review instructions

- Compare the four-column observations with `DrawWaveformComparison` in `0106-papers3-epd-qualification/main/app_main.cpp`.
- Review the controlled commands near the end of the tmux transcript: `epd waveform text white` and `epd waveform text black`.

### Technical details

Four-column visual ranking, left to right:

```text
QUALITY  lightest; strange gradients
TEXT     light; textured
FAST     gray; very uniform
FASTEST  deepest black; textured
```

The controlled full 960×540 TEXT-mode black update after a full TEXT-mode white update was almost white and retained slight ghosting. Active configuration: Cell C, ESP-IDF 5.3.4, M5GFX 0.2.25, M5Unified 0.2.18.

## Step 9: Return to panel physics, analog rails, and waveform representation

Cell D reproduced Cell C's almost-white TEXT black under ESP-IDF 5.4.2, then passed the full boundary corpus and final heap check. That substantially lowers the probability of an ESP-IDF-specific display defect and shifts the investigation toward the ED047TC1 waveform, fixed VCOM, high-voltage rail behavior, temperature, and transition history.

I stopped treating M5GFX mode names as experimental explanations. I downloaded and inspected the official panel datasheet and PaperS3 schematic directly, traced the actual source/gate/VCOM circuits, read the M5GFX phase LUT and update state machine, collected EPDiy waveform documentation plus an ED047TC1-specific header, and wrote a causal investigation plan in `analysis/01-papers3-epd-waveform-and-physical-drive-investigation-plan.md`.

### Prompt Context

**User prompt (verbatim):** "so if you were to go back to fundamentals, and maybe examine the waveform itself or do some physically grounded / electrochemically grounded analysis, how would you proceed? it feels we are digging int the dark a little bit here. no? what are your assessments of the situation?"

**User follow-up (verbatim):** "you can look at images with the read tool yourself, btw"

**User clarification (verbatim):** "i wouldn't trust the tool all that much necessarily."

**Assistant interpretation:** Stop trial-and-error mode cycling; personally inspect primary-source schematic/datasheet images and code, explain the electrophoretic and electrical mechanisms, rank hypotheses, and define discriminating optical/electrical/waveform experiments.

**Inferred user intent:** Reach a causal, physically defensible diagnosis before building reader abstractions or modifying waveforms and hardware blindly.

**Commit (code):** N/A — no firmware changed in this step

**Commit (research/evidence):** `60c3c94c3bf2e724023cedb13a3ccf25c14c117a` — "Docs: analyze PaperS3 waveform fundamentals"

### What I did

- Flashed matrix Cell D: ESP-IDF 5.4.2 with the same M5GFX 0.2.25 and M5Unified 0.2.18 SHAs as Cell C.
- Repeated TEXT white→black; the operator reported the same almost-white result.
- Ran Cell D boundaries 0–3. Final status passed heap integrity after 267 updates.
- Downloaded the official ED047TC1 datasheet and PaperS3 V1.0 schematic, rendered enlarged schematic crops, and inspected them directly with the `read` tool.
- Verified the board's MT3608 source-rail regulator, 120 kΩ/5.1 kΩ feedback divider, discrete VGH/VGL networks, and fixed VCOM divider (5.6 kΩ from VNEG, 1 kΩ to ground).
- Calculated nominal VPOS near +14.7 V and nominal VCOM near −2.27 V from schematic values and normal regulator assumptions.
- Verified the datasheet requires approximately ±15 V source rails, +22/−20 V gate rails, a panel-assigned VCOM within ±0.1 V, and the associated controller/waveform for guaranteed optics.
- Read M5GFX's LUT comments and update state machine. PaperS3 installs no panel-specific LUT; it receives generic quality/text/fast/fastest arrays.
- Downloaded an ED047TC1 EPDiy waveform header. It contains origin→target transition data and 15/30-phase programs rather than M5GFX's simpler target-level LUT plus generic eraser.
- Collected waveform/physics references and wrote a staged diagnosis plan covering known-good firmware, controlled transition matrices, optical measurement, rail/VCOM probing, logic timing, offline LUT decoding, temperature, and safe waveform engineering.

### Why

- Matching failures across C and D make another IDF swap low-value.
- An EPD pixel is an analog state reached by a time sequence of ±15 V/no-op actions relative to VCOM. “Black” is not a framebuffer value that the panel directly understands.
- The observed area/history dependence can arise from waveform calibration, rail droop, VCOM mismatch, or combinations thereof; only controlled experiments can separate them.

### What worked

- Cell D remained machine-stable: all rotations passed, free internal heap was 305,655 bytes, and free SPIRAM was 7,088,344 bytes.
- The enlarged schematic is legible enough to read key component values directly; no interpretation model is needed for the central VCOM conclusion.
- Primary sources align: the datasheet's voltage/VCOM requirements, schematic rail design, M5GFX drive codes, and EPDiy physical explanation form one coherent model.

### What didn't work

- The first separate image-question tool call failed with a network reset. A later tool interpretation also overreached on test points. Direct image inspection showed no TP-designated test points and provided clearer component values, so the analysis does not rely on those speculative claims.
- Plain PDF text extraction destroyed schematic spatial relationships. Rendering and cropping the vector schematic was necessary.
- A current M5GFX mode comparison cannot reveal a vendor waveform's behavior because M5GFX's representation and EPDiy's origin→target representation are structurally different.

### What I learned

- VCOM is fixed in PaperS3 hardware at approximately −2.27 V nominal, not firmware-adjustable. The panel datasheet nevertheless expects its assigned VCOM within ±0.1 V.
- The TEXT black sequence begins with several lighten frames and only a small net darken excess. Its almost-white endpoint is physically plausible when the sequence is not calibrated for the actual panel/temperature/rails.
- FASTEST's dark texture is consistent with its direct no-erase path, not evidence that it is a high-quality reader waveform.
- The deep checkerboard versus pale full black suggests full-area load and spatial pattern must be investigated alongside waveform history.

### What was tricky to build

- Physical EPD behavior spans four layers: electrophoretic particle dynamics, analog high-voltage generation/VCOM, gate/source scan timing, and software transition LUTs. A conclusion drawn from only one layer is underdetermined.
- Waveforms must be assessed as origin→target paths over time. Counting target pulses alone misses generic eraser state, prior physical state, frame duration, and DC-balance obligations.
- Hardware probing has real risk: rails reach about +22 V and −20 V, there are no explicit test points, and an earth-referenced scope ground can damage hardware if attached incorrectly.

### What warrants a second pair of eyes

- Review the schematic calculations and identify the panel's printed VCOM assignment before any resistor change.
- Review whether to port the EPDiy ED047TC1 waveform wholesale or extend M5GFX's transition model.
- Review the proposed high-voltage probing procedure with someone experienced in switching converters and fine-pitch e-paper hardware.

### What should be done in the future

- Run official factory firmware on this same board as the first known-good baseline.
- Install exact ESP-IDF 5.3.3 and run Cell A rather than substituting another IDF.
- Add an offline M5GFX/EPDiy waveform decoder and controlled area/history test fixture.
- Delay long endurance soaks until the nearly-white black endpoint has a causal explanation.

### Code review instructions

- Read the new physical investigation plan first.
- Cross-check rail values against the rendered schematic and datasheet text in `sources/hardware/`.
- Cross-check software claims against M5GFX `Panel_EPD.cpp`, `Bus_EPD.cpp`, and `M5GFX.cpp` at the pinned Cell C/D SHA.

### Technical details

Ranked causes:

```text
1. Generic/non-panel-specific M5GFX waveform or transition model
2. Full-area high-voltage/VCOM droop or ripple
3. Fixed VCOM mismatch for this panel
4. Temperature mismatch
5. Board/panel fault (pending factory baseline)
6. ESP-IDF/color/geometry bug (now low probability)
```

## Step 10: Run the factory control and search for matching failures

The official FactoryTest V0.5 includes exactly the transient optical control we needed: QUALITY full black, QUALITY full white, and sixteen grayscale bars on every boot. I downloaded M5Stack's published merged binary, verified and flashed it without rebuilding through the unavailable IDF 5.3.3 toolchain, and preserved the artifact and flash transcript.

The operator saw a similar concern during factory full black but found the final text dashboard decently crisp. A targeted literature and bug search then produced direct PaperS3 reports of M5GFX-driven progressive gray, residual images, excessive panel strain, unstable gradations, and unresolved driver/circuit/panel hypotheses. The decisive source comparison is that factory M5GFX 0.2.15 and current M5GFX 0.2.25 contain byte-identical quality, text, fast, fastest, and eraser LUTs. Factory V0.5 is therefore an official application/toolchain control, but not an independent waveform control.

### Prompt Context

**User prompt (verbatim):** "go ahead. I'm not sure if the stock firmware has a black / white test."

**User follow-up (verbatim):** "❯ i think it looks like it might have had the same kind of issues when doing a whole black view.

the final dashboard is decently crisp at least for text and such.

Did you search the web for similar bug reports?"

**User clarification (verbatim):** "remember to download relevant resources (potentially with defuddle to have markdown)"

**Assistant interpretation:** Run an unmodified stock control, preserve its visual disposition, search specifically for the broad-black/crisp-text symptom and adjacent PaperS3 optical defects, and download complete reproducible sources rather than relying on snippets.

**Inferred user intent:** Determine whether the observed output is a known PaperS3/M5GFX limitation and identify a genuinely independent driver control before risking local waveform or hardware modifications.

**Commit (code):** N/A — no project firmware source changed; the official factory binary was flashed as a black-box control

**Commit (research/evidence):** `1e25338dcfdd10d4ca716e2ed439339cf8bb83b3` — "Docs: investigate PaperS3 factory display behavior"

### What I did

- Inspected upstream FactoryTest V0.5 source and confirmed `boot_display_test()` holds full black and full white for two seconds each, followed by sixteen grayscale bars.
- Downloaded official release asset `C139-PaperS3-FactoryTest-V0.5_0x0.bin`, SHA-256 `d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624`.
- Exited the sole tmux monitor with `Ctrl-]`, verified the USB Serial/JTAG port had no owner, and flashed the merged binary at address `0x0` with esptool v4.11.0 at 115200 baud.
- Recorded the operator's tentative factory full-black failure and qualified dashboard pass in `sources/hardware/factory-v0.5/02-operator-observations.md`.
- Searched M5GFX, M5Unified, the factory demo, M5Stack community pages, ED047TC1 projects, FastEPD, EPDiy, and broader web results for black, gray, ghosting, smearing, VCOM, and rail symptoms.
- Added `scripts/06-download-epd-bug-reports.py` to preserve complete GitHub issue bodies and comments as Markdown.
- Used Defuddle for the FastEPD PaperS3 grayscale-matrix post and the modern PaperS3 EPDiy forum thread.
- Downloaded a commit-pinned selected snapshot of `tonywestonuk/EPD_Painter`, including PaperS3 waveform presets, hard-clear/DC-balance implementation, documentation, and waveform calibrator.
- Added `scripts/08-compare-m5gfx-luts.py`; it downloaded exact M5GFX 0.2.15 and 0.2.25 `Panel_EPD.cpp` sources and proved all five built-in EPD LUT initializers are identical.
- Wrote `analysis/02-similar-papers3-epd-bug-reports-and-independent-driver-controls.md` with relevance grading and the proposed independent-driver A/B test.

### Why

- The stock result only helps if its software lineage is understood. Reusing the same waveform under a different app cannot distinguish waveform inadequacy from hardware behavior.
- Search snippets omitted crucial maintainer comments about control overload, long-lived panel history, individual panel instability, and M5GFX-versus-EPDiy comparisons.
- A complete, pinned source corpus lets later reviewers distinguish close matches, indirect analog evidence, fixed regressions, and unrelated bugs.

### What worked

- The official merged binary flashed successfully and verified its data hash.
- Factory V0.5 booted and displayed its normal dashboard, showing that the board remains functional after prior qualification tests.
- The targeted search found multiple direct PaperS3 optical-control reports and at least three independent driver paths: EPD_Painter, FastEPD, and EPDiy.
- The LUT comparison produced an unambiguous result: M5GFX 0.2.15 and 0.2.25 use identical built-in quality/text/fast/fastest/eraser pulse arrays.
- Defuddle produced useful Markdown captures for both requested standard web pages.

### What didn't work

- My first image inspection command used esptool's hyphenated `image-info` spelling and failed exactly with:

  ```text
  esptool: error: argument operation: invalid choice: 'image-info'
  ```

  Esptool v4.11.0 expects `image_info`; the corrected command validated the ESP32-S3 image, checksum, and validation hash.
- Initial `gh search issues` calls passed repository-qualified queries in a form the CLI quoted incorrectly and returned:

  ```text
  Invalid search query "repo:\"m5stack/M5GFX PaperS3 black\" type:issue".
  ```

  I switched to GitHub's `search/issues` API with explicit `q` parameters and retrieved the full matches.
- The exact public symptom was not found verbatim. The evidence is a cluster of close PaperS3 driver/gradation/ghosting failures, not a single authoritative duplicate.
- The operator did not separately disposition factory white or the sixteen grayscale bars, so the factory optical record remains partial.

### What I learned

- Factory V0.5 is not an independent waveform baseline: its M5GFX 0.2.15 pulse tables are identical to the M5GFX 0.2.25 tables exercised by Cells C/D.
- A crisp sparse dashboard beside weak broad black fits either broad-area analog loading or a waveform/transition model that is acceptable for text but poor at area fills.
- M5GFX issue 119 directly reports untouched PaperS3 regions becoming progressively gray under M5GFX while EPDiy gave better results.
- M5GFX maintainers have previously identified excessive EPD strain, reverse gradation after release, and individual PaperS3 units with unstable gradations.
- EPD_Painter offers PaperS3-specific high/normal/fast waveform tables and explicit DC-balance/clear operations, making it a higher-information control than another M5GFX version.

### What was tricky to build

- “Factory firmware” sounded independent until source lineage showed it reused the same waveform family. The core challenge was separating application/toolchain independence from drive-algorithm independence.
- Similar bug reports mix several mechanisms: framebuffer overruns, GPIO mode regressions, partial-update policy, panel damage, waveform mismatch, VCOM, and rail ripple. Each report had to be relevance-graded rather than treated as confirmation.
- GitHub issue comments contain the most valuable maintainer evidence, so preserving only issue opening text would have produced a materially misleading source set.

### What warrants a second pair of eyes

- Review the EPD_Painter waveform and power-control code before allowing it to drive this panel; it is active third-party code from 2026, not a panel-vendor waveform.
- Review whether FactoryTest's short two-second black hold allowed a reliable operator judgment and whether a camera/video baseline should be captured on reset.
- Review whether previous M5GFX experiments could have left long-lived panel history before the factory test.

### What should be done in the future

- Build a minimal, pinned EPD_Painter control with hard white clear, HIGH full black, HIGH full white, area-fraction fixtures, realistic text, and explicit cleanup.
- Keep the official factory firmware available for replay; do not repeatedly flash merely to reproduce the same shared LUT.
- Capture factory white and grayscale dispositions if the board is reset again.
- If independent drivers also weaken primarily at high black area, measure PaperS3 rails/VCOM under small and full loads before tuning pulse tables.

### Code review instructions

- Start with `analysis/02-similar-papers3-epd-bug-reports-and-independent-driver-controls.md`.
- Verify factory provenance and flash success under `sources/hardware/factory-v0.5/`.
- Run `scripts/06-download-epd-bug-reports.py` and `scripts/08-compare-m5gfx-luts.py`; compare generated files for a clean reproducibility check.
- Inspect the pinned EPD_Painter M5 preset in `sources/code/epd-painter-753c521.../src/EPD_Painter_presets.h` before any hardware run.

### Technical details

The direct LUT comparison result is:

```text
lut_quality=IDENTICAL
lut_text=IDENTICAL
lut_fast=IDENTICAL
lut_fastest=IDENTICAL
lut_eraser=IDENTICAL
```

Revised interpretation:

```text
factory broad-black concern + crisp dashboard
  does establish: issue is reproducible in the official app/toolchain lineage
  does not establish: independent waveform failure
  next control: independent PaperS3 waveform/scan implementation
```

## Step 11: Publish the complete investigation as a textbook-style Obsidian report

I consolidated the complete reader-program context, electrophoretic foundations, PaperS3 electrical design, M5GFX software path, qualification methodology, Cell C/D results, factory control, related reports, hypothesis ranking, independent drivers, and next experiments into one long-form report. The report is written for a technically capable reader who starts with no e-paper background, so it defines the physical state variables and scan path before interpreting any firmware result.

The ticket report is the source document. The Obsidian article contains the full body rather than a reduced summary, with vault-specific frontmatter, a project-context callout, and links to existing PaperS3 notes. It explicitly corrects older vault material that identified PaperS3 as an IT8951 system; this board directly drives the ED047TC1 panel.

### Prompt Context

**User prompt (verbatim):** "Ok, also  write a detailed project report for the obsidian vault as a deep dive technical analysis blog post using a textbook writing style (no analogies, see skill).
 Commit and push the bsidian vault when done (go-go-parc vault).

For the investigations so far, and all the work and research you did. Go in deep, the readers will know nothing about eink at first, so cover all the base stuff, and explain it in depth . This article can be substantially longer than usual since there is so much ground to cover"

**Assistant interpretation:** Create a substantially longer-than-normal, foundational technical article covering the entire project and investigation; preserve it as a full ticket report and full Obsidian article; use precise textbook prose without analogies; then commit and push only the intended vault note.

**Inferred user intent:** Turn the transient investigation into durable, self-contained engineering knowledge that a new reader can use to understand e-paper physics, reproduce the PaperS3 evidence, and continue the diagnosis safely.

**Commit (code):** N/A — documentation only

**Commit (research report):** `fdb97055f0638e5e16dc29d39d0369956c900ef0` — "Docs: publish PaperS3 EPD technical deep dive"

**Commit (Obsidian vault):** `218cd195a0e593fa3f1c465a5f48896468db422b` — "Research: document PaperS3 EPD qualification" (pushed to `origin/main`)

### What I did

- Loaded the Obsidian research-report, vault-writing, textbook-authoring, and Obsidian Markdown skills plus their writing, source, structure, and delivery references.
- Read the vault's long-form article exemplar and existing PaperS3/e-ink notes to match frontmatter, wikilink, callout, and prose conventions.
- Added `design-doc/02-papers3-e-paper-qualification-deep-dive.md` as the 64 KB primary ticket report.
- Explained electrophoretic particle motion, active-matrix row scanning, source/gate rails, VCOM, temporal grayscale, temperature, transition history, and DC balance before introducing the software stack.
- Covered the complete reader-program context: the fluent prototype, prior firmwares, native-first roadmap, one-owner architecture, and reason MicroQuickJS remains deferred.
- Included four Mermaid diagrams, systematic comparison tables, equations, pseudocode, concrete logs, operator quotes, decision records, acceptance criteria, working rules, and a categorized reference section.
- Created the full Obsidian article at `Research/2026/07/14/ARTICLE - PaperS3 E-Paper Qualification - Physics, Waveforms, and Physical Drive.md`.
- Added vault links to `[[PROJ - PaperS3 E-Reader - Interactive Book Reader on E-Ink]]` and `[[E-Ink Display Driving]]` while preserving the new report as a standalone article.
- Validated ticket frontmatter, vault YAML, balanced code fences, Markdown whitespace, article size, internal-link presence, and the no-analogy requirement.

### Why

- The investigation crosses physics, analog circuitry, scan timing, memory safety, software state, and application architecture. A concise incident summary would omit the causal relationships a new engineer needs.
- The vault already contains older PaperS3 notes with low-level inaccuracies. A new dated report preserves history while establishing a corrected technical reference without overwriting older notes.
- A full source report in the ticket keeps claims tied to downloaded evidence, while the vault copy makes the same depth available where the user reads long-form material.

### What worked

- The source report is 64,886 bytes and approximately 9,000 words, within the requested substantial-report range.
- The vault article is 63,170 bytes and preserves the complete technical body with vault-specific metadata.
- `docmgr validate frontmatter` passed for the ticket report.
- YAML parsing passed for the vault article and confirmed required article metadata.
- Both repositories passed `git diff --check` for the intended files.
- A targeted phrase scan found none of the prohibited analogy patterns.

### What didn't work

- N/A for report generation. The existing vault had unrelated untracked `.pi/npm/`, `.pi/settings.json`, and `.ttmp.yaml` entries; they were intentionally left untouched and will not be staged.

### What I learned

- The earlier vault on-ramp incorrectly described PaperS3 as an IT8951 device and simplified full/partial refresh into fixed sequences. The new report corrects those claims with direct ED047TC1, schematic, M5GFX, and EPDiy evidence.
- The project narrative is clearer when the reader first understands the three distinct states: target framebuffer, driver transition state, and physical panel state.
- The factory result only becomes meaningful after the LUT source comparison; without that context, readers could incorrectly treat factory firmware as an independent waveform validation.

### What was tricky to build

- The article had to teach e-paper from first principles without replacing technical mechanisms with analogies. State variables, field polarity, scan diagrams, equations, and concrete transition tables provide the required clarity.
- The report needed to separate measured facts, operator judgments, source claims, calculated nominal values, and untested hypotheses. Each section labels those evidentiary levels rather than collapsing them into one diagnosis.
- The vault copy needed different YAML and internal links while retaining the full ticket body. The adaptation changes metadata and project context only; it does not shorten the technical report.

### What warrants a second pair of eyes

- Review the simplified particle-displacement equation and ensure readers do not interpret it as a complete electrohydrodynamic model.
- Review the nominal rail and VCOM calculations against the original schematic render and the panel's actual printed VCOM assignment when available.
- Review the characterization of EPD_Painter as the preferred next control; its documentation and source are current, but its safety and optical claims remain unverified on this unit.
- Review the explicit corrections to older vault notes before those notes are separately updated or superseded.

### What should be done in the future

- Build and run the minimal independent-driver control described in the report.
- Create a separate dated correction or update for the older `E-Ink Display Driving` on-ramp rather than silently rewriting its historical content.
- Add objective optical photographs and rail measurements to the article when those experiments are complete.

### Code review instructions

- Read `design-doc/02-papers3-e-paper-qualification-deep-dive.md` from the beginning; it is designed to be self-contained.
- Compare its evidence claims with `analysis/01-*`, `analysis/02-*`, and `sources/README.md`.
- Inspect the vault article frontmatter, project-context callout, Mermaid blocks, and final references.
- Validate with `docmgr validate frontmatter`, `docmgr doctor`, `git diff --check`, and a YAML parse of the vault frontmatter.

### Technical details

```text
Ticket report: 64,886 bytes; 1,154 lines; ~9,000 words
Vault article: 63,170 bytes; 1,139 lines
Mermaid diagrams: 4
Markdown tables/rows: 117
Code-fence markers: 34 (balanced)
Prohibited analogy phrase matches: 0
```

## Step 12: Gate the independent driver with a reproducible pre-hardware audit

I converted the proposed EPD_Painter control from a general next step into six explicit Phase 0 tasks. Before creating firmware, I audited the complete pinned driver against M5GFX's known PaperS3 pin and power path. The audit confirms that the candidate addresses the correct pins and offers a balanced hard-clear primitive, but the upstream source is not approved for direct hardware execution.

The audit found five blockers: incorrect GPIO pad-selection arguments, uninitialized packed state buffers, incomplete allocation validation, asynchronous `paint()` completion with no bounded idle wait, and an unchecked Adafruit framebuffer allocation. It also found two controlled differences that require review: power-off sequencing and an implicit three-stage convergence policy. No independent-driver firmware has been built or flashed.

### Prompt Context

**User prompt (verbatim):** "go ahead. continue keeping a detailed diary and a detailed design / analysis document to explain where we are at, what you found, how it all fits together, what the next steps are, what experiments we are going to run, what they prove, etc...

Then, implement task by task, and keep a detailed diary as you work, and commit at appropriate intervals"

**User follow-up (verbatim):** "save all your experiments in the scripts/ folder of the ticket, retroactively too, so that we can keep a detailed trace of all your work. No firmwares in /tmp or so"

**Assistant interpretation:** Continue Phase 0 through small committed gates, preserve every experiment and replay command under the ticket's numbered scripts, keep firmware projects in numbered repository directories, and maintain a detailed causal design document and diary throughout.

**Inferred user intent:** Make the hardware investigation fully reproducible and reviewable while preventing undocumented temporary experiments or unsafe direct-driver execution.

**Commit (code/research):** `4c1c89c76e22768d142310b75db631132379a711` — "Docs: audit PaperS3 independent EPD control"

### What I did

- Added tasks P0.13 through P0.18 for driver audit, experiment design, firmware build, bounded commands, smoke flash, and controlled optical matrix.
- Expanded `scripts/07-download-epd-painter-reference.sh` to capture the complete build-relevant source directory at commit `753c521da8aef59756df07c1a4eb88f1c64c8227`.
- Expanded `scripts/08-compare-m5gfx-luts.py` to preserve exact M5GFX 0.2.25 board source for pin-map comparison.
- Added `scripts/09-replay-factory-v0.5-flash.sh`. Its default `--check` mode verifies binary SHA, image format, serial path, and port ownership without changing the device; `--execute` refuses to flash if another process owns the port.
- Ran the factory replay check and preserved its timestamped output under `scripts/output/`.
- Added and ran `scripts/10-audit-epd-painter.py`, which parses the pinned sources, compares pin assignments, counts waveform actions, checks source invariants, and emits `scripts/output/10-epd-painter-pre-hardware-audit.md`.
- Removed the preliminary read-only source clone from `/tmp` after the user clarified artifact policy; the complete source now lives under ticket `sources/code/`.
- Created `analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md` and recorded the pre-hardware gate, architecture, findings, and exact allowed patch scope.

### Why

- EPD_Painter directly controls panel scan timing and high-voltage enable signals. A successful public demo does not substitute for reviewing the exact pinned path used on this board.
- The independent-driver result is useful only if local changes preserve waveform content and alter correctness/observability alone.
- Ticket-owned scripts make the source acquisition, factory replay, pin comparison, and safety decision repeatable by another engineer.

### What worked

- EPD_Painter and M5GFX use identical PaperS3 power, control, clock, and D0–D7 pin mappings.
- EPD_Painter's direct-GPIO power-on order matches M5GFX: OE high, 100 µs delay, then PWR high.
- HARD clear uses 6/2/4/8 alternating phases, totaling ten actions of each polarity before a neutral scan.
- Automatic reset-toggle shutdown can be disabled before driver initialization.
- The factory replay check validated the official binary and confirmed that the serial port had no owner without reflashing the board.
- The audit script completed deterministically and set the gate to blocked with five blockers and two review items.

### What didn't work

- The upstream driver cannot be used unchanged. `EPD_Painter.cpp` calls `esp_rom_gpio_pad_select_gpio()` with `GPIO_PIN_MUX_REG[pin]` rather than the GPIO number expected by the ESP-IDF API.
- Packed physical-state buffers are allocated but not initialized before the paint task starts.
- The allocation guard omits `packed_paintbuffer` and `bitmask`.
- `paint()` returns after the task accepts the buffer rather than after panel scanning is complete.
- The Adafruit wrapper calls `memset` without checking whether its PSRAM allocation succeeded.
- The initial audit source was cloned under `/tmp` before the user's explicit artifact-location instruction. It was read-only, never built or flashed, and was deleted after the complete pinned source was captured under the ticket.

### What I learned

- A matching pin map is necessary but does not validate an independent direct driver; initialization, power-down state, completion semantics, and stage count all affect the experiment.
- EPD_Painter's HIGH mode changes both its action tables and inter-pass delay. It is not merely a renamed speed setting.
- The Adafruit binding implicitly selects three-stage convergence, so API call count is not waveform-pass count.
- A first hard clear must be preceded by initialized software state; otherwise even the setup path can perform a differential update from garbage history.

### What was tricky to build

- The source mixes asynchronous application semantics with direct physical control. The code intentionally returns early for UI responsiveness, but that makes it unsuitable for evidence capture until a bounded `waitIdle()` exists.
- Power-off ordering differs from M5GFX. Choosing one sequence requires a documented electrical decision rather than assuming the newer implementation is safer.
- Waveform action counts are descriptive only. Code values, scan duration, chunk direction, rail voltage, and inter-pass delay must be traced before interpreting counts as physical dose.

### What warrants a second pair of eyes

- Confirm the GPIO pad-selection diagnosis against the exact Arduino-ESP32/ESP-IDF headers selected by the eventual build.
- Review whether OE should fall before or after PWR on PaperS3 shutdown, and define safe LE/SPV/SPH states.
- Review the proposed zero-initialized software state plus HARD clear as the first physical reconciliation sequence.
- Trace waveform codes through the assembly packer to source pins before describing code 1/2 as measured panel polarity.

### What should be done in the future

- Complete P0.14 by extending the analysis with the command state machine, safety gates, evidence schema, area/history matrix, and claim table.
- Create the independent firmware only in the next numbered repository directory.
- Add ticket scripts for dependency preparation, patch application, build, flash, serial capture, and result extraction.
- Do not flash until the local hardening patch, build metadata, and first-command gate pass review.

### Code review instructions

- Start with `analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md`.
- Run `scripts/07-download-epd-painter-reference.sh`, `scripts/08-compare-m5gfx-luts.py`, and `scripts/10-audit-epd-painter.py`.
- Review generated `scripts/output/10-epd-painter-pre-hardware-audit.md` against the pinned source lines.
- Run `scripts/09-replay-factory-v0.5-flash.sh --check`; do not use `--execute` while a monitor owns the port.

### Technical details

```text
candidate commit: 753c521da8aef59756df07c1a4eb88f1c64c8227
pin-map result: exact match
pre-hardware gate: BLOCKED
blockers: 5
review items: 2
firmware created: no
firmware flashed: no
board state: official FactoryTest V0.5
serial owner after checks: none
```

## Step 13: Design the bounded independent-driver experiment

I completed P0.14 before creating firmware. The design fixes the toolchain and dependency boundary, defines an explicit no-drive boot state, specifies a constrained hardening patch, and turns every physical operation into a bounded state transition with machine-readable evidence. It also defines the exact transition, area, checker-inversion, and reader-page experiments plus what each outcome can and cannot establish.

A deeper pass over initialization expanded the upstream audit from five to eight blockers. In addition to the first findings, DMA row buffers are dereferenced before their delayed guard, pure ESP-IDF compilation uses an Arduino-only `log_w` macro, and semaphore/task creation is unchecked. The generated audit and long-form analysis now agree on the expanded gate.

### Prompt Context

**User prompt (verbatim):** (same request as Step 12)

**Assistant interpretation:** Finish and commit the safety and causal design for the independent EPD control before implementing its firmware, while preserving every reproducible action in the ticket.

**Inferred user intent:** Ensure the first non-M5 waveform experiment is safe, reviewable, and capable of distinguishing software-waveform behavior from shared physical-panel behavior.

**Commit (code/research):** `e7e4848d9544b902dcf79246fa520f039c2d74ee` — "Docs: design independent PaperS3 EPD experiment"

### What I did

- Re-read the complete raw EPD_Painter initialization, task, clear, power, and packed-buffer paths.
- Added source checks for DMA dereference-before-guard, Arduino-only logging in the pure-IDF path, and unchecked FreeRTOS resource creation.
- Regenerated the audit with eight blockers and two review items.
- Selected pure ESP-IDF 5.4.2 to match existing Cell D while excluding Arduino, Adafruit_GFX, M5GFX, M5Unified, Wi-Fi, touch, SD, RTC, and application frameworks.
- Fixed the future firmware location as `0107-papers3-epd-painter-control/` and required vendor preparation through a numbered ticket script plus exact local patch.
- Defined safe boot and shutdown pin states, unchanged HIGH waveform content, explicit two-stage convergence, and a semaphore-backed bounded `waitIdle()` contract.
- Defined `BOOT_LOCKED`, `WHITE_KNOWN`, `TARGET_KNOWN`, and `FAULT` states with a deliberately small command grammar.
- Specified full-field, centered area, checker inversion, and generated reader-page fixtures.
- Specified fixed-camera optical capture, operator disposition fields, automatic versus optical acceptance, stop conditions, risk controls, and a result-to-hypothesis decision table.
- Checked task P0.14 and committed the design milestone.

### Why

- Changing waveform content while changing drivers would make the result difficult to interpret. The local patch is therefore constrained to correctness and observability.
- A timeout cannot safely be followed by automatic cleanup if the scan task may still own the panel. A terminal FAULT state is required.
- Area fraction, immediate history, mixed-direction transitions, and capture delay are all possible determinants of the pale-black symptom and need separate controlled fixtures.

### What worked

- EPD_Painter's base class can operate without Arduino, making a smaller independent control possible after one logging fix.
- Cell D provides a same-IDF M5GFX comparison baseline, so IDF 5.4.2 can be held constant.
- The active semaphore can prove final scan completion when combined with stage progression; `paintStage == 0` alone cannot.
- The document now gives each experiment an explicit supported claim and non-claim.
- `docmgr doctor` passes with the expanded design and generated audit.

### What didn't work

- The upstream `begin()` zeroes DMA pointers before checking them, so the first allocation guard cannot be retained in place.
- The non-Arduino fallback still calls `log_w`, contradicting the nominal pure-IDF compatibility path.
- `xSemaphoreCreateBinary()` and `xTaskCreatePinnedToCore()` results are not checked.
- A simple poll of `paintStage` cannot prove idle because the task decrements the final stage before performing its scan.
- No firmware was built or flashed in this step; implementation and hardware execution remain gated.

### What I learned

- `clear()` is mostly synchronous because it takes the active semaphore before direct hardware phases, whereas `paint()` returns after target pickup. Their completion contracts must not be treated as equivalent.
- The raw driver with packed 2-bpp fixtures avoids both the large 8-bpp framebuffer and the Adafruit wrapper's implicit three-stage policy.
- A two-stage policy is needed for mixed transition directions within a 64-pixel chunk even when a uniform full field should converge in one active stage.
- The firmware must report commanded origin rather than physical origin; only photographs and operator disposition establish the latter.

### What was tricky to build

- Idle proof has a race if the active semaphore is sampled before the task accepts work. The design relies on `paint()` first observing stage decrement, then `waitIdle()` waiting for final stage progression and taking the semaphore held through the scan.
- A hard cleanup is the preferred boundary but is itself a twenty-action full-panel operation. The protocol uses it sparingly and stops if white residue worsens.
- A same-area rectangle can still vary source/gate loading with geometry. The first area matrix uses centered rectangles and reserves orientation as a follow-up rather than conflating variables.
- Power-off needed an explicit choice. The design mirrors the already-tested M5GFX PaperS3 order and labels that as a control decision, not proof that the upstream order is unsafe.

### What warrants a second pair of eyes

- Review the active-semaphore `waitIdle()` algorithm against all task interleavings before implementation.
- Review the proposed safe-low boot order against ESP-IDF GPIO latch semantics and the PaperS3 schematic.
- Confirm that two-stage non-interlaced convergence handles checker inversion as intended.
- Review whether 30-second origin rest plus 10/60-second captures are sufficient for visible post-drive relaxation.
- Confirm that no local patch accidentally changes waveform arrays, row padding, LCD clock, or hard-clear counts.

### What should be done in the future

- Implement P0.15 in `0107-papers3-epd-painter-control/` with ticket-owned preparation/build scripts.
- Add P0.16's bounded command state machine, deterministic fixtures, runner, and evidence schema.
- Build and inspect with exact ESP-IDF 5.4.2 before requesting any live hardware action.
- Keep the board on FactoryTest V0.5 until P0.17 is explicitly reviewed and ready.

### Code review instructions

- Read the experiment-design half of `analysis/03-epd-painter-independent-driver-audit-and-experiment-design.md`, beginning at “Independent-control experiment design.”
- Regenerate `scripts/output/10-epd-painter-pre-hardware-audit.md` with `scripts/10-audit-epd-painter.py` and confirm eight blockers/two review items.
- Inspect the command state diagram, timeout behavior, Experiment 0/1 gates, and result-to-hypothesis table.
- Validate with `docmgr doctor --ticket ESP-50-PAPERS3-EREADER-PRIMITIVES --stale-after 30`.

### Technical details

```text
implementation project: 0107-papers3-epd-painter-control
selected ESP-IDF: 5.4.2
selected driver commit: 753c521da8aef59756df07c1a4eb88f1c64c8227
upstream audit gate: BLOCKED
expanded blockers: 8
review items: 2
initial quality: HIGH
stage policy: 2, explicit non-interlaced
boot panel operations: 0
first allowed operation: HARD white cleanup + bounded completion
first hardware task: P0.17, not yet started
```

## Step 14: Build the hardened no-drive independent control

I completed P0.15 in the numbered `0107-papers3-epd-painter-control` directory. The project is a pure ESP-IDF 5.4.2 application with a vendored EPD_Painter component reconstructed from ticket evidence plus a zero-fuzz local patch. The current console exposes only help and status; static inspection confirms that no panel operation is reachable from the command surface.

The final build is warning-free and passes a twelve-check source/configuration/ELF audit. No flash or serial attach occurred, so the board remains on FactoryTest V0.5. The implementation also exposed a critical timing requirement: a 100 Hz FreeRTOS tick would truncate EPD_Painter's 4 ms and 8 ms waveform gaps to zero, so the control fixes and records a 1000 Hz tick.

### Prompt Context

**User prompt (verbatim):** (same request as Step 12)

**Assistant interpretation:** Implement the next approved task in a numbered repository firmware directory, preserve every preparation/build/audit attempt under ticket scripts, and stop before hardware flashing.

**Inferred user intent:** Obtain a reproducible independent-driver binary whose local differences are reviewable and whose initial behavior cannot accidentally run a panel waveform.

**Commit (code/research):** `f7c3e7347ebe75c9d654a9c9d92a5ae7f439dfd7` — "Firmware: add hardened PaperS3 EPD control"

### What I did

- Created `0107-papers3-epd-painter-control` with ESP-IDF project files, custom 4 MiB factory partition, octal PSRAM, USB Serial/JTAG, and 1000 Hz FreeRTOS tick defaults.
- Added a status-only `BOOT_LOCKED` application that validates exact PaperS3 dimensions/pins and PSRAM before calling driver `begin()`.
- Added `scripts/patches/11-epd-painter-pure-idf-hardening.patch` and a zero-fuzz preparation script that verifies upstream SHA manifests and waveform byte identity.
- Corrected GPIO mux arguments, safe-low initialization, GDMA return handling, allocation ordering/checks, packed-state initialization, semaphore/task checks, pure-IDF logging, and current IDF API use.
- Added `waitIdle()`, synchronous `powerDown()`, pending-stage, initialized, and panel-power-control-state diagnostics.
- Disabled the boot/NVS/shutdown controller at compile time and excluded its source from the component build.
- Added `scripts/12-build-epd-painter-control.sh`, which recreates sdkconfig/build state, enforces exact IDF 5.4.2, rejects warnings, captures size/hashes, and performs no flash.
- Added `scripts/13-audit-built-epd-control.py`, which checks waveform identity, compile definitions, ELF/archive symbols, no-drive command surface, tick/console/PSRAM config, packed-buffer initialization, warnings, and absence of flash operations.
- Preserved all five build logs and failed/passing binary-audit snapshots under `scripts/output/`.
- Checked task P0.15 and committed the firmware/evidence milestone.

### Why

- The independent control must differ from M5GFX in driver/waveform representation, not in undocumented local pulse edits.
- Safe initialization and a status-only first binary let firmware construction be reviewed separately from physical execution.
- The 1000 Hz tick is required to preserve upstream millisecond delay intent; leaving the default at 100 Hz would silently create a different waveform timing experiment.

### What worked

- Strict upstream `MANIFEST.txt` verification passes.
- The 469-line patch applies with zero fuzz.
- Prepared and upstream `EPD_Painter_presets.h` hashes are identical: `98152d0a16bfe02d4c150617822ebd39dae940884aca7a9d5bcb5900b0169f47`.
- Exact ESP-IDF 5.4.2 clean build succeeds with zero warnings.
- Application size is 293,248 bytes in a 4 MiB partition.
- Static/binary audit passes all twelve checks.
- Build and audit scripts explicitly report `hardware_modified=no`.
- Build directory and generated sdkconfig are ignored and were not committed.

### What didn't work

- The first clean configure failed with:

  ```text
  Failed to resolve component 'esp_driver_gdma' required by component
  'epd_painter': unknown name.
  ```

  IDF 5.4.2 exposes the private GDMA API through `esp_hw_support`; the component requirement was corrected.

- The next compilation failed under IDF warning policy with:

  ```text
  error: this 'if' clause does not guard... [-Werror=misleading-indentation]
  ```

  Braces were added around the existing `dither()` statement without changing behavior.

- The following successful build exposed duplicate `IRAM_ATTR` declaration/definition section attributes and report generation printed:

  ```text
  scripts/12-build-epd-painter-control.sh: line 69: text: command not found
  ```

  The duplicate definition attributes were removed while retaining declaration placement, and Markdown code fences in the expanding heredoc were changed to tildes.

- Initial binary audit failed only because `waitIdle()` was compiled into the component archive but garbage-collected from the status-only ELF. The audit was corrected to inspect the component archive for APIs intentionally unused by P0.15. The failed audit snapshot remains preserved.

### What I learned

- In IDF 5.4.2, `gdma_new_ahb_channel()` and `esp_private/periph_ctrl.h` avoid deprecated compatibility paths used by the upstream source.
- `EPD_DELAY_MS(4)` and `EPD_DELAY_MS(8)` are not preserved by `pdMS_TO_TICKS` at 100 Hz. Toolchain configuration is part of waveform identity.
- A static library symbol check is the correct P0.15 gate for an API deliberately not called by the no-drive application.
- EPD_Painter's assembly consumes almost all separately reported IRAM even in the minimal build.

### What was tricky to build

- Driver initialization crosses GPIO routing, LCD_CAM, private GDMA, internal DMA-capable memory, PSRAM, FreeRTOS resources, and asynchronous power handling. Failure checks had to occur before any pointer dereference or command exposure.
- The local patch had to preserve `EPD_Painter_presets.h` byte-for-byte while still adapting private IDF API use and non-waveform code quality.
- The source uses `IRAM_ATTR` on both declarations and definitions. IDF's section macro includes a counter, so duplicate attributes name conflicting sections; retaining only one placement removes the warning.
- Evidence logs contained cosmetic trailing spaces from CMake/size tables. The build script now normalizes those after capture so committed raw messages pass Git whitespace checks.

### What warrants a second pair of eyes

- Review `waitIdle()` for the final-stage/semaphore ordering described in Step 13.
- Review `PanelPowerGuard::powerDownNow()` and the direct PWR→OE→SPV/CKV/LE/SPH safe-state sequence.
- Review that `EPD_PAINTER_DISABLE_BOOTCTL` removes all reset-toggle and NVS behavior without leaving required lifecycle references.
- Review the 1000 Hz tick as an intended preservation of 4/8 ms gaps.
- IRAM is 16,383/16,384 bytes, leaving one byte. P0.16 must add no IRAM-attributed code and must rerun size validation.

### What should be done in the future

- Implement P0.16's constrained cleanup/target commands, packed fixtures, transaction records, FAULT behavior, and host runner.
- Rerun clean build and twelve-check audit after P0.16.
- Add exclusive flash/smoke scripts but do not execute them until P0.17 review and operator readiness.
- Keep every new command sequence and result under ticket `scripts/output/`.

### Code review instructions

- Start with `0107-papers3-epd-painter-control/README.md` and `main/app_main.cpp`.
- Review `scripts/patches/11-epd-painter-pure-idf-hardening.patch`, then rerun `scripts/11-prepare-epd-painter-control.sh`.
- Run `scripts/12-build-epd-painter-control.sh`; expect exact IDF 5.4.2, zero warnings, and `hardware_modified=no`.
- Run `scripts/13-audit-built-epd-control.py`; expect twelve passes and the IRAM review item.
- Verify `git check-ignore -v` for `build-ticket/` and `sdkconfig.ticket`.

### Technical details

```text
project: 0107-papers3-epd-painter-control
upstream: 753c521da8aef59756df07c1a4eb88f1c64c8227
patch SHA-256: 89e34a7f24060763c3f38aae7d4aaceeb8773e112256f1d21200b4a11fd1557b
app SHA-256: e8cac94e9062a7b1a4cfc4d989d63e4e5bce5181e0d3f70a201b03dfec6ccbe1
ELF SHA-256: fd973bc3f3439a05cca9e1d699a9bb3a0a4e970eea42945a0b5ad317167f98d0
application bytes: 293248
IRAM: 16383 / 16384 bytes
build warnings: 0
binary audit: 12 / 12 pass
panel commands exposed: 0
hardware flashed: no
board firmware: FactoryTest V0.5
```

## Step 15: Add bounded physical commands and deterministic fixtures

After reporting that physical testing was one implementation gate away, I completed P0.16. The firmware still performs no EPD operation at boot, but now exposes the exact constrained cleanup, full-field, area, checker, reader-page, wait, status, and heap commands required by the experiment design.

Every physical command is state-gated, runs through a bounded worker, emits a machine-readable begin/end transaction, proves final scan idle, and synchronously returns panel controls to idle. The final exact-IDF build is warning-free and the expanded static/binary audit passes fourteen checks. Hardware remains untouched pending P0.17.

### Prompt Context

**User prompt (verbatim):** "where are we at / when are we going to test things?"

**User follow-up (verbatim):** "ok, go ahead"

**Assistant interpretation:** Explain the remaining gate, then complete P0.16 so the project can proceed immediately to the first controlled hardware smoke test.

**Inferred user intent:** Move from prolonged audit/build preparation into a safe, observable physical experiment without skipping the final reproducibility and command-safety gate.

**Commit (code/research):** `e9f3769dc417adb1623ac0a1435b891c5f936d0f` — "Firmware: add bounded PaperS3 EPD experiments"

### What I did

- Added `BOOT_LOCKED`, `WHITE_KNOWN`, `TARGET_KNOWN`, and `FAULT` command-state enforcement.
- Required exact `epd cleanup CONFIRM` as the first physical operation.
- Added fixed HIGH/two-stage full white/black, 1/10/25/50/100% area, checker A/B, reader-page, wait, status, and heap commands.
- Added a core-1 operation worker with a 120-second console bound, 110-second idle bound, and 5-second synchronous power-down bound.
- Added transaction ids, target SHA-256, commanded origin, target, policy, elapsed time, stages, power-control state, and heap records.
- Added terminal FAULT behavior with `FAULT_NO_AUTOMATIC_CLEANUP` after a timeout.
- Added a ticket-owned fixture generator using pinned DejaVu Serif and ImageMagick.
- Embedded the resulting 129,600-byte 2-bpp page and preserved a PNG preview, source text, and manifest.
- Verified the preview objectively as exactly two colors despite a vision model inferring grayscale from its rendering.
- Fixed two clean-build failures, regenerated final hashes, and passed fourteen static/binary audit checks.
- Checked task P0.16 and committed the firmware/evidence milestone.

### Why

- A hardware test needs commands that encode valid experiment order, not a general low-level waveform console.
- Cleanup after a timeout is unsafe if the scan task may still own LCD_CAM/GDMA; FAULT therefore refuses subsequent physical operations.
- Offline page generation isolates display-drive behavior from runtime font, layout, storage, and allocation effects.

### What worked

- No-drive boot inspection passes: `app_main()` only initializes state/resources and starts the console.
- The reader fixture is readable, unclipped, non-overlapping, and objectively bilevel.
- Packed fixture SHA-256 is `14dcffa9d13e0daabda8dc56c038bcec2eb8b01c4d8ac97ae170de5509207e90`.
- Final application builds warning-free at 433,776 bytes with exact ESP-IDF 5.4.2.
- Expanded binary audit passes 14/14 checks.
- IRAM remains unchanged at 16,383/16,384 bytes because no new function was IRAM-attributed.

### What didn't work

- The first P0.16 build failed because ESP32-S3's `uint32_t` is `unsigned long` under this toolchain:

  ```text
  error: format '%u' expects argument of type 'unsigned int', but argument 2 has type 'uint32_t' {aka 'long unsigned int'} [-Werror=format=]
  ```

  All operation and timeout fields now use `PRIu32`.

- The second build linked the generated fixture object but used the wrong symbol names:

  ```text
  undefined reference to `_binary_fixtures_reader_page_bin_end'
  undefined reference to `_binary_fixtures_reader_page_bin_start'
  ```

  Inspection of generated `reader_page.bin.S` showed `_binary_reader_page_bin_start/end`; the declarations were corrected.

- The vision model interpreted the preview background and antialiased-looking edges as grayscale. `identify` and an exact histogram disproved that interpretation: `Type: Bilevel`, `Colors: 2`, 34,946 black pixels, and 483,454 white pixels.

### What I learned

- ESP-IDF's `EMBED_FILES` symbol names derive from the generated basename in this build, not the source subdirectory path.
- Target identity can be logged without runtime rendering by hashing the final packed PSRAM buffer immediately before dispatch.
- The page fixture's 6.741127% black coverage gives a realistic low-area workload distinct from full-field and centered-area tests.
- A host timeout and a driver idle timeout serve different purposes and must both be visible in the protocol.

### What was tricky to build

- The driver updates `paintStage` before completing physical scans. The worker therefore combines `waitIdle()`'s active-semaphore proof with explicit synchronous `powerDown()` before reporting success.
- Timeout handling cannot safely delete the worker or initiate cleanup. The request storage remains static, the state becomes terminal FAULT, and the potentially active worker is allowed to finish without a competing command.
- Checker inversion requires command history: checker B is accepted only after checker A; area fixtures require `WHITE_KNOWN`; repeated page and repeated black are intentionally narrow exceptions.

### What warrants a second pair of eyes

- Review the worker timeout/static-request lifetime and task-notification ordering.
- Review whether 120/110/5-second nested bounds are adequate for HARD clear and HIGH operations.
- Review that all success paths call `powerDown()` and all timeout paths avoid cleanup.
- Review the centered rectangle dimensions and actual area fractions before interpreting area scaling.
- IRAM still has only one byte free; future changes must not add IRAM content.

### What should be done in the future

- Add P0.17's exclusive flash and serial runner scripts with check/dry-run modes.
- Verify the stable port has no owner and preserve the pre-flash factory state.
- Flash once, observe no-drive boot/status, then execute only the bounded smoke chain.
- Stop for operator visual disposition before any P0.18 area/checker/page matrix.

### Code review instructions

- Review `0107-papers3-epd-painter-control/main/app_main.cpp`, especially `RunOperation`, `OperationTask`, and `CommandEpd`.
- Regenerate the page with `scripts/14-generate-epd-control-fixtures.py` and compare hashes.
- Run `scripts/12-build-epd-painter-control.sh` and `scripts/13-audit-built-epd-control.py`; expect zero warnings and 14/14 passes.
- Confirm the board was not flashed by reviewing the final build report/log.

### Technical details

```text
P0.16 commit: e9f3769dc417adb1623ac0a1435b891c5f936d0f
application SHA-256: 2791e8334e2dae02612cf57ef58437758420a8168487fde3994d4fc73f3c5135
ELF SHA-256: 451b4ffa026217a7fe10ff545174e0d6c62dd92b1ba2e9817577a7411f983358
application bytes: 433776
reader fixture SHA-256: 14dcffa9d13e0daabda8dc56c038bcec2eb8b01c4d8ac97ae170de5509207e90
reader black coverage: 6.741127%
binary audit: 14 / 14 pass
boot operations: 0
hardware flashed: no
board firmware: FactoryTest V0.5
next hardware task: P0.17
```

## Step 16: Flash the exact independent control and stop on HARD-white ghosting

This step crossed the hardware gate with an exact audited binary, proved no-drive boot, and executed one operator-observed HARD-white cleanup. The transaction completed cleanly in software, but the panel retained substantial content from the previous FactoryTest screen.

That optical failure is the important result. I stopped before the black/white smoke sequence, powered the panel controls down through the bounded operation path, stopped the monitor, and released serial ownership rather than adding more waveform history.

### Prompt Context

**User prompt (verbatim):** "i am watching"

**Subsequent operator observation (verbatim):**

> 1. lots of ghosting from the previous screen
> 2. i don't think so
> 3. how aggressive are you on the power stuff? but no

**Assistant interpretation:** The operator was ready for the first physical cleanup, then reported that it failed to erase prior-screen content while showing no noticed abnormal heat, smell, sound, or power behavior.

**Inferred user intent:** Safely determine whether the unchanged EPD_Painter control can establish a clean white baseline and understand how electrically aggressive the test is.

**Commit (tooling):** `4b1cd7e46de05c206998b7c86b08bbd6b50140cb` — "Tools: make PaperS3 EPD flashing deterministic"

### What I did

- Fixed verbose build behavior so complete ESP-IDF output is redirected to the timestamped evidence log; normal stdout now contains only progress and the final size/hash summary, while failures show the final 100 lines.
- Fixed preview determinism by stripping PNG date/time metadata and verified two consecutive generations were byte-identical.
- Pinned `PROJECT_VER=esp50-p0.16-e9f3769` and changed flashing from `idf.py flash` to direct esptool `@flash_args` execution.
- Rebuilt and passed all 14 static/binary audit checks.
- Flashed application SHA-256 `f24705a69ac0355006d82ea1873191c6084f96bc7a79fcd1008ef433208437f9` and verified that the artifact hash was unchanged after flashing.
- Verified version `esp50-p0.16-e9f3769`, ELF prefix `1f0134ada`, `BOOT_LOCKED`, zero pending stages, idle rails, initialized PSRAM, prompt continuity, and stable heap.
- Sent exactly one physical command: `epd cleanup CONFIRM`.
- Captured `EPD_OP_END id=1 result=ok elapsed_ms=397 pending=0 rails=idle` and the subsequent `WHITE_KNOWN` status.
- Asked for immediate visual disposition, recorded substantial ghosting, stopped the matrix, exited the serial monitor, and confirmed no serial owner remained.

### Why

- Exact-artifact flashing makes the preflight audit meaningful; a post-preflight relink defeats binary identity even when source behavior is unchanged.
- HARD white was the required history-normalization gate. If it cannot clear retained content, later black/page comparisons lack a trustworthy origin state.
- Stopping after an optical failure avoids compounding panel history or confusing an explanatory experiment with blind waveform cycling.

### What worked

- Direct esptool flashing preserved the audited application hash before and after hardware modification.
- The fixed project version appeared correctly in the boot transcript.
- No-drive boot, console, status, heap, bounded completion, neutral end state, power-down, and serial ownership controls all behaved as designed.
- The operator noticed no abnormal heat, smell, sound, or power behavior.
- Build output is now concise without losing the complete evidence log.

### What didn't work

- The first version of the flash script called `idf.py flash`. IDF reran CMake after repository evidence changed, embedded version `9c59ed6-dirty`, and flashed SHA `dabe3338...` after preflight had audited `2791e833...`. No waveform was run on that image.
- The corrected EPD_Painter HARD-white operation did not produce a clean optical endpoint. Operator report: `lots of ghosting from the previous screen`.
- Automatic `result=ok` therefore did not imply optical success.

### What I learned

- `idf.py flash` is not an exact-artifact operation when CMake considers the project dirty; direct esptool execution is required for this evidence model.
- EPD_Painter's unchanged HARD clear is not, by itself, a successful independent white-baseline control on this panel/history.
- The 397 ms operation consisted of high scan activity but no local rail-amplitude change: HARD uses 20 full-panel passes in `6/2/4/8` alternating phases, ten passes of each code polarity overall, 5 ms gaps, and a final neutral scan.
- Rail/VCOM values and current remain unmeasured. Software can prove enable duration and control state, not the analog voltages that reached the panel.

### What was tricky to build

- Repository state affected application bytes through the default project-version descriptor. The symptom was a mismatch between the preflight hash and the flashed image despite no intentional source edit. Pinning `PROJECT_VER` removed Git-state variability, and direct esptool invocation eliminated reconfiguration during flash.
- Raw tmux capture contained carriage returns, NULs, ANSI escapes, and trailing whitespace. The stop path now normalizes those artifacts after closing `pipe-pane`, preserving readable evidence while preventing diff hygiene failures.
- The panel transaction passed every software invariant while failing its actual optical purpose. The experiment therefore had to preserve two separate dispositions rather than collapsing them into a single pass/fail flag.

### What warrants a second pair of eyes

- Confirm the exact physical meaning/polarity of EPD_Painter patterns `0x55` and `0xAA` against ED047TC1 source-driver semantics.
- Review whether the `6/2/4/8` ordering is appropriate for ending white despite equal aggregate code counts.
- Review whether 397 ms is credible for 20 full-panel HARD scans plus state-machine activity on the configured scan timing.
- Review whether the clear path's logical screen buffers and unknown physical starting state undermine the preliminary white-target stage.
- Do not infer safe rail amplitudes from `rails=idle`; analog probing remains outstanding.

### What should be done in the future

- Do not run the black/white smoke chain or P0.18 area/checker/page matrix yet.
- Analyze the HARD-clear implementation, physical code polarity, and prior-state assumptions against the ED047TC1 datasheet and known-good driver behavior.
- Decide whether the next discriminating experiment is a reviewed polarity-ending cleanup, an official/factory white comparison, or safe rail/VCOM measurement.
- Obtain a fixed-camera image and temperature measurement if the white endpoint is reproduced.

### Code review instructions

- Start with `scripts/output/17-p0.17-hard-white-observation.md` and the normalized `16-epd-control-monitor-20260714T210836Z.log`.
- Review `EPD_Painter::clear` in `0107-papers3-epd-painter-control/components/epd_painter/src/EPD_Painter.cpp`, especially `totpass={6,2,4,8}`, pattern alternation, and final neutral scan.
- Review `scripts/15-flash-epd-control.sh` for pre/post SHA checks and direct `@flash_args` use.
- Confirm no later `EPD_OP_BEGIN` exists after operation ID 1.

### Technical details

```text
application SHA-256: f24705a69ac0355006d82ea1873191c6084f96bc7a79fcd1008ef433208437f9
ELF SHA-256: 1f0134ada20285026c0c9df12b89a7c5cf9bba26d9bb9b030e97bb9172d1ffc2
project version: esp50-p0.16-e9f3769
operation: EPD_OP_END id=1 result=ok elapsed_ms=397 pending=0 rails=idle
post-state: WHITE_KNOWN
optical disposition: FAIL — lots of ghosting from the previous screen
abnormal heat/smell/sound/power behavior: none noticed
serial owner after stop: none
```

## Step 17: Make runtime waveform traces and immutable experiments first-class evidence

The failed independent white cleanup showed that endpoint descriptions alone are not enough. This step changed the next branch from another visual trial into a layered measurement program: static waveform decoding, minimally perturbing runtime traces, external physical capture, and immutable per-run records.

I implemented the first non-invasive layer immediately. The generated decoder proves that the legacy/factory-family M5GFX 0.2.15 and current 0.2.25 Panel_EPD LUTs have canonical SHA-256 `d24b2df...`, while preserving every raw row and all sixteen target schedules for later runtime joins.

### Prompt Context

**User prompt (verbatim):** "we could potentially instrument and log the low-level details of M5GFX / the stock firmware to examine what it actually does and which waveforms and timings it uses. Then we can use the gather information and further things we collect to also just keep track of our experiments in a more scientific fashion."

**Subsequent user prompt (verbatim):** "I think logging at runtime would also be valuable, no?"

**Assistant interpretation:** Add runtime tracing—not merely static source inspection—and structure every hardware run as a preregistered, provenance-complete experiment.

**Inferred user intent:** Replace ambiguous visual trial-and-error with enough software, timing, electrical, optical, and provenance evidence to compare drivers and explain outcomes scientifically.

### What I did

- Authored `analysis/04-m5gfx-runtime-waveform-instrumentation-and-scientific-experiment-ledger.md`.
- Defined four evidence layers: static decoding, minimally perturbing runtime timing events, optional perturbing drive-code histograms, and external digital/analog capture.
- Defined runtime events for application operation, queueing, eraser/target arming, power transitions, frame boundaries, idle, overflow, and completion.
- Forbade printing, allocation, filesystem access, and JSON formatting while panel rails are active; trace dumping occurs only after idle/power-off.
- Defined observer-effect controls comparing trace-disabled and trace-enabled builds.
- Defined immutable per-experiment directories with preregistration, manifest, JSONL events, physical/optical evidence, hashes, verbatim observation, and separate automatic/optical dispositions.
- Added seven P0.17 sub-tasks for decoding, trace design/implementation/audit, experiment ledger, factory video, and external capture.
- Implemented and ran `scripts/17-decode-m5gfx-epd-waveforms.py` without hardware access.

### Why

- Static LUTs do not identify the actual origin, target, eraser insertion, queue merging, frame count, frame duration, or rail-on interval of a runtime operation.
- Serial printing in a scan loop would perturb the timing under investigation and could create a false explanation.
- The exact factory binary cannot be internally instrumented without ceasing to be exact; it requires external capture, while a source-equivalent rebuild can expose semantic runtime events.
- Immutable experiment manifests prevent transition history, binary identity, environment, and optical judgments from being lost or conflated.

### What worked

- The decoder found all five expected LUTs in both source snapshots and validated all rows/codes/terminators.
- Legacy and current canonical LUT SHA-256 values match exactly: `d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3`.
- Static evidence records the 16 MHz PaperS3 bus, eight encoded padding bytes, and exact power GPIO ordering.
- Generated JSON preserves machine-readable schedules; Markdown gives a concise review surface.
- No serial port was opened and no hardware was modified.

### What didn't work

- Runtime trace hooks are designed but not yet implemented or built.
- The exact source-equivalent factory build remains blocked by missing ESP-IDF 5.3.3.
- Actual physical timing, drive polarity, rails, VCOM, current, temperature, and factory-white endpoint remain unmeasured.

### What I learned

- M5GFX QUALITY black statically schedules `BWWBBBBBB--BB...`; QUALITY white schedules `BWWBBB-WWWWWW...`, while its eraser schedule depends on the current tone.
- M5GFX's power command ordering is OE→PWR→SPV on and PWR→OE→SPV off, with fixed microsecond/millisecond delays.
- Runtime semantics and exact factory identity require different controls: source hooks answer scheduler questions; external capture answers exact-binary timing questions.

### What was tricky to build

- The generated schedules contain deliberate no-op padding before termination. The decoder preserves that padding because it can affect runtime frame count even though it commands no particle motion.
- Software names code 1 “toward black” and code 2 “toward white,” but those comments are not proof of physical source voltage polarity. Reports label them as software semantics until externally verified.
- Runtime code-count collection is attractive but belongs in a separate perturbing mode because counting DMA nibbles in or near the row loop may stretch scan timing.

### What warrants a second pair of eyes

- Review whether every proposed trace event can be emitted lock-free and allocation-free.
- Review the M5GFX step-frame state machine to locate eraser/target pixel counts without changing behavior.
- Review whether `esp_timer_get_time()` at frame boundaries measurably alters inter-frame timing.
- Validate static decoded schedules against M5GFX's assembly `blit_dmabuf` state transitions, not only source comments.
- Review safe physical probe points before any external capture.

### What should be done in the future

- Implement compile-time-gated weak trace hooks and a fixed-size ring in a source-instrumented M5GFX control.
- Build trace-off and trace-timing variants before either is flashed.
- Add immutable experiment-directory generation and schema validation.
- Replay/video the exact FactoryTest sequence separately from source-instrumented tests.

### Code review instructions

- Start with `analysis/04-m5gfx-runtime-waveform-instrumentation-and-scientific-experiment-ledger.md`.
- Run `scripts/17-decode-m5gfx-epd-waveforms.py`; expect identical canonical hashes and `hardware_modified=no`.
- Inspect the JSON companion for all sixteen schedules per LUT.
- Compare claims against `Panel_EPD.cpp`, `Bus_EPD.cpp`, and the PaperS3 configuration in `M5GFX.cpp`.

### Technical details

```text
static schema: esp50.m5gfx-waveform-static.v1
legacy M5GFX: c6f92dc03226cdc04d67c705a2020f62ad21ad01
current M5GFX: ad9b814264d4e2000e9f30070002310bbccaffc9
canonical LUT SHA-256: d24b2df188e4261d5891a0884e2510567ea45c38bcaebeb66ade1d4f4b979af3
bus speed: 16000000 Hz
line padding: 8 bytes
trace hot-path printing: forbidden
hardware modified: no
```

## Step 18: Implement and audit fixed-ring M5GFX runtime traces without flashing

This step implemented the complete no-hardware runtime instrumentation gate. The new numbered project builds the same M5GFX source twice: once with every trace argument and counter compiled out, and once with fixed-ring timing events at operation, queue, update, power, frame, and idle boundaries.

The observer audit passes 18/18 checks. Most importantly, extracted trace-off frame-scheduler and power-control machine-code sections are byte-identical to the earlier clean Cell D build, while trace-on perturbations are isolated outside the 540-row loop and measured at the binary/memory level.

### Prompt Context

**User prompt (verbatim):** "go ahead. do all the Next Steps: Next steps: Implement fixed-ring trace hooks, build trace-off/on variants, and audit observer effects without flashing."

**Assistant interpretation:** Complete trace implementation, reproducible dual builds, and a source/binary observer-effect audit while preserving the board's current firmware and panel state.

**Inferred user intent:** Obtain trustworthy runtime instrumentation whose disabled control is demonstrably unchanged and whose enabled perturbations are explicit before collecting more physical evidence.

**Commit (code):** `2badb87b0ae91d2f5dd022551d822328c5de2fba` — "Firmware: instrument M5GFX runtime traces"

### What I did

- Created `0108-papers3-m5gfx-runtime-trace` with pinned project version, no-drive boot, USB Serial/JTAG console, and 100 Hz FreeRTOS baseline.
- Added a Kconfig-selectable 512 × 48-byte fixed trace ring using monotonic atomics and release-published commit markers.
- Added operation bracketing and explicit `epd trace status|dump|reset`; dumping waits for display idle and occurs outside the worker.
- Added M5GFX hooks for enqueue, dequeue, update preparation, power, frame, and idle boundaries.
- Counted target/eraser two-pixel units only during existing update-preparation loops; did not count drive codes or call hooks in the 540-row loop.
- Added exact-source preparation script `18`, dual warning-free build script `19`, patch evidence, and observer audit `20`.
- Redirected full build output to ticket logs, printing concise success summaries and only filtered/truncated tails on failure.
- Built trace-off and trace-timing variants from clean state with ESP-IDF 5.4.2.
- Compared trace-off critical `.text` sections to clean Cell D and proved byte identity.
- Verified the timing ring is 24,576 BSS bytes, app delta is 1,584 bytes, and there are ten bounded linked hook call sites.
- Did not flash, monitor, or open the serial device.

### Why

- Runtime records are useful only if the control proves instrumentation can be removed without changing the code under study.
- Fixed records avoid serial, allocation, JSON, and storage latency while rails are active.
- Frame-boundary timestamps answer scheduler and rail-duration questions without adding work to every physical row.
- Trace-on size and call-site evidence makes the observer effect reviewable before physical use.

### What worked

- Final trace-off and trace-on builds completed with zero warnings.
- Patched M5GFX retained canonical LUT SHA-256 `d24b2df...`.
- Trace-off ELF contains no trace hook or ring.
- Trace-off `Panel_EPD::task_update` and `Bus_EPD::powerControl` text sections exactly match clean Cell D.
- The timing build links one strong hook, a 512-record ring, and no per-row trace calls.
- Audit result: 18/18 PASS; hardware modified: no.

### What didn't work

- The first off build failed with:

```text
error: 'CONFIG_PAPERS3_M5GFX_RUNTIME_TRACE' was not declared in this scope
```

  A disabled Kconfig boolean is absent rather than defined as zero. I replaced direct expression use with a preprocessor-selected `kTraceVariant` constant.
- The first failure path ran `idf.py size` after compilation had already failed, duplicating a long compiler command. I changed the three build actions to an `&&` chain and truncate failure-tail lines to 500 characters.
- A later off build produced:

```text
warning: unused variable 'continuation' [-Wunused-variable]
```

  I removed the temporary and passed the pre-reset `remain` expression directly to the compile-time hook macro.
- An initial empty-inline-function design removed the hook body but still evaluated side-effectful arguments such as `uxQueueMessagesWaiting()`. Trace-off `task_update` differed from clean Cell D. Replacing it with a variadic compile-time macro, plus compile-out counter macros, restored exact text-section identity.

### What I learned

- “No-op function” is not equivalent to “no instrumentation”: C++ argument evaluation can survive and perturb behavior even if an inline callee is empty.
- Relocatable section comparison is a stronger control than final ELF addresses or symbol sizes. Final link placement differs between applications, but extracted function text can still be identical.
- A 512-record ring costs exactly 24 KiB BSS at 48 bytes per record; this is acceptable for a bounded experiment but should not silently enter production firmware.
- M5GFX's trace-on worker growth is concentrated in frame scheduling (+209 bytes) and power control (+96 bytes), with no row-loop hook.

### What was tricky to build

- The trace hook is called from M5GFX while its implementation lives in the application. A weak C declaration plus one strong application definition keeps the patch narrow and lets the linker prove whether instrumentation is present.
- The ring has writers on application and display-task contexts. Atomic sequence reservation gives each writer a unique slot; a release commit marker lets the later idle-time dump reject a partially published or overwritten record.
- Observer auditing must distinguish source equivalence, relocatable machine-code equivalence, final-link identity, and physical timing equivalence. Only the first three are possible without flashing.

### What warrants a second pair of eyes

- Review ring wrap behavior if more than 512 events occur before dump; overwritten count is explicit, but such a run should be rejected.
- Review operation association when queue coalescing spans application boundaries.
- Verify target/eraser counts are correctly described as two-pixel units, not pixels.
- Review whether one `esp_timer_get_time()` plus atomic publication per frame is sufficiently small relative to the 100 Hz inter-frame delay; this still requires physical A/B evidence.
- Confirm `M5.begin(clear_display=false)` does not energize the panel on actual boot before any future flash authorization.

### What should be done in the future

- Implement the immutable experiment-directory generator before collecting trace data.
- Review and authorize a no-drive boot of one variant only after exact binary selection.
- Use external capture or trace-off/trace-on matched optical runs to measure runtime perturbation.
- Preserve the exact FactoryTest video baseline separately from source-instrumented builds.

### Code review instructions

- Review `0108-papers3-m5gfx-runtime-trace/main/epd_trace_runtime.cpp` for publication, wrap, reset, and dump ordering.
- Review patch `scripts/patches/18-m5gfx-runtime-trace-hooks.patch`, especially that no hook appears in the row loop.
- Run scripts `18`, `19`, and `20`; normal build output should remain concise and audit should pass 18/18.
- Inspect `scripts/output/20-m5gfx-runtime-trace-audit-latest.md` for exact section hashes and size deltas.
- Confirm build scripts contain no flash/monitor operation.

### Technical details

```text
project: 0108-papers3-m5gfx-runtime-trace
ESP-IDF: 5.4.2
M5GFX: ad9b814264d4e2000e9f30070002310bbccaffc9
M5Unified: b1ffcc677014ed8bd01e5a1f240736ae654bfe12
trace patch: 6c21e45e0909accd2b5df5ae3178534192b10a51ec4a319ebc2309bfe983d89f
trace-off app: 609aba851db118ee26a3051d4f78ae96255229493f9783f60f43334355925e68
trace-on app: a081daabe5a77d7405cde68e43955279ed5e5c0f954c2aee027b62d03fd9f6ea
app delta: +1584 bytes
ring BSS: 24576 bytes
observer audit: 18/18 PASS
hardware modified: no
```

## Step 19: Build and preregister the exact-IDF FactoryTest F0/F1/F2 controls

This step closed the missing ESP-IDF 5.3.3 prerequisite, built the formerly blocked matrix cells, and created a separate FactoryTest V0.5 lineage project rather than projecting current-driver traces onto stock firmware. The resulting clean, F1/off, and F2/trace builds are statically qualified without touching the board.

It also converted the planned physical comparison into three immutable experiments. Each treatment now has a frozen hypothesis, decision rule, binary identity, camera protocol, stop conditions, and separate automatic/optical disposition before the first F0 flash.

### Prompt Context

**User prompt (verbatim):** "ok, update design doc, and then get to work until you need me"

**Subsequent continuation prompt:** "continue"

**Assistant interpretation:** Revise P0.17f into the exact-vendor/source-off/source-trace experiment, install the exact missing toolchain, implement and audit 0109, preregister all physical treatments, and stop only at the operator video gate.

**Inferred user intent:** Obtain a scientifically defensible stock-firmware comparison with runtime telemetry while avoiding further unstructured panel drives.

**Commit (code):** `4ab273a69231d50ccc51fcc5e839715e89fdfa57` — "Firmware: trace FactoryTest V0.5 display sequence"

### What I did

- Updated the runtime-instrumentation design with F0/F1/F2 semantics and execution ordering.
- Installed exact ESP-IDF tag `v5.3.3`, commit `6db3dc25...`, under `/home/manuel/esp/esp-idf-5.3.3`.
- Changed the matrix build script to redirect complete output to evidence logs and print concise hashes/failure tails.
- Built matrix Cells A and B without flashing; both completed without warnings.
- Created numbered project `0109-papers3-factory-v0.5-runtime-trace` from exact FactoryTest V0.5 source.
- Added a legacy M5GFX 0.2.15 trace patch and clean/F1/F2 component preparation.
- Preserved `boot_display_test()` byte-for-byte while ordering F2's dump after its final dwell and `waitDisplay()`.
- Built clean, F1/off, and F2/trace variants under exact IDF 5.3.3.
- Added and passed a 19-check source, toolchain, LUT, machine-code, ring, warning-set, and no-flash audit.
- Created immutable F0/F1/F2 experiment directories with preregistration hashes and docmgr-valid frontmatter.
- Added guarded treatment-specific flash checks and a no-input F2 serial/ring collector.
- Verified all F0/F1/F2 preflight checks with no serial owner and no hardware modification.

### Why

- M5GFX 0.2.15 and 0.2.25 share LUT bytes but differ in scheduler/assembly/platform code; 0108 cannot stand in for the stock runtime.
- The official merged binary is the optical authority, while a source-derived trace build is the semantic/runtime instrument. F0/F1/F2 keep those claims separate.
- Exact IDF 5.3.3 permits both the adjacent B/C IDF comparison and faithful FactoryTest source-lineage builds.
- Preregistration prevents camera settings, endpoint criteria, or treatment labels from changing after an unexpected result.

### What worked

- Exact IDF installation and version/tag/commit verification succeeded.
- Cell A app SHA is `cfb03a6f...`; Cell B app SHA is `3efe2423...`; neither was flashed.
- Factory `boot_display_test()` SHA is `9b5bca6e...` and matches V0.5 source exactly.
- F1 `app_main`, `Panel_EPD::task_update`, and `Bus_EPD::powerControl` text sections are byte-identical to clean.
- F1 contains no trace symbol or ring.
- F2 has ten bounded call sites, no row-loop hook, a 49,152-byte ring, and only 1,440 application bytes of growth.
- The audit passed 19/19 and every experiment preregistration checksum verifies.

### What didn't work

- The first IDF installation inherited the active 5.4 Python environment:

```text
ERROR: Python environment is set to /home/manuel/.espressif/python_env/idf5.4_py3.13_env which was generated for ESP-IDF 5.4 instead of the current 5.3.
```

  The installer now clears `IDF_PATH`, `IDF_VERSION`, and `IDF_PYTHON_ENV_PATH` before running `install.sh`.
- The first 0109 build omitted explicit component requirements:

```text
fatal error: mooncake_log.h: No such file or directory
```

  Adding M5Unified, mooncake, mooncake_log, console, and USB console requirements exposed the next missing HAL dependencies.
- The next build failed with:

```text
fatal error: esp_vfs_fat.h: No such file or directory
```

  The main component now explicitly requires driver, esp_adc, fatfs, esp_netif, esp_wifi, nvs_flash, and sdmmc.
- The first warning policy rejected upstream FactoryTest missing-initializer and legacy-ADC warnings. The final gate requires the normalized warning set to be identical across clean/F1/F2, proving tracing adds none, rather than pretending the stock source is warning-free.
- Initial experiment Markdown lacked docmgr frontmatter, then used unnumbered filenames. The generator was corrected and all ledgers were regenerated before execution; doctor now passes cleanly.

### What I learned

- A faithful source-derived control requires preserving not just LUTs but IDF, M5 versions, application sequence, tick rate, and scheduler machine code.
- Source lineage can be audited strongly even when final binary hashes differ due descriptors/build timestamps: relocatable critical text sections remain comparable.
- Upstream warnings are useful provenance when identical across variants; suppressing them would introduce another build difference.
- The exact two-second black→white transition gives each judged white endpoint a known immediately preceding black command even though cross-run starting dashboard history differs.

### What was tricky to build

- The V0.5 project relied on legacy implicit component visibility. Exact IDF 5.3.3's dependency checker required reconstructing explicit requirements without changing runtime source behavior.
- F2 must emit substantial JSONL output but cannot print while rails are active. The dump is compiled only in F2 and occurs after `waitDisplay()` following the complete boot test.
- The experiment files live under the ticket tree, so docmgr validates them. Numbered, frontmatter-complete templates were necessary even though they are operational evidence rather than primary design docs.
- The F2 collector must race USB re-enumeration after flash without sending bytes. It retries exclusive opens, captures passively, extracts JSONL, and rejects missing markers, invalid records, noncontiguous sequence numbers, or missing power/frame/idle events.

### What warrants a second pair of eyes

- Review whether adding USB Serial/JTAG console identically to clean/F1/F2 is an acceptable source-proxy deviation from F0.
- Review the decision to dump F2 before dashboard app installation; the judged built-in sequence is preserved, but dashboard timing differs afterward.
- Review the F2 ring capacity against expected frame count and reject any overwrite.
- Review camera locking and reference-patch placement before F0.
- F1/F2 must not be authorized automatically after F0; each preceding optical disposition is a gate.

### What should be done in the future

- Operator prepares a fixed 60 fps or faster camera with locked exposure, focus, white balance, stable illumination, and matte references.
- Execute F0 only, record the exact factory white endpoint, and complete its ledger.
- Review F0 before deciding whether F1 is eligible; similarly review F1 before F2.
- Run matrix Cells A/B physically only as a separate later experiment, not during the FactoryTest F0/F1/F2 chain.

### Code review instructions

- Start with `analysis/04-m5gfx-runtime-waveform-instrumentation-and-scientific-experiment-ledger.md` and `scripts/output/25-factory-v0.5-trace-audit-latest.md`.
- Review `0109-papers3-factory-v0.5-runtime-trace/main/main.cpp` around `boot_display_test()` and F2 dump ordering.
- Review patch `scripts/patches/23-m5gfx-0.2.15-factory-runtime-trace.patch` for compile-out behavior and absence of row-loop hooks.
- Run scripts 23, 24, and 25; expect 19/19 PASS and no hardware modification.
- Verify preregistration hashes and run script 27 in check mode for each treatment.

### Technical details

```text
IDF v5.3.3: 6db3dc25df7325c1c81b7cd7d4e42babff7a818e
Factory V0.5 source: 5e275ad4b70abb85f7193fda137844730e64c4db
M5GFX 0.2.15: c6f92dc03226cdc04d67c705a2020f62ad21ad01
clean app: ad858733ab2ddd5c664f33ab593a3ea7775b26dbe35c3e575a3fe47c235d753f
F1 app: 3d9bf37a5c5faa120fa1dccf357e8d0676a77495359754d062a5fa654dd2d2b3
F2 app: 95334c261762205ab95d3f578a5d3d0a0eac4fe7fffdfd1ada0e836ba8a2d755
F0 exact release: d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624
audit: 19/19 PASS
hardware modified: no
```

## Step 20: Capture F0 and establish a shared optical/firmware timebase

This step executed the exact-vendor F0 treatment once, ingested its original locked-camera recording, and turned the newly connected Printalyzer into a timestampable evidence source. A single NDJSON process can now capture both serial channels against one monotonic clock while preserving raw bytes and receive-time bounds.

The observer-control work also found a consequential failure: pyserial modem-state application reset the PaperS3 into ROM download mode. The failed trace is retained, the firmware channel was redesigned as a truly read-only OS descriptor, and the board was intentionally not rebooted because F0 would drive the panel again at boot.

### Prompt Context

**User prompt (verbatim):** "ok, write a little program to capture timestamps streaming events, that way you can correlate to the firmware logs, and thus you can really do synchronized readings and get full feedback, right?"

**Assistant interpretation:** Implement and validate a common-host-clock serial collector for Printalyzer optical events and PaperS3 firmware events, with explicit safety gates and enough metadata to correlate deferred firmware traces.

**Inferred user intent:** Replace subjective-only EPD judgments with synchronized, reproducible optical feedback that can be joined to firmware scheduling evidence.

**Commit (code):** `ec2bf1bc5efd366a684af1f345e3f0f8f62accf0` — "Experiment: capture F0 video and synchronized serial"

### What I did

- Flashed the exact F0 merged release once after operator camera readiness.
- Preserved the flash transcript, append-only experiment events, and separate automatic/optical disposition.
- Received the original 311,999,671-byte Blackmagic Camera MOV and stopped the temporary upload service.
- Hashed the MOV as `2968870a3609a8bda80440aaacaf1e9f8b5acf2f551fb6c0ff2343d448420c06`.
- Captured ffprobe metadata: iPhone 15 Pro 24 mm, Blackmagic Cam 3.3.100001, 60 fps, 1/120 s, ISO 149, 3210 K, tint +4, HEVC/BT.709.
- Generated a tracked contact sheet while keeping the 298 MiB original local and hash-addressed outside normal Git storage.
- Identified the Printalyzer at its stable by-id path and downloaded its pinned protocol/source/manual in the preceding source commit.
- Implemented shared-clock dual-source capture with raw bytes, first/last receipt timestamps, monotonic-derived UTC, parsing, and ordered NDJSON.
- Added passive, fixed read-only inventory, and explicitly gated raw-stream modes.
- Added a pseudo-terminal integration test that proves raw parsing and STOP → light-off → remote-exit cleanup order.
- Ran a successful read-only inventory of the physical Printalyzer v1.1.0.
- Replaced pyserial firmware attachment with `O_RDONLY | O_NOCTTY | O_NONBLOCK` and validated that it issues no modem-control ioctl or write.

### Why

- One host monotonic clock permits optical and firmware records to be ordered without trusting independently adjusted wall clocks.
- Raw bytes and receipt windows preserve evidence needed to estimate USB and integration latency later.
- Printalyzer raw streaming changes light/sensor state and is not calibrated density, so it needs a stronger gate than passive capture.
- ESP32 USB Serial/JTAG modem control is behaviorally active even when no application bytes are written; a scientific observer must not reset the treatment.

### What worked

- F0 flash completed and esptool verified the written data.
- The original video captures the sequence at 60 fps with fixed shutter, ISO, and white balance metadata.
- Preliminary contact-sheet review identifies black, white, and grayscale stages and suggests ordered grayscale with possible right-edge residue; this is not yet the optical disposition.
- Printalyzer read-only commands all returned under DTR asserted, including firmware/build identity and calibration getters.
- Physical inventory changed no calibration, light, remote mode, or panel state.
- The fake raw stream produced nine parsed `GD S` samples and exact cleanup ordering.
- A post-fix firmware-only attachment opened and closed with no reset output.

### What didn't work

- The first Printalyzer inventory used DTR deasserted and timed out:

```text
TimeoutError: timed out waiting for response to 'GS V'
```

  Source inspection confirmed the Printalyzer considers CDC connected only while DTR is asserted. The densitometer-specific path now uses DTR true; the firmware path does not use pyserial.
- The first dual-port “passive” smoke test opened ESP32 USB Serial/JTAG with DTR/RTS deasserted through pyserial and caused:

```text
ESP-ROM:esp32s3-20210327
rst:0x15 (USB_UART_CHIP_RESET),boot:0x0 (DOWNLOAD(USB/UART0))
waiting for download
```

  This was a real observer failure. The board remains in ROM download mode; no automatic recovery boot was performed.
- The first pseudo-terminal harness exited on Linux PTY `EIO` before the slave opened:

```text
OSError: [Errno 5] Input/output error
```

  The permanent test treats pre-open PTY `EIO` as a retry condition and then passes.

### What I learned

- “No serial payload” does not mean “no hardware effect”; modem-control ioctls can reset and strap an ESP32-S3.
- Printalyzer CDC and ESP USB Serial/JTAG need different open strategies despite both appearing as `/dev/ttyACM*`.
- The Printalyzer installed firmware is v1.1.0 build `g7101373`, not necessarily the latest upstream source snapshot.
- Common host timestamps improve correlation but do not erase sensor integration, USB buffering, or F2's deliberate post-idle dump delay.
- Video container metadata can preserve camera controls more reliably than later operator recollection.

### What was tricky to build

- Concurrent readers must serialize output without losing per-source order. The sink assigns one process-global sequence under a lock while retaining source-local line sequences.
- UTC must remain ordered if NTP changes the wall clock. Each event derives UTC from fixed realtime/monotonic anchors.
- Firmware observation needed a descriptor that could not transmit or touch DTR/RTS. A read-only non-controlling `os.open` path replaced pyserial entirely.
- Raw-stream cleanup must run after partial entry failure. Cleanup is armed before `IS REMOTE,1`, then attempts sensor stop, reflection light off, and remote exit in order.
- The original video exceeds ordinary Git hosting limits. It remains local with SHA-256 and full ffprobe metadata while compact review derivatives are tracked.

### What warrants a second pair of eyes

- Review whether read-only tty open truly remains modem-neutral on all target kernels, not only this host.
- Review first/last-byte timestamp semantics for chunks containing multiple lines; they are receipt bounds, not sensor integration bounds.
- Review raw-stream gain/integration/light values only after a static target saturation experiment; defaults are not an authorization.
- Review the video contact sheet at full-resolution representative frames before assigning F0 optical pass/fail.
- Review recovery from ROM mode before pressing reset because F0 boot replays title → black → white → grayscale.

### What should be done in the future

- Extract exact F0 stage boundaries and full-resolution representative frames from the original MOV.
- Complete operator and analytical F0 optical disposition before authorizing F1.
- Preregister a separate Printalyzer geometry/latency/saturation experiment.
- Add explicit device-time anchors before relying on sub-integration-period correlation.
- After operator authorization, reset the board from ROM mode with the camera running or replace F0 with a non-driving firmware by an explicitly reviewed flash path.

### Code review instructions

- Start with `scripts/output/29-synchronized-serial-capture-validation.md`.
- Review `scripts/29-capture-synchronized-serial.py`, especially `_open_firmware_read_only`, `enter_raw_stream`, and `leave_raw_stream`.
- Run `scripts/30-test-synchronized-serial.py`; expect the seven-command sequence and PASS.
- Run `ruff check` and `python3 -m py_compile` on scripts 29 and 30.
- Verify the F0 preregistration hashes still pass and compare `events.jsonl` with the flash/video hashes.
- Do not run actual raw streaming as part of code review.

### Technical details

```text
F0 firmware SHA-256: d6733a0ca378f95335fa5fba4d4d992fb1dd97c17557b20e9aebfca08ba6d624
F0 video SHA-256: 2968870a3609a8bda80440aaacaf1e9f8b5acf2f551fb6c0ff2343d448420c06
Printalyzer port: /dev/serial/by-id/usb-Dektronics_Printalyzer_Densitometer_323147103439323344002900-if00
Printalyzer firmware: v1.1.0, g7101373, 2023-06-13 17:41
Printalyzer UID: 323147103439323344002900
PaperS3 state after observer failure: ROM download mode
Actual Printalyzer raw stream executed: no
```

## Step 21: Validate passive calibrated Printalyzer events

This step exercised only the Printalyzer's ordinary local reflection-measurement workflow while the host passively timestamped CDC output. It established that normal calibrated readings arrive intact and repeat at the instrument's 0.01 D display resolution without remote mode, diagnostic illumination, or panel interaction.

The operator caught a setup mistake during collection. Those first six records remain in the immutable stream but were prospectively excluded before analysis; the subsequent three low- and three high-density reference readings form the valid result.

### Prompt Context

**User prompt (verbatim):** "ok, let's try"

**Subsequent correction (verbatim):** "ok i made a mistake, the first 6 readings are nonsense, the 3 cal-l0 readings should say 0.05"

**Completion report (verbatim):** "done (1.49)"

**Assistant interpretation:** Run a no-input passive logger while the operator takes repeated normal reflection measurements on the supplied CAL-LO and CAL-HI references, preserving mistakes rather than rewriting the record.

**Inferred user intent:** Confirm that synchronized optical capture works on the physical instrument before attempting automated raw sampling or PaperS3 measurements.

**Commit (evidence):** `3dc771a935ecd81936444f73e82d71c31447e235` — "Experiment: validate passive Printalyzer readings"

### What I did

- Started script 29 in densitometer-only, passive, indefinite mode inside tmux.
- Asked for three CAL-LO and three CAL-HI normal reflection measurements with repositioning.
- Inspected the live append-only NDJSON after the operator reported the setup mistake.
- Preserved and classified the first six samples as operator-invalid.
- Continued the same capture for valid CAL-HI measurements.
- Stopped the logger via SIGINT after the operator reported completion.
- Verified clean `capture_end`, `result=ok`, `serial_input_sent=false`, and released `/dev/ttyACM1`.
- Computed capture SHA-256 `5e8235e8...` and wrote a reference-result report.

### Why

- Normal calibrated measurement is the lowest-risk proof of physical event capture.
- Prospective operator exclusion prevents accidental setup events from contaminating repeatability statistics without deleting inconvenient evidence.
- Reference targets test the instrument and logger independently of PaperS3 cover-glass geometry.

### What worked

- Valid CAL-LO was `R+0.05D` three times.
- Valid CAL-HI was `R+1.49D` three times and matched the operator-reported reference value.
- Both valid groups had 0.00 D spread at BASIC output's 0.01 D resolution.
- The logger captured twelve density lines, global ordering, nanosecond host receipt timestamps, and a clean termination.
- No serial command was sent and no PaperS3 port was opened.

### What didn't work

- The first three records were taken in transmission mode:

```text
T+2.19D
T+2.19D
T+2.19D
```

- The next three reflection records were also classified by the operator as procedural mistakes:

```text
R+1.98D
R+1.98D
R+1.98D
```

  All six remain in the event stream and are explicitly excluded from valid statistics.

### What I learned

- Passive CDC capture works with normal on-device calibrated measurements.
- The reference workflow is repeatable at displayed precision, but BASIC mode hides sub-0.01 D variation.
- Immediate operator annotations are essential because serial values alone cannot reveal target identity or placement mistakes.
- Host line timestamps do not reveal Action-button press time or integration start.

### What was tricky to build

- The logger ran indefinitely in tmux, so shutdown had to be graceful rather than killing the process and losing the final fsync/capture-end records.
- The first six readings could not be deleted or silently relabeled. Their immutable sequence and the operator's prospective correction had to remain joined.
- CAL-LO/CAL-HI target identity comes from the operator; the Printalyzer line only identifies reflection versus transmission and density.

### What warrants a second pair of eyes

- Confirm that `(1.49)` denotes both the printed CAL-HI target and observed readings; the captured readings themselves are unambiguous.
- Review whether `SM FORMAT,EXT` is acceptable for the next normal-measurement run; it is transient but no longer strictly input-free.
- Do not infer true zero variance below 0.01 D from rounded BASIC output.

### What should be done in the future

- Run a separately labeled EXT-format normal reference check if sub-centidensity values/raw counts are needed.
- Measure button-to-result latency with an independent button marker or video/audio synchronization.
- Qualify a fixture and target plane before placing the Printalyzer on PaperS3 glass.
- Keep actual raw streaming blocked until gain, integration, light, and saturation are preregistered.

### Code review instructions

- Read `scripts/output/29-printalyzer-passive-reference-result.md` first.
- Verify the raw JSONL hash and sequences; confirm records 2–7 are excluded and 8–13 are valid groups.
- Confirm the final event says `result=ok`, `interrupted=true`, and `serial_input_sent=false`.
- Confirm neither `/dev/ttyACM0` nor `/dev/ttyACM1` has an owner after capture.

### Technical details

```text
valid CAL-LO: 0.05, 0.05, 0.05 D
valid CAL-HI: 1.49, 1.49, 1.49 D
BASIC precision: 0.01 D
valid display-level spread: 0.00 D for each target
capture SHA-256: 5e8235e8f9ed179806d914c75b4857b65d895ad7f15a17158190f6423560b8f7
serial input sent: no
PaperS3 panel operation: no
```
