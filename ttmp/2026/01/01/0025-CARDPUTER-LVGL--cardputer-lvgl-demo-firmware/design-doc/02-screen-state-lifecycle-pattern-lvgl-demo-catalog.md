---
Title: Screen state & lifecycle pattern (LVGL demo catalog)
Ticket: 0025-CARDPUTER-LVGL
Status: active
Topics:
    - esp32s3
    - cardputer
    - lvgl
    - ui
    - esp-idf
    - m5gfx
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0025-cardputer-lvgl-demo/main/demo_basics.cpp
      Note: Per-screen state + LV_EVENT_DELETE timer cleanup
    - Path: 0025-cardputer-lvgl-demo/main/demo_manager.cpp
      Note: Screen switching uses lv_scr_load + lv_obj_del_async
    - Path: 0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp
      Note: Per-screen state + LV_EVENT_DELETE timer cleanup
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-02T08:56:31.16190593-05:00
WhatFor: ""
WhenToUse: ""
---


# Screen state & lifecycle pattern (LVGL demo catalog)

## Executive Summary

The `0025-cardputer-lvgl-demo` firmware is a “demo catalog”: it switches between multiple LVGL screens (Menu, Basics, Pomodoro). Each screen has **screen-local state** (pointers to LVGL objects, timers, counters). This document explains the screen-state pattern used today and why it exists: LVGL timers are not automatically tied to object lifetimes, and naïve “static state + timers” will crash when the screen is destroyed.

The current pattern is:
- each screen’s `*_create()` allocates a per-screen `State` object,
- the screen root registers an `LV_EVENT_DELETE` callback to delete LVGL timers and free the `State`,
- the screen manager switches screens via `lv_scr_load(next)` and deletes the old root via `lv_obj_del_async(old)`.

Then this document proposes how to evolve this into a more robust lifecycle model suitable for a larger UI:
- schedule screen switches outside LVGL callbacks (`lv_async_call`),
- introduce an explicit `destroy()` hook and a `ScreenContext` to track timers/async work,
- centralize invariants and debug checks so “timer use-after-free” is harder to reintroduce.

## Problem Statement

### What went wrong (the crash we fixed)

We hit a classic LVGL lifecycle bug while switching demos:

- The Basics demo created a repeating `lv_timer_t` (`status_timer_cb`) that periodically called `lv_label_set_text()` on a label.
- When we switched away from Basics, the Basics screen root was destroyed.
- LVGL timers kept running anyway.
- The timer callback ran after the screen was destroyed and attempted to dereference pointers to freed LVGL objects.

Result: a Guru Meditation / `LoadProhibited` with a backtrace inside LVGL style access (e.g. `lv_obj_style.c`), and a pointer value that looked like a poison pattern (`0xa1b2c3d4`), which is consistent with use-after-free.

This is an API contract issue, not LVGL “misbehaving”:
- `lv_timer_create()` creates timers that live until explicitly deleted (`lv_timer_del()`).
- `lv_obj_del()` deletes objects, but does not automatically delete timers you created.

### Why this class of bug is easy to create

If you treat “screen state” as globals/static variables, you tend to:
- store raw `lv_obj_t*` pointers in global state,
- create repeating timers for UI refresh,
- switch screens by deleting the root object,
- forget to stop timers.

That combination almost guarantees use-after-free when screens are destroyed.

## Proposed Solution

### Current pattern: per-screen state + cleanup-on-delete

The firmware uses a deliberately boring but reliable lifecycle:

1. Each demo uses a dedicated `State` struct holding only what it needs:
   - pointers to LVGL objects it created (`lv_obj_t*`)
   - pointers to LVGL timers it created (`lv_timer_t*`)
   - any counters/time bookkeeping for the screen
2. Each demo’s `*_create()` allocates `State` on the heap (`new State{}`).
3. The demo registers an `LV_EVENT_DELETE` callback on the screen root to:
   - `lv_timer_del()` any screen timers
   - clear pointers (optional but good hygiene)
   - `delete` the `State`
4. The screen manager switches screens via:
   - `lv_scr_load(next)`
   - `lv_obj_del_async(old)` (so delete happens safely later)

This makes timers “screen-scoped”: they cannot outlive the screen root.

### A simple ownership picture (what owns what)

It helps to keep a single mental diagram in your head:

```
DemoManager
  ├── lv_group_t* group               (shared; lives for app lifetime)
  └── active screen root (lv_obj_t*)  ────────────────┐
                                                       │ (LVGL object tree)
Screen State (heap)                                    │
  ├── lv_obj_t* pointers (children in the tree)         │
  ├── lv_timer_t* timers  ──► callbacks use state + objs│
  └── counters/bookkeeping                              │
                                                       │
LV_EVENT_DELETE(root)  ◄──────── cleanup hook ──────────┘
  ├── lv_timer_del(...)
  └── delete State
```

Key point: **LVGL timers are not in the object tree**. They will happily outlive the screen unless you explicitly delete them.

### Lifecycle invariants (rules that prevent 90% of bugs)

These are the “house rules” implied by the current pattern:

- Every screen has exactly one root created with `lv_obj_create(NULL)` (or equivalent widget root).
- Every repeating timer created by a screen must be deleted when the screen is deleted.
- Timer callbacks must only dereference pointers in their own `State` struct.
- A screen’s `State` must not be reused across multiple screen instances.
- The screen manager may delete old screens asynchronously (`lv_obj_del_async`), so “cleanup when deletion actually happens” matters.
- Cross-screen pointers to `lv_obj_t*` are forbidden unless you can prove lifetime (or you treat them as weak references and validate).

When you stick to these rules, you can screen-switch aggressively without crashes.

### Concrete structure: what our demos look like

The Basics demo:
- has a status timer that updates a label periodically
- must delete that timer when leaving the screen

The Pomodoro demo:
- has a tick timer that updates an arc and a label
- must delete that timer when leaving the screen

Both demos follow the same high-level structure:

```c++
struct DemoState {
  lv_obj_t* root = nullptr;
  lv_timer_t* timer = nullptr;
  lv_obj_t* some_label = nullptr;
};

static void timer_cb(lv_timer_t* t) {
  DemoState* st = (DemoState*)t->user_data;
  lv_label_set_text(st->some_label, "tick"); // only safe while root exists
}

static void root_delete_cb(lv_event_t* e) {
  DemoState* st = (DemoState*)lv_event_get_user_data(e);
  if (st->timer) lv_timer_del(st->timer);
  delete st;
}

lv_obj_t* demo_create() {
  DemoState* st = new DemoState{};
  st->root = lv_obj_create(NULL);
  st->some_label = lv_label_create(st->root);
  st->timer = lv_timer_create(timer_cb, 250, st);
  lv_obj_add_event_cb(st->root, root_delete_cb, LV_EVENT_DELETE, st);
  return st->root;
}
```

### Screen switching: why `LV_EVENT_DELETE` is the right hook

The demo manager deletes old screens asynchronously (`lv_obj_del_async()`).
This is a good practice because it avoids deleting objects in the middle of LVGL internals.

But it also means:
- the code that requests deletion is not the code that observes the exact deletion moment.

Attaching cleanup to the root’s `LV_EVENT_DELETE` callback ensures cleanup runs when deletion actually happens, not when it was requested.

### Why “cleanup-on-delete” is surprisingly ergonomic

This pattern “feels” manual at first, but it has a few properties that scale nicely:

- It is **local**: each demo owns its timers and cleans them up; the manager doesn’t need to know details.
- It is **composable**: adding a second timer is just another field and another `lv_timer_del()` line.
- It is **resilient to refactors**: even if screen switching changes (or other code deletes the screen), the cleanup still runs.
- It matches LVGL’s ownership model instead of trying to pretend timers are object-owned.

### What to watch for (common failure modes)

If you ever see:
- `LoadProhibited` in LVGL internals
- `0xa1b2c3d4` / `0xdeadbeef`-looking pointers
- a backtrace that goes through `lv_timer_handler → lv_timer_exec → your_timer_cb`

Then the first suspicion should be:
- “A timer is still running after its screen was destroyed”

Other common lifecycle bugs:
- **Async delete + stale globals**: `lv_obj_del_async(old)` schedules deletion; globals pointing at `old` become invalid later.
- **State lifetime mismatch**: you free `state` but forget to stop a timer that still holds `state` as `user_data`.
- **Event handler re-entrancy**: switching screens inside an LVGL event callback is often OK, but gets risky as you add transitions/animations.

## Design Decisions

### Use per-screen allocations instead of static `State`

We avoid:

```c++
static PomodoroState P;
P.timer = lv_timer_create(..., &P);
// later: screen deleted but timer still runs and uses P->label pointer -> crash
```

Per-screen state makes it much easier to reason about:
- which pointers are valid,
- when cleanup happens,
- and whether resources can survive a screen switch.

### Treat “timers” as first-class resources

Screen code must assume:
- timers outlive objects unless explicitly deleted
- therefore every screen that creates timers must have a cleanup path

### Cleanup is local to the screen, not the manager

The manager is responsible for switching screens.
Each screen is responsible for cleaning up its own timers/resources.

This keeps ownership consistent and avoids the manager becoming a grab-bag of per-screen teardown hacks.

## Alternatives Considered

### Never delete screens; hide/show instead

Pros:
- avoids deletion-related use-after-free by avoiding deletion entirely

Cons:
- memory grows as you accumulate screens and widgets
- hidden timers still run unless you explicitly pause them
- focus/group handling becomes more complex as multiple screens stay alive

This can work for tiny apps but isn’t ideal for a demo catalog that may grow.

### Use static screen state with “reset” functions

Pros:
- fewer heap allocations

Cons:
- easy to accidentally keep stale `lv_obj_t*` pointers around
- easy to forget timer deletion
- makes re-entrancy and repeated entry/exit brittle

We prefer explicit per-screen state and cleanup-on-delete.

## Implementation Plan

### Already implemented (current baseline)

- Per-screen state allocated on enter
- Cleanup via root `LV_EVENT_DELETE`
- Screen manager uses `lv_scr_load()` + `lv_obj_del_async()`

### Future hardening plan (if we grow beyond a demo)

1. **Switch screens asynchronously** via `lv_async_call` to avoid re-entrancy hazards.
2. Introduce a `ScreenContext` that tracks resources (timers, async jobs, animations) and cleans them up centrally.
3. Add an explicit `destroy()` hook (pre-delete teardown) for screens that need “leave transitions” or persistence.
4. Add a small “lifecycle stress test” mode (enter/exit loop) to catch timer leaks quickly.

Below is what those steps look like in more concrete terms.

### Hardening idea #1: `demo_manager_load_async()` (don’t switch screens inline)

When your app grows, the “when is it safe to switch screens?” question becomes surprisingly thorny. A robust approach is: **never switch screens inline from an LVGL callback**; instead, schedule the switch.

API sketch:

```c++
struct DemoSwitchRequest {
  DemoManager* mgr;
  DemoId id;
};

static void do_switch(void* p) {
  auto* req = static_cast<DemoSwitchRequest*>(p);
  demo_manager_load(req->mgr, req->id);
  lv_mem_free(req); // or delete/free depending on allocator choice
}

void demo_manager_load_async(DemoManager* mgr, DemoId id) {
  auto* req = static_cast<DemoSwitchRequest*>(lv_mem_alloc(sizeof(DemoSwitchRequest)));
  req->mgr = mgr;
  req->id = id;
  lv_async_call(do_switch, req);
}
```

What you get:
- no screen switches during the middle of a widget event handler,
- fewer “it works until we add one more widget” surprises.

### Hardening idea #2: `ScreenContext` for resource tracking

Right now, each screen “remembers” its own timers and deletes them manually. That works, but a `ScreenContext` is a small abstraction that pays off fast:

- It centralizes cleanup logic.
- It makes it harder to forget to track a resource.
- It provides a natural place to add debug checks (e.g. log number of timers on cleanup).

Conceptual API:

```c++
struct ScreenContext {
  lv_obj_t* root = nullptr;
  std::vector<lv_timer_t*> timers;
  std::vector<void*> heap_allocations;    // optional

  void track(lv_timer_t* t) { if (t) timers.push_back(t); }

  void cleanup() {
    for (auto* t : timers) lv_timer_del(t);
    timers.clear();
    for (auto* p : heap_allocations) free(p);
    heap_allocations.clear();
  }
};
```

Then a screen becomes:

```c++
struct MyScreenState {
  ScreenContext ctx;
  lv_obj_t* label = nullptr;
};

static void on_root_delete(lv_event_t* e) {
  auto* st = static_cast<MyScreenState*>(lv_event_get_user_data(e));
  st->ctx.cleanup();
  delete st;
}
```

### Hardening idea #3: explicit `destroy()` hook (two-phase teardown)

`LV_EVENT_DELETE` is a great “last chance cleanup”, but it’s sometimes too late. If you need:
- to stop timers before playing a transition animation,
- to persist state,
- to detach input groups cleanly,

…you want an explicit “leaving screen now” hook:

```c++
struct Screen {
  lv_obj_t* root = nullptr;
  void (*bind_group)(Screen*, lv_group_t*);
  void (*destroy)(Screen*); // stop timers, cancel async work, free state
  void* state = nullptr;
};
```

The manager can do:

```c++
cur.destroy(&cur);           // phase 1 teardown (explicit)
lv_scr_load(next.root);
lv_obj_del_async(cur.root);  // phase 2 deletion (safety net)
```

This makes teardown behavior explicit and testable.

### Hardening idea #4: a “screen transition contract” (boring, but powerful)

Once you have 5–10 screens, bugs start coming from inconsistent assumptions. It helps to write down a contract like:

- Screen creation must be “pure”: it must not assume it is the active screen until after `lv_scr_load()`.
- Screen destruction must be idempotent: calling `destroy()` twice is safe.
- Screen timers must not run after `destroy()` returns.
- Screen switching is always done by the manager (no direct `lv_scr_load()` from inside screens).

It’s not glamorous, but it prevents the “random crash after 20 minutes” class of bug.

### Hardening idea #5: lightweight RAII helpers (optional, C++-only)

If you prefer C++ ergonomics, you can wrap LVGL resources:

```c++
struct LvTimerGuard {
  lv_timer_t* t = nullptr;
  ~LvTimerGuard() { if (t) lv_timer_del(t); }
  LvTimerGuard(const LvTimerGuard&) = delete;
  LvTimerGuard& operator=(const LvTimerGuard&) = delete;
};
```

You still need a reliable moment to destroy the guard (screen teardown), but it reduces the chance of forgetting a `lv_timer_del()`.

## API sketches (a practical “next step” if we grow)

If we wanted a slightly more formal pattern while staying lightweight, this is a good balance:

```c++
struct ScreenVTable {
  lv_obj_t* (*create)(DemoManager* mgr, void** out_state);
  void (*bind_group)(DemoManager* mgr, void* state, lv_group_t* group);
  void (*destroy)(void* state); // stop timers, cancel async work, free state
};

struct ScreenHandle {
  DemoId id;
  lv_obj_t* root;
  void* state;
  const ScreenVTable* vt;
};
```

Manager flow:

```c++
// Build next screen
ScreenHandle next = screens[id].create(...);
// Switch (prefer async in the future)
lv_scr_load(next.root);
// Tear down old screen
old.vt->destroy(old.state);
lv_obj_del_async(old.root);
```

It’s only a small step up from what we do now, but it makes ownership explicit and testable.

## Open Questions

- Should we standardize “Back to menu” as LVGL `LV_KEY_ESC` and document the physical chord (Cardputer: `Fn+\``), or should we add an explicit “Back” action distinct from Esc?
- Do we want to enforce a rule that no screen callback may call `demo_manager_load()` directly (must go through `demo_manager_load_async`)?
- Should we unify allocation (use `lv_mem_alloc/lv_mem_free` vs `new/delete`) for easier tracking?

## References

- Demo manager: `esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_manager.cpp`
- Basics demo cleanup: `esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_basics.cpp`
- Pomodoro demo cleanup: `esp32-s3-m5/0025-cardputer-lvgl-demo/main/demo_pomodoro.cpp`
- Diary crash note: `ttmp/2026/01/01/0025-CARDPUTER-LVGL--cardputer-lvgl-demo-firmware/reference/01-diary.md`
