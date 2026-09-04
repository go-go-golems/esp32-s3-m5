---
Title: 'PULP design system: tokens, idioms, and the per-screen visual audit'
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
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Screenshot-driven visual audit of every PULP OS screen, the design tokens and idioms that fix the findings (margins, control typography, shared keyboard/button/row builders), and the root-cause of the broken margins (mquickjs has no object-literal getter semantics; os.M was a function object).
LastUpdated: 2026-08-20T09:59:45.952081398-04:00
WhatFor: ""
WhenToUse: ""
---

# PULP design system: tokens, idioms, and the per-screen visual audit

## 1. The instrument

Screens are now capturable: console op `shot` (ESP-56 P0) streams the
M5GFX framebuffer as QOI over USB serial; `scripts/01-pulp-shot.py` holds
the port, optionally drives taps, and saves a PNG. Baseline captures of
all 16 screens are in `sources/baseline/`.

## 2. Root cause: os.M was a function

`tools/js/os/10-facade.js` declared `var os = { get M() { return M; } }`.
mquickjs parses this **without getter semantics**: `os.M` is a plain
property whose value is the function object (host-verified:
`typeof os.M === 'function'`). Every `pad(0, os.M, 0, os.M)` therefore
coerced to `pad(0, 0, 0, 0)` — all app content lost its margins, while
headers (built by the kernel's `chrome()` from the global `M`) kept them.
This asymmetry is visible on every baseline capture: titles at 40 px,
content flush to x=0, Daily Pulp clipping glyphs at the panel edge.
pulpjsc accepted the syntax, so the defect shipped silently — the dialect
fact sheet's "getters yes" does not cover object-literal accessors.

## 3. Audit findings (Swiss-typography lens)

| # | Finding | Screens | Severity |
|---|---|---|---|
| 1 | Content margin = 0 (os.M bug); headers keep 40 px | library, reader, dice, blitz, tea, daily, gallery(empty), radio, settings (all screens), apps list | breaks the grid |
| 2 | Two unrelated button languages: display-sans shouts (dice `2d6 d20 coin d%`) vs bracketed 12pt-serif whispers (`[ +30s ]`, `[ new game ]`, `[ reveal ]`) | dice, tea, 2048, daily | no control identity |
| 3 | Four hand-rolled keyboards with drifting metrics (48 vs 52 px keys, different pads) | postcard, settings-pass, settings-url, browser | duplication + drift |
| 4 | Menu-row idiom duplicated 3× (launcher entryRow, settings setRow, apps rows) | home, settings | drift risk |
| 5 | Full-bleed hairlines only sometimes intentional (blitz zone rules yes; keyboard rules no) | blitz, keyboards | ambiguity |
| 6 | What is RIGHT (the reference): launcher pairing — grotesque display for identity, serif for metadata, right-aligned sublabels, title+thick-rule chrome, inverted serif chip (SEAL) as primary action | home, postcard SEAL, web-install QR | keep |

## 4. The system

Faces (s3paper text.h): `xs/sm` = serif UI, `md` = serif body,
`lg` = grotesque display, `xl` = grotesque hero, `title` = serif display.

Roles:
- **Identity** (grotesque `lg`/`xl`): screen titles, launcher app names,
  hero numerals (clocks, ROLL, score). Never controls.
- **Text** (serif `xs/sm/md/title`): body, subtitles, metadata, hints,
  book titles.
- **Controls** (serif `md`, no brackets): every tappable action.
  Primary action = inverted chip. Fat targets (min 100×56).

Tokens and idioms (facade; mirrored in the page `ui` helpers):
- `os.M` — a **number** (facade field kept in sync by `os.setMargin`,
  which also updates the kernel global and persists).
- `os.body(padTop)` — the content column: `col().pad(padTop|16, M, 0, M)`.
- `os.menuRow(menu, label, sub, fn)` — THE row (launcher/settings/apps
  share one implementation).
- `os.button(label, fn, {w, primary, size})` — serif md, centered,
  height 56; primary inverts.
- `os.buttonRow()` — `row().pad(10,0,0,0).gap(16).mainAlign(1)`.
- `os.keyboard(body, rows, onKey, {del})` — one keyboard: 48×56 keys,
  hairline row rules inside the 24 px keyboard gutter.

## 5. Verification

Re-capture every baseline screen after the refactor; the gate is visual
(margins at 40 px, one control language) plus fingerprints (row counts
unchanged) and zero exceptions. Seeded SD copies override ROM, so the
gate pushes the changed apps over HTTP (the P5 loop) or reseeds.
