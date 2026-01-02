---
Title: 'Project analysis: Command palette overlay (Ctrl+P)'
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp
      Note: Non-LVGL list selection + scrolling model to port conceptually
    - Path: 0025-cardputer-lvgl-demo/main/action_registry.cpp
      Note: v0 action list backing palette
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: |-
        Best place to detect global key chords (Ctrl+P) before feeding LVGL
        Ctrl+P chord detection and palette toggle integration
    - Path: 0025-cardputer-lvgl-demo/main/command_palette.cpp
      Note: Palette overlay implementation
    - Path: 0025-cardputer-lvgl-demo/main/demo_manager.cpp
      Note: Screen switching and shared lv_group_t ownership model
    - Path: 0025-cardputer-lvgl-demo/main/demo_menu.cpp
      Note: Minimal keyboard-driven selection UI pattern in LVGL
    - Path: 0025-cardputer-lvgl-demo/main/input_keyboard.cpp
      Note: Produces KeyEvent including ctrl/alt/fn flags; usable for chord detection
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp
      Note: Current KeyEvent->LVGL translation drops modifier state; informs shortcut strategy
    - Path: ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/design-doc/02-screen-state-lifecycle-pattern-lvgl-demo-catalog.md
      Note: Screen lifecycle constraints that matter for modal overlays
ExternalSources: []
Summary: Analyzes how to build a Ctrl+P command palette overlay in LVGL on Cardputer, including focus management, shortcut handling, filtering, and lifecycle constraints.
LastUpdated: 2026-01-02T09:39:30.635547986-05:00
WhatFor: ""
WhenToUse: ""
---


# Project analysis: Command palette overlay (Ctrl+P)

This document analyzes how to add a “command palette” overlay (think Ctrl+P / Spotlight) to the Cardputer LVGL demo catalog. The command palette is a *modal* UI element that:

- pops up over any demo screen,
- accepts a query string,
- filters a list of actions (“Open Pomodoro”, “Screenshot”, “Toggle HUD”, “Set minutes 25”…),
- runs the selected action,
- closes and returns focus to whatever you were doing.

This is a great “moderately challenging” project because it forces you to build reusable infrastructure:

- **actions registry** (names + shortcuts + callbacks)
- **modal focus ownership** (who gets keys while the palette is open?)
- **shortcut detection** (Ctrl+P) on a keyboard-first device
- **UI-state restoration** (return to previous focused widget/screen)

It also becomes the glue that makes future features feel like a real “micro OS”.

---

## What already exists in 0025 (and what it implies)

### LVGL runs on a single UI thread

`0025-cardputer-lvgl-demo/main/app_main.cpp` is the authoritative UI thread:

- input is polled and fed into LVGL
- `lv_timer_handler()` runs in the same loop
- screen switches happen through `DemoManager` on this thread

This is good for a command palette because it means:

- no cross-thread UI mutation
- deterministic key handling

### Key events have modifier metadata, but LVGL currently does not see it

`KeyEvent` contains `ctrl/alt/fn/shift` flags (`0025-cardputer-lvgl-demo/main/input_keyboard.cpp`). However, `lvgl_port_cardputer_kb_translate` currently maps only:

- navigation actions → `LV_KEY_*`
- single characters → ASCII

and returns `0` for anything else. It **does not** encode modifier state into LVGL keycodes.

Implication: the clean way to implement **Ctrl+P** is *not* inside LVGL event handlers; it is in the pre-feed filter loop in `app_main.cpp`, where we still have the full `KeyEvent`.

---

## Relevant patterns elsewhere in the repo (conceptual reuse)

### A selection + scrolling model (non-LVGL)

`0022-cardputer-m5gfx-demo-suite/main/ui_list_view.cpp` is a compact “list view” that solves:

- selection movement (Up/Down/PageUp/PageDown)
- “ensure selection visible” scrolling
- fitting/truncating labels with ellipsis

We can port the *behavior* to LVGL without porting the code:

- use `lv_list` and programmatically maintain selection index + scrolling, or
- build a simple “rows container” with N labels and re-render them from a filtered model.

### Simple selection UI in LVGL already exists

`0025-cardputer-lvgl-demo/main/demo_menu.cpp` already implements:

- selection state (`selected`)
- keyboard handler via `LV_EVENT_KEY`
- highlight style changes for “selected row”

This is a direct starting point for the command palette’s results list.

---

## Two design options for the palette’s implementation

### Option A (recommended): An LVGL modal overlay on `lv_layer_top()`

Create a palette container on the top layer:

- It’s not a separate screen.
- It doesn’t require screen switching.
- It visually overlays the current demo.

It should:

- capture focus (so keys go to the palette while open)
- be removed/destroyed when closed

LVGL primitives you’ll likely use:

- `lv_layer_top()` for parent
- `lv_obj_create()` for a translucent backdrop
- `lv_textarea` (query input)
- `lv_obj` container for results list (rows)
- optional animations (`lv_anim_t`) for slide/fade

### Option B: A dedicated “palette screen”

Switch to a screen that looks like an overlay.

Pros:

- simpler focus model (screen root is the only focus target)

Cons:

- loses “overlay” vibe
- forces full screen lifecycle handling (timers/state cleanup)
- makes it harder to keep context of what you were doing

Given the goal (“overlay”), Option A is a better fit.

---

## Proposed architecture (recommended)

### 1) Introduce an app-wide `ActionRegistry`

The palette’s job is to expose actions. That suggests a central registry:

```c++
enum class ActionId {
  OpenMenu,
  OpenBasics,
  OpenPomodoro,
  Screenshot,
  // later: TogglePerf, ToggleHud, SdOpen, SdCat, ...
};

struct Action {
  ActionId id;
  const char* title;      // "Open Pomodoro"
  const char* keywords;   // "pomodoro timer focus"
  const char* shortcut;   // "Ctrl+P" for opening palette is separate; per-action optional
};
```

Then palette filtering is simply “score actions against query”.

Execution should not directly mutate LVGL in generic action code unless it is running on the UI thread. In 0025, it will be running on the UI thread, but we still want discipline because other callers (like `esp_console` scripting) will not be. A good compromise:

- palette actions enqueue `CtrlEvent` (same control-plane as the split-pane console analysis)
- the main loop executes those `CtrlEvent`s and mutates LVGL/demos

This makes the action registry reusable in both:

- the palette UI, and
- `esp_console` commands.

### 2) Detect Ctrl+P in `app_main.cpp` (before feeding LVGL)

Because `KeyEvent` contains `ctrl`, do chord detection here:

Pseudocode (in the loop in `app_main.cpp`):

```c++
for (const auto& ev : keyboard.poll()) {
  const bool ctrl_p = ev.ctrl && (ev.key == "p" || ev.key == "P");
  if (ctrl_p) {
    palette_toggle();
    continue; // do not feed into LVGL
  }

  // Existing logic:
  //  - translate to LVGL key code
  //  - global ESC handler (return to menu)
  //  - feed to lvgl_port_cardputer_kb
}
```

This avoids having to encode modifier state into LVGL key events.

### 3) Palette focus ownership and restoration

The palette should “own keys” while open. Suggested state:

```c++
struct PaletteState {
  bool open;
  lv_obj_t* backdrop;
  lv_obj_t* panel;
  lv_obj_t* query;
  lv_obj_t* results_root;
  int selected;
  lv_obj_t* prev_focused; // capture before open
};
```

When opening:

- capture `prev_focused = lv_group_get_focused(group)` (or track it yourself)
- create overlay objects
- add the palette panel (or query) to the group and focus it

When closing:

- delete overlay objects (sync or async)
- restore focus: if `prev_focused` still exists, `lv_group_focus_obj(prev_focused)`

**Lifecycle pitfall:** the focused object might have been deleted (e.g., if the underlying screen was switched while palette open). That’s why restoring focus needs to tolerate null/stale pointers.

If we keep the palette from allowing screen switches “underneath it” (recommended), this becomes simpler.

---

## UI layout: how to fit it on 240×135

A practical palette layout for Cardputer:

```
┌──────────────────────────────┐
│  ░░░░░░░░░░ (dim backdrop)   │  ← lv_layer_top overlay
│  ┌────────────────────────┐  │
│  │ > query...             │  │  ← one-line textarea
│  │────────────────────────│  │
│  │  Open Pomodoro         │  │
│  │  Screenshot            │  │  ← 4–6 visible rows, scrollable
│  │  Open Menu             │  │
│  │  ...                   │  │
│  └────────────────────────┘  │
└──────────────────────────────┘
```

Design notes:

- Keep it keyboard-first:
  - Up/Down to select
  - Enter to run
  - Esc to close
- Filtering should be incremental as you type.
- The palette itself should be “cheap” to redraw; avoid re-creating the entire UI every keystroke if possible (but with a small action list, it’s fine to rebuild rows).

---

## Filtering strategy (simple first, then fancy)

### Simple substring match (good enough initially)

Normalize:

- query lowercased
- action title + keywords lowercased

Score:

- if query is empty: show “top actions” (most used)
- else:
  - title contains query → score 100
  - keywords contain query → score 50
  - else score 0 (excluded)

Pseudocode:

```c++
struct Scored { const Action* a; int score; };

vector<Scored> filter(const vector<Action>& actions, string q) {
  q = lower(trim(q));
  vector<Scored> out;
  for (auto& a : actions) {
    string hay = lower(a.title) + " " + lower(a.keywords);
    int score = 0;
    if (q.empty()) score = 1;
    else if (hay.find(q) != npos) score = 100;
    if (score) out.push_back({&a, score});
  }
  sort(out, by score desc then title);
  return out;
}
```

### “Moderately challenging” upgrade: fuzzy match

If we want the palette to feel premium, we can implement a lightweight fuzzy matcher:

- query “pm” matches “PoModoro”
- query “scr” matches “Screenshot”

Keep it deterministic and small; don’t pull in heavy libraries.

---

## Key handling: who consumes what?

While the palette is open:

- **Esc** closes palette
- **Up/Down** changes selection
- **Enter** runs selected action and closes palette
- **Printable characters** should go to the query input
- **Backspace** deletes query characters

Important: the palette should prevent those keys from “leaking” to the underlying screen.

Implementation detail:

- register a key handler on the palette panel itself:
  - `lv_obj_add_event_cb(panel, key_cb, LV_EVENT_KEY, &state)`
- focus the panel (or query textarea) so it receives key events via the shared group

If you focus the query textarea, LVGL may interpret Up/Down as cursor navigation inside the textarea. To keep “Up/Down selects results” behavior, it’s often cleaner to focus the panel and manually append characters to the query.

That approach yields a “terminal-like” command palette rather than a “form widget”.

---

## Integration points with other planned features (console + screenshot)

The palette becomes the UI front-end for actions that are also reachable via:

- `esp_console` REPL (host scripting)
- split-pane on-device console

If we keep the action/control-plane model consistent:

- “screenshot” action → enqueue `CtrlEvent::ScreenshotPngToSerial`
- “open pomodoro” action → enqueue `CtrlEvent::OpenPomodoro`

That makes the palette a *view* over the same action set, not a separate ad-hoc system.

---

## For a new developer: what you need to know to implement this safely

### 1) LVGL focus + groups are the keyboard routing mechanism

In 0025:

- `DemoManager` owns a single `lv_group_t*`
- the keyboard indev is bound to that group
- key events go to the focused object in the group

Therefore:

- opening a modal means “move focus into modal”
- closing a modal means “restore focus to previous object”

### 2) Modals must respect lifecycle and screen switching

If you delete the underlying screen while a modal holds pointers into it, you can end up restoring focus to a deleted object.

Practical rule:

- while palette is open, don’t allow screen switches underneath it
- run the action, then close the palette, then allow the action to switch screens

This sequencing eliminates an entire class of focus restoration bugs.

### 3) Modifier shortcuts should be detected outside LVGL (with current ports)

Because LVGL doesn’t see modifier state, chord detection belongs in `app_main.cpp` where we still have `KeyEvent.ctrl`.

If we later *want* LVGL to see modifier state, that becomes a porting decision (change `lvgl_port_cardputer_kb_translate` contract), and it will affect every screen. Don’t do it unless we really need it.

---

## Implementation checklist (pragmatic)

1. Add an `ActionRegistry` with 5–10 actions.
2. Implement palette overlay UI on `lv_layer_top()`:
   - backdrop + panel + query label + results rows
3. Add `Ctrl+P` detection in `app_main.cpp` to toggle palette.
4. Add selection + filtering logic; update rows as query changes.
5. Add focus capture/restore logic.
6. Make each action enqueue a `CtrlEvent` so it can also be triggered from `esp_console`.

---

## v0 action list (implemented)

The v0 palette actions are intentionally small and map 1:1 onto UI-thread-safe `CtrlEvent`s:

- Open Menu
- Open Basics
- Open Pomodoro
- Open Console
- Open System Monitor
- Pomodoro: Set minutes 15
- Pomodoro: Set minutes 25
- Pomodoro: Set minutes 50
- Screenshot (USB-Serial/JTAG)

Expected keyboard behavior while open:

- `Ctrl+P`: toggle palette
- Type: updates query and filters actions
- `Up`/`Down`: changes selection
- `Enter`: runs selected action (then closes palette)
- `Esc`: closes palette

Expected visible UI text (OCR regression anchors):

- Title placeholder: `Search...`
- At least one result row title (e.g., `Open Menu`)
