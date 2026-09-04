---
Title: Native redesign review diary
Ticket: PBUI-HANDHELD-1
Status: active
Topics: [pbui, architecture, design, research]
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Fresh evidence-first review, cross-repository ticket migration, native redesign, and reMarkable delivery.
LastUpdated: 2026-09-04T15:00:00-04:00
WhatFor: Review and resume the native redesign with exact evidence and commit boundaries.
WhenToUse: Read before continuing this review or implementing its replacement design.
---

# Diary

## Goal

Move the ticket into the firmware repository, independently examine the semantic and device sources, replace unsupported design decisions with explicit contracts and experiments, and deliver an intern-facing foundational guide to reMarkable. This diary supplements rather than rewrites the earlier investigator's diary.

## Step 1: Relocate the ticket without losing its provenance

The ticket lived in the pbui repository, even though its implementation destination is the ESP32 firmware repository. I moved the complete directory to the same date-based location under `esp32-s3-m5/ttmp`, verifying all eleven existing files by SHA-256 before repairing structured references. The original guides and diary remain historical evidence; the new review gets a separate document and diary.

The destination already contained unrelated untracked assets and dirty submodules. These were recorded during orientation and are excluded from all commits. This migration requires two Git commits because the source and destination are separate repositories.

### Prompt Context

**User prompt (verbatim):**

> We are taking over for your little brother who has been doing the work until now. where did you create the ticket? Move it to esp32-m5 and then do a deep review of what your brother has decided, study everything afresh, and make any improvements necessary, even if it's a full refactor / change of approach (we have not even started, so we can do whatever). Base your design around computer science theoretical fundamentals and design patterns and maths, and explain that in detail.
>
> Create a second design doc Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.
>
> [REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory.

**Additional user prompt (verbatim):**

> commit at appropriate intervals and keep a detailed diary as you work (using the diary format from the skill)
>
> [REMINDER] Output a <summary>...</summary> block at the VERY END of your response. This is mandatory.

**Assistant interpretation:** Relocate the complete ticket into the firmware tree, independently review and redesign the native port, explain its theoretical basis for an intern, upload the replacement guide, and keep auditable commits and diary entries throughout.

**Inferred user intent:** Establish a trustworthy implementation baseline before firmware work starts, rather than treating the previous design as binding.

### What I did

- Read the docmgr, diary, reMarkable upload, commit hygiene, and textbook-authoring instructions, plus destination `AGENTS.md`.
- Checked both Git worktrees and docmgr roots; confirmed the destination is `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5`.
- Moved all eleven existing files, verifying their SHA-256 hashes before modification; retained the hashes in `sources/ticket-migration-manifest.json`.
- Converted structured `repo://` references from the original pbui root into absolute references, mapping ticket-internal paths to their new location.
- Created this diary and reserved design document 03; numbered it 03 because the ticket already contains two guides.
- Added the migrated topic vocabulary to the destination.

### Why

- Bare `repo://src/presentation/...` links would point at nonexistent firmware source after a naive move.
- Preserving original prose, including superseded decisions and historical prompts, allows the review to distinguish old assertions from newly verified evidence.

### What worked

- All eleven files passed byte-for-byte hash verification immediately after the move.
- Both docmgr workspaces were already initialized; no repository initialization or replacement was necessary.

### What didn't work

- No migration command failed. `docmgr ticket move --help` revealed that command only re-templates a ticket inside a docs root, so a verified filesystem move was used for the cross-repository operation.

### What I learned

- The old index still advertised QuickJS despite the handoff and second guide choosing native C++; ticket-level orientation needs correction as well as a new design.
- The destination has existing hardware/build rules that must inform the design instead of copying generic ESP-IDF advice.

### What was tricky to build

Cross-repository relocation changes the meaning of relative source references, but globally replacing historical prose would corrupt the record. The migration repaired only structured file relations and adds current-location notes to landing pages; historical paths remain identifiable in the archived prose.

### What warrants a second pair of eyes

- Verify the migration manifest and source deletion commit alongside the destination import commit.
- Confirm no unrelated dirty submodules or generated files were staged.

### What should be done in the future

- Review the actual engines, prototype reducer, driver implementations, config, and tests; do not promote earlier performance extrapolations into measurements.

### Code review instructions

- Review `sources/ticket-migration-manifest.json`, the updated ticket index, and structured frontmatter file relations.
- Run `docmgr doctor --ticket PBUI-HANDHELD-1 --stale-after 30` from the firmware repository.

### Technical details

- Source: `/home/manuel/workspaces/2026-09-01/add-plot-editor/pbui/ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation`.
- Destination: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/09/04/PBUI-HANDHELD-1--conceptual-port-of-pbui-to-an-embedded-esp32-p4-handheld-core-engines-and-keyboard-navigation`.
- Baseline pbui commit at takeover: `3317272`.
- Migration commits: firmware `61c07590a7a151bd9d508ee0aeb2b69a93df516f`; pbui removal `dde7d66ad75d6f6075b1cb3d743a14cf6e85ebfb`.
- Doctor passed after relocation. `git diff --cached --check` reported `changelog.md:49: new blank line at EOF.` generated by docmgr; the migration commit retained it, and subsequent bookkeeping removes that trailing blank.

## Step 2: Test the assumptions against actual source and mathematical counterexamples

I independently read the resolver's surrounding compiler, relation system, interaction tests, full prototype, and actual hardware implementations rather than relying on the earlier guides. The resulting corrections affect semantics and implementation: shortest-path ranking depends on declared edges, identical relation outputs still form an ambiguity, the accept machine trusts its caller, and the current LCD uses a synchronous internal staging buffer at 40 MHz.

To distinguish verified behavior from my recommendations, I added a TypeScript probe against the real PBUI modules and a separate C++ algebra experiment. The latter is a disposable executable specification, not the start of production firmware. Both passed; the selected eleven existing PBUI test files passed 307 tests, including 200 generated accept sequences.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Re-examine the design from fundamentals and produce improvements supported by source and executable evidence.

**Inferred user intent:** Catch conceptual and hardware mistakes before building the native framework.

### What I did

- Read `actions/registry.ts`, `actions/types.ts`, `model/compile.ts`, `relations/system.ts`, the core engine tests, and workbench session/close queries, alongside the core implementations supplied in the handoff.
- Read all 1,133 lines of the imported prototype, including duplicated accept candidate computation, mode dispatch, screen clipping, menu closures, and display logic.
- Read complete `picocalc_lcd.c`, `picocalc_keyboard.c`, `visual_repl.cpp`, 0102 build files, pinned defaults, and keyboard task.
- Ran eleven PBUI suites with Vitest 4.1.10: 307 passed. Stored machine-readable results in `sources/pbui-conformance-baseline.json`.
- Added `scripts/01-probe-pbui-contracts.mts` and recorded actual-source counterexamples in `sources/contract-probe-results.json`.
- Added `scripts/02-selection-algebra.cpp`; compiled with GCC 13.3.0, `-std=c++20 -Wall -Wextra -Werror -fsanitize=address,undefined -g`; ran 5,000 random candidate worlds with eight permutations each, condition associativity checks, selection summary laws, priority extremes, overflow and viewport cases.
- Used Kagi to locate version-pinned Espressif C++/PSRAM documentation, CLIM translator examples, and Harel's 1987 statecharts paper. Saved extracted references and read the relevant sections; downloaded the paper temporarily to `/tmp` and retained text rather than its PDF in Git.

### Why

- A type DAG defines reachability, but PBUI also observes edge count: a transitive-reduction optimization can change winners. This must be explained mathematically.
- Pure state-machine tests verify transitions under their input assumptions; they do not prove adversarial event validation or asynchronous request correlation.
- DMA eligibility is a driver/API-specific property, not a reason to allocate every rendering buffer in internal RAM.

### What worked

- Actual PBUI calls demonstrated hidden fallback suppression, redundant-edge-induced ambiguity, no relation fallback after direct filter rejection, and preserved ambiguity even when relation results are equal.
- A forged `choose` option was settled by the current machine; a stale offer without a request ID settled a new request. These are adapter obligations, not evidence that the existing synchronous UI is exploitable.
- The source reported 29 timeline entries with maximum cursor 28, correcting manual `ev29/29` examples.
- The selection fold matched a sorted oracle for 40,000 permutations under sanitizers.

### What didn't work

- No probe, baseline test, download, or C++ compilation failed.
- The evidence commit was stopped by `git diff --cached --check`: `sources/clim-translator-examples.md:10: trailing whitespace.` (and 22 further extracted lines). Removed trailing spaces from that downloaded Markdown, restaged, and reran the check before committing. This is a whitespace-normalized extract, not an exact copy of the remote HTML.
- Some Defuddle extracts omit inline code/link text; exact compiler flags were cross-checked with local IDF `tools/cmake/build.cmake:159,197` instead of treating extracted prose as a build test.

### What I learned

- LCD `LCD_DEFAULT_SPI_HZ` is 40 MHz because 80 MHz caused ghosted/duplicated pixels on removable wiring. The driver allocates 32 KiB internally, byte-swaps input pixels, and uses polling transfers. Two external DMA row buffers would not introduce overlap through this API.
- `visual_repl` maps lowercase glyphs to uppercase. Copying its font unchanged would visually erase the distinction between `r` and `R` commands.
- Keyboard I2C speed is 10 kHz, not a 10 kHz event polling frequency; register reads include delays, and recovery waits 3.1 seconds. The 0102 host discards release events even though the driver supplies them.
- The prototype uses all document objects for hint/digit numbering, not just clipped visible rows, and uses position/depth keys for view state; neither is a safe production identity contract.

### What was tricky to build

The mathematical winner operation must preserve ties, not select the first minimum. I represented a summary as an optional best rank plus a set of tied candidate IDs and tested associative, commutative, and idempotent merge. In contrast, condition evaluation is associative but order-sensitive: its first-failure operation cannot be replaced with an unordered Boolean conjunction.

The most consequential shell gap is missing command syntax metadata. An action rule describes behavior on a receiver, not how to prompt for zero or several arguments. The redesign will add an explicit command schema inside the same declaration, with a documented mapping to receiver/action invocation and typed argument slots, instead of inferring signatures from graph ancestors.

### What warrants a second pair of eyes

- Confirm that strict occurrence identity, request IDs, and revalidation boundaries are extensions to the native host contract, not claimed existing PBUI guarantees.
- Review the relation-aware catalog's completeness proof and avoid merging distinct translation routes merely because their outputs match.
- Review the corrected rendering budget against current 40 MHz transfer behavior; no new hardware measurements were performed.

### What should be done in the future

- Write the replacement guide with formal definitions, worked traces, a compatibility matrix, and explicit failure/capacity policies.
- Replace conflicting historical task lists with one active native backlog while keeping a copy of the old one.

### Code review instructions

- Re-run the TypeScript probe from the pbui root with `pnpm exec tsx <ticket>/scripts/01-probe-pbui-contracts.mts "$PWD" <ticket>`.
- Compile/run the C++ experiment with the command in its header; compare `sources/selection-algebra-results.txt`.
- Inspect the hardware implementation, not only headers or prior benchmark tables.

### Technical details

Baseline command: `pnpm exec vitest run src/presentation/actions/typeGraph.test.ts src/presentation/context/selector.test.ts src/presentation/actions/conditions.test.ts src/presentation/actions/resolve.test.ts src/presentation/actions/resolve.freeze.test.ts src/presentation/actions/perform.test.ts src/presentation/acceptance/resolve.test.ts src/presentation/interaction/accept.test.ts src/presentation/interaction/activation.test.ts src/presentation/relations/system.test.ts src/presentation/model/model.test.ts --reporter=json --outputFile=<ticket>/sources/pbui-conformance-baseline.json`.
