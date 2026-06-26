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
    - Path: components/qjs_service/qjs_service.cpp
      Note: Existing eval-only Promise drain; async route dispatch must decide where to drain jobs.
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp
      Note: Current firmware dynamic route bridge through qjs_service_run.
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp
      Note: Current synchronous handler result conversion.
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_server.cpp
      Note: Dynamic-first/static-fallback HTTP wildcard dispatch.
ExternalSources: []
Summary: Chronological diary for the async dynamic route handler design ticket.
LastUpdated: 2026-06-25T20:55:00-07:00
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
