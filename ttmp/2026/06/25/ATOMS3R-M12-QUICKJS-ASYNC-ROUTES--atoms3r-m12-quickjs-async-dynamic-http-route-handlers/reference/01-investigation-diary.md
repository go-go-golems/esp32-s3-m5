---
Title: Investigation Diary
Ticket: ATOMS3R-M12-QUICKJS-ASYNC-ROUTES
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - http
    - concurrency
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0103-atoms3r-m12-native-quickjs/README.md
      Note: Documents Promise-returning route support and error mapping (commit 0a620dd)
    - Path: 0103-atoms3r-m12-native-quickjs/examples/scripts/async-routes.js
      Note: Device async route example for js run workflow (commit 0a620dd)
    - Path: 0103-atoms3r-m12-native-quickjs/host/native-http/examples/async-routes.js
      Note: Host async route example (commit 0a620dd)
    - Path: 0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh
      Note: Host smoke coverage for Promise
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp
      Note: Current firmware dynamic route bridge through qjs_service_run.
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp
      Note: |-
        Current synchronous handler result conversion.
        Promise-aware dynamic route dispatch
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_server.cpp
      Note: |-
        Dynamic-first/static-fallback HTTP wildcard dispatch.
        504 Gateway Timeout status line for never-settling route Promises (commit 0a620dd)
    - Path: components/qjs_service/qjs_service.cpp
      Note: Existing eval-only Promise drain; async route dispatch must decide where to drain jobs.
ExternalSources: []
Summary: Chronological diary for the async dynamic route handler design ticket.
LastUpdated: 2026-06-25T22:35:00-07:00
WhatFor: Use to resume design or implementation of Promise-aware dynamic route handlers.
WhenToUse: Read before changing `http.get()` dispatch, Promise draining, or route response conversion.
---


# Diary

## Goal

Capture the research and design path for adding Promise-aware async dynamic HTTP route handlers to the AtomS3R QuickJS firmware.

## Step 1: Create the async route handler design ticket

This step creates the follow-on ticket for dynamic route handlers that can return Promises. The existing firmware can dispatch synchronous `http.get()` handlers safely through the QuickJS owner task, but it does not await Promise-returning handlers. This ticket records the architecture needed to add that behavior without violating QuickJS ownership rules.

The design intentionally separates async route dispatch from worker-backed fetch. Async route dispatch is about awaiting a handler's returned Promise inside the HTTP dispatch job. Worker-backed fetch is about moving network I/O off the owner task. They interact, but each has a different implementation boundary.

### Prompt Context

**User prompt (verbatim):** "ok, create 2 new docmgr ticket then for each of those, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a dedicated docmgr ticket for async dynamic route handlers, write a detailed intern-facing analysis/design/implementation guide, and prepare it for reMarkable upload.

**Inferred user intent:** Turn the open question from the final project report into actionable design work for a future implementation ticket.

**Commit (code):** N/A — ticket/design step only.

### What I did

- Created ticket `ATOMS3R-M12-QUICKJS-ASYNC-ROUTES`.
- Added a design document at `design-doc/01-analysis-design-and-implementation-guide.md`.
- Added this diary at `reference/01-investigation-diary.md`.
- Replaced generated tasks with a phased implementation plan.
- Gathered evidence from:
  - `components/qjs_service/include/qjs_service.h`
  - `components/qjs_service/qjs_service.cpp`
  - `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.{h,cpp}`
  - `0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp`
  - `0103-atoms3r-m12-native-quickjs/main/http_server.cpp`
  - `0103-atoms3r-m12-native-quickjs/main/js_command.cpp`

### Why

- The current route dispatcher converts handler return values synchronously.
- Async JavaScript functions return Promises, not immediate response objects.
- The current Promise drain exists only in the eval path, not in dynamic route dispatch.
- A future route that calls `await fetch(...)` needs the dispatch path to await Promise settlement before sending the HTTP response.

### What worked

- The codebase already has the key pieces needed for a clean design:
  - owner-task jobs via `qjs_service_run()`,
  - response conversion in the shared core,
  - dynamic/static routing separation in `http_server.cpp`,
  - eval-time Promise draining as a precedent.
- The design can preserve the existing `http.get(path, handler)` API and add support for `Promise<response>` without changing synchronous handlers.

### What didn't work

- No implementation was attempted in this step.
- The existing `qjs_service` drain helper is private and eval-oriented, so the implementation will need either a reusable internal helper or route-dispatch-specific drain logic.

### What I learned

- The core distinction is route miss versus route failure. `ESP_ERR_NOT_FOUND` should still mean "fall back to static serving," while Promise rejection should produce a dynamic route error response.
- The simplest safe design is to await the Promise inside the existing owner-task dispatch job, not to make the HTTP server task Promise-aware.

### What was tricky to build

- The design has to preserve two invariants at once: the HTTP server task must not touch QuickJS, and a matched dynamic route must not fall through to static serving just because its Promise rejects.
- Promise settlement requires running QuickJS pending jobs. Running those jobs in the wrong place can either fail to execute user callbacks or accidentally execute JavaScript on the wrong task.

### What warrants a second pair of eyes

- Review the proposed Promise detection and settlement capture approach before implementation.
- Review the timeout/error mapping: 500 for rejection, optional 504 for timeout, no static fallback after route match.
- Review whether dispatch should await only native-resolved Promises or any thenable.

### What should be done in the future

- Implement host smoke tests first.
- Add Promise-aware route dispatch in the shared core.
- Validate on hardware with Promise-resolving, async-function, rejection, and timeout routes.

### Code review instructions

- Start with `http_namespace_core.cpp::convert_handler_result()` and `Runtime::dispatch_get()`.
- Then review `http_namespace.cpp::dynamic_get_handler()` and `dispatch_get_job()`.
- Confirm all QuickJS access remains on the owner task.
- Validate with host smoke and `idf.py -C 0103-atoms3r-m12-native-quickjs build` before flashing.

### Technical details

- Current dynamic dispatch timeout: `1000 ms`.
- Existing pending Promise job cap in eval path: `64` jobs.
- Existing response shapes remain `{json: ...}`, `{text: ...}`, primitives, and `undefined`/`null`.

## Step 2: Implement and hardware-validate Promise-returning routes

This step implements the route Promise design in the shared HTTP core. Dynamic route handlers may now return either an immediate response object or a real QuickJS Promise that settles to a supported response object. The dispatch path drains bounded pending Promise jobs while still running on the QuickJS owner task.

The key behavior change is that a matched route failure is no longer treated as a route miss. A Promise rejection returns a 500 response, and a Promise that cannot settle during the bounded drain returns a 504 response. Static fallback is still reserved for true route misses.

### Prompt Context

**User prompt (verbatim):** "route Promise"

**Assistant interpretation:** Implement the async dynamic route handler follow-on first, focusing on Promise-returning `http.get()` handlers.

**Inferred user intent:** Move from design to code for Promise-aware route dispatch before worker-backed fetch.

**Commit (code):** `0a620dd` — "0103: support Promise-returning HTTP routes"

### What I did

- Updated `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp`:
  - added Promise detection with `JS_PromiseState`,
  - added bounded route Promise draining with `JS_ExecutePendingJob`,
  - added route error response helpers,
  - preserved synchronous handler conversion,
  - converted Promise fulfillment through existing response conversion,
  - mapped Promise rejection to 500,
  - mapped never-settling or excessive pending jobs to 504.
- Updated `0103-atoms3r-m12-native-quickjs/main/http_server.cpp` with `504 Gateway Timeout` status-line mapping.
- Added host example `0103-atoms3r-m12-native-quickjs/host/native-http/examples/async-routes.js`.
- Extended `0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh` to cover:
  - `Promise.resolve(...)` route,
  - `async function` route,
  - rejected Promise route.
- Added device example `0103-atoms3r-m12-native-quickjs/examples/scripts/async-routes.js`.
- Updated firmware example docs and README to document Promise-returning routes.
- Ran host smoke.
- Built firmware.
- Flashed AtomS3R over the by-id USB Serial/JTAG path.
- Hardware-validated Promise-resolving, async-function, rejected, and never-settling routes.
- Validated runtime usability and reset behavior after timeout route dispatch.

### Why

- The previous dynamic route implementation only converted immediate handler results. It did not understand that `async function` returns a Promise.
- The existing `fetch()` Promise job drain fixed console eval, but route dispatch uses `qjs_service_run()` and therefore needed its own bounded drain point.
- The route miss versus route failure distinction matters because static fallback should not hide JavaScript errors.

### What worked

- Host smoke passed:
  - `/api/promise` returned `200` with `{"ok":true,"kind":"promise","path":"/api/promise"}`.
  - `/api/async` returned `200` with `{"ok":true,"kind":"async-value","path":"/api/async"}`.
  - `/api/reject` returned `500` with `route promise rejected: Error: route boom`.
  - Existing fetch smoke still passed.
- Firmware build passed:
  - Command: `idf.py -C 0103-atoms3r-m12-native-quickjs build`.
  - Binary size: `0x16c190`.
  - App partition free: `0x293e70` bytes, 64% free.
- Hardware validation passed at `192.168.4.22`:
  - `/async-promise` returned `HTTP/1.1 200 OK` and body `{"ok":true,"kind":"promise","path":"/async-promise"}`.
  - `/async-await` returned `HTTP/1.1 200 OK` and body `{"ok":true,"kind":"async-value","path":"/async-await"}`.
  - `/async-reject` returned `HTTP/1.1 500 Internal Server Error` and body `route promise rejected: Error: route boom`.
  - `/async-never` returned `HTTP/1.1 504 Gateway Timeout` and body `route promise error: route promise did not settle`.
- After `/async-never`, `js eval "1+2"` still returned `3`.
- After `js reset`, `typeof http + " " + http.status().running` returned `object true`.
- After reset, `/async-promise` returned `404 Not Found` and `/healthz` returned `200 OK` on retry.

### What didn't work

- The first post-reset `/healthz` curl timed out during a WiFi beacon timeout/reconnect window. The immediate retry succeeded with `HTTP/1.1 200 OK` and body `ok`. The route reset behavior was still validated because `/async-promise` returned `404` after reset.
- This implementation supports real QuickJS Promise objects. It does not attempt generic JavaScript thenable assimilation for arbitrary objects with a `then` method.

### What I learned

- Promise-aware route dispatch can be added without changing the public `http.get(path, handler)` API.
- The shared core is the right place for Promise detection because Promise state and response conversion are JavaScript semantics, not HTTP server semantics.
- A route that returns a never-settling Promise can fail quickly and safely when `JS_ExecutePendingJob` reports no pending jobs and the Promise is still pending.

### What was tricky to build

- The route dispatch path previously returned `false` both for route miss and handler failure. That is wrong for async errors. The implementation now returns a concrete 500/504 response for matched-route failures while preserving `false` for actual route miss.
- The Promise drain must run inside `Runtime::dispatch_get()` while already on the owner task. Running it in the HTTP server task would violate the QuickJS ownership rule.
- Rejected Promises need their rejection reason converted to text without throwing a second exception. The implementation uses `JS_PromiseResult()` and string conversion to produce the error body.

### What warrants a second pair of eyes

- Review whether only real Promises are sufficient, or whether route dispatch should call `Promise.resolve(result)` to assimilate thenables.
- Review whether `504 Gateway Timeout` is the right status for a pending Promise with no pending jobs versus `500`.
- Review whether route dispatch should expose a configurable pending-job cap instead of the current `64` jobs.
- Review whether conversion errors should remain 500 text responses or become structured JSON errors.

### What should be done in the future

- Add worker-backed fetch before encouraging routes to perform slow upstream network calls.
- Consider async route examples that call `fetch()` after worker-backed fetch exists.
- Consider a per-route timeout option if scripts need longer bounded setup.

### Code review instructions

- Start with `http_namespace_core.cpp::Runtime::dispatch_get()`.
- Review the helper functions near the top of `http_namespace_core.cpp`:
  - `is_promise`,
  - `drain_route_promise`,
  - `make_error_response`,
  - `convert_or_error_response`.
- Review `http_server.cpp::status_line_for()` for the added 504 mapping.
- Review host coverage in `host/native-http/tests/run-smoke.sh`.
- Validate with:
  - `0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh`
  - `idf.py -C 0103-atoms3r-m12-native-quickjs build`
  - hardware curls for `/async-promise`, `/async-await`, `/async-reject`, and `/async-never`.

### Technical details

- Route Promise drain cap: `64` pending jobs.
- Firmware validation IP: `192.168.4.22`.
- Final build size for this milestone: `0x16c190`.
- Status mapping added: `504 Gateway Timeout`.
