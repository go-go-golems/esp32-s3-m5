---
Title: Investigation Diary
Ticket: ATOMS3R-M12-QUICKJS-WORKER-FETCH
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - http
    - fetch
    - concurrency
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp
      Note: Current blocking firmware fetch adapter using esp_http_client.
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.h
      Note: Current synchronous HostOps.fetch and FetchRequest/FetchResult DTOs.
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp
      Note: Current fetch parser, response object, and Promise helpers.
    - Path: components/qjs_service/qjs_service.cpp
      Note: Owner-task loop and existing Promise job drain after eval.
    - Path: 0103-atoms3r-m12-native-quickjs/examples/scripts/fetch-healthz.js
      Note: Current fetch smoke example.
ExternalSources: []
Summary: Chronological diary for the worker-backed firmware fetch design ticket.
LastUpdated: 2026-06-25T20:55:00-07:00
WhatFor: Use to resume design or implementation of worker-backed fetch.
WhenToUse: Read before moving fetch network I/O off the QuickJS owner task.
---

# Diary

## Goal

Capture the research and design path for replacing the current blocking firmware `fetch()` adapter with a worker-backed architecture that keeps network I/O off the QuickJS owner task.

## Step 1: Create the worker fetch design ticket

This step creates the follow-on ticket for worker-backed `fetch()`. The existing firmware `fetch()` is hardware-validated and correct for small requests, but it performs `esp_http_client` work on the QuickJS owner task. This ticket records how to move the network wait to a worker task while preserving JavaScript Promise semantics and QuickJS ownership rules.

The design is intentionally separate from async route handlers. Worker fetch changes where outbound HTTP work runs. Async route handlers change how inbound dynamic route responses await Promise-returning handlers. They should be designed together but implemented in separate, reviewable steps.

### Prompt Context

**User prompt (verbatim):** "ok, create 2 new docmgr ticket then for each of those, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create a dedicated docmgr ticket for worker-backed fetch, write a detailed intern-facing analysis/design/implementation guide, and prepare it for reMarkable upload.

**Inferred user intent:** Turn the open question from the final project report into actionable design work for a future implementation ticket.

**Commit (code):** N/A — ticket/design step only.

### What I did

- Created ticket `ATOMS3R-M12-QUICKJS-WORKER-FETCH`.
- Added a design document at `design-doc/01-analysis-design-and-implementation-guide.md`.
- Added this diary at `reference/01-investigation-diary.md`.
- Replaced generated tasks with a phased implementation plan.
- Gathered evidence from:
  - `components/qjs_service/include/qjs_service.h`
  - `components/qjs_service/qjs_service.cpp`
  - `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.{h,cpp}`
  - `0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp`
  - `0103-atoms3r-m12-native-quickjs/examples/scripts/fetch-healthz.js`

### Why

- The current `op_fetch()` blocks the QuickJS owner task for the duration of `esp_http_client` network I/O.
- That is acceptable for the first milestone, but it limits concurrency and makes future route handlers that call `fetch()` more expensive.
- Worker-backed fetch allows network waiting to happen off the owner task while Promise settlement remains on the owner task.

### What worked

- The existing `FetchRequest` and `FetchResult` structs are plain-native and contain no `JSValue`, making them suitable for cross-task transfer.
- The current blocking adapter is a good reference implementation for the worker's network I/O function.
- The existing Promise-drain bug fix clarifies that settlement jobs must also drain pending jobs after resolving or rejecting a Promise.

### What didn't work

- No implementation was attempted in this step.
- The existing `HostOps.fetch` contract is synchronous, so worker fetch likely needs an optional async HostOps path or a carefully justified firmware-only override.

### What I learned

- The worker must never touch QuickJS. It should only run `esp_http_client` and write native result data.
- The hardest part is not the HTTP client code; it is safe delayed Promise settlement across reset and owner-task boundaries.
- A stale worker completion after `js reset` needs generation-based invalidation.

### What was tricky to build

- The design has to preserve the existing JavaScript API while changing when and where native work happens. That requires a pending operation table, resolver value ownership, completion jobs, and reset invalidation.
- Promise settlement is a two-step operation: first resolve or reject the Promise on the owner task, then drain pending jobs so user `.then()`/`.catch()` callbacks actually run.

### What warrants a second pair of eyes

- Review whether `HostOps` should gain an optional `fetch_async` callback or whether firmware should override `fetch()` locally.
- Review pending table ownership and reset invalidation carefully; this is where use-after-reset bugs are most likely.
- Review worker stack sizing, queue capacity, and backpressure behavior before implementation.

### What should be done in the future

- Add host fake-async tests before firmware worker code.
- Implement a single worker and bounded queue first.
- Validate reset during a pending fetch on hardware.
- Keep HTTPS/TLS out of this ticket.

### Code review instructions

- Start with `http_namespace_core.h::FetchRequest`, `FetchResult`, and `HostOps`.
- Review `http_namespace.cpp::op_fetch()` as the behavior-preserving reference for worker I/O.
- Review `qjs_service.cpp::drain_pending_jobs()` and the owner-task message loop before designing settlement jobs.
- Confirm no worker code calls any `JS_*` API.

### Technical details

- Current firmware fetch supports `http://` only.
- Current default fetch timeout: `1000 ms`, capped by the parser at `5000 ms`.
- Current default max response body: `16 KiB`.
- Current firmware fetch read chunk: `512` bytes.
