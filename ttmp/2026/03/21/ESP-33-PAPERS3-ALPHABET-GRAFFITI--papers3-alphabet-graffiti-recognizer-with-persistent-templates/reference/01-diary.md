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

## Step 2: Add persistent glyph storage and a real alphabet training UI

This step replaces the placeholder app shell with the first actually useful milestone: a buildable trainer for `A-Z` and `0-9` with flash-backed persistence. The new runtime can page through all 36 glyphs, draw a stroke, save the selected glyph to SPIFFS, delete an existing glyph, reload from disk, and preview recognition results against the trained set.

The key boundary for this commit is that the app is now a real alphabet template manager instead of a sketch. `WRITE` mode is still a forward-looking tab in this step, but the persistent data model and recognition path that writing mode will depend on are already in place.

### Prompt Context

**User prompt (verbatim):** "ok, let's create a third app where this time we have two modes, one where I input templates for the entire aphabet (ABCDEF...0123456789) and then one where I can just write graffiti style. Make nice simple UIs. store the templates on disk. I can switch to writing whenver, even if not all letters are registered.

Work task by task, commit as you go, keep a detailed diary"

**Assistant interpretation:** The next task boundary after scaffolding is persistent template management plus a training-oriented UI that scales from 8 demo slots to the full 36-glyph alphabet and digit set.

**Inferred user intent:** Make the app genuinely operable for collecting a personal single-stroke alphabet before tackling live text entry.

**Commit (code):** `b04bd3e` — `feat(papers3): add alphabet training storage workflow`

### What I did

- replaced the copied `0076` runtime with a new `alphabet_graffiti::AlphabetApp`
- introduced `glyph_store.h` and `glyph_store.cpp`
- mounted SPIFFS at `/spiffs` and stored templates in `/spiffs/glyph_templates.txt`
- added a `storage` partition in `partitions.csv`
- changed the right-side panel into a paged `4 x 3` glyph picker so all `36` symbols fit in a clean UI
- added `SAVE GLYPH`, `CLEAR STROKE`, `DELETE GLYPH`, and `RELOAD DISK` controls
- kept the Protractor resampling/vectorization path from `0076` and reused it for recognition preview
- updated ticket metadata to reflect the new storage-backed training milestone

### Why

- the placeholder UI was not enough to validate whether the alphabet-scale workflow would fit on PaperS3
- flash-backed storage is required before the app can be useful across reboots
- building the training path first creates the durable substrate that `WRITE` mode will consume later

### What worked

- the paged picker fits the PaperS3 layout without crowding the canvas
- the simple text file format is easy to inspect and easy to regenerate
- Protractor template comparison works fine when widened from a small slot set to the full glyph table
- the custom `storage` partition integrated cleanly once SPIFFS was mounted from the app

### What didn't work

- my first attempt reused copied `0076` files too literally, which left the namespace/class names mismatched with `app_main.cpp`
- the first full build failed because `main/CMakeLists.txt` did not declare `M5Unified` in `REQUIRES`
- the second build failed on `std::clamp` because the arguments were deduced as mixed integer types

### What I learned

- the right reuse boundary was the Protractor math layer, not the whole `0076` app shell
- once `main` includes `M5Unified.hpp` directly, the dependency must be explicit in the component registration
- small integer type mismatches in embedded C++ are easy to miss and are worth documenting because they surface late in the build

### What was tricky to build

- fitting 36 selectable glyphs into a UI that still leaves room for a large canvas and useful feedback
- deciding on a storage format that is simple enough for debugging but still preserves the exact trained vectors
- keeping `WRITE` mode visible in the UI without pretending it already has a finished text-entry workflow

### What warrants a second pair of eyes

- whether the current page size (`12` glyphs per page) feels right on device or should become `9` larger targets instead
- whether the SPIFFS file should eventually include metadata such as template count, threshold hints, or stroke statistics
- whether the current mode-tab placement reads clearly enough on e-paper in physical use

### What should be done in the future

- implement the actual graffiti writing workflow and recognized text buffer
- add write-mode controls for spacing, correction, and clearing text
- decide whether to store multiple templates per glyph or only the most recent one

### Code review instructions

- start with `0077-papers3-alphabet-graffiti/main/glyph_store.cpp`
- then inspect `0077-papers3-alphabet-graffiti/main/alphabet_app.h`
- then inspect `0077-papers3-alphabet-graffiti/main/alphabet_app.cpp`
- verify the storage partition change in `0077-papers3-alphabet-graffiti/partitions.csv`
- compare the ticket updates in `tasks.md` and `changelog.md` with the code changes

### Technical details

Commands used:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh && idf.py set-target esp32s3 && idf.py build
source /home/manuel/esp/esp-idf-5.3.4/export.sh && idf.py build
```

Observed build failures and fixes:

```text
fatal error: M5Unified.hpp: No such file or directory
```

Resolution:

- add `REQUIRES M5Unified` to `0077-papers3-alphabet-graffiti/main/CMakeLists.txt`

```text
error: no matching function for call to 'clamp(long int, int, long int)'
```

Resolution:

- normalize `ChangePage()` clamp operands to `int32_t`

Validation result:

```text
Generated .../build/papers3_alphabet_graffiti.bin
papers3_alphabet_graffiti.bin binary size 0x744a0 bytes
Smallest app partition is 0x400000 bytes
0x38bb60 bytes (89%) free
```

## Step 3: Finish the live writing mode and connect it to the trained templates

This step completes the second half of the product. The app already knew how to classify a stroke against the saved templates; this task turns that recognition into a writing workflow with its own UI, output buffer, and editing controls.

The important implementation choice here was to not create a second recognizer. `WRITE` mode uses the exact same resample/vectorize/match path as `TRAIN` mode, and only changes what happens after recognition: instead of saving a template, it appends the best matching glyph when the score clears a confidence threshold.

### Prompt Context

**User prompt (verbatim):** "ok, let's create a third app where this time we have two modes, one where I input templates for the entire aphabet (ABCDEF...0123456789) and then one where I can just write graffiti style. Make nice simple UIs. store the templates on disk. I can switch to writing whenver, even if not all letters are registered.

Work task by task, commit as you go, keep a detailed diary"

**Assistant interpretation:** After the training/storage milestone, the next task is the actual writing-mode workflow: draw a stroke, recognize it against the saved glyphs, and maintain editable output text.

**Inferred user intent:** The app should now be usable as a real two-mode handwriting prototype instead of only a template collection utility.

**Commit (code):** `625dba3` — `feat(papers3): add alphabet graffiti write mode`

### What I did

- added a write buffer state to `AlphabetApp`
- changed the top-right card into a writing buffer when `WRITE` mode is active
- added automatic append after stroke release when the top match exceeds the configured confidence threshold
- added mode-specific write controls:
  - `SPACE`
  - `BACKSPACE`
  - `CLEAR TEXT`
  - `CLEAR STROKE`
- added write-mode status messaging and compact recognition summaries
- rebuilt the firmware to verify the full two-mode app

### Why

- without a text buffer and append logic, `WRITE` mode would just be a relabeled preview screen
- reusing the same classifier avoids inconsistent behavior between training and writing
- explicit edit controls are necessary because single-stroke recognition is inherently approximate

### What worked

- branching the right-side panel by mode kept the layout simple without reworking the whole screen
- auto-append makes the write workflow feel materially different from the training workflow
- the same saved templates can now be used immediately in writing mode, even if the full alphabet is not yet trained

### What didn't work

- no new build failures in this step

### What I learned

- the app architecture was already in a good place after Task 2; adding write mode mostly required post-recognition state handling, not another structural rewrite
- the simplest useful writing UX on e-paper is automatic append plus explicit correction buttons

### What was tricky to build

- making `WRITE` mode feel distinct enough from `TRAIN` mode while still sharing the same underlying gesture pipeline
- fitting status, candidate feedback, and editable text into the same right-side column without making the UI feel crowded

### What warrants a second pair of eyes

- whether the current confidence threshold (`0.82`) is the right default on physical hardware
- whether `SPACE` should stay manual or eventually be inferred from timing/gap heuristics
- whether the writing buffer needs stronger wrapping or pagination for longer text

### What should be done in the future

- test recognition confidence and ergonomics on real PaperS3 hardware
- consider multiple templates per glyph for improved user-specific accuracy
- decide whether to add punctuation in addition to `A-Z` and `0-9`

### Code review instructions

- inspect the mode-specific logic in `0077-papers3-alphabet-graffiti/main/alphabet_app.cpp`
- look at `TryAppendRecognizedGlyph()`, `DrawPaletteCard()`, `DrawControlsCard()`, and `DrawResultsCard()`
- confirm that training-mode behavior still remains intact while write-mode branches are active

### Technical details

Commands used:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh && idf.py build
```

Validation result:

```text
Generated .../build/papers3_alphabet_graffiti.bin
papers3_alphabet_graffiti.bin binary size 0x74f70 bytes
Smallest app partition is 0x400000 bytes
0x38b090 bytes (89%) free
```

## Step 4: Write the intern guide, validate the ticket, and upload to reMarkable

This step turns the implementation into a durable handoff package. The firmware already worked, but the user also wanted a process trail and documentation strong enough for a new intern. That means the job is not complete until the code, guide, ticket metadata, and delivery path all line up.

### Prompt Context

**User prompt (verbatim):** "ok, let's create a third app where this time we have two modes, one where I input templates for the entire aphabet (ABCDEF...0123456789) and then one where I can just write graffiti style. Make nice simple UIs. store the templates on disk. I can switch to writing whenver, even if not all letters are registered.

Work task by task, commit as you go, keep a detailed diary"

**Assistant interpretation:** After the firmware milestones are committed, finish the full handoff workflow: create the long-form guide, validate the ticket, and publish the bundle to reMarkable.

**Inferred user intent:** Make the result reviewable, teachable, and easy to annotate away from the terminal.

**Commit (docs/delivery):** `3d7b4b7` — `docs(ticket): finalize alphabet graffiti guide and delivery`

### What I did

- created `design-doc/01-papers3-alphabet-graffiti-analysis-design-and-implementation-guide.md`
- documented the donor stack, runtime architecture, storage schema, mode behavior, rendering model, and debugging guidance
- added the missing `storage` topic to the workspace vocabulary so the ticket validates cleanly
- ran `docmgr doctor` and verified the ticket passed
- uploaded the ticket bundle to reMarkable
- verified the uploaded folder contents with `remarquee cloud ls`

### Why

- a new intern should not need to reverse-engineer the app from `alphabet_app.cpp`
- the ticket should validate cleanly before it is treated as finished delivery
- reMarkable upload is part of the requested documentation workflow

### What worked

- the guide fit naturally into a single design-doc because the app remains compact enough to explain end-to-end
- the `storage` vocabulary warning was resolved cleanly by adding the topic rather than weakening the document metadata
- `remarquee upload bundle` succeeded without needing a retry or overwrite flag

### What didn't work

- my first attempt to add the `storage` vocabulary entry omitted the required `--description` flag

### What I learned

- docmgr ticket hygiene matters even for small app tickets because the validation report is often the last thing touched before delivery
- the bundle upload workflow is stable enough to use as the final delivery step once the local ticket passes validation

### What warrants a second pair of eyes

- the intern guide is broad and detailed, but it still assumes the reader can navigate C++ and ESP-IDF basics
- the one remaining functional gap is physical hardware validation

### What should be done in the future

- flash the app to a real PaperS3 and test touch ergonomics, recognition confidence, and e-paper behavior
- revise the guide after physical testing if the threshold or UI affordances need adjustment

### Technical details

Commands used:

```bash
docmgr doc add --ticket ESP-33-PAPERS3-ALPHABET-GRAFFITI --doc-type design-doc --title "PaperS3 alphabet graffiti analysis design and implementation guide"
docmgr vocab add --category topics --slug storage --description "Persistent or on-device storage systems"
docmgr doctor --ticket ESP-33-PAPERS3-ALPHABET-GRAFFITI --stale-after 30
remarquee upload bundle ttmp/2026/03/21/ESP-33-PAPERS3-ALPHABET-GRAFFITI--papers3-alphabet-graffiti-recognizer-with-persistent-templates --name "ESP-33 PaperS3 Alphabet Graffiti" --remote-dir "/ai/2026/03/21/ESP-33-PAPERS3-ALPHABET-GRAFFITI" --toc-depth 2 --non-interactive
remarquee cloud ls /ai/2026/03/21/ESP-33-PAPERS3-ALPHABET-GRAFFITI --long --non-interactive
```

Observed command issue and fix:

```text
Parameter description is required
```

Resolution:

- rerun `docmgr vocab add` with `--description`

Validation and delivery results:

```text
docmgr doctor -> All checks passed
remarquee upload bundle -> OK: uploaded ESP-33 PaperS3 Alphabet Graffiti.pdf
remarquee cloud ls -> [f] ESP-33 PaperS3 Alphabet Graffiti
```

## Step 5: Decouple write-mode input from slow refresh and simplify the writing UI

This step is a usability refinement requested after the main app was already complete. The core issue was that `WRITE` mode still behaved too synchronously: it drew each stroke segment immediately and also scheduled full-screen updates in ways that could make fast writing feel tied to e-paper refresh speed. The UI also still showed low-level recognition detail that is useful for debugging but not necessary when the goal is simply to write text.

### Prompt Context

**User prompt (verbatim):** "queue input movements so i can write fast even if refresh is slow. in write mode, no need to show the parsed / low level info"

**Assistant interpretation:** Make stroke capture less dependent on immediate display work, and simplify the write-mode UI so it stops surfacing recognizer internals.

**Inferred user intent:** Optimize for writing feel, not debugging visibility.

### What I did

- added a queued stroke-segment buffer inside `AlphabetApp`
- changed stroke capture so touch handling appends points and pending segments instead of drawing immediately
- added `ProcessPendingDisplayWork()` to flush:
  - pending canvas clears
  - pending stroke segments
  - deferred full-screen UI redraws
- used `M5.Display.displayBusy()` to avoid starting new display work while the panel is still busy
- deferred full `WRITE` mode redraws until the user is idle or a max-latency timeout is reached
- simplified `WRITE` mode cards so they now focus on:
  - writing buffer
  - text length
  - saved glyph count
  - writer status
- removed top-score/candidate-style low-level recognition readouts from the write-mode side panels

### Why

- touch input should remain responsive even if e-paper updates lag behind
- immediate full redraw after every stroke is unnecessary in `WRITE` mode
- low-level recognition details are useful during algorithm development, but they are noise during actual writing

### What worked

- the queued display-work model compiled cleanly and fits the existing single-threaded loop
- prioritizing queued stroke segments ahead of deferred UI redraws preserves the visual ink path while reducing blocking behavior
- the simplified write-mode cards leave the writing workflow much clearer

### What didn't work

- no failures in this step

### What I learned

- the main requirement was not a brand-new input system, but a better ordering of existing work: capture first, flush display second, redraw the whole UI last
- `displayBusy()` is the right donor-level hook for this type of pacing on PaperS3

### What warrants a second pair of eyes

- the current idle/defer timings (`180 ms` idle, `600 ms` max latency) are sensible defaults, but should be tuned on real hardware
- if very long strokes are drawn quickly, it may still be worth profiling whether the queued segment flush batch size should increase

### Technical details

Commands used:

```bash
source /home/manuel/esp/esp-idf-5.3.4/export.sh && idf.py build
```

Validation result:

```text
Generated .../build/papers3_alphabet_graffiti.bin
papers3_alphabet_graffiti.bin binary size 0x75360 bytes
Smallest app partition is 0x400000 bytes
0x38aca0 bytes (89%) free
```
