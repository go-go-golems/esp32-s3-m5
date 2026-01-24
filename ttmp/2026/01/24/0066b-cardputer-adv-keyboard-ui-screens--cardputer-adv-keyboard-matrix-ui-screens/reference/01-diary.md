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
