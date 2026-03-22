---
Title: Diary
Ticket: ESP-33-PAPERS3-ALPHABET-GRAFFITI
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
    - storage
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-03-21T21:03:59.403386936-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Capture the implementation of the third PaperS3 handwriting app: a two-mode alphabet trainer and graffiti-style writer with persistent template storage.

## Step 1: Create the new ticket and a buildable app skeleton

This step establishes the new workstream cleanly instead of piling the feature into `0076`. The result is a new ticket, a dedicated diary, and a fresh `0077` project that already builds as a named PaperS3 app with a minimal two-mode placeholder UI.

The key thing this unlocked is safe incremental work. Later commits can add storage, training controls, and writing behavior without needing to disentangle them from ticket/bootstrap setup.

### Prompt Context

**User prompt (verbatim):** "ok, let's create a third app where this time we have two modes, one where I input templates for the entire aphabet (ABCDEF...0123456789) and then one where I can just write graffiti style. Make nice simple UIs. store the templates on disk. I can switch to writing whenver, even if not all letters are registered.

Work task by task, commit as you go, keep a detailed diary"

**Assistant interpretation:** Create a new third PaperS3 app with separate training and writing modes, keep the work split into logical tasks, commit incrementally, and maintain a detailed diary.

**Inferred user intent:** Build a more complete handwriting prototype that moves beyond ad hoc templates into an alphabet-scale persistent recognizer while keeping the development process reviewable and well documented.

**Commit (code):** `99512ac` — `feat(papers3): scaffold alphabet graffiti app`

### What I did

- created ticket `ESP-33-PAPERS3-ALPHABET-GRAFFITI`
- added the diary document
- created new project `0077-papers3-alphabet-graffiti`
- reused the donor component wiring and copied `protractor_math.*` as the algorithm foundation
- added `alphabet_app.*` with a simple placeholder two-mode UI
- updated the ticket index, tasks, and changelog for task-based execution

### Why

- a new numbered app keeps the experimentation isolated from `0076`
- a buildable skeleton makes the first commit safe and useful
- reusing the Protractor math avoids re-deriving already working gesture primitives

### What worked

- ticket creation and project setup were straightforward
- the placeholder UI gives the next tasks a stable visual frame
- the project structure mirrors the earlier PaperS3 apps, which keeps repo conventions consistent

### What didn't work

- no failures in this step

### What I learned

- the cleanest path is to treat this app as a sibling of `0076`, not as a mutation of it

### What was tricky to build

- the main design decision was defining a first commit boundary that was meaningful but not prematurely feature-heavy

### What warrants a second pair of eyes

- whether the final product should keep one app with mode switching or later split training and writing into separate deployables

### What should be done in the future

- add persistent storage and glyph-management UI in the next task

### Code review instructions

- start with `0077-papers3-alphabet-graffiti/CMakeLists.txt`
- then inspect `main/alphabet_app.cpp` and `main/app_main.cpp`
- confirm the ticket task breakdown in `tasks.md`

### Technical details

Commands used:

```bash
docmgr ticket create-ticket --ticket ESP-33-PAPERS3-ALPHABET-GRAFFITI --title "PaperS3 alphabet graffiti recognizer with persistent templates" --topics esp32-s3,esp32s3,firmware,m5stack,m5gfx,ui,storage
docmgr doc add --ticket ESP-33-PAPERS3-ALPHABET-GRAFFITI --doc-type reference --title "Diary"
mkdir -p 0077-papers3-alphabet-graffiti/main
```

## Related

- `../tasks.md`
- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0077-papers3-alphabet-graffiti`
