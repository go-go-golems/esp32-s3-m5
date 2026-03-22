---
Title: Investigation diary
Ticket: ESP-32-PAPERS3-PROTRACTOR
Status: active
Topics:
    - esp32-s3
    - esp32s3
    - firmware
    - m5stack
    - m5gfx
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0076-papers3-protractor-trainer/CMakeLists.txt
      Note: Captured donor component wiring
    - Path: 0076-papers3-protractor-trainer/sdkconfig.defaults
      Note: Recorded console and target defaults
    - Path: 0076-papers3-protractor-trainer/main/protractor_math.cpp
      Note: Captured algorithm porting work
    - Path: 0076-papers3-protractor-trainer/main/trainer_app.cpp
      Note: Captured UI and interaction implementation work
ExternalSources:
    - local:protractor_gesture_recognizer_demo.html
Summary: ""
LastUpdated: 2026-03-21T20:26:40.631195799-04:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Record the actual implementation sequence for the PaperS3 Protractor trainer: what was inspected, what was built, why the design changed from the browser demo, what worked, what remained risky, and how another engineer should review the outcome.

## Step 1: Create the ticket and import the source demo

### Prompt Context

User prompt (verbatim):

> did you commit?
>
> Then, create a new ticket and create a new application that this time uses the protractor algorithm and create an app where I can train the protractor by creating templates and seeing results and clearing it and all, nice UI.
>
> docmgr import file --file ~/Downloads/protractor_gesture_recognizer_demo.html --ticket ...
>
> and then read it and use the algorithm and ideas for layut from there.
>
> Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file
>   references.
>   It should be very clear and detailed. Store in the ticket and the nupload to remarkable.

Follow-up prompt (verbatim):

> keep a diary as you work

Assistant interpretation:

- confirm commit status from the previous task
- create a new ticket
- import the local browser demo into that ticket
- build a second PaperS3 app, this time using Protractor rather than free drawing alone
- create detailed implementation documentation
- keep an implementation diary
- upload the ticket bundle to reMarkable after the ticket is complete

Inferred user intent:

- produce a useful device demo, not just a code sketch
- keep the algorithm traceable to the imported HTML source
- leave behind enough context that a new intern can continue the work

### What I did

- confirmed that the previous task had not been committed
- created ticket `ESP-32-PAPERS3-PROTRACTOR`
- added the initial design-doc and reference docs
- imported `/home/manuel/Downloads/protractor_gesture_recognizer_demo.html` into the ticket with `docmgr import file`

### Why

The imported source was explicitly part of the requested workflow. Making it part of the ticket keeps the algorithm reference co-located with the implementation and documentation.

### What worked

- ticket creation worked normally
- the HTML import succeeded and the imported file was indexed into the ticket workspace

### What didn't work

- nothing failed in this step

### What I learned

- the imported demo already contained the exact Protractor primitives needed for the firmware port
- the browser demo was also opinionated about layout, which made it useful for UI adaptation rather than only algorithm reuse

### What was tricky to build

- the main subtlety this early was not technical failure, but deciding how much of the HTML demo should be copied literally versus translated into a PaperS3-native interface

### What warrants a second pair of eyes

- whether fixed template slots are the right UX compared to a future keyboard-backed naming system

### What should be done in the future

- consider whether template persistence belongs in a follow-up ticket

### Code review instructions

- inspect the imported HTML in `sources/local/protractor_gesture_recognizer_demo.html`
- compare its algorithm sections against the final C++ port

### Technical details

Command used:

```bash
docmgr import file --file /home/manuel/Downloads/protractor_gesture_recognizer_demo.html --ticket ESP-32-PAPERS3-PROTRACTOR
```

## Step 2: Inspect the imported demo and design the PaperS3 adaptation

### Prompt Context

Assistant interpretation:

- understand both the Protractor math and the browser layout
- decide what must stay algorithmically identical
- decide what must change for a touch-only e-paper device

Inferred user intent:

- retain the recognizer’s behavior
- keep the resulting app pleasant and understandable on-device

### What I did

- inspected the imported HTML
- identified the key functions: `resample`, `vectorize`, `optCosDistance`, `recognize`
- inspected the UI sections for score bars, template chips, record/clear/reset controls, and preprocessing stats
- designed a PaperS3 adaptation with:
  - a large left canvas
  - right-side template slots
  - action buttons
  - preprocessing metrics
  - recognition result bars

### Why

The imported source was both a technical reference and a UX reference. Reading only the algorithm would have lost the user’s intent for the trainer workflow.

### What worked

- the HTML demo was easy to map into a device-side architecture
- the algorithm port path was straightforward because the functions were already isolated

### What didn't work

- the browser’s free-form template naming did not transfer cleanly to the device

### What I learned

- fixed template slots are a better PaperS3 fit than trying to invent ad hoc text input
- the browser demo’s panels translate well into card-based e-paper UI regions

### What was tricky to build

- preserving the spirit of the browser layout without forcing a browser-shaped product onto the device

### What warrants a second pair of eyes

- the recognition score presentation choice: on-device cosine similarity bars rather than the HTML’s inverse-distance sorting display

### What should be done in the future

- if the product evolves beyond demo status, revisit naming and persistence together

### Code review instructions

- compare the imported HTML algorithm at:
  - `resample` around lines 134-154
  - `vectorize` around lines 156-176
  - `optCosDistance` around lines 178-187
- verify that the card layout in `trainer_app.cpp` reflects the imported panel concepts rather than ignoring them

### Technical details

Most relevant imported HTML areas:

- algorithm: lines 134-198
- live drawing and overlay: lines 225-300
- stats and vector bars: lines 302-328
- templates and controls: lines 331-383

## Step 3: Build the new firmware project and port the algorithm

### Prompt Context

Assistant interpretation:

- create a new PaperS3 app rather than modifying the earlier draw-only project
- keep the code maintainable for an intern

Inferred user intent:

- produce a complete, teachable firmware example

### What I did

- created `0076-papers3-protractor-trainer`
- pointed it at donor components through `EXTRA_COMPONENT_DIRS`
- added `sdkconfig.defaults`, `README.md`, `partitions.csv`, and `main/CMakeLists.txt`
- implemented the algorithm in `protractor_math.h` and `protractor_math.cpp`
- created `TrainerApp` to own layout, state, touch routing, rendering, and training actions

### Why

I split math and UI code because the requested documentation was intern-focused. A monolithic file would make the guide much harder to write and much harder to read.

### What worked

- the project structure came together cleanly
- the math port stayed very close to the HTML source
- the UI could explain preprocessing visually without much extra code

### What didn't work

- nothing broke during authoring, but several design choices from the HTML version had to be changed:
  - template names became fixed slots
  - browser buttons became explicit touch targets
  - canvas redraw behavior was adapted to e-paper update modes

### What I learned

- `TrainerApp` benefited from a strict separation between “state mutation” methods and “draw UI” methods
- displaying resampled points, centroid, and angle makes the recognizer feel much less opaque

### What was tricky to build

- coordinating live stroke drawing with e-paper update constraints
- avoiding accidental button activation during touch movement
- choosing a score display that reads sensibly on-device

### What warrants a second pair of eyes

- whether the selected slot and matched slot visual treatments are distinct enough
- whether the metrics card should expose more of the vector contents in a later version

### What should be done in the future

- consider factoring score formatting and slot rendering further if the UI grows

### Code review instructions

- review `protractor_math.cpp` first
- then review `trainer_app.h`
- then review `trainer_app.cpp` starting with `Run()`, `HandleTouch()`, and the save/delete/clear/reset methods

### Technical details

Files added:

- `0076-papers3-protractor-trainer/CMakeLists.txt`
- `0076-papers3-protractor-trainer/sdkconfig.defaults`
- `0076-papers3-protractor-trainer/main/protractor_math.h`
- `0076-papers3-protractor-trainer/main/protractor_math.cpp`
- `0076-papers3-protractor-trainer/main/trainer_app.h`
- `0076-papers3-protractor-trainer/main/trainer_app.cpp`
- `0076-papers3-protractor-trainer/main/app_main.cpp`

## Step 4: Validate the build and write the ticket documents

### Prompt Context

Assistant interpretation:

- validate the real firmware build
- write the plan, guide, and diary against the finished code

Inferred user intent:

- leave behind a ticket that stands on its own

### What I did

- built the project with `ESP-IDF 5.3.4`
- confirmed that `papers3_protractor_trainer.bin` was produced successfully
- began writing the detailed guide, implementation plan, and this diary against actual line references

### Why

The documentation is much more credible when written after a real build rather than before compilation.

### What worked

- the first full build succeeded without source changes after project creation
- donor component reuse and the new module split were both sound

### What didn't work

- no compile failures occurred in this step

### What I learned

- the project wiring was correct on the first compile
- compile logs confirmed the donor GT911 and IT8951-related code paths were part of the build

### What was tricky to build

- the main effort here was gathering accurate references so the guide could explain the donor stack, the algorithm port, and the device runtime together

### What warrants a second pair of eyes

- the long-form docs should be skimmed by someone familiar with the PaperS3 donor stack to confirm nothing important was oversimplified

### What should be done in the future

- perform an actual flash-and-run session on hardware and extend this diary with hardware observations

### Code review instructions

- build the app with:

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0076-papers3-protractor-trainer
source /home/manuel/esp/esp-idf-5.3.4/export.sh
idf.py set-target esp32s3
idf.py build
```

- review the main product flow in:
  - `main/trainer_app.cpp`
  - `main/protractor_math.cpp`

### Technical details

Build result:

- binary: `build/papers3_protractor_trainer.bin`
- size: `0x6b360`
- free space in smallest app partition: approximately `90%`

## Step 5: Validate the ticket and upload to reMarkable

### Prompt Context

Assistant interpretation:

- finish ticket bookkeeping and external delivery

Inferred user intent:

- the completed ticket should be both locally valid and externally accessible on reMarkable

### What I did

- updated related file metadata with `docmgr doc relate`
- ran `docmgr doctor --ticket ESP-32-PAPERS3-PROTRACTOR --stale-after 30`
- uploaded the ticket bundle to reMarkable as `ESP-32 PaperS3 Protractor Trainer`
- verified the remote folder contents with `remarquee cloud ls`

### Why

These are delivery tasks rather than implementation tasks, but they are part of the requested end-to-end outcome.

### What worked

- the ticket already had all required source material and docs in place by this point
- `docmgr doctor` reported all checks passed
- the upload completed successfully
- remote listing confirmed the document exists in the intended folder

### What didn't work

- nothing failed in this step

### What I learned

- the most effective point to run validation is after all long-form docs exist, not before
- uploading the whole ticket directory works well because the bundler automatically picks up the markdown set in lexicographic order

### What was tricky to build

- keeping the diary honest about what had already happened versus what was still pending

### What warrants a second pair of eyes

- only the eventual hardware run still warrants external confirmation

### What should be done in the future

- append a future diary entry after physical hardware testing

### Code review instructions

- confirm that ticket tasks and changelog reflect the final delivery status
- verify that the uploaded document appears at `/ai/2026/03/21/ESP-32-PAPERS3-PROTRACTOR`

### Technical details

Commands used:

```bash
docmgr doc relate --ticket ESP-32-PAPERS3-PROTRACTOR ...
docmgr doctor --ticket ESP-32-PAPERS3-PROTRACTOR --stale-after 30
remarquee upload bundle ttmp/2026/03/21/ESP-32-PAPERS3-PROTRACTOR--papers3-protractor-gesture-trainer-and-recognizer --name "ESP-32 PaperS3 Protractor Trainer" --remote-dir "/ai/2026/03/21/ESP-32-PAPERS3-PROTRACTOR" --toc-depth 2 --non-interactive
remarquee cloud ls /ai/2026/03/21/ESP-32-PAPERS3-PROTRACTOR --long --non-interactive
```

## Related

- `design-doc/01-papers3-protractor-gesture-trainer-analysis-design-and-implementation-guide.md`
- `design-doc/02-papers3-protractor-gesture-trainer-detailed-implementation-plan.md`
- `sources/local/protractor_gesture_recognizer_demo.html`

## Step 6: Prepare the git commit and backfill commit hygiene

### Prompt Context

User prompt (verbatim):

> commit it all, as you work. keep a detailed diary (backfill if needed)

Assistant interpretation:

- commit the PaperS3 work produced in this session and the immediately prior PaperS3 session
- avoid scooping up unrelated dirty files from elsewhere in the repository
- record the commit-packaging decisions in the diary

Inferred user intent:

- get the work safely into git
- preserve a reviewable history
- document what was and was not included

### What I did

- inspected the new project trees and ticket directories before staging
- identified local generated artifacts inside `0075` and `0076`
- added project-local `.gitignore` files for `build/`, generated `sdkconfig`, and `.envrc`
- prepared to stage only:
  - `0075-papers3-touch-draw-demo`
  - `0076-papers3-protractor-trainer`
  - the two PaperS3 ticket bundles under `ttmp/2026/03/21/`

### Why

Both tutorial projects had successful local builds, which meant the directories contained generated files. Committing those would make the history noisier and less portable.

### What worked

- the generated-file audit was quick and clear
- the local `.gitignore` approach kept the source trees clean without touching repo-wide ignore rules

### What didn't work

- nothing failed in this step

### What I learned

- because the projects were still entirely untracked, adding `.gitignore` before staging was the cleanest path

### What was tricky to build

- the only subtlety was honoring “commit it all” without accidentally including unrelated preexisting changes in other repo areas

### What warrants a second pair of eyes

- commit boundaries: whether the user prefers one combined commit or one commit per deliverable

### What should be done in the future

- if more PaperS3 tutorial apps are added, consider establishing a standard local ignore template for them

### Code review instructions

- confirm that no `build/` outputs or local env files are part of the staged changes
- confirm that both project directories still rebuild cleanly from committed sources alone

### Technical details

Generated files intentionally excluded from git:

- `0075-papers3-touch-draw-demo/build/`
- `0075-papers3-touch-draw-demo/sdkconfig`
- `0075-papers3-touch-draw-demo/.envrc`
- `0076-papers3-protractor-trainer/build/`
- `0076-papers3-protractor-trainer/sdkconfig`
- `0076-papers3-protractor-trainer/.envrc`

## Step 7: Record the final git commit

### Prompt Context

Assistant interpretation:

- finish the user-requested “commit it all” workflow and make the diary reflect the real commit outcome

Inferred user intent:

- leave no ambiguity about whether the work was actually committed

### What I did

- committed the staged PaperS3 source and documentation set with:

```bash
git commit -m "feat(papers3): add draw and protractor trainer demos"
```

### Why

The work was no longer only a local implementation artifact. The user explicitly asked for it to be committed.

### What worked

- the commit succeeded on the first try
- the commit included:
  - `0075-papers3-touch-draw-demo`
  - `0076-papers3-protractor-trainer`
  - the `ESP-31` ticket bundle
  - the `ESP-32` ticket bundle

### What didn't work

- nothing failed in this step

### What I learned

- staging only the relevant PaperS3 paths before the commit kept unrelated dirty files out of scope

### What was tricky to build

- the only tricky part was respecting the user’s broad “commit it all” request without including unrelated preexisting workspace noise

### What warrants a second pair of eyes

- only whether a future cleanup should split the large PaperS3 commit into finer historical steps; functionally the content is correct

### What should be done in the future

- after hardware testing, add a follow-up commit with any real-device adjustments

### Code review instructions

- inspect commit `c1e527c`
- verify it contains only the intended PaperS3 changes

### Technical details

Commit created:

- `c1e527c` - `feat(papers3): add draw and protractor trainer demos`
