---
Title: Panel EPD analysis for PaperS3 WAMR interference
Ticket: ESP-39-PAPERS3-ESPRESSIF-WAMR-MIGRATION
Status: active
Topics:
    - papers3
    - wasm
    - firmware
    - esp-idf
    - debugging
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp
      Note: Core buffer mutation, dirty-rect tracking, cache writeback, and background update task
    - Path: ../../../../../../../M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp
      Note: Class layout and state fields for the PaperS3 e-ink panel implementation
    - Path: 0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp
      Note: PaperS3 canvas wrapper that drives `M5.Display` and enters the `Panel_EPD` path.
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp
      Note: Queued host-command bridge from WAMR guest code into PaperS3 draw and present calls.
    - Path: 0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp
      Note: Wasm execution lifecycle, host-frame flush timing, and worker-thread experiment.
ExternalSources:
    - https://github.com/m5stack/M5GFX/compare/0.2.15...0.2.19
    - https://github.com/m5stack/M5GFX/commit/fd824eea23d6cf9d527052884b744f8a53a9e20e
    - https://github.com/m5stack/M5GFX/commit/c899961dbd3dd02896c8a46b17fc6c336f7d5811
Summary: Explain how `Panel_EPD` works on PaperS3, how our `0079` firmware reaches it from WAMR, and why this subsystem is the current leading interference point after the generic ESP32-S3 WAMR issues were eliminated.
LastUpdated: 2026-03-22T21:57:00-04:00
WhatFor: Give a new intern a precise mental model of the PaperS3 display stack and a concrete theory set for how it can interact badly with WAMR execution.
WhenToUse: Read this before patching `Panel_EPD`, interpreting PaperS3 crash traces, or designing the next isolation experiment in the WAMR-on-PaperS3 investigation.
---


# Panel EPD analysis for PaperS3 WAMR interference

## Executive summary

`Panel_EPD` is the lowest PaperS3-specific layer in our active crash path. It is the `M5GFX` panel implementation that owns the PaperS3 grayscale framebuffer, tracks dirty rectangles, performs cache writeback before transfer, and hands display updates to a background FreeRTOS task that talks to the e-ink bus.

That detail matters because our current WAMR bug is no longer a generic "WAMR does not run on ESP32-S3" problem. We eliminated the generic runtime bring-up issues by migrating to Espressif's WAMR component and restoring two local platform assumptions. After that recovery:

- `wasm run-preflush return-42` succeeds on PaperS3
- `wasm run-preflush log-only` succeeds on PaperS3
- `wasm replay hello-frame` succeeds after reset
- `wasm run-preflush hello-frame` still crashes
- a successful non-drawing WAMR call can poison a later PaperS3 replay in the same boot

That combination points away from guest logic and toward the interaction boundary between:

- WAMR execution and teardown
- `wasm_host_api.cpp` queued host-frame flush
- `papers3_canvas.cpp` frame lifecycle
- `M5.Display`
- `Panel_EPD`

The important conclusion for an intern is simple:

- `Panel_EPD` is not just a passive drawing helper
- it owns mutable shared state, background tasks, queues, DMA staging buffers, and cache-sensitive memory
- that makes it a credible place where "WAMR ran earlier" can still matter later, even if the guest did not directly draw anything

## Why this document exists

If you only look at the panic line in `Panel_EPD::writeFillRectPreclipped(...)`, it is tempting to think the problem is "a bad rectangle" or "one bad color write." That is too shallow. `Panel_EPD` is part of a deeper pipeline. A correct debugging approach requires understanding:

- where the framebuffer lives
- who writes into it
- when cache maintenance happens
- which work is synchronous versus deferred
- what `waitDisplay()`, `startWrite()`, `endWrite()`, and `display()` actually imply
- which facts we have proven experimentally and which are still hypotheses

This guide is written so that someone new to the codebase can pick up the whole picture without redoing every debugging session from scratch.

## The system in one picture

At a high level, the display path looks like this:

```text
esp_console command
    |
    v
CmdWasm(...) in wasm_command.cpp
    |
    v
RunEmbeddedWasmModule(...) in wasm_module_runner.cpp
    |
    +--> WAMR guest executes host imports
    |        |
    |        v
    |   Host imports do NOT draw immediately
    |   They only queue commands in wasm_host_api.cpp
    |
    v
FlushWasmHostFrame(...)
    |
    +--> PaperCanvasScreenClear(...)
    +--> PaperCanvasDrawRect(...)
    +--> PaperCanvasFillRect(...)
    +--> PaperCanvasPresent(...)
            |
            v
        M5.Display methods
            |
            v
        M5GFX / LGFXBase
            |
            v
        Panel_EPD
            |
            +--> mutate _buf
            +--> expand dirty rect
            +--> cache writeback
            +--> queue update_data_t
            +--> background task_update()
            +--> Bus_EPD / panel transfer
```

The critical observation is that there are really two distinct phases:

1. A synchronous phase where draw operations modify `Panel_EPD::_buf`.
2. An asynchronous phase where `Panel_EPD::display(...)` and `task_update(...)` turn those buffered changes into actual e-ink transfers.

That split is exactly why contamination bugs can be tricky. The system can appear healthy during the draw stage and fail later during flush or update.

## The files you must know first

Start with these files in this order:

1. `0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp`
2. `0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp`
3. `0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp`
4. `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp`
5. `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`

These are the roles:

- `wasm_module_runner.cpp`
  - loads and instantiates the Wasm module
  - calls the exported guest function
  - decides whether to flush the queued host frame before or after Wasm cleanup
  - owns the worker-thread experiment used in the later ticket slices

- `wasm_host_api.cpp`
  - registers host imports like `host_screen_clear` and `host_draw_rect`
  - queues host commands instead of touching the display immediately
  - replays the queue later in `FlushWasmHostFrame(...)`

- `papers3_canvas.cpp`
  - wraps `M5.Display` in a simple frame model
  - uses `waitDisplay()`, `startWrite()`, `endWrite()`, and `setEpdMode(...)`
  - is the narrowest local place where we intentionally enter the PaperS3 display stack

- `Panel_EPD.hpp` and `Panel_EPD.cpp`
  - implement the PaperS3 e-ink panel backend itself
  - own the panel buffer, dirty-rect logic, queue, staging buffers, and update task

## What `Panel_EPD` is conceptually

`Panel_EPD` is a specialized `M5GFX` panel backend for e-ink hardware. It is not a simple immediate-mode "draw one pixel to the device" object. It behaves more like a small display subsystem.

Key idea:

- `Panel_EPD` keeps a software-side image of the screen in `_buf`
- drawing calls mutate `_buf`
- later, `display(...)` enqueues an update request
- an internal worker task turns that into the waveform-driven bus traffic the e-ink panel needs

In other words:

```text
draw API call
    -> software buffer mutation
    -> dirty rect growth
    -> explicit display/update request
    -> background transfer task
```

That architecture is reasonable for e-ink, because e-ink panels are slower and more stateful than a typical TFT. But it also creates several moving parts that can go wrong independently.

## `Panel_EPD` state and why it matters

The core class definition is in `Panel_EPD.hpp`.

Important fields for our debugging:

- `_update_queue_handle`
  - FreeRTOS queue used to hand update requests to the background task

- `_task_update_handle`
  - handle for the background EPD writer task

- `_dma_bufs[2]`
  - DMA-side temporary buffers used during transfer

- `_step_framebuf`
  - stepwise update framebuffer used by the EPD waveform/update logic

- `_step_table`
  - step table supporting the staged refresh model

- `_lut_2pixel`
  - lookup table for converting panel pixel state into hardware update encoding

- `_display_busy`
  - volatile flag used by `waitDisplay()` and the update loop

- `_buf`
  - inherited panel framebuffer from `Panel_HasBuffer`
  - this is the main logical screen image that drawing functions mutate

Important configuration fields in `config_detail_t`:

- `line_padding`
  - affects DMA write line length

- `task_priority`
  - background EPD update task priority

- `task_pinned_core`
  - which core the background task runs on, or unpinned

Why this matters for WAMR:

- `Panel_EPD` owns multiple buffers, not just one
- the display stack is split across at least two execution contexts
- queue state and busy state are persistent across calls
- cache visibility matters because the buffers are later consumed by bus/DMA logic

This means a previous WAMR run does not need to corrupt the panel outright to create trouble. It only needs to leave one of these shared pieces of state in a bad condition.

## The immediate draw path

The first half of `Panel_EPD` is "draw into the software buffer."

### `waitDisplay()` and `displayBusy()`

`Panel_EPD::waitDisplay()` at `Panel_EPD.cpp:307-310` spins until `_display_busy` is false.

`Panel_EPD::displayBusy()` at `Panel_EPD.cpp:312-319` also checks whether the update queue is full.

Interpretation:

- `waitDisplay()` is the panel's blocking "drain previous display work" primitive
- the queue fullness check means "busy" is broader than one simple flag

This matters because our wrapper in `papers3_canvas.cpp` relies on `waitDisplay()` as a clean boundary before starting a new frame.

### `writeFillRectPreclipped(...)`

This is currently the most famous function in the investigation because our decoded panic lands inside it.

Relevant code region:

- `Panel_EPD.cpp:338-373`

What it does:

1. Computes `xs`, `xe`, `ys`, `ye`.
2. Stores those as the current transfer bounds.
3. Calls `_update_transferred_rect(...)` to merge this rectangle into the dirty region.
4. Builds a 4-bit dither tile from the requested grayscale value.
5. Iterates over every pixel in the rectangle.
6. Writes 4-bit grayscale nibbles into `_buf`.

The important write is:

```text
buf[idx] = (buf[idx] & mask) | value;
```

Specifically around the crash line:

- `Panel_EPD.cpp:370`

Why this line is sensitive:

- it assumes `_buf` is valid
- it assumes the `y * stride + idx` computation points into valid backing memory
- it performs read-modify-write access, not just blind store
- it is part of a path that can be reached by full-screen clear and rectangle drawing

That last point matters because our later reduced replay experiments showed that the post-WAMR crash is broader than one single clear operation.

### `writeImage(...)`, `writePixels(...)`, `_draw_pixels(...)`

These functions do the same class of work through slightly different entry points.

Relevant regions:

- `writeImage(...)`: `Panel_EPD.cpp:375-399`
- `writePixels(...)`: `Panel_EPD.cpp:401-446`
- `_draw_pixels(...)`: `Panel_EPD.cpp:466-532`

They all eventually mutate `_buf`.

Important implications:

- `fillScreen`, `fillRect`, `drawRect`, and image operations are not separate hardware pipelines
- they converge on the same mutable framebuffer
- if `_buf` or its visibility is compromised, many draw APIs can fail in similar ways

That matches our experiments:

- `clear-only` crashed
- `frame-no-clear` also crashed

So the problem space is not "screen clear logic only." It is likely any nontrivial mutation of `Panel_EPD::_buf` after certain WAMR activity.

## Dirty rectangle tracking

`_update_transferred_rect(...)` at `Panel_EPD.cpp:544-551` rotates coordinates and expands `_range_mod`.

That means drawing calls do not push updates immediately. They mark a region as modified. Later, `display(...)` turns that accumulated region into an actual queued update.

Conceptually:

```text
for each draw:
    rotate coordinates if needed
    dirty.left   = min(dirty.left,   rect.left)
    dirty.right  = max(dirty.right,  rect.right)
    dirty.top    = min(dirty.top,    rect.top)
    dirty.bottom = max(dirty.bottom, rect.bottom)
```

Why an intern should care:

- a bug can live in buffer mutation
- or in dirty-rect accumulation
- or in the later conversion of dirty rect into update request

These are separate failure surfaces.

## The deferred display path

The second half of `Panel_EPD` is "turn the software buffer into real EPD work."

### `display(...)`

Relevant region:

- `Panel_EPD.cpp:553-589`

What it does:

1. Merges any explicit `(x, y, w, h)` arguments into `_range_mod`.
2. Returns early if the dirty region is empty.
3. Aligns the dirty region to panel and nibble boundaries.
4. Builds an `update_data_t` struct with x/y/w/h/mode.
5. Sets `_display_busy = true`.
6. Calls `cacheWriteBack(...)` on the framebuffer region.
7. `vTaskDelay(1)`.
8. Sends `update_data_t` into `_update_queue_handle`.
9. Clears `_range_mod` if the queue send succeeds.

This function is extremely important for the WAMR investigation because it is where three worlds meet:

- application-level draw intent
- framebuffer/cache coherence
- background display task handoff

The most suspicious lines for interference analysis are:

- `_display_busy = true`
- `cacheWriteBack(...)`
- `xQueueSend(...)`

These suggest plausible failure modes involving:

- stale or partially visible writes in memory
- a broken or stale queue/task relationship
- a draw stage that succeeded locally but becomes invalid when handed to the update path

### `task_update(...)`

Relevant region:

- `Panel_EPD.cpp:887+`

This is the background task that consumes queued updates and prepares actual panel transfer data.

At a high level it:

1. Waits on `_update_queue_handle`.
2. Marks the panel busy.
3. Locates source pixels in `_buf`.
4. Locates the corresponding region in `_step_framebuf`.
5. Applies mode-dependent update logic.
6. Uses `blit_dmabuf(...)` and related machinery to generate bus-side data.
7. Drives the EPD bus through `Bus_EPD`.
8. Eventually lets the panel return to not-busy.

Conceptual pseudocode:

```text
loop forever:
    wait for update_data_t from queue
    mark display busy
    for each affected line:
        compare _buf against _step_framebuf
        generate staged waveform/update data
        prepare DMA transfer buffers
        push to EPD bus
    finish update
    clear busy when no more work remains
```

Why this matters:

- the panel has a background execution context independent of WAMR
- memory produced in one context is consumed in another
- buffer lifetime and cache visibility are therefore critical
- a bug may only appear after WAMR because later display work reads data or state that WAMR-side activity indirectly destabilized

## The PaperS3 wrapper layer in `papers3_canvas.cpp`

Our firmware does not call `Panel_EPD` directly. It goes through `M5.Display`.

The relevant wrapper is `papers3_canvas.cpp`.

### Frame lifecycle

`BeginFrameIfNeeded()` at `papers3_canvas.cpp:66-77` does:

1. `M5.Display.waitDisplay()`
2. `M5.Display.setEpdMode(...)`
3. `M5.Display.startWrite()`
4. mark frame active

Then the draw helpers call:

- `PaperCanvasScreenClear(...)` -> `M5.Display.fillScreen(...)`
- `PaperCanvasDrawRect(...)` -> `M5.Display.drawRect(...)`
- `PaperCanvasFillRect(...)` -> `M5.Display.fillRect(...)`

Finally `PaperCanvasPresent(...)` at `papers3_canvas.cpp:160-171` does:

1. set the EPD mode again
2. `M5.Display.endWrite()`
3. `M5.Display.waitDisplay()`
4. clear frame-active state

Conceptual model:

```text
begin frame
    wait for prior display work
    start a new write transaction

mutate framebuffer through draw calls

present
    end write transaction
    block until panel says display work is done
```

That model is clean in principle. The problem is that it assumes `waitDisplay()` really is a complete fence for all relevant state, and that no previous subsystem interaction has left the panel or memory subsystem in a poisoned condition.

## How WAMR reaches `Panel_EPD`

This is the key bridge between the runtime and the display bug.

### Host imports do not draw immediately

In `wasm_host_api.cpp`, functions like:

- `HostScreenClear(...)`
- `HostDrawRect(...)`
- `HostFillRect(...)`
- `HostPresent(...)`

only queue `HostCommand` objects.

That is important. It means guest execution and display side effects are intentionally decoupled.

### Actual PaperS3 side effects happen in `FlushWasmHostFrame(...)`

`FlushWasmHostFrame(...)` at `wasm_host_api.cpp:196-238` replays queued commands and calls:

- `PaperCanvasScreenClear(...)`
- `PaperCanvasDrawRect(...)`
- `PaperCanvasFillRect(...)`
- `PaperCanvasPresent(...)`

So the real flow is:

```text
Wasm guest code
    -> queue host commands
    -> return to host
    -> FlushWasmHostFrame()
    -> papers3_canvas.cpp
    -> M5.Display
    -> Panel_EPD
```

This explains an important debugging fact:

- a crash in `Panel_EPD` does not prove the guest function itself was still running
- the crash can happen after WAMR execution has logically completed

### `wasm_module_runner.cpp` and flush timing

`RunEmbeddedWasmModuleOnCurrentThread(...)` in `wasm_module_runner.cpp:131-244` loads, instantiates, and executes the guest, then flushes the host frame either:

- before Wasm cleanup, or
- after Wasm cleanup

depending on `WasmFlushTiming`.

That distinction mattered in earlier debugging because it let us test whether teardown itself was the trigger. It was not enough to eliminate the PaperS3 issue.

## What we have proven experimentally

These points are facts from our ticket history and on-device experiments.

### Fact 1: generic ESP32-S3 WAMR bring-up was not the final blocker

After migrating to Espressif's WAMR component and restoring two local platform assumptions, PaperS3 can successfully run:

- `return-42`
- `log-only`

That means the current issue is not "WAMR cannot execute anything on this class of chip."

### Fact 2: the remaining failure is downstream in the PaperS3 display path

After the generic runtime recovery:

- `wasm replay hello-frame` succeeds after reset
- `wasm run-preflush hello-frame` crashes in the PaperS3 path

That means basic PaperS3 drawing can work, and basic WAMR execution can work, but the combination is unstable.

### Fact 3: a non-drawing WAMR run can poison a later PaperS3 replay

Same-boot probes showed:

- `wasm run-preflush return-42`
- then `wasm replay hello-frame`
- crash

and also:

- `wasm run-preflush log-only`
- then `wasm replay hello-frame`
- crash

This is one of the most important facts in the entire investigation.

Interpretation:

- the failure is not limited to guest-issued draw imports
- simply executing WAMR successfully is enough to change the later PaperS3 behavior

### Fact 4: moving WAMR onto a worker thread did not fix it

The worker-thread experiment added:

- `wasm run-worker`
- `wasm run-preflush-worker`

But the problem persisted. Therefore:

- the bug is not explained solely by "WAMR ran on the `esp_console` task"

### Fact 5: the PaperS3 failure is broader than one screen-clear primitive

Reduced replay controls showed:

- `clear-only` can crash
- `frame-no-clear` can also crash

This means the problematic surface is not just one `fillScreen(...)` call. It appears to be broader mutation of `Panel_EPD` state or buffer access.

## Why `Panel_EPD` is a credible interference point

This section is theory, but theory anchored to the code we have already inspected.

### Mechanism A: cache coherence around panel buffers

`Panel_EPD::display(...)` explicitly calls `cacheWriteBack(...)` before queueing the update request.

That suggests the panel buffers are in memory where cache visibility matters to the later consumer.

If WAMR execution, teardown, allocator behavior, or memory mapping changes cache-related state or timing, then `Panel_EPD` could be the first place where that matters visibly.

Why this is plausible:

- the bug survives a successful non-drawing WAMR run
- the panic class earlier in the project involved cache-disabled / cached-region access
- e-ink update code is naturally more memory-coherence-sensitive than ordinary CPU-only logic

### Mechanism B: shared PSRAM or external-memory sensitivity

PaperS3 is unusual compared with simpler S3 display boards because the e-ink path uses a heavier buffered update model. If framebuffer or step buffers live in PSRAM or other external memory, then writeback and later DMA/bus consumption become more delicate.

We do not yet have a final proof of exact buffer placement for every relevant structure in this build, so treat this as a strong hypothesis rather than a confirmed root cause.

### Mechanism C: stale task/queue/display state that survives WAMR

`Panel_EPD` owns:

- `_display_busy`
- `_update_queue_handle`
- `_task_update_handle`
- `_step_framebuf`

If a successful WAMR run perturbs timing, task scheduling, or synchronization in a way our current probes do not catch, a later PaperS3 draw could hit the panel in an invalid state even though basic task/interrupt counters look normal.

This mechanism is plausible because:

- the system is asynchronous
- `waitDisplay()` is only as good as the underlying busy/task semantics
- same-boot contamination implies persistent state, not just one bad guest draw command

### Mechanism D: `Panel_EPD::_buf` remains valid in some scenarios and invalid in others

The crash line in `writeFillRectPreclipped(...)` is a read-modify-write into `_buf`. A bug in object lifetime, backing allocation, or visibility of `_buf` would naturally show up there.

This is not yet proven, but it is a direct explanation for why both clear-style and rectangle-style replays can crash in similar low-level buffer mutation code.

## What we have not proven

These are common overclaims to avoid.

- We have **not** proven that WAMR directly corrupts `_buf`.
- We have **not** proven that PSRAM alone is the root cause.
- We have **not** proven that `Panel_EPD` itself contains a standalone bug independent of WAMR.
- We have **not** proven that the background update task is the precise site of contamination.
- We have **not** proven that the upstream `M5GFX` newer PaperS3 fix resolves this issue.

This distinction matters because good debugging avoids collapsing "plausible mechanism" into "established cause."

## Why the AtomS3R result changed our interpretation

The cross-device experiment on AtomS3R was important because it removed one large class of explanations.

After applying the same recovered WAMR platform fixes:

- AtomS3R succeeded on trivial Wasm probes
- AtomS3R succeeded on replay and full `hello-frame`

That means:

- generic ESP32-S3 WAMR execution is not the remaining blocker
- generic "S3 plus PSRAM" is not sufficient to explain the PaperS3 issue
- the remaining suspicion narrows toward the PaperS3 display stack, especially `Panel_EPD` and its e-ink-specific buffering/update model

This does not prove `Panel_EPD` is wrong, but it makes it the leading local subsystem to inspect next.

## How newer upstream `M5GFX` history fits in

Our local nested `M5GFX` checkout is at `0.2.15`.

Useful upstream history:

- `c899961dbd3dd02896c8a46b17fc6c336f7d5811`
  - message: `fix PSRAM cache write back for Tab5 + PaperS3`
  - already older than `0.2.15`, so we already have it locally

- `fd824eea23d6cf9d527052884b744f8a53a9e20e`
  - message: `Improved refresh behavior for PaperS3.`
  - newer than our checkout

Interpretation:

- there has already been at least one historical PaperS3/PSRAM/cache-related fix in this code family
- there is also a newer PaperS3 refresh fix after our local version
- but our earlier diff review suggested the newer refresh fix does not touch the exact `writeFillRectPreclipped(...)` region where our current panic lands

That means the upstream history supports caution, not certainty:

- it is reasonable to suspect `Panel_EPD`
- but we should not pretend upstream already shipped a clearly matching fix for our exact crash

## Walkthrough of the active crash path

For `wasm run-preflush hello-frame`, the practical path is:

```text
1. console command dispatch
2. RunEmbeddedWasmModule(...)
3. wasm guest executes and queues:
   - screen_clear
   - draw_rect / fill_rect
   - present
4. FlushWasmHostFrame(...)
5. PaperCanvasScreenClear(...)
6. M5.Display.fillScreen(...)
7. LGFXBase::fillRect(...)
8. Panel_EPD::writeFillRectPreclipped(...)
9. panic during _buf read-modify-write
```

For the more subtle contamination case:

```text
boot
-> wasm run-preflush return-42   // succeeds, no drawing
-> wasm replay hello-frame       // later crashes in Panel_EPD
```

That second case is why the investigation shifted from "guest drawing bug" to "post-WAMR PaperS3 state contamination."

## Pseudocode model of the combined system

### WAMR execution plus queued host flush

```text
function run_embedded_module(module):
    reset_host_command_queue()
    reset_papers3_frame()

    wasm_module = wasm_runtime_load(module.bytes)
    module_inst = wasm_runtime_instantiate(wasm_module)
    exec_env = wasm_runtime_create_exec_env(module_inst)

    call_guest_export(exec_env, "run")

    flush_host_frame()

    destroy_exec_env()
    deinstantiate_module()
    unload_module()
```

### Host command flush

```text
function flush_host_frame():
    for command in queued_commands:
        switch command.type:
            case ScreenClear:
                PaperCanvasScreenClear(command.color)
            case DrawRect:
                PaperCanvasDrawRect(...)
            case FillRect:
                PaperCanvasFillRect(...)
            case Present:
                PaperCanvasPresent(command.mode)
```

### PaperS3 wrapper

```text
function PaperCanvasScreenClear(color):
    BeginFrameIfNeeded()
    M5.Display.fillScreen(color)

function BeginFrameIfNeeded():
    if frame_active:
        return
    M5.Display.waitDisplay()
    M5.Display.setEpdMode(default_mode)
    M5.Display.startWrite()
    frame_active = true
```

### `Panel_EPD` internal model

```text
function writeFillRectPreclipped(x, y, w, h, color):
    update_dirty_rect(x, y, w, h)
    for each pixel in rect:
        compute dithered 4-bit value
        write nibble into _buf

function display():
    if dirty_rect empty:
        return
    make update_data_t from dirty rect
    mark busy
    cacheWriteBack(relevant framebuffer range)
    queue update_data_t

background task_update():
    forever:
        recv update_data_t
        consume pixels from _buf
        update _step_framebuf
        build dma transfer lines
        drive EPD bus
```

## The most important mental model

If you only remember one thing, remember this:

```text
WAMR does not talk to the e-ink bus directly.
WAMR talks to a queued host API.
The queued host API talks to a PaperS3 frame wrapper.
The frame wrapper talks to M5GFX.
M5GFX mutates a software framebuffer and later hands work to a background EPD task.
```

That layered structure is why the bug feels indirect. The crashing function is real, but the destabilizing event may have happened much earlier.

## Best next experiments

These are the highest-value next steps if we continue to chase the PaperS3 interaction.

### Experiment 1: inspect backing allocation and memory capabilities for panel buffers

Goal:

- determine exactly where `_buf`, `_step_framebuf`, and DMA buffers live
- confirm whether they sit in internal RAM, PSRAM, or mixed capability allocations

Why:

- this directly tests the strongest memory/cache hypothesis

### Experiment 2: add instrumentation immediately before `writeFillRectPreclipped(...)`

Goal:

- print `_buf` address, panel dimensions, computed stride, and rect bounds before the first crashing write

Why:

- if `_buf` changes across the contamination boundary, that is a major clue
- if it remains stable, the failure may be about cache visibility or stale lower-level state instead

### Experiment 3: patch or A/B the newer upstream PaperS3 refresh commit

Goal:

- test whether the newer `fd824ee...` PaperS3 refresh behavior change shifts the failure boundary

Why:

- it is a bounded upstream-alignment experiment with low conceptual cost

### Experiment 4: force a stronger reset of panel/update state after each Wasm run

Goal:

- explicitly reinitialize or drain more `Panel_EPD` state than `PaperCanvasResetFrame()` currently does

Why:

- same-boot contamination suggests persistent display-side state matters

### Experiment 5: inspect whether `waitDisplay()` is a sufficient fence

Goal:

- determine whether `waitDisplay()` plus queue-space checks really imply all display-side state is safe for a new frame

Why:

- our wrapper assumes it does
- contamination bugs sometimes survive because a "busy" check is narrower than the real hazard surface

## Practical advice for a new intern

If you continue this investigation:

- do not treat the panic line as the whole bug
- always distinguish buffer mutation from queued display transfer
- record clean-boot versus same-boot results separately
- separate facts from hypotheses in every note
- prefer small A/B tests over large refactors
- keep cross-device results in view, because AtomS3R already removed many bad theories

## References

### Local code references

- `0079-papers3-wamr-assemblyscript-console/main/wasm_module_runner.cpp`
- `0079-papers3-wamr-assemblyscript-console/main/wasm_host_api.cpp`
- `0079-papers3-wamr-assemblyscript-console/main/papers3_canvas.cpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.hpp`
- `M5PaperS3-UserDemo/components/M5GFX/src/lgfx/v1/platforms/esp32/Panel_EPD.cpp`

### Upstream `M5GFX` references

- `M5GFX` compare `0.2.15...0.2.19`
- `fd824eea23d6cf9d527052884b744f8a53a9e20e` `Improved refresh behavior for PaperS3.`
- `c899961dbd3dd02896c8a46b17fc6c336f7d5811` `fix PSRAM cache write back for Tab5 + PaperS3`

## Bottom line

`Panel_EPD` is a strong interference candidate not because we have proven it is "the bug," but because it is the first subsystem where all of the following intersect:

- PaperS3-specific hardware behavior
- mutable buffered display state
- cache writeback
- background task handoff
- same-boot persistence
- the exact functions named in our decoded crash traces

That is enough to justify a focused `Panel_EPD` investigation as the next serious PaperS3 debugging step.
