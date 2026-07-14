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
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/output/10-epd-painter-pre-hardware-audit.md
      Note: Expanded eight-blocker audit (commit e7e4848d9544b902dcf79246fa520f039c2d74ee)
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
