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
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/00-research-log.md
      Note: Retroactive reproducibility trace requested by the user
    - Path: repo://ttmp/2026/07/14/ESP-50-PAPERS3-EREADER-PRIMITIVES--papers3-e-reader-native-primitives-and-future-javascript-api/scripts/05-add-phase-tasks.sh
      Note: Idempotent source for the detailed phase task breakdown
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
