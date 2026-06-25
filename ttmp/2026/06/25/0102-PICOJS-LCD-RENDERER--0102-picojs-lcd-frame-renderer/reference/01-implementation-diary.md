---
Title: Implementation Diary
Ticket: 0102-PICOJS-LCD-RENDERER
Status: active
Topics:
    - esp32-p4
    - quickjs
    - picocalc
    - visual-repl
    - javascript
    - firmware
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: components/picocalc_lcd/picocalc_lcd.c
      Note: 40 MHz LCD SPI default that fixed ghosted display output
    - Path: components/visual_repl/visual_repl.cpp
      Note: PicoJS dump-frame LCD renderer and screen dump parity
    - Path: ttmp/2026/06/25/0102-PICOJS-LCD-RENDERER--0102-picojs-lcd-frame-renderer/scripts/01-lcd-renderer-probe.py
      Note: UART probe for render and dump parity
ExternalSources: []
Summary: Diary for Frame-oriented LCD rendering path for PicoJS screen buffers.
LastUpdated: 2026-06-25T15:30:00-07:00
WhatFor: Chronological implementation notes for 0102 PicoJS LCD Frame Renderer.
WhenToUse: Read before resuming this phase, before committing, and before hardware probe work.
---


# Diary

## Goal

This diary records the implementation of `0102-PICOJS-LCD-RENDERER`. The phase goal is to display PicoJS frame buffers on the PicoCalc LCD without relying on scrollback semantics.

## Step 1: Create child ticket and phase guide

This step split the broad PicoJS device integration effort into a focused child ticket. The design guide defines the phase scope, console contract, validation strategy, and review risks so implementation can proceed in small commits.

The child ticket links back to the umbrella `0102-PICOJS-DEVICE-INTEGRATION` plan. Its work should remain narrow; later-phase features belong in their own child tickets unless the umbrella design is updated.

### Prompt Context

**User prompt (verbatim):** "alright, do it, and for each ticket write a design + implementation guide as usual (and upload to remarkable), then stsart working on tasks one by one, committing at appropriate intervals, and keeping a detailed diary."

**Assistant interpretation:** Create the phase tickets and their implementation guides, upload the documentation package to reMarkable, then begin work phase-by-phase with commits and diary updates.

**Inferred user intent:** Keep a complex firmware integration manageable, reviewable, and recoverable by splitting it into documented tickets with concrete validation gates.

**Commit (code):** N/A — ticket/documentation setup only.

### What I did

- Created child ticket `0102-PICOJS-LCD-RENDERER`.
- Added this implementation diary.
- Wrote `design-doc/01-design-and-implementation-guide.md`.
- Defined initial scope, non-goals, console contract, validation strategy, and risks.

### Why

- The umbrella task is too broad to implement safely as one monolithic ticket.
- This phase has its own validation target and can be reviewed independently.

### What worked

- The ticket workspace and document structure were created through `docmgr`.
- The phase guide now provides an implementation boundary and a stopping condition.

### What didn't work

- N/A for this documentation setup step.

### What I learned

- Splitting by console-observable vertical slices gives each child ticket a concrete pass/fail loop.

### What was tricky to build

- The main challenge is scope control: adjacent phases touch the same files, especially `app_main.cpp`, so commits must remain explicit and diaries must record which ticket owns each behavior.

### What warrants a second pair of eyes

- Whether the phase boundaries are still right after the first firmware build/probe results.

### What should be done in the future

- Implement the phase tasks and update this diary after each commit or failed validation attempt.

### Code review instructions

- Start with this ticket's design guide.
- Review only changes that are in scope for `0102-PICOJS-LCD-RENDERER`.
- Validate using the commands listed in the design guide.

### Technical details

Phase commands to keep in view:

- `picojs render`
- `picojs dump`
- `screen dump`
- `status`



## Step 2: Render PicoJS frame dumps directly to the PicoCalc LCD

This step connected the PicoJS 40x20 text buffer to the hardware LCD. The renderer now parses the stable `picojs dump` text format, blits each row through the existing `visual_repl` fixed-cell renderer, and keeps `screen dump` in parity with the last rendered PicoJS frame.

It also fixed the display corruption observed during hardware testing by lowering the PicoCalc LCD SPI default from 80 MHz to 40 MHz while retaining the ESP32-P4 SPLL clock source.

### Prompt Context

**User prompt (verbatim):** "go ahead, test it all, then do the rendering + kieyboard on the hardware impementation. I'm going out running errands and what you to be done when i come back. Commit at appropriate intervals, and keep a detailed diary as you work."

**Assistant interpretation:** Finish the callback/input/LCD phases while the user is away, validate on the physical ESP32-P4 PicoCalc hardware, commit at useful boundaries, and keep the ticket diaries current.

**Inferred user intent:** Leave the device in a working state where PicoJS apps are time-driven, interactive, visible on the LCD, and backed by repeatable UART probes instead of manual inspection only.

**Commit (code):** 263323f6a46fb5cfae2fe3cf6b518aa8cc55a48b — "0102: add PicoJS timers input and LCD rendering"

### What I did
- Added `visual_repl_render_dump_frame(const char *dump_text)` to render `[NN] ...` PicoJS dump rows as a full 40x20 LCD frame.
- Added frame storage in `visual_repl` so `screen dump` reports the last rendered PicoJS frame instead of stale REPL history.
- Added `render_picojs_to_lcd()` in `app_main.cpp` and wired it after `picojs load`, `picojs frame`, `picojs run`, `picojs key`, and explicit `picojs render`.
- Added `picojs render` console command.
- Lowered `components/picocalc_lcd/picocalc_lcd.c` default SPI rate to 40 MHz after the user reported ghosted/duplicated pixels; retained `SPI_CLK_SRC_SPLL` because `SPI_CLK_SRC_DEFAULT` rejected the device config on this IDF.
- Added probe `scripts/01-lcd-renderer-probe.py`.

### Why
- PicoJS apps are frame-oriented and should not be forced through the visual REPL scrollback model.
- `picojs dump` remains the UART oracle, but a hardware user needs the mounted app visible on the LCD after each load/frame/key.
- The ghosted display symptom looked like signal margin or panel bus timing rather than a text layout bug; reducing SPI clock was the smallest reversible hardware fix.

### What worked
- Build passed and flashed to the by-id ESP32-P4 port.
- User confirmed after the 40 MHz flash: "ok it works now also".
- `scripts/01-lcd-renderer-probe.py` passed.
- Probe output showed `actual_khz=40000`, `picojs render: ESP_OK`, and parity between `picojs dump` and `screen dump` for the dashboard frame.
- Latest binary size: `0xe1aa0`; smallest app partition remains `0x400000`, with about 78% free.

### What didn't work
- Attempting `SPI_CLK_SRC_DEFAULT` with 40 MHz failed on the device: `spi_master: spi_bus_add_device(432): invalid sclk speed`; the fix was to keep `SPI_CLK_SRC_SPLL` and only lower `LCD_DEFAULT_SPI_HZ`.
- Before frame storage was added, `screen dump` still described visual REPL history after `picojs render`; this made LCD parity validation impossible over UART.

### What I learned
- The existing `visual_repl` row renderer is a good first LCD backend for PicoJS because it already owns glyph rasterization and row blits.
- A separate frame snapshot inside `visual_repl` is enough to keep `screen dump` useful without permanently replacing REPL history; `visual_repl_render()` clears frame mode when returning to the REPL.

### What was tricky to build
- The display corruption report overlapped with the app-rendering work. I first tried a conservative clock-source change to `SPI_CLK_SRC_DEFAULT`, but the ESP-IDF SPI device rejected it. The working compromise was `SPI_CLK_SRC_SPLL` at 40 MHz.
- Render parity required more than drawing pixels: because `screen dump` reads the visual model, not the panel, `visual_repl_render_dump_frame()` now stores the last frame rows for diagnostic dumping.

### What warrants a second pair of eyes
- Review whether `visual_repl` should keep the PicoJS frame snapshot long-term or whether a dedicated PicoJS LCD renderer component should replace it later.
- Review the 40 MHz LCD default against other PicoCalc boards; it is safer for this hardware, but users with shorter/cleaner signal paths may want a configurable rate.

### What should be done in the future
- Add dirty-row rendering if full-frame redraw time becomes visible.
- Add color/style mapping from PicoJS widgets instead of rendering all app rows as `VISUAL_REPL_STYLE_OUTPUT`.

### Code review instructions
- Start in `components/visual_repl/visual_repl.cpp` at `visual_repl_render_dump_frame()` and `visual_repl_dump_text()`.
- Then review `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` at `render_picojs_to_lcd()` and `cmd_picojs()`.
- Review `components/picocalc_lcd/picocalc_lcd.c` for the 40 MHz SPI default and SPLL source comment.
- Validate with: `ttmp/2026/06/25/0102-PICOJS-LCD-RENDERER--0102-picojs-lcd-frame-renderer/scripts/01-lcd-renderer-probe.py`.

### Technical details
- Failed experiment: `LCD_SPI_CLK_SRC SPI_CLK_SRC_DEFAULT` at 40 MHz produced `spi_master: spi_bus_add_device(432): invalid sclk speed` and `lcd init: ESP_ERR_INVALID_ARG actual_khz=0`.
- Working LCD status: `lcd_requested=40000000 actual_khz=40000 max_transfer=32768`.
- Probe command: `ttmp/2026/06/25/0102-PICOJS-LCD-RENDERER--0102-picojs-lcd-frame-renderer/scripts/01-lcd-renderer-probe.py`.
- Parity evidence: both `picojs dump` and `screen dump` contained `PicoJS Dashboard`, `ESP32-P4 native DSL`, battery/heap gauges, and `dashboard native picojs`.
