---
Title: Diary
Ticket: ESP-03-DOC-JS
Status: complete
Topics:
    - microquickjs
    - quickjs
    - javascript
    - esp-idf
    - esp32s3
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: components/mqjs_service/mqjs_service.cpp
      Note: Primary implementation reference for diary entries
    - Path: ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md
      Note: The main textbook analysis being authored
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-25T08:52:56.940184047-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Keep a frequent, detailed research and writing log while producing a “textbook-style” analysis of the `mqjs_service` component for ticket `ESP-03-DOC-JS`.

## Step 1: Ticket Setup + Initial Code Map (2026-01-25)

This step establishes the docmgr ticket workspace and gathers a first-pass map of what `mqjs_service` is, where it lives, and where it’s used. The intent is to start writing from primary sources (the code) while capturing enough context to structure the forthcoming “textbook chapter” narrative.

The core outcome is a concrete file list for the component’s public API, implementation, and the two in-repo consumers (`0048` and `0066`) that demonstrate the patterns `mqjs_service` is designed to enable.

### Prompt Context

**User prompt (verbatim):** "create a docmgr ticket ESP-03-DOC-JS and create a detailed analysis of the mqjs_service component, like a full detailed textbook written by peter norvig, with pseudocode, prose paragraphs, full callouts / sections to explain the fundamentals, symbols, reference to files, bullet point lists, diagrams. Analyze usages.

Also make a section where you critically assess the quality to make the reader aware of potential shortcomings ro things that might need to get addressed.

Store the textbook analysis in the ticket.

Keep a frequent detailed diary of your research, at intermediate steps (not just at the end)."

**Assistant interpretation:** Create a new docmgr ticket and produce a long-form, structured “textbook” explanation of `components/mqjs_service` (API + implementation + concurrency model) including diagrams/pseudocode and an explicit critical assessment section; also keep an ongoing diary with intermediate updates as research progresses.

**Inferred user intent:** Build durable, high-signal documentation that lets future readers confidently use/extend `mqjs_service` (and recognize its risks/limitations) without re-deriving the design from scattered code.

**Commit (code):** N/A (documentation-only so far; no code commits requested)

### What I did
- Created the docmgr ticket `ESP-03-DOC-JS` and added two documents: a `design-doc` for the textbook and a `reference` doc for this diary.
- Searched the repository for `mqjs_service` and enumerated all compile-time usages and includes.
- Read the public header and implementation for `mqjs_service`, and the supporting VM host primitive `mqjs_vm`.
- Identified the two real consumers (`0048-cardputer-js-web` and `0066-cardputer-adv-ledchain-gfx-sim`) as the primary “usage case studies” for the textbook.

### Why
- The analysis needs to be anchored on the actual contract and implementation behavior (`mqjs_service.h` + `mqjs_service.cpp`), then validated against consumers to avoid speculative documentation.

### What worked
- `docmgr ticket create-ticket` and `docmgr doc add` succeeded and created a stable ticket workspace under `ttmp/2026/01/25/ESP-03-DOC-JS--...`.
- `rg` and targeted `sed` reads were sufficient to locate all in-repo uses: only `0048` and `0066` include `mqjs_service.h`.

### What didn't work
- N/A.

### What I learned
- `mqjs_service` is a single-owner “JS VM task” pattern implemented with a FreeRTOS queue: multiple producers post `eval` or “job” messages to a dedicated service task that lazily initializes the MicroQuickJS VM (`MqjsVm`) on first use.
- The component splits work into two primary paths: `mqjs_service_eval` (string eval + captures output/error) and `mqjs_service_run/post` (arbitrary host callbacks executed on the VM thread).

### What was tricky to build
- N/A (no implementation work yet).

### What warrants a second pair of eyes
- N/A (analysis still in progress; later steps will call out concurrency/memory-lifetime risks precisely).

### What should be done in the future
- Write the main textbook doc, including: message flow diagrams, a precise ownership/lifetime model for `Pending` structs, and critical assessment of deadline semantics + stop/shutdown behavior.

### Code review instructions
- Start at `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md` once populated.
- Review this diary at `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/reference/01-diary.md`.

### Technical details
- Key component files:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/include/mqjs_service.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_service.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/include/mqjs_vm.h`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_vm.cpp`
- Primary consumer case studies:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/js_service.cpp`

## Step 2: Deep Dive Notes + First Full Textbook Draft (2026-01-25)

This step converts the “file map” into a coherent mental model and begins writing the long-form textbook document. The focus is on making the concurrency and timeouts legible, because those are the areas where embedded JS integrations most often fail in practice.

The core outcome is an initial end-to-end draft of the textbook document, including diagrams, pseudocode, usage case studies, and a “warning label” critical assessment section.

### Prompt Context

**User prompt (verbatim):** "create a docmgr ticket ESP-03-DOC-JS and create a detailed analysis of the mqjs_service component, like a full detailed textbook written by peter norvig, with pseudocode, prose paragraphs, full callouts / sections to explain the fundamentals, symbols, reference to files, bullet point lists, diagrams. Analyze usages.

Also make a section where you critically assess the quality to make the reader aware of potential shortcomings ro things that might need to get addressed.

Store the textbook analysis in the ticket.

Keep a frequent detailed diary of your research, at intermediate steps (not just at the end)."

**Assistant interpretation:** Draft the main textbook analysis, grounding it in `mqjs_service` + `mqjs_vm` implementation details and the `0048`/`0066` usage patterns, and include an explicit shortcomings section.

**Inferred user intent:** Produce a durable “how it works” reference that can be used as onboarding material and as a design/debugging aid.

**Commit (code):** N/A (documentation edits only; no code commits requested)

### What I did
- Read and summarized the internal message loop and pending/completion mechanism in `components/mqjs_service/mqjs_service.cpp`.
- Read and summarized `MqjsVm` to understand deadline enforcement and output/exception capture (`components/mqjs_service/mqjs_vm.cpp`).
- Read `0066`’s `mqjs_timers` to show how timer callbacks are marshaled to the JS owner thread.
- Wrote the initial full draft at `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md`.

### Why
- The correctness and safety of `mqjs_service` is almost entirely about ownership, lifetimes, and the real meaning of “timeouts.” Documenting these precisely is higher value than line-by-line narration.

### What worked
- The component’s structure maps cleanly to an actor/message-passing explanation, which makes it possible to write clear diagrams and pseudocode aligned with the actual implementation.

### What didn't work
- N/A.

### What I learned
- `timeout_ms` in `mqjs_service_eval`/`mqjs_job_t` is a VM *deadline*, not a caller-side *wait timeout*; synchronous calls still wait indefinitely on `portMAX_DELAY`.
- `mqjs_service_post` is the critical primitive for “don’t run JS in non-JS tasks” patterns (timers, IRQ-ish producers, and coalesced event delivery).
- `mqjs_service_stop` is intentionally not a graceful shutdown; it is best treated as “only safe when quiescent.”

### What was tricky to build
- Translating “timeouts” into a precise 3-part model (queue admission vs VM deadline vs caller wait) without overpromising behavior.

### What warrants a second pair of eyes
- The “critical assessment” section in the textbook doc calls out potential issues (deadlock, shutdown semantics, handshake ordering). It’s worth verifying that none of these are already mitigated elsewhere (e.g., by usage discipline) or contradicted by MicroQuickJS invariants.

### What should be done in the future
- Relate the key code files to the textbook doc and diary via `docmgr doc relate` so the ticket index stays navigable.
- Optionally update the ticket `index.md` overview with direct links to the textbook + diary docs.

### Code review instructions
- Read the textbook doc top-to-bottom once: `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md`.
- Spot-check claims by comparing to:
  - `components/mqjs_service/mqjs_service.cpp`
  - `components/mqjs_service/mqjs_vm.cpp`
  - `0048-cardputer-js-web/main/js_service.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/mqjs/mqjs_timers.cpp`

### Technical details
- Textbook draft path: `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md`

## Step 3: Relate Files + Docmgr Validation Pass (2026-01-25)

This step makes the ticket navigable: it links the most important implementation files directly to the textbook doc, updates the ticket index/tasks, and runs docmgr checks. The goal is that a future reader can jump from the doc straight to the code without re-searching the repo.

It also captures a small but important tooling lesson: for `docmgr validate frontmatter`, the `--doc` argument is relative to the docs root (so passing a path that already includes `ttmp/` doubles it).

### Prompt Context

**User prompt (verbatim):** "create a docmgr ticket ESP-03-DOC-JS and create a detailed analysis of the mqjs_service component, like a full detailed textbook written by peter norvig, with pseudocode, prose paragraphs, full callouts / sections to explain the fundamentals, symbols, reference to files, bullet point lists, diagrams. Analyze usages.

Also make a section where you critically assess the quality to make the reader aware of potential shortcomings ro things that might need to get addressed.

Store the textbook analysis in the ticket.

Keep a frequent detailed diary of your research, at intermediate steps (not just at the end)."

**Assistant interpretation:** Make sure the ticket is “docmgr-clean” and that the analysis is easy to navigate from the ticket workspace.

**Inferred user intent:** Treat the documentation as a real deliverable: linked, structured, and verifiable.

**Commit (code):** N/A

### What I did
- Related 7 key code files to the textbook doc via `docmgr doc relate` (public API, implementation, VM primitive, and the 0048/0066 case studies).
- Related the textbook doc (and the main implementation file) to the diary doc.
- Updated ticket `index.md` with direct links to the textbook and diary.
- Updated `tasks.md` to reflect completed work and the remaining “doctor/validate” step.
- Ran `docmgr doctor --ticket ESP-03-DOC-JS`.
- Ran `docmgr validate frontmatter` for the ticket index, textbook doc, and diary doc.

### Why
- “RelatedFiles” is the fastest way for readers to move between explanation and implementation; without it, tickets decay into dead ends.
- Docmgr validation prevents subtle frontmatter drift that makes search and tooling less reliable.

### What worked
- `docmgr doctor` reported “All checks passed” for `ESP-03-DOC-JS`.
- After using doc-root-relative paths, `docmgr validate frontmatter` succeeded for all three documents.

### What didn't work
- Initial validation call used a `--doc` path that already included `ttmp/`, resulting in a doubled path:
  - Command: `docmgr validate frontmatter --doc ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md --suggest-fixes`
  - Error: `Error: open /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md: no such file or directory`
  - Fix: pass a doc-root-relative path: `--doc 2026/01/25/.../design-doc/...`

### What I learned
- Docmgr treats `--doc` paths as relative to the docs root, so `ttmp/` should not be included in the argument.

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- N/A (no code changes; doc changes are straightforward).

### What should be done in the future
- If/when `mqjs_service` changes, update the “Critical assessment” section first (it is the most sensitive to behavioral drift).

### Code review instructions
- Review the RelatedFiles frontmatter in:
  - `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md`
  - `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/reference/01-diary.md`
- Re-run: `docmgr doctor --ticket ESP-03-DOC-JS`

### Technical details
- Key docmgr commands used:
  - `docmgr doc relate --doc 2026/01/25/ESP-03-DOC-JS--.../design-doc/01-textbook-mqjs-service-deep-dive.md --file-note "...:..."`
  - `docmgr doctor --ticket ESP-03-DOC-JS --stale-after 30`
  - `docmgr validate frontmatter --doc 2026/01/25/ESP-03-DOC-JS--.../index.md --suggest-fixes`

## Step 4: DevRel Guide + Component README (2026-01-25)

This step adds a developer-facing walkthrough that explains how to adopt `mqjs_service` in a friendly, narrative style and introduces a component-level README so the guidance is discoverable outside the ticket.

The outcome is a “new developer” section embedded in the textbook doc and a `components/mqjs_service/README.md` file that summarizes usage patterns, safety notes, and file layout.

### Prompt Context

**User prompt (verbatim):** "quick note, write documentation for the component if not already, in a very verbose engaging style that explains both implementation and how to use for new developers using the component, think like a devrel technical writer."

**Assistant interpretation:** Add a verbose, engaging devrel-style guide to the existing textbook documentation and create a standalone component README with quick-start usage.

**Inferred user intent:** Ensure the component is approachable and self-documented for new contributors without forcing them to read a long internal design doc first.

**Commit (code):** N/A

### What I did
- Added a “Developer Guide: A DevRel Walkthrough for New Contributors” section to the textbook doc.
- Expanded that section with concrete usage examples, anti-patterns, and backpressure explanations.
- Added `components/mqjs_service/README.md` with quick-start examples for both the service and VM-only modes.
- Related the new README to the textbook doc via `docmgr doc relate`.

### Why
- The ticket doc is deep and long; a devrel-style guide improves onboarding and makes the component usable without heavy context.

### What worked
- The new guide builds directly on the existing implementation semantics, so it stays aligned with the actual API.

### What didn't work
- N/A.

### What I learned
- A component-level README is the fastest way to make the merged `mqjs_service` + `MqjsVm` story discoverable by new developers.

### What was tricky to build
- N/A.

### What warrants a second pair of eyes
- N/A (documentation-only updates).

### What should be done in the future
- N/A.

### Code review instructions
- Read the new guide section in:
  - `ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md`
- Review the README at:
  - `components/mqjs_service/README.md`

### Technical details
- `docmgr doc relate --doc /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/25/ESP-03-DOC-JS--mqjs-service-component-textbook-analysis/design-doc/01-textbook-mqjs-service-deep-dive.md --file-note ".../components/mqjs_service/README.md:Component-level devrel documentation for mqjs_service"`
