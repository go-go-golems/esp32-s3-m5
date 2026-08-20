---
Title: Investigation diary
Ticket: ESP-56-PULP-DESIGN-SYSTEM
Status: active
Topics:
    - papers3
    - esp32s3
    - microquickjs
    - eink
    - ui
    - design-system
    - javascript
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-08-20T09:59:46.615996237-04:00
WhatFor: ""
WhenToUse: ""
---

# Investigation diary — ESP-56 design-system pass

## Goal

Chronological record of the screenshot-driven design pass: the capture
tooling, the visual audit, the root-cause of the broken margins, the
design tokens/idioms, and the refactor.

## Step 1: Eyes on the device — the shot pipeline

The pass began by making screens visible: without seeing the panel, a
design audit is guesswork. The repo's `screenshot_qoi` component (QOI
framebuffer streaming over USB serial) was wired into 0114 as a console
op, and a cdc_acm-safe capture client decodes the stream to PNG.

### Prompt Context

**User prompt (verbatim):** "often I have text that doesn't match the overall design, the margin is not respected, and such. do a pass over the UI design system API to make it look good, like the front page landing app browser looks good. Create a new ticket for that." (follow-up: "See how the settings / apps and such doesn't seem to respect the margin. Also, the sans serif font should not be used for main functionality like buttons and such. Or maybe you remove the margin righ not but the app title still seems to have a margin? imagine you're a swiss typography designer when assessing the images")

**Assistant interpretation:** New ticket; audit every screen visually against the launcher's design; fix the margin violations and the control typography; codify a design system in the facade.

**Inferred user intent:** One coherent visual system, enforced by API rather than per-app discipline.

**Commit (code):** 3844a06d — "ESP-56 P0: shot console op …"

### What I did
- `ConsoleOp::Shot` + `shot` command → owner-side `ShotToConsole()`
  (`main/app_shot.cpp`, capture on the owner so it never races a present).
- Wired `components/screenshot_qoi` into 0114; its `REQUIRES M5Unified`
  hardcoded the Arduino-style name, while 0114 uses the managed component
  `m5stack__m5unified` — made the name parameterizable via an ENVIRONMENT
  variable (plain and cache variables do not reach IDF's
  requirements-expansion pass, which runs in a separate cmake process;
  two failed attempts proved that).
- `scripts/01-pulp-shot.py`: hold-open port client + pure-python QOI
  decode + PIL PNG; drives taps before the shot.
- Gotcha: after `source export.sh`, `python3` is the IDF venv (no PIL) —
  the client must run under `/usr/bin/python3`.

### What worked
- First capture of the launcher: 540×960 PNG, ~46 KB QOI, pixel-perfect.

## Step 2: The audit, and the getter that ate the margins

Sixteen baseline captures (sources/baseline/) made the pattern obvious
and matched the user's report exactly: headers at 40 px, app content
flush to x=0 (Daily Pulp even clips glyphs). Root cause, host-verified:
mquickjs parses `get M() { return M; }` in an object literal WITHOUT
accessor semantics — `os.M` was the function object, so every
`pad(0, os.M, …)` coerced to 0. The launcher looked right because the
kernel uses the global `M` directly. Full findings + the design system in
`design-doc/01`.

**Commit (docs):** d5cb1f29 — baseline + audit.

### What was tricky
- pulpjsc accepts the getter syntax, so nothing failed at build; the
  defect was invisible until the screens were.

## Step 3: Tokens, idioms, refactor — and a chip that never had a label

The facade now owns the system: `os.M` is a plain number with
`os.setMargin`; `os.body/menuRow/button/buttonRow/keyboard` are the
idioms; launcher and settings share `menuRow`; four hand-rolled keyboards
became one; dice/tea/2048/daily buttons moved from display-sans shouts
and bracketed-serif whispers to one serif-md control language with the
primary action as an inverted chip.

Re-capture then exposed a pre-existing renderer bug: inverted chips
(SEAL, JOIN, GET) drew as blank black slabs — in the baselines too. The
m5 backend's `BlitCoverage` composited glyph coverage "over an assumed
white background" and skipped 255-valued runs as untouched background;
white ink (255) therefore computed to 255 everywhere and every glyph
pixel was skipped. Fixed by compositing light ink (≥128) over an assumed
black background with a 0-valued skip sentinel
(`components/s3paper_m5/src/m5_backend.cpp`). SEAL now renders
white-on-black for what is likely the first time on hardware.

### What I did
- Facade rewrite + app refactors (dice, 2048, tea, daily, postcard,
  settings ×5 screens, browser); host-parse lint over every JS file
  (only 90-boot "fails" — it executes serve.get against host stubs,
  expected).
- Seeded SD copies shadow ROM, so the five changed apps were pushed over
  HTTP (the ESP-55 dev loop as intended) before re-capture.
- After-captures in sources/after/: margins restored everywhere, one
  control language, uniform keyboards, labeled primary chips.

### What warrants a second pair of eyes
- `BlitCoverage`'s new light-ink path (mid-gray inks 128..254 now assume
  a black background — correct for inverted chips, would mislight a
  hypothetical light-gray-on-white label; none exists today).
- The keyboard idiom's key sizing (48×56 + del 70) against fat-finger
  reality.

### What should be done in the future
- Extend `ui` page helpers with the same button idiom; re-run the ESP-55
  soak; capture the remaining after-screens (2048, blitz, radio, browser)
  for the record.

### Code review instructions
- Compare `sources/baseline/` vs `sources/after/` side by side; then
  `git show` the three commits; on device: `shot` + any screen.
