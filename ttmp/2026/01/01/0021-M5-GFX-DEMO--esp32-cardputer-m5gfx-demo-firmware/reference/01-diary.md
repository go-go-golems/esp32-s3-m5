---
Title: Diary
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - ui
    - keyboard
    - esp32s3
    - esp-idf
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../.local/bin/remarkable_upload.py
      Note: Uploader used to publish docs as PDFs to reMarkable
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: Autodetect details referenced by diary
    - Path: 0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: Existing tutorial used as baseline for DMA-safe present
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/analysis/01-m5gfx-functionality-inventory-demo-plan.md
      Note: Analysis doc this diary supports
    - Path: ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/01-demo-firmware-real-world-scenario-catalog.md
      Note: Design doc created in Step 5
ExternalSources: []
Summary: Research diary logging the step-by-step investigation of M5GFX/Cardputer capabilities and demo design.
LastUpdated: 2026-01-01T19:46:16.322345172-05:00
WhatFor: ""
WhenToUse: ""
---






# Diary

## Goal

Capture the research and decision trail while designing a **Cardputer M5GFX demo-suite firmware**, including where the relevant APIs live, what example code already exists in this repo, and what concrete demos should be implemented to cover the library.

## Step 1: Ticket bootstrap + locating the relevant code

This step established the documentation workspace and identified the concrete code locations we need to read to build a “full coverage” graphics demo for the Cardputer. The main outcome is that M5GFX is already vendored (via `M5Cardputer-UserDemo/components/M5GFX`) and used by several existing ESP-IDF tutorials in `esp32-s3-m5/`, so we can base the demo-suite’s bring-up and rendering patterns on known-good code.

### What I did
- Read local workflows in `~/.cursor/commands/docmgr.md` and `~/.cursor/commands/diary.md`.
- Created ticket `0021-M5-GFX-DEMO` and added:
  - `analysis/01-m5gfx-functionality-inventory-demo-plan.md`
  - `reference/01-diary.md`
- Located existing M5GFX usage in this repo:
  - `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
  - `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp`
  - `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`
  - `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`
- Located the vendored library sources:
  - `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.h`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Button.hpp`

### Why
- We need a demo-suite that is grounded in the exact version of M5GFX that this repo vendors and builds against (ESP-IDF component).
- Existing tutorials already contain “known good” patterns like `M5Canvas` full-screen buffers and `waitDMA()` to avoid flicker; reusing these reduces risk.

### What worked
- `docmgr status --summary-only` confirmed `esp32-s3-m5/ttmp` is a working docmgr root.
- `docmgr ticket create-ticket ...` created the ticket workspace under `esp32-s3-m5/ttmp/2026/01/01/...`.
- A quick `rg` showed M5GFX is used across multiple tutorials and that M5GFX is pulled from `M5Cardputer-UserDemo/components/M5GFX`.

### What didn't work
- N/A (no blockers yet).

### What I learned
- The Cardputer display bring-up is intended to be “just” `m5gfx::M5GFX display; display.init();` and relies heavily on `M5GFX::autodetect()` (board-specific pins, offsets, rotation, backlight PWM).
- “Widgets” in this library are primarily low-level (e.g., `LGFX_Button`) rather than a full retained-mode UI toolkit; the demo firmware should treat them as building blocks.

### What was tricky to build
- Separating “what M5GFX provides” from “what this repo’s demos provide”: a lot of UI convenience lives in `M5Cardputer-UserDemo`’s HAL and apps, while M5GFX itself is the rendering/IO layer.

### What warrants a second pair of eyes
- Demo scope: “exhaustive” can balloon; we should validate that the proposed demo suite covers the API surface in a way that’s still shippable on-device.

### What should be done in the future
- Once the demo-suite is implemented, lock in a small set of “canonical” demo code paths and reference them from future tickets (avoid copy/paste drift).

### Code review instructions
- Start with `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/analysis/01-m5gfx-functionality-inventory-demo-plan.md` and confirm:
  - the feature inventory matches the vendored M5GFX headers, and
  - the proposed demos actually map to those APIs.

### Technical details
- Primary search commands used:
  - `rg -n "\\bM5GFX\\b|\\blgfx\\b|LovyanGFX" esp32-s3-m5 -S`
  - `rg -n "M5GFX|M5Canvas|LGFX_Sprite|LGFX_Button" M5Cardputer-UserDemo/main -S`

## Step 2: Cardputer autodetect + display bring-up details

This step focused on the specific Cardputer wiring assumptions baked into the vendored M5GFX code. The main outcome is a concrete, code-anchored understanding of why `display.init()` “just works” on Cardputer: autodetect configures an ST7789 panel, the correct visible-window offsets, and the backlight PWM setup.

### What I did
- Read the Cardputer branch in `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp` (within `M5GFX::autodetect`).
- Cross-checked this with the bring-up pattern used in `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`.

### Why
- All demos in the suite need a single canonical “bring-up” path that is reliable on real hardware.
- Offsets and rotation matter for UI layout; if we get them wrong, all demos become confusing (objects “draw off-screen” or appear shifted).

### What worked
- The autodetect path clearly sets:
  - panel type: ST7789
  - panel size and visible offsets (Cardputer’s 240×135 visible region)
  - rotation to `1` for Cardputer
  - backlight PWM pin/channel/frequency/offset

### What didn't work
- N/A.

### What I learned
- The Cardputer’s visible window offset is the key difference from “naive ST7789 bring-up” and is the reason we should not hand-roll panel init for the demo suite unless absolutely necessary.
- `display.waitDMA()` is a practical requirement for full-screen sprite animation on this device; the existing tutorial uses it explicitly to avoid flutter/tearing.

### What was tricky to build
- Turning the raw autodetect code into “human-readable, actionable” facts (pins/offsets/rotation/backlight) that can be used to reason about demos.

### What warrants a second pair of eyes
- Confirm that the chosen “canonical” init path for the demo suite should always be `display.init()` (autodetect), rather than `display.init(panel)` with an explicitly configured `Panel_ST7789`.

### What should be done in the future
- If we add a demo that relies on `readRect`/readback or bus sharing, validate it against the autodetect config (e.g., ensure `cfg.readable = true` is set for Cardputer).

### Code review instructions
- Start in `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`, search for `board_M5Cardputer`, and verify the documented offsets/rotation/backlight configuration matches the code.

### Technical details
- Commands used:
  - `rg -n "Cardputer" M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`
  - `sed -n '1370,1700p' M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`

## Step 3: Inventory the M5GFX / LGFX API surface (and map it to demos)

This step moved from “bring-up” to “what can we actually draw and how?”. The output is the bulk of `analysis/01-m5gfx-functionality-inventory-demo-plan.md`: a categorized inventory of the LGFXBase drawing API, sprite/canvas APIs, image decode support, QR code support, clipping/scrolling, and the small widget surface (`LGFX_Button`). The second output is a coverage-driven list of demos that should exercise each of those capabilities in a way that’s teachable on-device.

### What I did
- Read and excerpted the main public headers:
  - `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.h`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Button.hpp`
- Skimmed key implementation files to confirm behavior and “gotchas”:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp` (PNG/QOI decode behavior, `createPng`, QR code rendering)
- Wrote a demo list and a coverage matrix tying demos to capabilities.

### Why
- A graphics demo suite only succeeds if it is organized by “what a user wants to learn”. The library is organized by implementation, so we need a mapping layer: capabilities → demos.
- Without a coverage matrix, “exhaustive” becomes subjective; the matrix makes it auditable.

### What worked
- The M5GFX/LGFX API surface is broad but still demo-able: most features are immediate-mode and can be shown with deterministic visuals.
- The library includes a few “unexpected gems” worth demoing:
  - QR code rendering
  - PNG screenshot encoding (`createPng`)
  - clipping + scroll region helpers

### What didn't work
- `rg` did not find meaningful usage of `drawPng/drawJpg/qrcode` inside `M5Cardputer-UserDemo/main/` (so our demos will likely need embedded assets and self-contained examples rather than “copying an existing app”).

### What I learned
- “Widgets” are intentionally minimal: `LGFX_Button` is a draw helper + press state, not a layout system. That’s fine; it suggests the demo suite should focus on primitives + sprites, and treat widgets as optional extras.
- Image decode utilities exist and are valuable for demos, but for ESP-IDF we should plan for embedded assets or implement an IDF-friendly `DataWrapper` source.

### What was tricky to build
- Avoiding false claims of support: M5GFX is huge, and it’s easy to assume a feature exists because it exists in another graphics stack. The analysis tries to stay grounded in headers and source.

### What warrants a second pair of eyes
- The coverage matrix and demo list: confirm they match what we actually care about for the Cardputer (e.g., whether `createPng` and scanline demos are worth the complexity).

### What should be done in the future
- When the demo suite is implemented, relate each demo’s `.cpp` files back to the analysis doc so the documentation stays “code-linked”.

### Code review instructions
- Read `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/analysis/01-m5gfx-functionality-inventory-demo-plan.md` and sanity-check:
  - the API signatures match the headers, and
  - the proposed demos are feasible on Cardputer hardware (memory, input, runtime).

### Technical details
- Commands used:
  - `sed -n '1,260p' M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.h`
  - `sed -n '1,240p' M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
  - `sed -n '1,260p' M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.hpp`
  - `rg -n "setClipRect|setScrollRect|scroll\\(" M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`

## Step 4: Validate docs + upload PDFs to reMarkable

This step “published” the work: first we ran `docmgr doctor` to ensure metadata and ticket structure were consistent, then we uploaded the analysis and diary to the reMarkable as PDFs using the `remarkable_upload.py` workflow. One practical outcome was noticing that the DejaVu fonts used by the converter don’t include some Unicode symbols, so we adjusted the coverage matrix legend to use ASCII markers before re-uploading.

### What I did
- Ran `docmgr doctor --ticket 0021-M5-GFX-DEMO` and fixed Topics to match the vocabulary.
- Performed a dry-run upload for the analysis + diary with mirrored ticket structure.
- Ran the real upload to `ai/2026/01/01/0021-M5-GFX-DEMO/...` on the device.
- Replaced Unicode checkmarks in the coverage matrix with ASCII markers to avoid missing-glyph warnings, then re-uploaded with `--force`.

### Why
- The analysis is most useful when it’s easy to read and annotate during implementation; reMarkable PDFs are the simplest workflow for that.
- Eliminating missing-glyph warnings helps ensure the rendered PDF is faithful to the Markdown source.

### What worked
- The uploader correctly inferred the destination folder from the ticket date and created intermediate directories on-device (`rmapi mkdir`).
- Both PDFs uploaded successfully under:
  - `ai/2026/01/01/0021-M5-GFX-DEMO/analysis/`
  - `ai/2026/01/01/0021-M5-GFX-DEMO/reference/`

### What didn't work
- Initial PDF conversion emitted warnings about a missing Unicode checkmark glyph in the chosen DejaVu fonts. This was resolved by switching the legend/table markers to ASCII (`X`, `~`, `-`).

### What I learned
- For predictable PDF output, the docs should prefer ASCII markers in tables (or ensure an emoji-capable font is used by the converter).

### What was tricky to build
- Balancing readability (“nice looking checkmarks”) with reproducible conversion across environments; the ASCII approach is the most portable.

### What warrants a second pair of eyes
- Confirm we actually want the mirrored remote layout (`ai/YYYY/MM/DD/<ticket>/...`) long-term, or if we prefer a flatter folder structure on reMarkable.

### What should be done in the future
- If we add more “status tables” to docs that we plan to upload, standardize on the same ASCII legend to avoid regressions.

### Code review instructions
- Open the PDFs on the reMarkable and confirm the coverage matrix renders correctly (no blank squares where markers should be).

### Technical details
- Commands used:
  - `docmgr doctor --ticket 0021-M5-GFX-DEMO --stale-after 30`
  - `python3 /home/manuel/.local/bin/remarkable_upload.py --dry-run --ticket-dir ... --mirror-ticket-structure --remote-ticket-root 0021-M5-GFX-DEMO <analysis.md> <diary.md>`
  - `python3 /home/manuel/.local/bin/remarkable_upload.py --ticket-dir ... --mirror-ticket-structure --remote-ticket-root 0021-M5-GFX-DEMO <analysis.md> <diary.md>`
  - `python3 /home/manuel/.local/bin/remarkable_upload.py --force --ticket-dir ... --mirror-ticket-structure --remote-ticket-root 0021-M5-GFX-DEMO <analysis.md> <diary.md>`

## Step 5: Design a scenario-driven demo catalog (what to build, not just what exists)

This step produced a “real-world usability” list of demos for the firmware. The goal was to map M5GFX capabilities to patterns you actually need in firmware: menus, list views, terminals, dashboards, overlays, image viewing, and developer tools like screenshots. The primary artifact is the new design doc, which is intended to be the blueprint for the eventual demo-suite implementation order.

### What I did
- Wrote `design/01-demo-firmware-real-world-scenario-catalog.md` as a scenario-driven demo list, grouped by real application categories (UI building blocks, dev tools, visualization, media, productivity, power).
- Kept demos keyboard-first and described consistent navigation/controls for the whole firmware.
- Included implementation sketches that emphasize stable rendering strategies (full-screen canvas vs scrollRect incremental updates vs layered sprites).

### Why
- A pure API tour is hard to apply: developers still have to do the “translation” into usable screens.
- Scenario-based demos create reusable building blocks and reduce future UI “wheel re-invention”.

### What worked
- The resulting catalog naturally prioritized a few demos as “most reusable” (terminal console, list view, status bar, charting, screenshot), which gives a good implementation order.

### What didn't work
- N/A.

### What I learned
- The Cardputer keyboard constraint is a feature: it forces every demo to be explicit about focus, navigation, and input conventions, which makes the examples more portable.

### What was tricky to build
- Staying “real-world” without drifting into non-graphics scope (networking, storage, etc.). The design keeps those optional and frames them as UI patterns first.

### What warrants a second pair of eyes
- The demo list’s scope and ordering: verify it’s ambitious enough to be a true showcase, but not so large that it never ships.

### What should be done in the future
- After implementing each demo, add a short “what to steal” section back into the demo’s on-device help overlay and link the implementation file(s) back into the design doc via `docmgr doc relate`.

### Code review instructions
- Read `esp32-s3-m5/ttmp/2026/01/01/0021-M5-GFX-DEMO--esp32-cardputer-m5gfx-demo-firmware/design/01-demo-firmware-real-world-scenario-catalog.md` and confirm:
  - demos match real firmware needs you care about, and
  - the proposed rendering strategies are appropriate for Cardputer (memory and performance).

### Technical details
- Primary source files referenced while designing the scenarios:
  - `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
  - `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`
  - `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`

## Step 6: Break the first five scenarios into implementable tasks + intern guides

This step turned the “what should we build first?” discussion into actionable work items and intern-readable implementation guides. The main output is five design docs (one per starter scenario) that tell a new engineer exactly where to look in the repo, which M5GFX/LGFX APIs matter, and what a correct implementation should look like.

### What I did
- Added explicit tasks for the five starter scenarios to `tasks.md`.
- Created five `DocType: design` implementation guides:
  - A2 List view + selection
  - A1 Status bar + HUD overlay
  - B2 Frame-time + perf overlay
  - E1 Terminal/log console
  - B3 Screenshot to serial
- Related each guide to the most relevant source files (`LGFXBase.hpp`, `LGFX_Sprite.hpp`, and tutorials like `0015` and `0011`) so reviewers can jump straight into code.

### Why
- These scenarios are foundational: once they exist, most other demos can reuse their infrastructure (navigation UI, chrome overlays, performance instrumentation, logging/screenshot tools).
- Intern guides reduce ramp-up time and reduce the chance of “re-inventing the wrong thing”.

### What worked
- The docs aligned naturally around reusable patterns:
  - sprite redraw-on-dirty for correctness and simplicity
  - scrollRect/scroll as an optional “fast path” once correctness is proven
  - layered sprites for predictable z-ordering

### What didn't work
- N/A.

### What I learned
- The biggest leverage comes from documenting *layout and state math* (scroll offset rules, line height math, truncation rules), not just naming APIs.

### What was tricky to build
- Keeping the guides focused on “how to build the thing” without prematurely committing to a specific future file layout for the demo-suite project (which does not exist yet).

### What warrants a second pair of eyes
- The screenshot-to-serial framing protocol: verify it matches the serial backend we choose for the demo firmware (USB-Serial-JTAG vs UART) and won’t be corrupted by logs.

### What should be done in the future
- After the demo-suite project exists, update the guides with the concrete file paths where the implementations live and link them via `docmgr doc relate`.

### Code review instructions
- Start with the five design docs under `design/02-...` through `design/06-...` and confirm each guide:
  - cites the right existing reference implementation in this repo, and
  - defines clear acceptance criteria and a minimal API surface.
