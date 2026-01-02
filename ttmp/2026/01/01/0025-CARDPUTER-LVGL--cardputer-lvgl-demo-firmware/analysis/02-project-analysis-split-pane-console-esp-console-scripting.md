---
Title: 'Project analysis: Split-pane console + esp_console scripting'
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
    - Path: 0013-atoms3r-gif-console/main/console_repl.cpp
      Note: Canonical esp_console REPL startup + command registration pattern in this repo
    - Path: 0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp
      Note: Manual line-based REPL (no esp_console) and control-plane patterns
    - Path: 0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp
      Note: Screenshot framing + task-based PNG encode/send (reusable command target)
    - Path: 0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp
      Note: Existing split-pane console model (log + input + scrollback) in M5GFX
    - Path: 0025-cardputer-lvgl-demo/main/app_main.cpp
      Note: Main LVGL loop + global key handling insertion point for console/palette
    - Path: 0025-cardputer-lvgl-demo/main/console_repl.cpp
      Note: v0 esp_console command set and CtrlEvent enqueue pattern
    - Path: 0025-cardputer-lvgl-demo/main/control_plane.h
      Note: CtrlType/CtrlEvent contract used to keep LVGL single-threaded
    - Path: 0025-cardputer-lvgl-demo/main/demo_split_console.cpp
      Note: LVGL SplitConsole screen implementation (log+input)
    - Path: 0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp
      Note: KeyEvent->LVGL key translation and feed queue
    - Path: THE_BOOK.md
      Note: High-level rationale and patterns for console systems in this repo
ExternalSources: []
Summary: Analyzes how to add an LVGL split-pane console UI and a host-scriptable esp_console control plane, reusing existing command/queue patterns while keeping LVGL single-threaded.
LastUpdated: 2026-01-02T09:39:23.861596859-05:00
WhatFor: ""
WhenToUse: ""
---



# Project analysis: Split-pane console + `esp_console` scripting

This document is a “read the repo before you build” analysis for a moderately challenging but high-leverage feature set:

1. A **split-pane on-device console UI** (logs on top, command line on bottom) built with LVGL, using the Cardputer keyboard.
2. A **host-scriptable control plane** using ESP-IDF’s `esp_console` so you can drive demos (including screenshot capture) from a serial REPL.

The key constraint is architectural: **LVGL must remain single-threaded**. Anything that touches LVGL objects must run on the LVGL “UI thread” (the `app_main` loop in `0025-cardputer-lvgl-demo/main/app_main.cpp`). Therefore, `esp_console` command handlers should **never directly mutate LVGL state**; they should enqueue a control-plane event consumed by the main loop.

This is exactly the pattern already used elsewhere in the repo (most clearly: `0013-atoms3r-gif-console`).

---

## Why this project is “moderately challenging” and reusable

This project touches multiple reusable subsystems that come up repeatedly in embedded UX work:

- **Command routing** (keys → focused widget vs global shortcuts vs modal overrides)
- **UI composition** (persistent output pane + input pane + scroll behavior)
- **Async boundaries** (serial REPL runs in its own task; UI runs in the LVGL loop)
- **Debuggability** (log visibility, deterministic reproduction via scripting)
- **Binary transport** (screenshot PNG framing over USB-Serial/JTAG)

Once built, the same “console + control plane” scaffolding can power:

- an interactive demo catalog (“open demo pomodoro”, “set duration 15”, “screenshot”)
- a “debug shell” for future Cardputer projects (Wi-Fi, BLE, file browser)
- automated capture / regression scripts (build → flash → run commands → capture screenshot)

---

## What already exists in the repo (relevant files, APIs, and concepts)

### 1) The current LVGL firmware baseline (ticket 0025)

**Files to read first:**

- `0025-cardputer-lvgl-demo/main/app_main.cpp`
  - Owns the main loop:
    - polls `CardputerKeyboard`
    - converts events to LVGL keys
    - calls `lv_timer_handler()`
    - yields via `vTaskDelay(1)`
- `0025-cardputer-lvgl-demo/main/lvgl_port_m5gfx.cpp`
  - LVGL display driver:
    - allocates draw buffers (DMA preferred)
    - flush callback writes RGB565 to `m5gfx::M5GFX`
    - `esp_timer` drives `lv_tick_inc`
- `0025-cardputer-lvgl-demo/main/lvgl_port_cardputer_kb.cpp`
  - LVGL keypad indev adapter:
    - translates `KeyEvent` to `LV_KEY_*` or ASCII
    - pushes keys into a small queue polled by LVGL
- `0025-cardputer-lvgl-demo/main/demo_manager.cpp`
  - Screen switching via `lv_scr_load(next)` + `lv_obj_del_async(old)`
  - Shared `lv_group_t` for keypad focus

**Key concept:** the project is intentionally “LVGL-simple”: a single thread owns LVGL.

### 2) A proven split-pane console model (M5GFX) you can port conceptually

**Files:**

- `0022-cardputer-m5gfx-demo-suite/main/ui_console.h`
- `0022-cardputer-m5gfx-demo-suite/main/ui_console.cpp`

This is *not* LVGL, but it already solves the “console UX” details:

- persistent log lines (`std::deque<std::string> lines`)
- input line (`std::string input`)
- scrollback mode (`scrollback=0 follow tail, >0 user scrolled up`)
- line-wrapping based on font metrics (`g.textWidth(...)`)
- redraw-on-dirty model

**Reusable design idea:** keep the console state as a pure data model, separate from rendering. In LVGL you’ll render to:

- a read-only `lv_textarea` (for output), and
- a one-line `lv_textarea` (for input), or an `lv_label` “prompt + input” line.

### 3) A canonical `esp_console` pattern (commands + REPL startup)

**Files:**

- `0013-atoms3r-gif-console/main/console_repl.h`
- `0013-atoms3r-gif-console/main/console_repl.cpp`

This is the best “how we do esp_console here” example:

- define one function per command:
  - signature: `static int cmd_name(int argc, char **argv)`
- register each via:

```c
esp_console_cmd_t cmd = {};
cmd.command = "list";
cmd.help = "List available GIFs";
cmd.func = &cmd_list;
ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
```

- start a REPL transport via one of:
  - `esp_console_new_repl_uart(...)`
  - `esp_console_new_repl_usb_serial_jtag(...)`
- then:
  - `esp_console_register_help_command()`
  - `esp_console_start_repl(repl)`

**Important design choice in 0013:** commands do not directly “do work”; they enqueue events:

```c++
ctrl_send({.type = CtrlType::PlayIndex, .arg = idx});
```

That lets the real work happen on the main loop, in the right thread/context.

### 4) A manual REPL (no esp_console) that shows “line input + queue” patterns

**Files:**

- `0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp`
- `0016-atoms3r-grove-gpio-signal-tester/main/control_plane.cpp`

Even if we use `esp_console` for host scripting, this code is useful for:

- minimal parsing patterns (tokens, ints, help)
- explicit backpressure / non-blocking IO expectations
- a strong “control plane” separation from “main work loop”

### 5) Screenshot capture over USB-Serial/JTAG (including known pitfalls)

**Files (implementation):**

- `0022-cardputer-m5gfx-demo-suite/main/screenshot_png.h`
- `0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp`
- `0022-cardputer-m5gfx-demo-suite/tools/capture_screenshot_png.py`

**Docs (pitfalls, fixes, why they happened):**

- `ttmp/2026/01/01/0024-B3-SCREENSHOT-WDT--.../analysis/02-postmortem-screenshot-png-over-usb-serial-jtag-caused-wdt-root-cause-fix.md`
- `ttmp/2026/01/01/0026-B3-SCREENSHOT-STACKOVERFLOW--.../analysis/01-bug-report-investigation-stack-overflow-during-createpng-screenshot-send.md`

**Why this matters for this project:** once you have a console/control plane, “screenshot” becomes a natural command. But screenshot has sharp edges:

- `usb_serial_jtag_write_bytes()` can fail or wedge if the driver isn’t installed (WDT bug 0024)
- PNG encoding can overflow the `main` task stack unless you run it in a dedicated task (bug 0026)

So screenshot is a great “moderately challenging” feature because it forces correct task boundaries and correct IO chunking.

### 6) Meta: this repo’s philosophy on console systems

**File:**

- `THE_BOOK.md` (search for “Console Systems”)

This isn’t an API reference, but it explains *why* there are multiple console patterns in the repo (`esp_console` vs manual REPL) and when each is appropriate.

---

## Proposed architecture (high level)

We want two interacting “console” surfaces:

- **On-device console UI (LVGL)**: friendly, always visible, keyboard-first
- **Host scripting REPL (esp_console)**: deterministic automation; can run without touching the on-device UI

They should share:

- the same command set (“open menu”, “open pomodoro”, “set duration”, “screenshot”, “heap”, “help”)
- the same execution model (enqueue events; main loop applies them)

### The golden rule: LVGL mutations only on the LVGL loop

If an `esp_console` command runs in the REPL task and calls `lv_label_set_text()` directly, you will eventually hit one of:

- random LVGL corruption
- “works… until it doesn’t” crashes during screen switches
- deadlocks as LVGL and display flush/timers get interleaved unpredictably

Therefore: **commands must be “pure” and post work to the UI thread**.

### Dataflow diagram

```
                         (USB-Serial/JTAG or UART)
Host scripts  ────────────────► esp_console REPL task
  (python, etc)                    │
                                   │ calls command handler
                                   ▼
                            enqueue CtrlEvent (FreeRTOS queue)
                                   │
                                   ▼
                         app_main LVGL loop (single thread)
                          ├─ polls Cardputer keyboard
                          ├─ feeds LVGL indev
                          ├─ drains CtrlEvent queue
                          │    └─ mutates demo manager / LVGL widgets
                          └─ lv_timer_handler + vTaskDelay
```

If we later want the on-device console input line to execute commands too, it should:

1. parse a line into a `CtrlEvent` (or a “RunCommand” event), and
2. enqueue it into the same queue consumed by `app_main`.

---

## Concrete building blocks to implement (what we should add to 0025)

This section is intentionally “module-level” so it’s actionable and reviewable.

### A) A control-plane event queue (shared by esp_console + UI)

**Concept:** model commands as explicit events so we don’t call LVGL from random contexts.

Example:

```c++
enum class CtrlType {
  Help,
  OpenMenu,
  OpenBasics,
  OpenPomodoro,
  PomodoroSetMinutes,
  ScreenshotPngToSerial,
  // later: SdList, SdCat, SdOpen, ...
};

struct CtrlEvent {
  CtrlType type;
  int arg = 0;              // e.g. minutes
  char text[64] = {0};      // e.g. filename, optional
};
```

Implementation detail: use a FreeRTOS queue:

```c
QueueHandle_t q = xQueueCreate(/*len=*/16, sizeof(CtrlEvent));
bool ctrl_send(const CtrlEvent* e) { return xQueueSend(q, e, 0) == pdTRUE; }
bool ctrl_recv(CtrlEvent* out) { return xQueueReceive(q, out, 0) == pdTRUE; }
```

### B) Command handlers (one registry, two front-ends)

Two ways to reuse command definitions:

1. **“esp_console is the source of truth”**
   - register commands via `esp_console_cmd_register`
   - have LVGL input call `esp_console_run(line, &ret)` to execute them
   - requires thinking about output capture (stdout/printf) and reentrancy

2. **“our registry is the source of truth”** (recommended)
   - define a small table of commands in our app (`name`, `help`, `fn(argc,argv)`)
   - *adapt* that table into:
     - `esp_console_cmd_t` registrations, and
     - the on-device console (autocomplete/history could be simpler)

The second approach keeps LVGL predictable and makes it easy to surface commands in a command palette later.

### C) LVGL split-pane console UI (log + input)

For LVGL, the simplest, stable UI is:

- `lv_textarea` output:
  - read-only
  - multiline
  - scrollable
- `lv_textarea` input:
  - one-line
  - captures typed characters
  - Enter triggers “submit”

Sketch:

```c
lv_obj_t* out = lv_textarea_create(root);
lv_textarea_set_one_line(out, false);
lv_textarea_set_text_selection(out, false);
lv_obj_set_size(out, 240, 135 - input_h);

lv_obj_t* in = lv_textarea_create(root);
lv_textarea_set_one_line(in, true);
lv_textarea_set_placeholder_text(in, "> ");
lv_obj_align(in, LV_ALIGN_BOTTOM_MID, 0, 0);
```

When submitting:

- take input string
- append to output (echo)
- parse into CtrlEvent and enqueue
- clear input
- scroll output to bottom (unless user has scrolled up)

Scrollback model (port from `ui_console.cpp`):

- if the user scrolls up, freeze auto-scroll
- if the user scrolls to bottom, re-enable follow-tail

### D) Output/log routing options (what to do with printf/ESP_LOG*)

This is the “hard part” if we want the on-device console to mirror logs.

Options:

- **Minimal (ship first):** only show “console output” (responses to commands), not global logs.
- **Better:** add an in-app log sink:
  - for LVGL display: append formatted lines to output textarea
  - for serial: still use `ESP_LOGI` / `printf`
- **Full:** route ESP-IDF logging into the on-device console by installing a custom `vprintf`:
  - `esp_log_set_vprintf(my_vprintf)`
  - write to both UART/USB and the UI buffer
  - must be careful about reentrancy/locking/allocations

Given this ticket’s scope, “Minimal” is likely the right starting point, and can be extended after the control plane is stable.

---

## API signatures and “cheat sheet”

This section is deliberately concrete so a new developer can implement without guessing.

### ESP-IDF `esp_console` (from usage in `0013-atoms3r-gif-console`)

Command registration (pattern):

```c
typedef struct {
  const char* command;
  const char* help;
  const char* hint;
  int (*func)(int argc, char** argv);
  void* argtable;
} esp_console_cmd_t;

esp_err_t esp_console_cmd_register(const esp_console_cmd_t* cmd);
void esp_console_register_help_command(void);
```

REPL creation + start (USB-Serial/JTAG example):

```c
typedef struct esp_console_repl_s esp_console_repl_t;
typedef struct { const char* prompt; /* ... */ } esp_console_repl_config_t;

esp_console_repl_config_t ESP_CONSOLE_REPL_CONFIG_DEFAULT(void);

typedef struct { /* ... */ } esp_console_dev_usb_serial_jtag_config_t;
esp_console_dev_usb_serial_jtag_config_t ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT(void);

esp_err_t esp_console_new_repl_usb_serial_jtag(
  const esp_console_dev_usb_serial_jtag_config_t* dev_cfg,
  const esp_console_repl_config_t* repl_cfg,
  esp_console_repl_t** out_repl
);

esp_err_t esp_console_start_repl(esp_console_repl_t* repl);
```

Command execution API (useful if we want LVGL input to call into esp_console):

```c
// Executes a command line as if typed in the console.
// Returns ESP_OK on parse/dispatch success (not the command's exit code).
esp_err_t esp_console_run(const char* cmdline, int* out_cmd_ret);
```

### LVGL keyboard + focus (already used in 0025)

Key event routing depends on focus group setup:

```c
lv_group_t* lv_group_create(void);
void lv_group_set_default(lv_group_t* g);
void lv_group_set_wrap(lv_group_t* g, bool en);
void lv_indev_set_group(lv_indev_t* indev, lv_group_t* g);
void lv_group_add_obj(lv_group_t* g, lv_obj_t* obj);
void lv_group_focus_obj(lv_obj_t* obj);
```

Key events are received via:

```c
lv_obj_add_event_cb(obj, cb, LV_EVENT_KEY, user_data);
uint32_t lv_event_get_key(lv_event_t* e);
```

### USB-Serial/JTAG write semantics (relevant for screenshot)

From the repo’s experience (tickets 0024/0026):

- `usb_serial_jtag_write_bytes(...)` can return:
  - `< 0` (fatal; driver not installed / error)
  - `0` (timeout / couldn’t enqueue)
  - `> 0` (bytes enqueued)
- Large payloads must be **chunked**
- Screenshot PNG encode should run in a **dedicated task** (stack budget)

---

## How this fits into the 0025 demo catalog (screen architecture impacts)

The existing `DemoManager` + “screen-local state” pattern (see `ttmp/.../design-doc/02-screen-state-lifecycle-pattern...`) is compatible with a console screen.

Where it intersects with a control plane:

- Commands that switch screens can call `demo_manager_load(...)`, but **only from the main LVGL loop**.
- Commands that need a result (“what demo is active?”) should:
  - either print from the main loop (by enqueuing a “respond” event), or
  - update a shared “response buffer” consumed by the REPL.

If we also add a command palette overlay later, the console command registry becomes the natural action list.

---

## For a new developer: “mental model” of the moving parts

### The three loops you must keep distinct

1. **Keyboard scan loop** (our code):
   - `CardputerKeyboard::poll()` returns edge events (`KeyEvent`)
2. **LVGL UI loop** (our code):
   - consumes input events
   - calls `lv_timer_handler()`
3. **esp_console REPL loop** (ESP-IDF component):
   - runs in its own task
   - reads serial input, parses a line, runs a handler

Only (2) is allowed to mutate LVGL.

### Where to add new features safely

- Want a new command? Implement `cmd_xxx(argc,argv)` that **enqueues a CtrlEvent**.
- Want that command accessible from:
  - host scripting? register it with `esp_console_cmd_register`
  - on-device console? add it to your local registry and show it in LVGL UI
- Want the command to update UI? handle the CtrlEvent in the main loop and update widgets there.

### Common failure modes (and how this repo already hit them)

- **WDT wedge during binary send**: infinite retry loop when USB driver missing (ticket 0024)
- **Stack overflow during PNG encode**: doing `createPng` on the `main` task (ticket 0026)
- **Use-after-free on screen switch**: LVGL timers outlive screens unless cleaned up (design-doc 02)

When in doubt, look for:

- “Do we need a separate task with a bigger stack?”
- “Should this be an event queued to the UI thread?”
- “Do we need bounded retries + yields?”

---

## Implementation checklist (pragmatic, in order)

1. Add `CtrlEvent` queue module (shared).
2. Add a minimal command registry:
   - `help`
   - `menu|basics|pomodoro`
   - `screenshot`
3. Start `esp_console` REPL on the chosen transport (USB-Serial/JTAG is easiest for Cardputer).
4. In `app_main`, drain the queue each frame and apply changes via existing `DemoManager`.
5. Add an LVGL console screen (optional at first; REPL alone already pays off).
6. Add screenshot command using the proven implementation from `0022-cardputer-m5gfx-demo-suite/main/screenshot_png.cpp` (task-based, chunked, framed).

---

## v0 command set + expected outputs (implemented)

This section is the “stable scripting surface” for v0. Commands should remain safe to call from the REPL task because they enqueue `CtrlEvent`s and do not touch LVGL directly.

- `help`
  - Expected: ESP-IDF `esp_console` help output listing registered commands.
- `heap`
  - Expected: `heap_free=<bytes>`
- `menu`
  - Expected: `OK`
- `basics`
  - Expected: `OK`
- `pomodoro`
  - Expected: `OK`
- `console`
  - Expected: `OK`
- `sysmon`
  - Expected: `OK`
- `setmins <minutes>`
  - Expected: `OK minutes=<n>` (where `<n>` is clamped to `1..99`)
  - Errors:
    - `ERR: usage: setmins <minutes>`
    - `ERR: invalid minutes: <arg>`
- `screenshot`
  - Expected: a framed PNG stream followed by a status line:
    - `PNG_BEGIN <len>\n` + `<len>` bytes + `\nPNG_END\n`
    - `OK len=<len>`
  - Errors:
    - `ERR: screenshot busy (queue full)`
    - `ERR: screenshot timeout`
    - `ERR: screenshot failed`
