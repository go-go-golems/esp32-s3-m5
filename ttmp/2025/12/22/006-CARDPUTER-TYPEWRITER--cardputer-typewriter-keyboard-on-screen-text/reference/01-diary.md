---
Title: Diary
Ticket: 006-CARDPUTER-TYPEWRITER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - keyboard
    - display
    - m5gfx
    - ui
    - text
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp
      Note: Keyboard scan + on-screen last token (commit 3c34342f540b6c11dd6a434f57458d0af3d744a1); initial scaffold commit 2110b78c349256bb7769ee2e30dfa696e5745c8e
    - Path: esp32-s3-m5/0012-cardputer-typewriter/main/Kconfig.projbuild
      Note: 0012 Kconfig surface for display/keyboard/typewriter settings.
    - Path: esp32-s3-m5/0012-cardputer-typewriter/README.md
      Note: Build/flash instructions and expected bring-up output for the 0012 chapter.
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-22T22:47:07.514128355-05:00
WhatFor: ""
WhenToUse: ""
---

# Diary

## Goal

Capture a step-by-step narrative of implementing the Cardputer “typewriter” chapter: keyboard input → on-screen text UI, grounded in known-good keyboard scanning and M5GFX display bring-up from this repo.

## Context

This ticket builds directly on:

- Cardputer keyboard scan ground truth from ticket `005` and tutorial `0007`
- Cardputer M5GFX autodetect + full-screen sprite present path from tutorial `0011`

## Quick Reference

N/A (the chapter project code is the copy/paste source).

## Usage Examples

N/A

## Related

- `analysis/01-setup-architecture-cardputer-typewriter-keyboard-on-screen-text.md`

## Step 1: Create ticket workspace + write setup/architecture analysis

This step set up the ticket workspace and captured the full “what do we need to wire up?” analysis for the typewriter app. The analysis ties together the repo’s known-good keyboard scanning code, the known-good M5GFX bring-up, and vendor text UI patterns so we can implement the chapter without re-deriving hardware details.

### What I did
- Created ticket `006-CARDPUTER-TYPEWRITER` and initial task list
- Related the key reference files from ticket `005`, tutorial `0007`, and tutorial `0011` to the new ticket index
- Wrote `analysis/01-setup-architecture-cardputer-typewriter-keyboard-on-screen-text.md` (architecture + checklist)

### Why
- Make the upcoming firmware work traceable and straightforward: clear requirements, clear ground truth, clear file layout.

### What worked
- The ticket now has a complete setup/architecture document and actionable tasks.

### What didn't work
- N/A

### What I learned
- The “typewriter” app is best implemented as a chapter project under `esp32-s3-m5/` reusing M5GFX via `EXTRA_COMPONENT_DIRS`, like the existing `0011`.

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

## Step 2: Create chapter 0012 scaffold + verify M5GFX display bring-up builds

This step created the new `esp32-s3-m5/0012-cardputer-typewriter/` chapter project by cloning the proven Cardputer M5GFX wiring pattern from `0011`. The immediate payoff is a “known-good compile surface”: we can now layer keyboard scanning and text rendering on top without fighting build system wiring.

I also hit an easy-to-miss workflow detail: the workspace root isn’t a git repo, but `esp32-s3-m5/` is — so code commits must be made from that subrepo, while ticket docs live in the separate `echo-base--openai-realtime-embedded-sdk/` repo.

**Commit (code):** 2110b78c349256bb7769ee2e30dfa696e5745c8e — "Tutorial 0012: scaffold + M5GFX display bring-up"

### What I did
- Added `esp32-s3-m5/0012-cardputer-typewriter/` with:
  - `CMakeLists.txt` using `EXTRA_COMPONENT_DIRS` to reuse `M5Cardputer-UserDemo/components/M5GFX`
  - `main/hello_world_main.cpp` with M5GFX autodetect init + solid-color bring-up + full-screen canvas allocation
  - `main/Kconfig.projbuild` + `sdkconfig.defaults` + `partitions.csv` + `dependencies.lock` + `README.md`
- Built the new chapter with ESP-IDF 5.4.1 to confirm wiring is correct
- Committed the chapter scaffold in the `esp32-s3-m5/` git repo (not workspace root)

### Why
- Establish a stable baseline (builds + display init) before layering in keyboard scanning and a text model.

### What worked
- `idf.py build` succeeded for the new chapter
- The code is structured like `0011`, so future diffs stay familiar (M5GFX init + sprite present path)

### What didn't work
- Initial attempt to commit from workspace root failed because `/home/manuel/workspaces/2025-12-21/echo-base-documentation/` is not a git repo.

### What I learned
- This workspace is a “multi-repo” tree: tutorials under `esp32-s3-m5/` are versioned separately from the main `echo-base--openai-realtime-embedded-sdk/` repo (which contains `ttmp/` docs).

### What was tricky to build
- Keeping the chapter scaffold minimal while still matching the proven M5GFX + sprite allocation pattern (the latter is the real “bring-up contract”).

### What warrants a second pair of eyes
- Confirm the `0012` Kconfig option names and defaults match the conventions in `0007`/`0011` (to avoid future drift and confusion).

### What should be done in the future
- Add a short note in the chapter README (or a shared dev note) about the multi-repo layout so future “commit at compile points” work doesn’t trip over git roots again.

### Code review instructions
- Start at `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp` and confirm it matches the `0011` bring-up approach (M5GFX autodetect + canvas sprite).
- Validate build:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && idf.py set-target esp32s3 && idf.py build`

### Technical details
- Build command used:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && idf.py set-target esp32s3 && idf.py build`

## Step 3: Port keyboard scan (0007) into 0012 and show key tokens on-screen

This step brought the Cardputer keyboard online in the new `0012` chapter by porting the proven GPIO matrix scan from tutorial `0007`. Instead of only logging to serial, the app now also updates the LCD on each key event, showing the **last decoded token** and modifier state. That gives us a tight feedback loop for verifying wiring, scan timing, and mapping before we build a full text buffer/editor model.

**Commit (code):** 3c34342f540b6c11dd6a434f57458d0af3d744a1 — "Tutorial 0012: add keyboard scan + on-screen last key"

### What I did
- Ported the `0007` keyboard scanning logic into `0012`:
  - scan-state loop (8 states) + 3 OUT pins
  - 7 input pins with pull-ups (pressed = low)
  - optional IN0/IN1 autodetect path (primary vs alt IN0/IN1)
  - per-key edge detection + debounce guardrail
- Reused the vendor-style `4x14` key legend mapping (normal vs shifted labels)
- Rendered a simple “last token” screen using the existing full-screen `M5Canvas` path (only repaints on key events)

### Why
- Validate keyboard scanning is correct (and stable) before we invest in editor semantics (buffer, scrolling, cursor UX).

### What worked
- `idf.py build` succeeded with the new keyboard code, and the loop is wired to repaint only when events occur.

### What didn't work
- N/A

### What I learned
- The “last token on-screen” approach is a really good intermediate milestone: it exercises both keyboard and display simultaneously without needing text-buffer complexity yet.

### What was tricky to build
- Preserving the vendor coordinate mapping (x/y transform) correctly while keeping the scan code readable and debuggable.

### What warrants a second pair of eyes
- Confirm the x/y mapping math and the key legend matrix match the vendor “picture” coordinate system (off-by-one mistakes here would silently scramble keys).

### What should be done in the future
- Turn token strings (`"space"`, `"enter"`, `"del"`, printable chars) into a typed event model (enum + payload) so the editor layer doesn’t depend on string comparisons everywhere.

### Code review instructions
- Start at `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp`:
  - search for `kb_scan_pressed` and verify scan + mapping logic
  - search for `dirty` and verify we only redraw on events
- Build:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && idf.py build`

### Technical details
- Build command used:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && idf.py build`

## Step 4: Implement MVP typewriter buffer + redraw rendering (enter/backspace/scroll + cursor)

This step turns the keyboard tokens into an actual “typewriter” document: printable keys append to the current line, `enter` creates a new line, and `del` acts as backspace (including the “join with previous line” behavior when at column 0). Rendering is intentionally simple and robust: we redraw the full-screen canvas only on edit events and keep the **last N lines** visible (where N fits on screen).

For now, the cursor model is “always at end of last line” (terminal-style). That keeps cursor invariants straightforward and is enough to validate that editor semantics + rendering behave correctly at real typing speed.

**Commit (code):** 1995080b24a3ccbc7c3d43a965127aaa82dc2cdc — "Tutorial 0012: typewriter buffer + render MVP"

### What I did
- Added a minimal text buffer using `std::deque<std::string>` (bounded by `CONFIG_TUTORIAL_0012_MAX_LINES`)
- Implemented edit operations:
  - insert printable char / space / tab (tab = 4 spaces)
  - newline on `enter` (drops oldest lines when over max)
  - backspace on `del` (join-with-previous-line when current line is empty)
- Implemented rendering MVP:
  - full-screen redraw on `dirty` (edit event)
  - compute `line_h = canvas.fontHeight()` and render the last visible lines
  - draw a simple underscore cursor at end-of-line
- Kept optional heartbeat logging via `CONFIG_TUTORIAL_0012_LOG_EVERY_N_EVENTS`

### Why
- Establish the end-to-end loop (keys → edits → redraw) before doing any optimization or fancy cursor navigation.

### What worked
- `idf.py build` succeeded after adding the buffer + rendering logic.

### What didn't work
- N/A

### What I learned
- Redrawing only on edit events (not on scan ticks) keeps CPU load low and makes DMA “tearing” issues easier to reason about.

### What was tricky to build
- Getting “join-with-previous-line” correct without introducing cursor state yet (we model “cursor at end of last line” so the join is simply popping an empty line).

### What warrants a second pair of eyes
- The rendering loop’s line positioning math (header rows vs body rows) and cursor placement/clamping near the right edge.

### What should be done in the future
- Convert the string-token approach into a typed event model (enum + payload) so the editor layer doesn’t depend on `strcmp()` everywhere.
- Consider adding a bounded max column behavior (wrap or clip) to keep cursor underline sane for very long lines.

### Code review instructions
- Start at `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp`:
  - search `buf_insert_char` / `buf_backspace` / `buf_newline`
  - search `line_h` and review the “render last N lines” logic
- Build:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && idf.py build`

### Technical details
- Build command used:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && idf.py build`

## Step 5: Validate on hardware (flash via by-id) and close out the ticket

This step validated that the current `0012` firmware is “good enough” on real hardware: flashing via the stable `/dev/serial/by-id/...` path worked, and the app behaves as expected (typing/backspace/enter/scroll + visible cursor). I also hit a tooling limitation in this environment: `idf.py monitor` requires a real TTY, so it can fail when run non-interactively; that’s fine for this ticket since the main validation is on-device behavior.

**Commit (docs):** 9272f1e73047e69ad226f30a8b014d9da519a305 — "Docs(006): diary Step 4 + typewriter MVP"

### What I did
- Built and flashed `esp32-s3-m5/0012-cardputer-typewriter` using the stable by-id port and conservative baud.
- Verified interactively on device:
  - printable keys appear
  - `del` backspaces (including join with previous line)
  - `enter` inserts new line
  - when lines exceed the visible region, the last N remain visible
  - underscore cursor updates on each edit
- Attempted to start `idf.py monitor` from a non-interactive session; it failed due to lacking a TTY.
- Marked ticket tasks complete and prepared to close the ticket.

### Why
- Ensure the tutorial is usable on real hardware (not just “builds”) and capture the exact flash command that avoids USB re-enumeration issues.

### What worked
- Flashing via `/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00` at 115200 succeeded reliably.
- The typewriter behavior is “good enough” for the tutorial MVP.

### What didn't work
- `idf.py monitor` failed in this environment with:
  - `Error: Monitor requires standard input to be attached to TTY. Try using a different terminal.`

### What I learned
- For automation/non-interactive runs, prefer capturing logs via a TTY-attached terminal (or use alternative tooling like `screen`) if console output is required.

### What was tricky to build
- Keeping the UI redraw simple and robust while still delivering the core UX (scrolling + cursor) without adding full editor complexity.

### What warrants a second pair of eyes
- Cursor placement and clipping with very long lines (we currently clamp the underline near the right edge).

### What should be done in the future
- If we want higher fidelity:
  - implement line wrapping (or horizontal scrolling) as a declared behavior/contract
  - move from string-token comparisons to a typed key event model

### Code review instructions
- Flash command used:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh && cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0012-cardputer-typewriter && idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 flash`
- If you want to monitor interactively, run from a real terminal (TTY) and consider:
  - `idf.py -p '/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D0:CF:13:0E:BE:00-if00' -b 115200 monitor`

