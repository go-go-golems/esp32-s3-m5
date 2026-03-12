---
Title: Remote Script Memory Lifecycle
Ticket: ESP-30-M5DIAL-MQJS-LAIN-DSL
Status: active
Topics:
  - esp32-s3
  - esp32s3
  - firmware
  - javascript
  - websocket
  - memory
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
  - Path: 0074-m5dial-web-remote/firmware/main/remote_client.cpp
    Note: Owns websocket receive, request allocation, and request submission into the JS service
  - Path: 0074-m5dial-web-remote/firmware/main/js_service.cpp
    Note: Owns queueing, worker-side evaluation, and request/result lifetime
  - Path: 0074-m5dial-web-remote/firmware/main/js_service.h
    Note: Defines the request shape and therefore the per-script body footprint
ExternalSources: []
Summary: Explains where remote script bodies live in memory, how ownership moves between websocket receive, queueing, and QuickJS evaluation, and why the original stack-overflow bugs happened.
LastUpdated: 2026-03-11T22:45:00-04:00
WhatFor: Use when debugging script-related panics, memory pressure, queue ownership, or deciding how large remote JavaScript payloads may safely be.
WhenToUse: Use when modifying script transport, request buffering, JS worker sizing, or QuickJS runtime memory configuration.
---

# Remote Script Memory Lifecycle

## Overview

Remote `script_eval` handling in `0074-m5dial-web-remote` moves a JavaScript body through three memory domains:

- websocket transport buffers owned by the websocket/LWIP stack,
- a heap-allocated `ScriptEvalRequest`,
- the MicroQuickJS runtime arena used during parsing and execution.

The important architectural point is that the durable copy of the script body now lives on the heap, not on the websocket task stack and not on the JS worker stack.

## Request Shape

The request structure is defined in `firmware/main/js_service.h`:

```c++
struct ScriptEvalRequest {
  uint32_t request_id = 0;
  uint32_t timeout_ms = 0;
  char filename[32] = {0};
  char code[CONFIG_TUTORIAL_0074_JS_MAX_BODY + 1] = {0};
};
```

With the current firmware config:

- `CONFIG_TUTORIAL_0074_JS_MAX_BODY=4096`
- `CONFIG_TUTORIAL_0074_JS_MEM_BYTES=65536`
- `CONFIG_TUTORIAL_0074_JS_QUEUE_LEN=4`

That means each durable queued request carries roughly 4 KB of script body plus metadata.

## End-to-End Lifetime

### 1. Websocket receive buffer

The Go server sends a `script_eval` JSON frame to `/ws/device`. On the device, `esp_websocket_client` receives that frame and exposes it to the callback as `data->data_ptr`.

Memory ownership at this point:

- owned by `esp_websocket_client`,
- only valid for the duration of the event callback,
- not safe to keep as a long-lived pointer.

### 2. Temporary JSON copy and parse tree

`handle_inbound_frame()` in `firmware/main/remote_client.cpp` allocates a temporary null-terminated copy of the websocket frame, then parses it with `cJSON_Parse(...)`.

Allocations in this phase:

- `new char[data_len + 1]` for the temporary JSON string,
- `cJSON` heap allocations for the parse tree.

These allocations are short-lived. They are released after the frame is dispatched.

### 3. Durable script request allocation

When the message type is `script_eval`, `handle_script_eval_message()` allocates a `ScriptEvalRequest` on the heap and copies the body into `request->code`.

This is the first durable copy of the script body.

Ownership at this point:

- allocated in `remote_client.cpp`,
- handed to `js_service_submit(...)`.

### 4. Queue handoff

The JS service queue stores `ScriptEvalRequest*`, not full request structs.

That matters because:

- the FreeRTOS queue only stores pointer-sized items,
- the worker task does not receive a 4 KB struct by value,
- no per-request body copy is made onto the worker stack.

### 5. Worker-side evaluation

`lain_js_worker` dequeues `ScriptEvalRequest*` and calls `mqjs_service_eval(...)` with:

- `request->code`,
- `strlen(request->code)`,
- `request->filename`,
- `request->timeout_ms`.

The script text still lives in the heap-allocated request block during this phase.

### 6. QuickJS runtime allocations

`mqjs_service` runs the evaluation inside the configured MicroQuickJS service arena:

- `CONFIG_TUTORIAL_0074_JS_MEM_BYTES=65536`

That 64 KB arena is where QuickJS parser/runtime objects are allocated. It is separate from:

- the websocket transport buffers,
- the `ScriptEvalRequest` heap block,
- the FreeRTOS queue storage.

### 7. Result and batch flush allocations

After evaluation, the worker may allocate temporary output structures:

- `mqjs_eval_result_t` output/error buffers,
- a temporary `batches.json` buffer from `__lain_take_batches()`,
- a temporary `cJSON` parse tree for that batch payload.

These are short-lived and are released after the result has been sent and commands/events have been extracted.

### 8. Request release

Once evaluation and result handling are complete, the worker frees the original `ScriptEvalRequest`.

That is the end of the durable lifetime of the incoming script body.

## Console Path Compared to Remote Path

`js eval ...` from `esp_console` does not use the websocket callback path and does not enqueue a `ScriptEvalRequest`.

Instead:

- `js_console.cpp` builds a local `std::string`,
- `js_service_eval_to_json(...)` evaluates that string directly.

This is why local console evaluation worked even when remote websocket-delivered evaluation was crashing.

## The Two Historical Stack Bugs

### Bug 1: websocket task overflow

Originally, the websocket callback created a full `ScriptEvalRequest` as a stack-local variable. Because the struct includes a 4096-byte `code` buffer, that consumed most of the default websocket task stack.

Symptoms:

- crash immediately after `queued script request ...`,
- backtrace inside websocket/LWIP/select code,
- crash reproduced even for tiny payloads like `"23"`.

Fix:

- allocate the request on the heap in `remote_client.cpp`,
- raise websocket client task stack to 8192 as a safety margin.

### Bug 2: `lain_js_worker` stack overflow

After the first fix, the JS worker still used:

```c++
ScriptEvalRequest request = {};
xQueueReceive(..., &request, ...)
```

That copied a full 4 KB request struct onto the worker stack on every dequeue.

Symptoms:

- `***ERROR*** A stack overflow in task lain_js_worker has been detected.`

Fix:

- queue `ScriptEvalRequest*`,
- have the worker dequeue a pointer,
- free the request after evaluation,
- raise worker stack to 8192.

## Current Ownership Model

For remote `script_eval`, the intended ownership chain is now:

1. websocket task owns transient incoming frame pointer,
2. `remote_client.cpp` allocates `ScriptEvalRequest*` on the heap,
3. JS service queue stores the pointer,
4. JS worker becomes the owner after dequeue,
5. JS worker frees the request after evaluation and result handling.

This model keeps large script bodies off task stacks and makes the request lifetime explicit.
