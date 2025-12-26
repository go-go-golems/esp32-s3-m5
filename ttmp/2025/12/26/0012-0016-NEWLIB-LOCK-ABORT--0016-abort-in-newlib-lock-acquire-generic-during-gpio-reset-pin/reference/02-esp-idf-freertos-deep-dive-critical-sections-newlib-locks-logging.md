---
Title: ESP-IDF + FreeRTOS deep dive (critical sections, newlib locks, logging)
Ticket: 0012-0016-NEWLIB-LOCK-ABORT
Status: active
Topics:
    - atoms3r
    - debugging
    - esp-idf
    - esp32s3
    - gpio
    - usb-serial-jtag
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/esp/esp-idf-5.4.1/components/newlib/locks.c
      Note: Where lock_acquire_generic() decides “ISR/critical context” via xPortCanYield() and can abort on recursive locks
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp
      Note: The bug/fix hotspot: critical section usage + applying GPIO config + status printing
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_rx.cpp
      Note: GPIO ISR edge counting + safe ISR-only behavior (atomics only)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/gpio_tx.cpp
      Note: TX uses esp_timer callbacks; shows “task context but time-sensitive” constraints
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/control_plane.cpp
      Note: Queue-based event bus (CtrlEvent) bridging REPL/UI and the single-writer state machine
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/manual_repl.cpp
      Note: Manual REPL task that feeds CtrlEvent; important for “what context are we in?”
ExternalSources: []
Summary: "Didactic deep dive for ESP-IDF 5.4.1 + FreeRTOS: task vs ISR vs critical section context, why xPortCanYield() matters, and how newlib/stdio/logging interacts with locks—grounded in the 0016 crash and fix."
LastUpdated: 2025-12-26T08:38:31.618893518-05:00
WhatFor: "Help debug and design firmware that mixes GPIO/ISRs/timers/logging without tripping FreeRTOS/newlib lock constraints (esp. avoid libc/logging in ISR/critical)."
WhenToUse: "When you see lock_acquire_generic aborts, weird logging deadlocks, or need to reason about what’s safe to do in esp_timer callbacks/ISRs/critical sections in ESP-IDF."
---

# ESP-IDF + FreeRTOS deep dive (critical sections, newlib locks, logging)

## Goal

Explain the ESP-IDF + FreeRTOS execution model and the “hidden” interactions between:

- **critical sections** (`portENTER_CRITICAL`, `portMUX_TYPE`)
- **task vs ISR context** (and the subtle “looks like a task, but can’t yield” case)
- **newlib locks** (stdio, `vprintf`, recursive mutexes)
- **ESP-IDF logging** (`ESP_LOG*` → `esp_log_writev` → `vprintf`)

…with a concrete case study: the `0016` signal tester boot-time abort in `lock_acquire_generic()` and why moving GPIO/log/printf outside a critical section fixes it.

## Context

This document is intentionally “didactic”: it teaches the mental model first, then anchors it in real code from this repo.

The motivating failure is a classic embedded gotcha: **you can be in “task code” but still be treated like ISR context** if you are inside a critical section where the system can’t yield. If you then call something that needs a lock (stdio/logging), the runtime may `abort()` or deadlock.

In our case, the crash backtrace shows:

- `gpio_reset_pin()` triggers driver/logging
- logging triggers `vprintf()` (newlib)
- newlib tries to take a recursive lock
- ESP-IDF decides we’re in “ISR/critical” because `xPortCanYield()` is false
- newlib aborts: “recursive mutexes make no sense in ISR context”

This is a *design constraint*, not a fluke: the safe design is to keep critical sections small and never call drivers or libc/stdio inside them.

## Quick Reference

### The 10-second rule of thumb

- **Never** call these while holding `portENTER_CRITICAL`:
  - `ESP_LOG*`
  - `printf` / `puts` / `vprintf`
  - GPIO driver calls that might log (and many drivers do)
  - anything that might allocate / lock / block / yield

If you need to change shared state + apply a hardware config:

- **Copy state under lock**
- **Apply hardware config outside lock**

### “Context” taxonomy (ESP-IDF + FreeRTOS)

Think in three buckets:

- **Task context (can yield)**:
  - typical FreeRTOS task execution
  - can take mutexes, can block, libc locks work
- **ISR context (cannot yield)**:
  - GPIO ISR, UART ISR, etc.
  - must not block; must not use libc/stdio
  - logging is dangerous; driver APIs generally unsafe
- **Critical section (cannot yield)**:
  - even though you’re *in a task*, you are temporarily “ISR-like”
  - `xPortCanYield()` is false during the critical section
  - newlib lock layer treats this as ISR context

### The specific abort condition (ESP-IDF 5.4.1 newlib locks)

In `locks.c`, newlib uses `xPortCanYield()` to decide if it can do normal locking:

- If **can’t yield**, it uses `xSemaphoreTakeFromISR()` and refuses recursive locks, and can abort.

Practical reading:

- If you call `vprintf()`/logging while in a critical section, you can trip the abort path.

### “Single-writer state machine” pattern (safe)

When you have:

- an ISR producing stats
- a UI task rendering
- a REPL/control task sending commands
- a central “state machine” applying hardware config

…use:

- **an event queue** (`QueueHandle_t`, `xQueueSend`, `xQueueReceive`)
- **a single writer** (only one task mutates the “truth” state)
- **small critical sections** only for reading snapshots (copying state)

## Deep Dive

### 1) What `app_main()` really means in ESP-IDF

ESP-IDF starts a FreeRTOS task called `main_task`, then calls your `app_main()` inside it. That means:

- you are in **task context** from the start of `app_main()`
- but you can still enter ISR-like constraints if you:
  - run inside a critical section, or
  - call APIs from actual ISRs / timer callbacks

### 2) FreeRTOS critical sections on ESP32 (portMUX)

ESP-IDF exposes:

- `portMUX_TYPE` as a lightweight spinlock / critical primitive
- `portENTER_CRITICAL(&mux)` / `portEXIT_CRITICAL(&mux)`

What this *means in practice*:

- interrupts are disabled / masked (implementation depends on port)
- the running code should be extremely short and non-blocking
- many “normal” functions become unsafe because they may take locks or call into subsystems that assume they can yield

### 3) The subtle killer: `xPortCanYield()`

ESP-IDF uses `xPortCanYield()` to ask: “is it legal to block/yield right now?”

- In ISRs: false
- In critical sections: false

So even though you’re in a task, inside a critical section you effectively become “ISR-like” from the perspective of locking libraries.

### 4) newlib locks: why stdio is not “just printing”

On embedded targets, newlib’s stdio uses locks to protect global state:

- `printf` → `vprintf` → `_vfprintf_r`
- these functions take newlib locks (often recursive) because stdio can call itself and because multiple threads may print

In ESP-IDF 5.4.1, newlib’s lock layer is implemented using FreeRTOS mutex semaphores.

When newlib tries to acquire a lock, it calls into:

- `lock_acquire_generic(_lock_t *lock, delay, mutex_type)`

and **if `xPortCanYield()` is false**, it assumes ISR/critical context.

The important consequence:

- recursive mutex locks don’t work there → abort

### 5) ESP-IDF logging is a stdio call chain

It’s easy to assume `ESP_LOGI` is “safe-ish”, but it ultimately prints formatted strings:

```text
ESP_LOGI
  -> esp_log_write
    -> esp_log_writev
      -> vprintf
        -> newlib locks
```

So: if you call `ESP_LOGI` in ISR/critical context, you can hit the same lock abort.

### 6) How the 0016 crash happened (diagram)

This is the real failure we saw on AtomS3R boot.

```text
app_main (task)
  |
  +-- tester_state_init()
        |
        +-- portENTER_CRITICAL(&s_state_mux)   <-- cannot yield now
        |
        +-- apply_config_locked()
              |
              +-- safe_reset_pin()
                    |
                    +-- gpio_reset_pin()
                          |
                          +-- (driver internals may log)
                                |
                                +-- ESP_LOG* -> vprintf -> newlib lock
                                      |
                                      +-- lock_acquire_generic()
                                            |
                                            +-- sees xPortCanYield()==false
                                            +-- abort()
```

Two independent footguns existed:

- GPIO driver calls inside critical section
- `printf()` inside critical section (`print_status_locked`)

Either can trigger newlib locking while `xPortCanYield()` is false.

### 7) The correct fix pattern (pseudocode)

The pattern used to fix `0016/main/signal_state.cpp` is:

```text
function handle_event(ev):
  bool needs_apply = false
  bool needs_status = false

  enter_critical(mux)
    mutate_state(ev)
    snapshot = copy(state)
    if ev implies apply: needs_apply = true
    if ev implies status: needs_status = true
  exit_critical(mux)

  // Now safe: can yield, can lock, can log, can call drivers
  if ev == RxReset: gpio_rx_reset()
  if needs_apply: apply_config(snapshot)
  if needs_status: print_status(snapshot)
```

Key idea: critical section is only for the shared `state` object; “doing things” happens after.

### 8) API cheat sheet (high-signal)

**FreeRTOS**

- `xQueueCreate(len, item_size)`
- `xQueueSend(queue, &item, ticks_to_wait)` (use 0 in ISR-like contexts)
- `xQueueReceive(queue, &item, ticks_to_wait)`
- `xTaskCreatePinnedToCore(fn, name, stack, arg, prio, &handle, core)`
- `xTaskGetTickCountFromISR()` (ISR-safe timebase)

**ESP-IDF time / timers**

- `esp_timer_get_time()` (microseconds; not always ISR-safe depending on usage—treat carefully)
- `esp_timer_start_periodic(handle, period_us)` (callbacks run in timer task by default, not in hard ISR)

**GPIO**

- `gpio_reset_pin(gpio_num_t)`
- `gpio_set_direction(gpio_num_t, gpio_mode_t)`
- `gpio_set_intr_type(gpio_num_t, gpio_int_type_t)`
- `gpio_install_isr_service(flags)`
- `gpio_isr_handler_add(pin, handler, arg)` (handler is ISR context)

**Logging**

- `ESP_LOGI(TAG, "...")` → formatted output using stdio locks
- Treat as unsafe in ISR/critical sections

### 9) Practical debugging checklist for “lock_acquire_generic abort”

When you see:

- abort in `newlib/locks.c: lock_acquire_generic`

Ask:

- Did I call `printf`/logging from:
  - a GPIO ISR?
  - a critical section (`portENTER_CRITICAL`)?
  - an esp_timer callback (still task context, usually ok, but check whether you entered a critical section inside it)?
- Did I call a driver function that *itself* logs while I’m in a critical section?
- Do I have nested locks (e.g., logging while holding my own mutex/critical), causing recursion?

Then fix by:

- moving logging/stdio out of critical/ISR contexts
- reducing the scope/duration of critical sections
- using “snapshot + apply outside” patterns

## Usage Examples

### Example 1: safe “single-writer + snapshot” architecture (0016)

The 0016 project is a good teaching example because it separates:

- **control events** (REPL) → queue (`CtrlEvent`)
- **single writer** (state machine applies config)
- **presentation** (LCD UI task reads snapshots)
- **ISR stats** (RX edge counter uses atomics only)

Diagram:

```text
manual_repl task ---> CtrlEvent queue ---> app_main loop ---> tester_state_apply_event()
     |                                                      |
     |                                                      +--> apply_config(snapshot) (GPIO driver calls)
     |
lcd_ui task <--- tester_state_snapshot() (copy under lock) ---+

gpio_rx ISR ---> atomic counters ---> gpio_rx_snapshot()
```

Where to look:

- `main/control_plane.cpp`: `ctrl_init()`, `ctrl_send()`
- `main/manual_repl.cpp`: token parsing → `ctrl_send(...)`
- `main/signal_state.cpp`: `tester_state_apply_event`, `tester_state_snapshot`
- `main/gpio_rx.cpp`: ISR-only increments + snapshots

### Example 2: “don’t printf under lock”

Bad:

```text
enter_critical(mux)
  printf("...")     // can take newlib locks; can abort if xPortCanYield() is false
exit_critical(mux)
```

Good:

```text
enter_critical(mux)
  snapshot = copy(state)
exit_critical(mux)
printf("... snapshot ...")
```

## Related

- Bug report (0012): `../analysis/01-bug-report-analysis-lock-acquire-generic-abort-on-boot.md`
- Ticket index: `../index.md`
- 0016 project: `../../../../../../0016-atoms3r-grove-gpio-signal-tester/`

### Key symbols/files (copy/paste paths)

- `/home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0016-atoms3r-grove-gpio-signal-tester/main/signal_state.cpp`
  - `tester_state_init`
  - `tester_state_apply_event`
  - `tester_state_snapshot`
- `/home/manuel/esp/esp-idf-5.4.1/components/newlib/locks.c`
  - `lock_acquire_generic`

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
