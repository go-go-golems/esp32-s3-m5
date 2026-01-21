---
Title: Design + implementation plan
Ticket: 0055-MQJS-SERVICE-COMPONENT
Status: active
Topics:
    - esp-idf
    - javascript
    - quickjs
    - microquickjs
    - freertos
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: REST eval calls into js_service
    - Path: 0048-cardputer-js-web/main/js_service.cpp
      Note: Reference queue-owned VM task and request queue
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:36:22.368871004-05:00
WhatFor: ""
WhenToUse: ""
---


# Design + implementation plan

## Executive Summary

Extract the queue-owned MicroQuickJS execution pattern (single VM owner task, multiple producers) into a reusable component `components/mqjs_service`, built on `components/mqjs_vm`.

Primary consumer: `0048-cardputer-js-web/main/js_service.cpp`, which should become a thin “0048-specific bindings + REST formatting” layer over the shared service.

## Problem Statement

Many firmwares want to:

- accept code snippets over REST/console,
- execute them in a JS VM,
- also execute JS callbacks triggered by hardware events,
- without ever calling MicroQuickJS from multiple tasks concurrently.

0048 solved this with a single-owner JS service task and an inbound queue:

- `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
  - `js_service_eval_to_json(...)`
  - `js_service_post_encoder_click(...)`
  - `js_service_update_encoder_delta(...)`

But it is project-local and includes project-local bootstraps (`encoder.on`, `emit`), making it hard to reuse.

## Proposed Solution

Create:

- `esp32-s3-m5/components/mqjs_service/`
  - `include/mqjs_service.h` (C or C++; C API is fine)
  - `mqjs_service.cpp`
  - depends on `mqjs_vm`

### Core abstraction: “job on the VM thread”

Expose two primitives:

1) Eval requests (common case)
2) Arbitrary jobs executed on the VM task (for bindings/callbacks)

Proposed C API sketch:

```c
typedef struct mqjs_service mqjs_service_t;

typedef struct {
  bool ok;
  bool timed_out;
  // output/error are malloc'ed by the service; caller frees.
  char* output;
  char* error;
} mqjs_eval_result_t;

typedef esp_err_t (*mqjs_job_fn_t)(JSContext* ctx, void* user);

typedef struct {
  mqjs_job_fn_t fn;
  void* user;
  uint32_t timeout_ms;
} mqjs_job_t;

esp_err_t mqjs_service_start(const mqjs_service_config_t* cfg, mqjs_service_t** out);
esp_err_t mqjs_service_eval(mqjs_service_t* s, const char* code, size_t len, uint32_t timeout_ms,
                            const char* filename, mqjs_eval_result_t* out);
esp_err_t mqjs_service_run(mqjs_service_t* s, const mqjs_job_t* job);
```

Service task loop (pseudocode):

```text
for msg in queue:
  ensure vm exists
  set deadline = now + msg.timeout
  switch msg.type:
    EVAL:
      v = JS_Eval(...)
      format ok/output/error via mqjs_vm helpers
      signal completion
    JOB:
      msg.fn(ctx, msg.user)
      signal completion
  clear deadline
```

This is the reusable heart; 0048-specific behavior becomes jobs and bootstraps layered above.

## Design Decisions

1) **The service does not define application globals (`encoder`, `emit`, etc.).**
   - Those are bindings owned by each firmware; the service only provides safe execution.

2) **Expose an explicit “job” primitive.**
   - This avoids forcing every use-case into “string eval”, and cleanly supports callback invocation.

## Alternatives Considered

1) Reuse `JsEvaluator` directly for service use.
   - Rejected: `JsEvaluator` is REPL-centric and does not model multi-producer queue ownership.

2) Provide only eval API, no generic jobs.
   - Rejected: bindings/callbacks inevitably need “run on VM thread” semantics.

## Implementation Plan

1) Implement `mqjs_service` using `MqjsVm` for:
   - stable opaque/log routing
   - deadline handling
   - safe value printing
2) Migrate 0048:
   - Replace most of `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp` with:
     - bootstraps (encoder + emit)
     - wrappers that call `mqjs_service_eval` and `mqjs_service_run`
   - Keep external symbol names stable for 0048 call sites:
     - `js_service_eval_to_json(...)` etc, at least initially.
3) Add a small “service smoke test” playbook (optional) for future reuse.

## Open Questions

1) Should the service expose “reset VM”?
   - Likely yes as an additive API (`mqjs_service_reset`) but not required for initial migration.

## References

- Reference implementation to extract from:
  - `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
- VM host primitive:
  - ticket `0054-MQJS-VM-COMPONENT`
