---
Title: Implementation Diary
Ticket: ESP-53-PULP-CONNECTIVITY
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - architecture
    - eink
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: "Chronological diary of ESP-53 work: design docs, onboarding guide, and (later) implementation steps."
LastUpdated: 2026-07-16T18:36:11.328080972-04:00
WhatFor: "Resuming ESP-53 work: read this before touching the ticket."
WhenToUse: "Before resuming or reviewing ESP-53 work."
---

# Diary

## Goal

Capture the ESP-53 connectivity journey: design phase (connectivity guide, system onboarding guide), then implementation (buzzer → files → wifi → http → serve → settings → hardening).

## Step 1: Full-system onboarding guide (design-doc/02)

The ticket already carried the connectivity design guide (design-doc/01), but that document assumes its reader knows the whole PULP OS stack. The user asked for a guide "explaining all the parts of the system needed to understand what it is" — a genuine onboarding document. So this step produced design-doc/02: a 15-section, ~530-line system guide covering the hardware and its constraints, repo layout and toolchain rules, all four s3paper components, the present pipeline, the five MicroQuickJS facts that dictate every binding convention, the binding layer (handles, `__cbs`, dispatch, tick), the stdlib/bytecode regeneration protocol, the complete v2 JS API reference (verified against `app_js_bindings.h`), pulp.js patterns, the owner/event architecture, serial and validation discipline, a pointer into design-doc/01, a ten-item gotcha catalog of bugs we actually hit, and a glossary.

It was written from verified sources, not memory: the API tables came from grepping `PULP_JS_FN` prototypes and the console command registrations, and the file map from listing the component include dirs. The PDF was uploaded to the reMarkable next to the connectivity guide.

### Prompt Context

**User prompt (verbatim):** "Ok, Create Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** The existing design-doc/01 is a 255-line connectivity design summary; the user (having just viewed it in md-view) wants the full intern treatment — a comprehensive document explaining every part of the system a newcomer needs, not just the ESP-53 delta. Store it in the ESP-53 ticket and upload the PDF to reMarkable.

**Inferred user intent:** Make the ticket self-sufficient for onboarding: someone (or a future session) should be able to read the ticket's docs alone and understand the whole stack well enough to implement connectivity safely.

### What I did
- `docmgr doc add` → design-doc/02, then wrote the 15-section guide (architecture diagrams, pipeline diagram, pseudocode for handle unpacking and dispatch, API tables, file references throughout).
- Verified API surface against `0114-papers3-pulp-os/main/app_js_bindings.h` (all `PULP_JS_FN` entries) and console commands against `app_console.cpp` registrations before writing sections 9 and 12.
- Related the four load-bearing source files to the doc; updated the ticket changelog.
- Uploaded via `remarkable_upload.py --force` (pandoc/xelatex → rmapi); the file landed in `ai/2026/07/16/` root, so moved it with `rmapi mv` into `ai/2026/07/16/ESP-53-PULP-CONNECTIVITY/` next to the connectivity guide.

### Why
- design-doc/01 deliberately stayed lean (255 lines) and defers system context; a new reader hitting it cold lacks the widget-tree, GC, and toolchain background it presumes.
- Writing API references from grep output rather than recall avoids documenting methods that don't exist — the guide is meant to be trusted verbatim.

### What worked
- The `PULP_JS_FN` prototype list in `app_js_bindings.h` is a complete, single-file inventory of the JS API — ideal documentation source of truth.
- `rmapi mv` cleanly relocated the misplaced upload without re-rendering.

### What didn't work
- `remarkable_upload.py <file.md>` without `--ticket-dir` infers the ticket dir as `~/.local` and uploads to the date root instead of the ticket subfolder. Worked around with `rmapi mv`; next time pass the md file *and* let the remote dir be fixed afterwards, or use `--mirror-ticket-structure` from the ticket dir.

### What I learned
- The upload script's remote-folder inference is based on the ticket dir of the *default documents*, not of the md arguments; explicit paths bypass it.

### What was tricky to build
- Scope control: the guide had to cover ten subsystems without duplicating the three vault deep-dives or design-doc/01. The resolution was a strict "what a newcomer needs before design-doc/01 makes sense" test per section, with pointers out to the deep-dives for rationale. The gotcha catalog (section 14) compresses the ESP-50/51/52 postmortems into ten actionable items instead of retelling them.

### What warrants a second pair of eyes
- Section 9's API tables: verified against binding prototypes, but signatures/argument orders (e.g. canvas verb parameter order `line(x0,y0,x1,y1,t,gray)`) should be spot-checked against `js_widgets.cpp` parsing before an intern treats them as gospel.
- Section 2's timing claims (full refresh ~1 s, partial ~hundreds of ms) are order-of-magnitude from observation, not measured on this exact panel firmware.

### What should be done in the future
- When ESP-53 implementation starts, keep design-doc/02 sections 9 and 12 updated as `wifi`/`http`/`serve`/`files`/`buzzer` singletons and any new console commands land — the guide claims to be the API reference.

### Code review instructions
- Read `design-doc/02-pulp-os-system-onboarding-guide-every-part-of-the-system-for-a-new-intern.md` end to end; cross-check section 9 against `0114-papers3-pulp-os/main/app_js_bindings.h` and section 12 against `0114-papers3-pulp-os/main/app_console.cpp` `RegisterCommand` calls.
- Validate the reMarkable copy exists: `rmapi ls ai/2026/07/16/ESP-53-PULP-CONNECTIVITY`.

### Technical details
- Upload pipeline: `python3 ~/.local/bin/remarkable_upload.py --force <md>` (pandoc/xelatex + DejaVu → PDF → `rmapi put`), then `rmapi mv "ai/2026/07/16/<name>" "ai/2026/07/16/ESP-53-PULP-CONNECTIVITY/"`.
- Guide structure: §1 what it is, §2 hardware/constraints, §3 repo+toolchain, §4 s3paper_core, §5 m5/storage/runtime, §6 MicroQuickJS facts, §7 binding layer, §8 bytecode toolchain, §9 JS API reference, §10 pulp.js, §11 owner/events, §12 console/validation, §13 ESP-53 pointer, §14 gotcha catalog, §15 glossary.
