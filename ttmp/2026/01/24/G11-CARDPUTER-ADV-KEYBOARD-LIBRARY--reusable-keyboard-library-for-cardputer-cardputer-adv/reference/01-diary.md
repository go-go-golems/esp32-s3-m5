---
Title: Diary
Ticket: G11-CARDPUTER-ADV-KEYBOARD-LIBRARY
Status: active
Topics:
    - cardputer
    - keyboard
    - esp-idf
    - esp32s3
    - console
    - usb-serial-jtag
    - ui
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp
      Note: Primary Cardputer-ADV keyboard reference used during research
    - Path: components/cardputer_kb/matrix_scanner.cpp
      Note: Primary Cardputer scanner reference used during research
    - Path: ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/design-doc/01-reusable-keyboard-library-design-analysis.md
      Note: Design analysis produced from this diary
ExternalSources: []
Summary: "Research diary for designing a reusable keyboard library spanning Cardputer and Cardputer-ADV."
LastUpdated: 2026-01-24T15:23:00-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

Keep a frequent, detailed research diary while designing a reusable keyboard library that works on both Cardputer and Cardputer-ADV, and that can deliver decoded keyboard events to multiple application styles (UI loops, LVGL, consoles, scripting runtimes, etc.).

## Step 1: Create Ticket Workspace + Establish Research Targets

Created the `docmgr` ticket and the initial document skeletons so the research and the final design write-up live in a single, searchable workspace. Next, I identified the specific code artifacts the prompt asked to examine (`0066-...` and `components/cardputer_kb`) and began reading them top-down to understand their implicit contracts.

The key decision in this step was to treat “keyboard library” as a layered system: hardware acquisition → normalization → decoding → delivery. That framing matters because the repo already contains code at multiple layers (a reusable “physical” scanner in `cardputer_kb`, and an application-specific decoder/dispatcher inside `0066`).

### Prompt Context

**User prompt (verbatim):**

```text
Create a docmgr ticket G11-CARDPUTER-ADV-KEYBOARD-LIBRARY and examine 0066 but also components/cardputer_kb and create a detailed analysis on how to create a reusable keyboard library for both cardputer and cardputer_adv and a way to deliver keyboard events with keycode decoding to different types of applications.

Write in a detailed textbook writte n by peter norvig style.

Store the analysis in the ticket.

Keep a frequent detailed diary and update it at intermediate steps of you research too.

Upload to remarkable
```

**Assistant interpretation:** Create a new ticket, study the existing Cardputer keyboard component and the Cardputer-ADV keyboard code in tutorial 0066, then write a detailed design analysis (Norvig-style) describing a reusable keyboard library architecture and event delivery model. Maintain a detailed diary during research and upload the finished docs to reMarkable.

**Inferred user intent:** Consolidate keyboard handling across two related devices and expose a clean, reusable API so future apps can “just subscribe” to high-level key events without reimplementing scanning + decoding ad hoc.

**Commit (code):** N/A

### What I did
- Ran `docmgr status --summary-only` to confirm ticket root and vocabulary.
- Created ticket `G11-CARDPUTER-ADV-KEYBOARD-LIBRARY` with relevant topics.
- Added two docs: `reference/01-diary.md` and `design-doc/01-reusable-keyboard-library-design-analysis.md`.
- Identified the two primary research targets:
  - `components/cardputer_kb/*` (existing reusable physical scanner + bindings).
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.*` + `tca8418.*` (ADV keyboard acquisition + app-level decode + dispatch).

### Why
- A good reusable library design needs to start from the concrete code and contracts already in the repo, then generalize carefully without breaking the working patterns.

### What worked
- `docmgr ticket create-ticket` and `docmgr doc add` produced a clean workspace under `ttmp/2026/01/24/...`.

### What didn't work
- N/A

### What I learned
- The repo already distinguishes two concepts: “physical key identity” (stable across apps) vs “semantic meaning” (UX-dependent). That separation is a good foundation for a generalized library.

### What was tricky to build
- N/A (setup-only step).

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Start at `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/reference/01-diary.md:1`.
- No validation commands for this step (docs-only).

### Technical details
- Ticket path: `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/`

## Step 2: Inspect Existing Keyboard Implementations (Cardputer vs Cardputer-ADV)

Read through the reusable `cardputer_kb` component and the Cardputer-ADV keyboard handling inside tutorial `0066`. The essential observation is that both implementations ultimately aim to describe key presses in the same *vendor “picture” coordinate system* (4 rows × 14 columns), but they obtain those presses in fundamentally different ways: GPIO matrix scanning vs an I2C keypad controller (TCA8418) emitting events.

This matters for a reusable library because it suggests a stable intermediate representation: “physical key position/keynum in picture space”, with the board-specific driver responsible for producing it. Decoding (text, modifiers, navigation actions, repeat) can then be shared.

### Prompt Context

**User prompt (verbatim):**

```text
Create a docmgr ticket G11-CARDPUTER-ADV-KEYBOARD-LIBRARY and examine 0066 but also components/cardputer_kb and create a detailed analysis on how to create a reusable keyboard library for both cardputer and cardputer_adv and a way to deliver keyboard events with keycode decoding to different types of applications.

Write in a detailed textbook writte n by peter norvig style.

Store the analysis in the ticket.

Keep a frequent detailed diary and update it at intermediate steps of you research too.

Upload to remarkable
```

**Assistant interpretation:** Extract the implicit contracts and mappings from `cardputer_kb` and `0066` so the design analysis can reuse what already works.

**Inferred user intent:** Avoid reinventing the wheel; repackage existing working approaches into a coherent, reusable library.

**Commit (code):** N/A

### What I did
- Read `components/cardputer_kb/matrix_scanner.cpp` to understand GPIO pins, scan algorithm, and coordinate mapping.
- Read `components/cardputer_kb/include/cardputer_kb/{scanner.h,layout.h,bindings.h}` to capture the public API.
- Read `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp` to understand:
  - TCA8418 init (I2C pins + INT GPIO).
  - Event draining loop (interrupt + poll fallback).
  - Per-key decoding into `ui_key_event_t` (mods, text vs special keys).
  - The remapping from TCA key numbers to the same 4×14 picture coordinates.
- Read `0066-cardputer-adv-ledchain-gfx-sim/main/tca8418.{h,c}` as the minimal register-level driver.
- Skimmed how the UI consumes events (`sim_ui.cpp` queue + `ui_overlay.cpp` behavior).

### Why
- The fastest way to design a reusable library is to treat existing code as “working reference implementations” and generalize only where the implementations differ.

### What worked
- The mapping strategy in both implementations is compatible: everything can be expressed as `(x,y)` in a 4×14 picture space plus a derived `keyNum` (1..56).

### What didn't work
- N/A (analysis only).

### What I learned
- Cardputer GPIO scan uses fixed pin sets (`out={8,9,11}` and `in={13,15,3,4,5,6,7}` with an `{1,2,...}` fallback), and it flips Y to match the “picture”.
- Cardputer-ADV uses a TCA8418 keypad controller on `I2C0 SDA=GPIO8 SCL=GPIO9`, with `INT=GPIO11` and `addr=0x34`, and it remaps the controller’s internal 10-wide key numbers into the same 4×14 picture coordinate system.
- `cardputer_kb` deliberately stops at “physical keys + simple chord decoding” and does not attempt character decoding; `0066` does the opposite (character/modifier decoding into a UI-specific event), suggesting an opportunity to factor out a shared decoder layer.

### What was tricky to build
- The “picture coordinate system” is a subtle but crucial invariant: it is not the electrical matrix and not necessarily the TCA’s row/col either. A reusable library must document this clearly and make it hard to mix coordinate systems by accident.

### What warrants a second pair of eyes
- The Cardputer-ADV remapping math in `decode_tca_event()` is compact and should be sanity-checked against a real device matrix map (risk: off-by-one or swapped axes that “mostly works” but mislabels corners).

### What should be done in the future
- If we decide to ship a unified component, consider moving the TCA8418 code + remap into a reusable driver module rather than leaving it in `0066` as application code.

### Code review instructions
- Start with `components/cardputer_kb/REFERENCE.md:1` (defines intended scope/contract).
- Then read `components/cardputer_kb/matrix_scanner.cpp:1` and `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:1` as the two reference implementations.

### Technical details
- Cardputer physical scan + keyNum model: `components/cardputer_kb/include/cardputer_kb/scanner.h:1`
- Cardputer-ADV acquisition + decode to app events: `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:1`

## Step 3: Draft the Reusable Library Design (Pipeline + Adapters)

Translated the observed code patterns into a design document centered on a “keyboard pipeline”: hardware acquisition → normalization → decoding → delivery. I wrote the proposal so it can be implemented incrementally: first by extracting the Cardputer-ADV TCA8418 backend and wrapping the Cardputer snapshot scanner with a diff-to-edges adapter; then by adding shared decoding and delivery adapters.

The main design “move” is to standardize on **edge events** (press/release) in **picture-space** `(x,y,keyNum)` for *both* devices. Once we do that, everything else becomes shared pure logic: maintain key state, compute modifiers, produce text/keycodes/actions, then route those events to queues/event-loops/LVGL/etc.

### Prompt Context

**User prompt (verbatim):**

```text
Create a docmgr ticket G11-CARDPUTER-ADV-KEYBOARD-LIBRARY and examine 0066 but also components/cardputer_kb and create a detailed analysis on how to create a reusable keyboard library for both cardputer and cardputer_adv and a way to deliver keyboard events with keycode decoding to different types of applications.

Write in a detailed textbook writte n by peter norvig style.

Store the analysis in the ticket.

Keep a frequent detailed diary and update it at intermediate steps of you research too.

Upload to remarkable
```

**Assistant interpretation:** Turn the research findings into a concrete, implementable architecture with explicit contracts and examples, written as a detailed design analysis.

**Inferred user intent:** Establish a durable reference that future keyboard-related work can follow, reducing duplication and subtle mismatches across firmwares.

**Commit (code):** N/A

### What I did
- Wrote and expanded the design doc: `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/design-doc/01-reusable-keyboard-library-design-analysis.md`.
- Centered the design around:
  - canonical representation: picture-space `(x,y)` + derived `keyNum` (1..56),
  - a unified raw edge stream (`down/up`) even for snapshot devices,
  - separate decoders (raw/keycode/text/action) over shared state,
  - delivery adapters for multiple application types.

### Why
- A reusable library is less about “more code” and more about stable interfaces: you want to be able to write new applications without being forced to learn each board’s electrical quirks.

### What worked
- Both reference implementations already suggest the same unifying coordinate system, so the design can be “extracted” rather than invented.

### What didn't work
- N/A

### What I learned
- The best reuse boundary is between the physical layer and decoding/delivery: the hardware differs, but almost everything above it can be shared if the representation is stable.

### What was tricky to build
- Documenting the invariants without overwhelming the reader: the design doc must make it clear why picture-space is chosen and why edges are the right abstraction, without turning into a hardware manual.

### What warrants a second pair of eyes
- Whether the proposed keymap/modifier approach matches the actual Cardputer-ADV “Opt as Fn” convention across other demos (not just `0066`).

### What should be done in the future
- If this becomes an implementation effort, add at least a small mapping test suite (host-based) for the TCA remap and snapshot-diff logic.

### Code review instructions
- Read `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/design-doc/01-reusable-keyboard-library-design-analysis.md:1`.
- Cross-check cited behaviors against:
  - `components/cardputer_kb/matrix_scanner.cpp:1`
  - `0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp:1`

### Technical details
- Proposed canonical key identity: `keyNum = y*14 + (x+1)` with `x∈[0,13]`, `y∈[0,3]`.

## Step 4: Relate Source Files + Validate Docs

Linked the key code artifacts that informed the design into the ticket documents using `docmgr doc relate`, then validated frontmatter so the ticket stays tidy and searchable. I also updated the ticket `index.md` and `tasks.md` to point at the produced design doc and diary.

This step is “boring but important”: without file relations, the design doc can drift away from the code it was based on, and reviewers lose the ability to quickly cross-check claims.

### Prompt Context

**User prompt (verbatim):**

```text
Create a docmgr ticket G11-CARDPUTER-ADV-KEYBOARD-LIBRARY and examine 0066 but also components/cardputer_kb and create a detailed analysis on how to create a reusable keyboard library for both cardputer and cardputer_adv and a way to deliver keyboard events with keycode decoding to different types of applications.

Write in a detailed textbook writte n by peter norvig style.

Store the analysis in the ticket.

Keep a frequent detailed diary and update it at intermediate steps of you research too.

Upload to remarkable
```

**Assistant interpretation:** Ensure the produced docs are properly linked to relevant code and pass docmgr validation, so the ticket is reviewable and maintainable.

**Inferred user intent:** Make the ticket useful as a long-lived reference, not a one-off essay.

**Commit (code):** N/A

### What I did
- Added `RelatedFiles` to the design doc and diary via `docmgr doc relate` (with absolute paths).
- Updated ticket docs:
  - `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/index.md`
  - `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/tasks.md`
- Validated frontmatter for the index, design doc, and diary via `docmgr validate frontmatter`.

### Why
- File relations + validated frontmatter are the difference between “some notes” and “a maintainable ticket artifact”.

### What worked
- `docmgr doc relate` cleanly captured the key source files used for the design.
- `docmgr validate frontmatter` confirmed the ticket docs are well-formed.

### What didn't work
- Initially ran `docmgr validate frontmatter --doc ttmp/...` and got:
  - `Error: open .../ttmp/ttmp/...: no such file or directory`
  - Fix: pass paths relative to the docmgr root (e.g., `2026/01/24/...`) rather than prefixing `ttmp/`.

### What I learned
- `docmgr` resolves `--doc` under the configured `Docs root`, so supplying `ttmp/...` duplicates the root prefix.

### What was tricky to build
- Avoiding over-linking: per docmgr conventions, keep `RelatedFiles` to a small set of “load-bearing” sources rather than dumping every file touched by a repo-wide search.

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- Start with `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/index.md:1`.
- Then read `ttmp/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/design-doc/01-reusable-keyboard-library-design-analysis.md:1`.
- Validate docs: `docmgr validate frontmatter --doc 2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY--reusable-keyboard-library-for-cardputer-cardputer-adv/index.md`

### Technical details
- Design doc related sources include:
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/matrix_scanner.cpp`
  - `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim/main/ui_kb.cpp`

## Step 5: Upload Ticket Docs to reMarkable

Bundled the ticket’s index, design analysis, and diary into a single PDF with a table of contents and uploaded it to reMarkable under a stable ticket folder. This makes the long-form “Norvig-style” analysis readable on-device and keeps it associated with the ticket ID and date.

### Prompt Context

**User prompt (verbatim):**

```text
Create a docmgr ticket G11-CARDPUTER-ADV-KEYBOARD-LIBRARY and examine 0066 but also components/cardputer_kb and create a detailed analysis on how to create a reusable keyboard library for both cardputer and cardputer_adv and a way to deliver keyboard events with keycode decoding to different types of applications.

Write in a detailed textbook writte n by peter norvig style.

Store the analysis in the ticket.

Keep a frequent detailed diary and update it at intermediate steps of you research too.

Upload to remarkable
```

**Assistant interpretation:** Export the ticket docs in a reMarkable-friendly form (PDF) and upload them.

**Inferred user intent:** Read and review the analysis away from the workstation, and keep it organized by ticket/date.

**Commit (code):** N/A

### What I did
- Confirmed uploader tooling is available: `remarquee status`.
- Ran a dry-run:
  - `remarquee upload bundle --dry-run ... --remote-dir "/ai/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY"`
- Uploaded the bundle:
  - `remarquee upload bundle ... --name "G11 Cardputer+ADV Keyboard Library" --toc-depth 2`
- Verified the remote folder:
  - `remarquee cloud ls /ai/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY --long --non-interactive`

### Why
- Bundling avoids “document scatter” on reMarkable and gives a single PDF with a ToC for fast navigation.

### What worked
- Upload succeeded and the document appeared in `/ai/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY`.

### What didn't work
- First upload attempt failed with pandoc/LaTeX error:
  - `! Undefined control sequence. l.1121 of applications.\\n`
  - Cause: literal `\\n` sequences in the diary prompt string were interpreted as LaTeX commands.
  - Fix: replaced the quoted prompt strings with fenced `text` blocks containing real newlines.
- Second attempt timed out in the runner (default 10s), so I re-ran with a longer timeout to allow pandoc to render the PDF.

### What I learned
- When targeting PDF output, treat backslash sequences in prose as hostile unless they’re inside code blocks (or properly escaped).

### What was tricky to build
- Getting the “verbatim prompt” requirement to coexist with PDF tooling: the safest compromise is to store verbatim text in fenced blocks, not in quoted inline strings.

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- If this becomes a repeated workflow, consider a small lint rule or script that flags suspicious LaTeX-unfriendly sequences (like bare `\\n`) in ticket docs before uploading.

### Code review instructions
- Open the uploaded PDF: `G11 Cardputer+ADV Keyboard Library` under `/ai/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY`.

### Technical details
- Remote path: `/ai/2026/01/24/G11-CARDPUTER-ADV-KEYBOARD-LIBRARY`
