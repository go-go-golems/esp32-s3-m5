---
Title: Implementation Diary
Ticket: 0102-PICOJS-INPUT-APP-MODE
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
    - Path: 0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp
      Note: App-mode keyboard routing and semantic key injection
    - Path: components/picojs_runtime/picojs_runtime.cpp
      Note: App.key callback storage and JS-task key execution
    - Path: ttmp/2026/06/25/0102-PICOJS-INPUT-APP-MODE--0102-picojs-input-and-app-mode/scripts/01-input-app-mode-probe.py
      Note: UART probe for app mode and key injection
ExternalSources: []
Summary: Diary for Semantic key injection and physical keyboard routing for mounted PicoJS apps.
LastUpdated: 2026-06-25T15:30:00-07:00
WhatFor: Chronological implementation notes for 0102 PicoJS Input and App Mode.
WhenToUse: Read before resuming this phase, before committing, and before hardware probe work.
---


# Diary

## Goal

This diary records the implementation of `0102-PICOJS-INPUT-APP-MODE`. The phase goal is to make PicoJS apps interactive while preserving an escape path back to the REPL.

## Step 1: Create child ticket and phase guide

This step split the broad PicoJS device integration effort into a focused child ticket. The design guide defines the phase scope, console contract, validation strategy, and review risks so implementation can proceed in small commits.

The child ticket links back to the umbrella `0102-PICOJS-DEVICE-INTEGRATION` plan. Its work should remain narrow; later-phase features belong in their own child tickets unless the umbrella design is updated.

### Prompt Context

**User prompt (verbatim):** "alright, do it, and for each ticket write a design + implementation guide as usual (and upload to remarkable), then stsart working on tasks one by one, committing at appropriate intervals, and keeping a detailed diary."

**Assistant interpretation:** Create the phase tickets and their implementation guides, upload the documentation package to reMarkable, then begin work phase-by-phase with commits and diary updates.

**Inferred user intent:** Keep a complex firmware integration manageable, reviewable, and recoverable by splitting it into documented tickets with concrete validation gates.

**Commit (code):** N/A — ticket/documentation setup only.

### What I did

- Created child ticket `0102-PICOJS-INPUT-APP-MODE`.
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
- Review only changes that are in scope for `0102-PICOJS-INPUT-APP-MODE`.
- Validate using the commands listed in the design guide.

### Technical details

Phase commands to keep in view:

- `picojs key left`
- `picojs key right`
- `picojs key enter`
- `picojs key a`
- `picojs dump`



## Step 2: Add semantic key injection and app-mode keyboard routing

This step made mounted PicoJS apps interactive. It added JS-side `App.key(token, fn)` callbacks, console key injection, an explicit `picojs mode app|repl` switch, and physical keyboard routing that sends semantic tokens to PicoJS while app mode is active.

The UART console remains the validation oracle. The physical keyboard path is wired through the same token sender as `picojs key`, and Escape exits app mode back to the visual REPL.

### Prompt Context

**User prompt (verbatim):** "go ahead, test it all, then do the rendering + kieyboard on the hardware impementation. I'm going out running errands and what you to be done when i come back. Commit at appropriate intervals, and keep a detailed diary as you work."

**Assistant interpretation:** Finish the callback/input/LCD phases while the user is away, validate on the physical ESP32-P4 PicoCalc hardware, commit at useful boundaries, and keep the ticket diaries current.

**Inferred user intent:** Leave the device in a working state where PicoJS apps are time-driven, interactive, visible on the LCD, and backed by repeatable UART probes instead of manual inspection only.

**Commit (code):** 263323f6a46fb5cfae2fe3cf6b518aa8cc55a48b — "0102: add PicoJS timers input and LCD rendering"

### What I did
- Added `App.key(token, fn)` storage in `components/picojs_runtime/picojs_runtime.cpp`.
- Added `picojs_runtime_key_js(JSContext *, ...)` so key callbacks execute on the QuickJS service task.
- Added `picojs key <token>` console injection and `picojs mode app|repl`.
- Added semantic key mapping for arrows, Enter, Home/Delete/End, and printable ASCII in `app_main.cpp`.
- Routed the PicoCalc keyboard task to PicoJS when `app_mode` is enabled; Escape switches back to REPL mode.
- Added `interactive` built-in app with live key text, tick counter, and key callback `print()` output.
- Added probe `scripts/01-input-app-mode-probe.py`.

### Why
- App code should see semantic tokens like `left` and `a`, not raw PicoCalc keycodes.
- Console key injection is repeatable and scriptable, while physical keyboard validation depends on the operator pressing keys.
- App mode must be explicit so normal REPL editing remains available.

### What worked
- Build and flash passed on the ESP32-P4 by-id port.
- `scripts/01-input-app-mode-probe.py` passed.
- Probe output showed `picojs mode: ESP_OK app_mode=1`, `KEY left`, `last key: left`, `KEY a`, `last key: a`, and then `picojs mode: ESP_OK app_mode=0`.
- The user reported the keyboard works and the REPL updates on the device.

### What didn't work
- The first implementation was not yet a full app-mode loop; it needed explicit render calls after key callbacks so LCD state changed immediately after `picojs key` and physical key events.
- Keyboard recovery warnings still appear intermittently (`ESP_ERR_INVALID_STATE`), but they did not block PicoJS key injection or console validation.

### What I learned
- Sharing the same `send_picojs_key_token()` path between console injection and physical keyboard routing keeps validation honest: if console key callbacks work, the physical path mostly reduces to keycode-to-token mapping and app-mode gating.
- Keeping Escape as a hard app-mode exit is important because a JS app can otherwise consume normal text keys indefinitely.

### What was tricky to build
- The keyboard task cannot call QuickJS directly. It now submits a small `PicoKeyJob` to `qjs_service_run()`, then renders the updated PicoJS screen to the LCD.
- The console task and keyboard task can both request key/frame work. The implementation keeps QuickJS execution serialized by the qjs service queue.

### What warrants a second pair of eyes
- Review the raw keycode map in `key_to_picojs_token()` against the PicoCalc firmware table; arrow keys and printable ASCII are covered, but modifiers and function keys are intentionally minimal.
- Review blocking behavior in the keyboard task: `qjs_service_run()` is bounded by `kEvalTimeoutMs`, but a future long-running app callback could still make a physical key feel slow.

### What should be done in the future
- Add richer key events if apps need press/repeat/release distinctions.
- Add a visible app-mode indicator or a dedicated command to show the active key map.

### Code review instructions
- Start in `0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp` at `key_to_picojs_token()`, `send_picojs_key_token()`, and `keyboard_task()`.
- Then review `components/picojs_runtime/picojs_runtime.cpp` at `KeyCallback`, `js_app_key()`, and `picojs_runtime_key_js()`.
- Validate with: `ttmp/2026/06/25/0102-PICOJS-INPUT-APP-MODE--0102-picojs-input-and-app-mode/scripts/01-input-app-mode-probe.py`.

### Technical details
- Probe command: `ttmp/2026/06/25/0102-PICOJS-INPUT-APP-MODE--0102-picojs-input-and-app-mode/scripts/01-input-app-mode-probe.py`.
- Passing substrings included: `app_mode=1`, `KEY left`, `last key: left`, `KEY a`, `last key: a`, and `app_mode=0`.
- Physical-key escape behavior: when app mode is active, keycode `0xb1` (`esc`) calls `picojs_runtime_set_app_mode(..., false)` and returns to editor routing.
