---
Title: 'MicroQuickJS + FreeRTOS: multi-VM / multi-task architecture + communication brainstorm'
Ticket: 0014-CARDPUTER-JS
Status: active
Topics:
    - esp32s3
    - esp-idf
    - cardputer
    - javascript
    - microquickjs
    - qemu
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2025-12-29T13:56:08.795278822-05:00
WhatFor: ""
WhenToUse: ""
---

# MicroQuickJS + FreeRTOS: multi-VM / multi-task architecture + communication brainstorm

## Executive Summary

We want to run MicroQuickJS (“mquickjs”) on ESP32-S3/Cardputer in a way that scales beyond a simple serial REPL: multiple scripts, background tasks, and safe cross-component communication.

MicroQuickJS is **not thread-safe** in the “share a context across tasks” sense, so the core recommendation is:

- **One VM (`JSContext`) is owned by exactly one FreeRTOS task.**
- Any other task interacts with that VM via **message passing** (queues/ringbuffers), never by calling MicroQuickJS APIs directly.

From there, we have two viable directions:

- **Single-VM / multi-script**: one JS context, one event loop, multiple scripts scheduled cooperatively.
- **Multi-VM / multi-task**: multiple JS contexts, each in its own FreeRTOS task, communicating via a C-level message bus.

This doc captures the analysis and a concrete design sketch for both, with a bias toward correctness and debuggability on constrained hardware (512KB SRAM, no PSRAM).

## Problem Statement

Our legacy example firmware (`imports/esp32-mqjs-repl/mqjs-repl/legacy/main.c`) is a blocking REPL:

- It creates a `JSContext` and then runs a `repl_task()` that blocks on UART reads.
- It can evaluate JS expressions interactively, and it can load JS files from SPIFFS at startup.

What it does *not* yet support:

- Running JS “in the background” (timers, polling, event loop).
- Running multiple scripts concurrently (or even cooperatively).
- Running multiple VM instances in different tasks safely.
- A clean communication model between:
  - JS ↔ ESP-IDF subsystems (WiFi, BT, I2C, GPIO, display, keyboard, audio),
  - JS ↔ JS (script-to-script),
  - JS ↔ non-JS FreeRTOS tasks.

We need an architecture that:

- Prevents VM corruption (single-threaded VM ownership).
- Supports event-driven IO and “async-ish” APIs without Promises (ES5.1 engine).
- Fits within Cardputer’s memory constraints.
- Is testable (in QEMU where possible, on device where required).

## Proposed Solution

### 1) Adopt an explicit “VM host” layer

Introduce a small C module (conceptually `mqjs_vm_host.*`) that owns:

- A `JSContext *ctx`
- The JS memory pool buffer (per VM)
- A FreeRTOS task that runs the VM loop
- An inbound message queue (and optionally an outbound queue)
- Stored callback references (`JSGCRef`) for event handlers

The host layer provides a strict API boundary:

- Other tasks may call:
  - `mqjs_vm_post(vm, msg)` (enqueue message)
  - `mqjs_vm_eval(vm, code, …)` (enqueue eval request)
- Other tasks may *not* call `JS_*()` directly.

### 2) Make the JS task event-driven (not “block forever on UART”)

Replace “REPL task blocks on UART read forever” with a loop that interleaves:

- console input (USB Serial JTAG on Cardputer)
- inbound message queue draining
- timers (optional)
- periodic GC / watchdog feeding (optional)

Sketch:

```c
for (;;) {
  // 1) drain messages (bounded work)
  // 2) service console input (non-blocking read, or short timeout)
  // 3) run timers / scheduled callbacks
  // 4) optionally JS_GC(ctx) based on pressure
}
```

This is the enabling move for “background JS”.

### 3) Communication: message passing + callback delivery inside the owning JS task

Core concept:

- **Messages are pure data at the FreeRTOS boundary.**
- Only the JS task turns those messages into JS objects and invokes JS callbacks.

Recommended JS-facing API:

- `ESP.onMessage(fn)` / `ESP.offMessage(fn)` (or per-channel: `ESP.on("wifi", fn)`)
- `ESP.postMessage(msg)` (sends to C bus; can route to other VMs/tasks)

Implementation concept:

- Store callback `fn` in a `JSGCRef` so GC can’t collect it.
- On inbound message:
  - Create a JS object representing the message (or a string payload)
  - Invoke callback via `JS_PushArg` + `JS_Call`

### 4) Multi-VM option: “N VMs, N tasks” with a shared C message bus

When you truly need concurrency or isolation:

- Each VM has:
  - its own memory pool (e.g. 64KB each)
  - its own inbound queue
  - its own owning task
- A central bus task (or just direct queue routing) forwards messages between VMs and system tasks.

This is heavier in RAM and stacks but gives:

- fault isolation (one script can OOM without killing others)
- per-VM policy (different heaps, different priorities)

### 5) Script loading model (pragmatic)

At startup:

- Load “boot script(s)” from SPIFFS/FATFS (`/spiffs/autoload/*.js` pattern already exists).

At runtime:

- Prefer “message-driven APIs” over `load()` to avoid filesystem IO from arbitrary scripts.
  - If we *do* expose `load()`, implement it as a C binding that reads from VFS and calls `JS_Eval()`.

## Design Decisions

### Decision: single-threaded ownership per `JSContext`

- **Decision**: treat a `JSContext` as single-thread-owned.
- **Why**: MicroQuickJS has no API contracts for multi-threaded access, and the VM uses internal stacks and state that are not protected.
- **Implication**: all cross-component interactions must be messages/events.

### Decision: callbacks are invoked only from the owning JS task

- **Decision**: even if another FreeRTOS task receives an IO event, it must enqueue that event to the JS task instead of calling JS directly.
- **Why**: prevents reentrancy and eliminates subtle “sometimes it works” corruption.
- **Tooling**: `JSGCRef` for stored callbacks; `JS_StackCheck` before calling.

### Decision: message format is intentionally boring

For constrained memory and ease of debugging:

- Start with `char *` (string payload) or a small fixed struct + payload bytes.
- Convert to a JS object inside the JS task.

If we want richer payloads:

- Option A: ship JSON strings (requires `JSON.parse` in stdlib)
- Option B: ship “tagged values” (type enum + numeric fields)

### Decision: prefer single-VM for early Cardputer bring-up

- Cardputer has tight SRAM; multiple 64KB pools + task stacks can add up quickly.
- A single VM with an event loop will unlock most user-facing scenarios first.

## Alternatives Considered

### Alternative: share one `JSContext` across tasks with a mutex

- **Rejected** for now.
- Even with a mutex, the VM can be re-entered from different call paths (callbacks, interrupts) and you will be fighting scheduling and deadlocks.
- It also encourages other tasks to call `JS_*()` directly, which tends to sprawl.

### Alternative: run JS “inline” in many tasks (no central VM task)

- **Rejected**: makes it too easy to violate ownership rules and makes communication patterns ad-hoc.

### Alternative: embed a different JS engine with native async/event-loop support

- **Deferred**: scope is to use MicroQuickJS and understand its constraints first. If we hit walls (e.g., lack of ES6 features, memory model), we can revisit.

## Implementation Plan

This is a high-level plan; we’ll refine it after the Cardputer REPL port is stable.

1. **Refactor the current REPL** into a VM host loop
   - Move all JS evaluation into the owning task loop.
   - Make console input non-blocking / short-timeout.
2. **Add a minimal message queue** for “system → JS”
   - Define message struct and queue.
   - Implement `ESP.onMessage(cb)` in C bindings (store cb using `JSGCRef`).
3. **Add a “JS → system” hook**
   - `ESP.postMessage(msg)` enqueues to a system queue/bus.
4. **Add timers (optional)**
   - Either:
     - FreeRTOS software timers + queue event to JS, or
     - `esp_timer` callbacks that only enqueue.
5. **Multi-VM (optional)**
   - Prototype a second VM task with a small heap and see if memory budget holds.
   - Add bus routing (VM A → VM B).
6. **Testing**
   - QEMU: validate message loop and deterministic JS calls where possible.
   - Hardware: validate USB Serial JTAG + real-time event delivery.

## Open Questions

- Do we want to include `JSON` in the generated stdlib for message payload parsing, or stick to primitive payloads?
- What is the minimum viable “event loop tick” cadence that keeps the UI responsive without starving other tasks?
- Should scripts be sandboxed (max exec time, max memory, allowed APIs), and how do we enforce that?
- How should we surface errors:
  - print to console only,
  - publish to an “error channel”,
  - or both?
- How do we integrate with Cardputer input/output:
  - keyboard events to JS,
  - display rendering from JS,
  - audio cues from JS?

## References

- Native extensions playbook: `../reference/02-microquickjs-native-extensions-on-esp32-playbook-reference-manual.md`
- Current firmware entry (legacy reference): `imports/esp32-mqjs-repl/mqjs-repl/legacy/main.c`
- MicroQuickJS API: `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/mquickjs.h`
- Upstream “how to extend” example:
  - `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/example.c`
  - `imports/esp32-mqjs-repl/mqjs-repl/components/mquickjs/example_stdlib.c`
