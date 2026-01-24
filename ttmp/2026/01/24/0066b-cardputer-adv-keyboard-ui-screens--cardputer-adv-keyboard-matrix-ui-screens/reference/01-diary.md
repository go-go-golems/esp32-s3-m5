---
Title: Diary
Ticket: 0066b-cardputer-adv-keyboard-ui-screens
Status: active
Topics:
    - cardputer
    - keyboard
    - ui
    - m5gfx
    - led-sim
    - console
    - web-ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-24T00:46:15.329644186-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Capture (with high fidelity) the design + implementation journey for adding a Cardputer-ADV keyboard-driven UI: how we wire the keyboard matrix scan, define input semantics (modifiers, chords), and implement each on-device screen.

## Context

We are implementing a keyboard-driven on-device UI for Cardputer-ADV-style firmwares in this repo. Two “spec” documents were provided as local files and imported into this ticket workspace so the rest of the work can reference stable paths.

Likely target firmware for implementation: `0066-cardputer-adv-ledchain-gfx-sim` (already has always-on LED preview and M5GFX rendering).

## Quick Reference

N/A (this diary is the narrative record; design docs contain the durable specs).

## Usage Examples

N/A

## Related

- `../analysis/01-cardputer-adv-keyboard-matrix-setup.md`
- `../design/01-ui-screen-designs-per-screen.md`

---

## Step 1: Create ticket, import UI specs, and pick the target firmware (2026-01-24)

This step establishes the workspace and the “ground truth” inputs. Two markdown documents (`/tmp/cardputer-ui.md` and `/tmp/cardputer-webui.md`) are treated as external sources and imported into the ticket so future work can cite stable paths. I also identified the most likely code target for implementation: tutorial `0066-cardputer-adv-ledchain-gfx-sim`, which already has the always-on LED preview screen and multiple control planes (console + web + JS).

The next step is to translate those specs into a keyboard matrix setup doc and a per-screen implementation design, then implement incrementally with small commits and docmgr task checkoffs.

### Prompt Context

**User prompt (verbatim):** "Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ... \n\nImport /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary."

**Assistant interpretation:** Create ticket 0066b, import two external markdown specs, author detailed keyboard-matrix and screen-design docs, then implement the UI in the firmware with tasks, frequent diary updates, and incremental commits.

**Inferred user intent:** Move from “UI mock text” to real firmware: keyboard-driven overlays for the simulator on Cardputer-ADV, with a clear design and an auditable implementation trail.

**Commit (code):** N/A (workspace/bootstrap step; code changes will come after design docs and tasks are created).

### What I did
- Created docmgr ticket `0066b-cardputer-adv-keyboard-ui-screens`.
- Created docs:
  - `analysis/01-cardputer-adv-keyboard-matrix-setup.md`
  - `design/01-ui-screen-designs-per-screen.md`
  - `reference/01-diary.md`
- Imported external specs into the ticket sources using the requested command form:
  - `docmgr import file --ticket 0066b-cardputer-adv-keyboard-ui-screens --file /tmp/cardputer-ui.md --name "Cardputer UI mock"`
  - `docmgr import file --ticket 0066b-cardputer-adv-keyboard-ui-screens --file /tmp/cardputer-webui.md --name "Cardputer Web UI mock"`
- Located the existing keyboard matrix component we should reuse:
  - `components/cardputer_kb` (scanner + bindings + docs)

### Why
- Importing `/tmp` documents into `ttmp/.../sources/` makes them stable, reviewable inputs for the rest of the work.
- Reusing `components/cardputer_kb` avoids re-deriving the vendor matrix scan wiring and Fn-chord conventions.

### What worked
- `docmgr import file` correctly copied the markdown sources into the ticket workspace and updated source metadata.

### What didn't work
- N/A

### What I learned
- This repo already has an explicit, reusable keyboard scan component plus “how to scan” documentation; the work is more about integrating it cleanly into the target UI loop.

### What was tricky to build
- N/A (no code yet).

### What warrants a second pair of eyes
- Confirming the target firmware/screen set: I’m assuming we implement the keyboard UI into `0066-cardputer-adv-ledchain-gfx-sim` because it already has the always-on preview. If you intended a different tutorial as the target, tell me now before we implement.

### What should be done in the future
- Author the keyboard matrix setup doc with concrete pin mappings and initialization.
- Author per-screen designs (menu/pattern/params/color sliders/JS/help) with state machines and input handling.
- Turn those designs into an implementation task list and start coding.

### Code review instructions
- Review the imported sources:
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/sources/local/Cardputer UI mock.md`
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/sources/local/Cardputer Web UI mock.md`
- Review the keyboard component docs:
  - `components/cardputer_kb/README.md`
  - `components/cardputer_kb/REFERENCE.md`
  - `components/cardputer_kb/TUTORIAL.md`

### Technical details
- The keyboard scan API is in `components/cardputer_kb/include/cardputer_kb/scanner.h` (`MatrixScanner::init()` and `MatrixScanner::scan()`).

---

## Step 2: Write keyboard + per-screen design docs (commit 99b46ae) (2026-01-24)

This step turns the imported UI mock text into two implementable documents: a keyboard matrix setup analysis (pins, scan semantics, event generation) and a per-screen UI design spec (mode machine, input conventions, and screen-by-screen behaviors). The aim is to make the implementation phase mostly mechanical: each screen becomes a state + renderer + input handler with clear transitions and side-effects.

I also related the design docs to the specific code that anchors them (the `cardputer_kb` scanner and the existing 0066 preview UI) so future readers can jump directly from prose to source.

### Prompt Context

**User prompt (verbatim):** "Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ... \n\nImport /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary."

**Assistant interpretation:** Produce detailed keyboard-matrix and per-screen design docs grounded in existing repo components, then implement.

**Inferred user intent:** Reduce ambiguity and iteration cost by making the UI and keyboard semantics explicit before coding.

**Commit (code):** 99b46ae — "0066b: add keyboard matrix + per-screen UI designs"

### What I did
- Wrote:
  - `analysis/01-cardputer-adv-keyboard-matrix-setup.md`
  - `design/01-ui-screen-designs-per-screen.md`
- Linked those docs to key reference files via `docmgr doc relate`:
  - `components/cardputer_kb/*`
  - `0022-cardputer-m5gfx-demo-suite/main/input_keyboard.cpp`
  - `0030-cardputer-console-eventbus/main/app_main.cpp`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`

### Why
- The keyboard matrix layer is easy to “kind of implement” but hard to get consistently right without an explicit contract (pins, active-low, chords, repeat).
- A per-screen spec prevents the classic embedded UI failure mode: adding features ad-hoc until behavior becomes inconsistent and untestable.

### What worked
- The repo already has stable, reusable keyboard infrastructure (`cardputer_kb`) so the docs could be grounded in real code rather than guesswork.

### What didn't work
- N/A

### What I learned
- The captured binding table (`bindings_m5cardputer_captured.h`) is the cleanest way to standardize Fn navigation without hardcoding keyNums in every firmware.

### What was tricky to build
- Balancing completeness vs MVP: the UI mock includes many screens; the design doc explicitly defines MVP subsets (e.g., JS Lab examples-only) so implementation can be incremental.

### What warrants a second pair of eyes
- Confirm whether the “Cardputer-ADV” keyboard wiring differs from the baseline Cardputer. The scanner supports alt IN0/IN1 autodetect, but if ADV changes more than that we should verify with the calibrator.

### What should be done in the future
- Implement the keyboard task + UI state machine in tutorial 0066.

### Code review instructions
- Start with:
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/analysis/01-cardputer-adv-keyboard-matrix-setup.md`
  - `ttmp/2026/01/24/0066b-cardputer-adv-keyboard-ui-screens--cardputer-adv-keyboard-matrix-ui-screens/design/01-ui-screen-designs-per-screen.md`

### Technical details
- N/A (documentation step).

---

## Step 3: Implement keyboard scan task + UI event queue in 0066 (commit 99002e2) (2026-01-24)

This step makes keyboard input real in the target firmware. The key architectural choice is to isolate matrix scanning into its own FreeRTOS task that pushes compact key events into a bounded queue. The UI task consumes those events and (for now) simply displays the last key pressed; in the next step, the UI state machine will turn those events into overlay navigation and parameter edits.

This mirrors the repo’s proven pattern from `0030` (keyboard task + event posting), but uses a simpler queue instead of `esp_event`, since tutorial 0066 currently doesn’t use an event loop.

### Prompt Context

**User prompt (verbatim):** "Create a new ticket 0066b-..., use docmgr import file --file ... --ticket ... \n\nImport /tmp/cardputer-ui.md and /tmp/cardputer-webui.md and then create a detailed document on how to set up the keyboard matrix for the cardputer adv, and then a set of detailed implementation design for each screen separately. create tasks and add to ticket, and then implement it, committing and checking off tasks, and keeping a detailed diary."

**Assistant interpretation:** Start implementing the on-device keyboard UI in tutorial 0066, using the repo’s keyboard scan component.

**Inferred user intent:** Get to a working keyboard-driven UI quickly, without destabilizing the existing simulator.

**Commit (code):** 99002e2 — "0066: add cardputer keyboard scan task + event queue"

### What I did
- Added `cardputer_kb` to tutorial 0066 build:
  - `0066-cardputer-adv-ledchain-gfx-sim/CMakeLists.txt` (`EXTRA_COMPONENT_DIRS`)
  - `0066-cardputer-adv-ledchain-gfx-sim/main/CMakeLists.txt` (`REQUIRES cardputer_kb`)
- Added a compact key-event layer and scanner task:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.h`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
- Added a Kconfig knob for scan period:
  - `CONFIG_TUTORIAL_0066_KB_SCAN_PERIOD_MS` in `0066-cardputer-adv-ledchain-gfx-sim/main/Kconfig.projbuild`
- Integrated into the existing UI loop:
  - `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp` creates a queue, starts the keyboard task, and displays the last key for quick smoke-testing.

### Why
- Keeping scanning in a dedicated task decouples input responsiveness from rendering cadence and makes it easy to add repeat/debounce later.
- A fixed-size `ui_key_event_t` avoids dynamic allocation in the message path.

### What worked
- `idf.py -C 0066-cardputer-adv-ledchain-gfx-sim build` succeeds with the new component.

### What didn't work
- N/A

### What I learned
- Reusing the same key legend table as `0022` keeps key strings consistent across projects (e.g., `"tab"`, `"enter"`, `"bksp"`).

### What was tricky to build
- Avoiding accidental coupling to “ASCII”: the UI needs both semantic nav actions and raw key strings; representing those cleanly in a small struct took care.

### What warrants a second pair of eyes
- Whether we should add best-effort drop counters/logs when the UI queue is full. (Currently it silently drops events if the queue is full.)

### What should be done in the future
- Implement the overlay state machine and connect `tab/p/e/...` shortcuts and arrow navigation to actual UI modes.

### Code review instructions
- `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`
- `0066-cardputer-adv-ledchain-gfx-sim/main/sim_ui.cpp`

### Technical details
- The keyboard task emits:
  - `UI_KEY_*` events for semantic actions (from the captured Fn binding table)
  - `UI_KEY_TEXT` for raw key edges (excluding modifiers and keys used in the active chord)
