---
Title: 'Pattern: esp_event-centric architecture (C and C++)'
Ticket: 0030-CARDPUTER-CONSOLE-EVENTBUS
Status: complete
Topics:
    - cardputer
    - keyboard
    - console
    - esp-idf
    - esp32s3
    - display
    - esp-event
    - ui
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0030-cardputer-console-eventbus/main/app_main.cpp
      Note: Source material for the extracted patterns and candidate modules.
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-05T08:03:17.916351029-05:00
WhatFor: ""
WhenToUse: ""
---


# Pattern: esp_event-centric architecture (C and C++)

This document extracts the core design pattern from `0030-cardputer-console-eventbus` and shows how to package it into reusable modules/components for other firmwares. It covers both a C-style “module API” and a C++ RAII/class-based approach.

## What 0030 is doing today (quick summary)

The firmware is small but it already demonstrates the recurring structure we want:

- **Internal bus**: a user `esp_event` loop is the “spine” of the system.
- **Many producers**: keyboard task, console task, heartbeat, random producers all post typed events.
- **One display owner**: the UI task calls `esp_event_loop_run()` and renders the UI.
- **Observation**: events are shown on screen; optionally mirrored to console via `evt monitor on|off`.

Single-file implementation: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`

## Design goals for a reusable pattern

1. **Make event definitions explicit** (base + IDs + payload structs).
2. **Keep producers dumb** (construct payload, post to bus; no UI logic).
3. **Keep handlers fast** (format/log small things; push heavy work to tasks).
4. **Centralize ownership** of shared resources (display, network, storage).
5. **Standardize observability** (screen log + optional console monitor + drop counters).

## Reusable units (component candidates)

From 0030 we can extract these as generic building blocks:

1. **Event loop wrapper**: create/destroy a user loop, expose `post()` helpers, queue sizing, and a “dispatch in UI thread” mode.
2. **Event log**:
   - bounded in-memory ring buffer of strings
   - optional pause/scrollback controls
   - “append line” API for handlers
3. **Console command registrar**:
   - a small helper that registers commands and starts REPL on configured backend
4. **Event monitor**:
   - togglable mirroring of bus traffic to console without blocking handlers
5. **Drop accounting**:
   - per-producer post failures and per-monitor-queue drops

The “keyboard scanner” itself is already a reusable component: `components/cardputer_kb/*`.

## C-style packaging (recommended for cross-project reuse)

### Module layout

Create modules with narrow APIs and explicit ownership:

- `bus.h/.c` (or `bus.hpp/.cpp` but expose C API)
  - creates the `esp_event_loop_handle_t`
  - exports an `esp_event_base_t` and ID enums
  - exports `bus_post_*()` helpers
- `event_log.h/.c`
  - ring buffer, `event_log_append()`, `event_log_clear()`, `event_log_render()`
- `console.h/.c`
  - registers console commands and wires them to bus posts
- `monitor.h/.c`
  - receives “formatted event lines” via a queue and prints to console when enabled

### Example API sketch

```c
// bus.h
ESP_EVENT_DECLARE_BASE(APP_BUS);

typedef enum {
  APP_EVT_KB_KEY = 1,
  APP_EVT_HEARTBEAT,
  APP_EVT_CONSOLE_POST,
} app_evt_id_t;

typedef struct { int64_t ts_us; uint8_t keynum; uint8_t mods; } app_kb_key_t;
typedef struct { int64_t ts_us; uint32_t heap_free; } app_hb_t;

esp_err_t app_bus_init(size_t queue_size);
esp_event_loop_handle_t app_bus_loop(void);
esp_err_t app_bus_post_kb_key(const app_kb_key_t* ev);
```

Key points:
- Only the bus module “owns” the loop handle.
- Producers call `app_bus_post_*()` which guarantees payload sizes/types.
- Handlers register via `esp_event_handler_register_with(app_bus_loop(), APP_BUS, ...)`.

## C++ packaging (RAII + type safety)

C++ makes it easier to avoid lifecycle bugs:

### Recommended classes

- `EventLoop` (RAII for `esp_event_loop_handle_t`)
  - `EventLoop::CreateUserLoop(args)`
  - `post(base, id, payload)`
- `HandlerRegistration` (RAII)
  - registers in ctor and unregisters in dtor
- `EventLog`
  - `append(std::string)`
  - bounded storage + scrollback state
- `ConsoleRepl`
  - starts REPL on configured backend
  - registers commands from modules
- `EventMonitor`
  - queue + consumer task
  - `set_enabled(bool)`; drop counters

### Example class sketch

```cpp
class EventLoop {
public:
  static esp_err_t CreateUserLoop(const esp_event_loop_args_t& args, EventLoop* out);
  esp_event_loop_handle_t handle() const;

  template <typename T>
  esp_err_t Post(esp_event_base_t base, int32_t id, const T& payload, TickType_t to = 0) {
    return esp_event_post_to(handle_, base, id, &payload, sizeof(T), to);
  }
private:
  esp_event_loop_handle_t handle_{};
};
```

Key points:
- Use typed `Post<T>` to avoid wrong `sizeof()` calls.
- Hide raw handles behind a thin wrapper; centralize creation/destruction.

## A design pattern that scales: “UI thread is the dispatcher”

0030 (and 0028) use an important pattern for display-heavy embedded apps:

- Create a user loop with `task_name = NULL`.
- The UI loop calls `esp_event_loop_run(loop, ticks)` once per frame (or per iteration).

Benefits:
- No other task runs handlers that touch UI state.
- You can safely update UI state inside event handlers because the handler runs in the UI loop.

This pattern generalizes to other “single-owner resources”:
- a network stack wrapper can similarly centralize access or forward into a control task
- a storage subsystem can serialize NVS writes via one worker task and receive events

## Observability as a first-class module

For event-driven systems, observability should be as reusable as the bus itself:

- **On-screen event log** (for demos and debugging without a PC).
- **Console event monitor** (for deep debugging and CI logs).
- **Drop counters** (queue full, post failures) surfaced in both places.

In 0030, `evt monitor on` mirrors events to console without printing inside the event handler; instead, the handler enqueues a line to a queue that a monitor task prints.

## What we would extract into a generic component next

If we wanted other firmwares to reuse this immediately, a good component boundary would be:

- `components/event_bus_core/`
  - `event_loop.[ch]` (create loop, typed post helpers)
  - `event_monitor.[ch]` (toggleable monitor with queue + task)
  - `console_backend.[ch]` (start REPL over whichever backend is configured)

Then each firmware defines:
- its event base + IDs + payloads
- its producers
- its handlers/UI

This keeps the generic component free of app-specific event IDs and payload structs, while still providing the “hard parts” (lifecycle, observability, console backend selection).

## Pointers to related code

- 0030 implementation: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0030-cardputer-console-eventbus/main/app_main.cpp`
- 0028 baseline demo (no console): `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0028-cardputer-esp-event-demo/main/app_main.cpp`
- esp_console REPL example pattern: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0020-cardputer-ble-keyboard-host/main/bt_console.c`
- keyboard component: `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/cardputer_kb/README.md`
