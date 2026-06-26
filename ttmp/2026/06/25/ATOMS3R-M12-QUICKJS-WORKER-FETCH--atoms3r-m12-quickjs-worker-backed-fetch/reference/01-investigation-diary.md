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
    - Path: 0103-atoms3r-m12-native-quickjs/README.md
      Note: Documents worker-backed fetch limits and reset behavior (commit b209ac4)
    - Path: 0103-atoms3r-m12-native-quickjs/examples/scripts/fetch-healthz.js
      Note: |-
        Current fetch smoke example.
        Worker-backed fetch example comment (commit b209ac4)
    - Path: 0103-atoms3r-m12-native-quickjs/host/native-http/src/host_http_ops.cpp
      Note: Desktop fake async fetch adapter for HostOps.fetch_async coverage (commit b209ac4)
    - Path: 0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh
      Note: Host smoke coverage for sync and fake-async fetch paths (commit b209ac4)
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp
      Note: |-
        Current blocking firmware fetch adapter using esp_http_client.
        Firmware pending table
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp
      Note: |-
        Current fetch parser, response object, and Promise helpers.
        fetch() Promise capability path and shared resolve/reject/drain helpers (commit b209ac4)
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.h
      Note: |-
        Current synchronous HostOps.fetch and FetchRequest/FetchResult DTOs.
        Async HostOps fetch contract and settlement helper declarations (commit b209ac4)
    - Path: components/qjs_service/qjs_service.cpp
      Note: Owner-task loop and existing Promise job drain after eval.
ExternalSources: []
Summary: Chronological diary for the worker-backed firmware fetch design ticket.
LastUpdated: 2026-06-25T23:25:00-07:00
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

## Step 2: Implement and hardware-validate worker-backed fetch

This step moves firmware `fetch()` network I/O off the QuickJS owner task. The JavaScript API remains `fetch(url, options) -> Promise`, but the firmware path now creates a pending Promise on the owner context, queues a native request to a single `qjs_fetch` worker task, and posts a completion job back to the QuickJS owner task to resolve or reject the Promise.

The important ownership invariant is unchanged: the worker never calls `JS_*`. It only copies native request data, runs `esp_http_client`, and posts native result data. Resolver and rejector `JSValue`s stay in the owner-task pending table and are freed or settled only on the owner task.

### Prompt Context

**User prompt (verbatim):** "go ahead"

**Assistant interpretation:** Proceed with the remaining follow-on implementation, which is worker-backed firmware `fetch()`.

**Inferred user intent:** Complete the planned worker-fetch ticket after the async route Promise work was finished.

**Commit (code):** `b209ac4` — "0103: move firmware fetch onto worker task"

### What I did

- Extended `qjs_http::HostOps` with optional `fetch_async` while preserving the existing synchronous `fetch` fallback.
- Added shared-core helper functions:
  - `resolve_fetch_promise(...)`,
  - `reject_fetch_promise(...)`,
  - `drain_pending_jobs(...)`.
- Updated `js_fetch()` so hosts with `fetch_async` create a Promise capability and let the host adapter settle it later.
- Added firmware pending fetch state in `http_namespace.cpp`:
  - four-slot pending table,
  - monotonic request id,
  - generation id for reset invalidation,
  - stored request DTO,
  - owner-task-owned resolve/reject `JSValue`s.
- Added one `qjs_fetch` worker task and a bounded FreeRTOS queue.
- Reused the existing bounded `esp_http_client` implementation inside the worker.
- Added owner-task settlement jobs via `qjs_service_post()`.
- Added reset invalidation in `clear_http_job()` so `js reset` rejects active fetch Promises and ignores stale worker completions.
- Added a desktop `--fake-async-fetch` host flag to exercise the optional async HostOps path in host smoke tests.
- Updated README and example docs to explain worker-backed settlement, four-pending-fetch limit, delayed console output, and reset cancellation.

### Why

- The previous firmware `fetch()` performed network I/O on the QuickJS owner task. That made the runtime unavailable while `esp_http_client` waited for sockets, headers, or response bytes.
- Worker-backed fetch preserves the same JavaScript API while making slow network waits native-worker work.
- Reset invalidation is required because a worker completion can arrive after the QuickJS runtime that created the Promise has already been destroyed.

### What worked

- Host smoke passed from a clean build, including the new fake async fetch mode:
  - route dispatch smoke,
  - Promise route smoke,
  - async function route smoke,
  - rejected route smoke,
  - synchronous host fetch smoke,
  - fake async host fetch smoke.
- JS examples passed `node --check`.
- Firmware build passed:
  - command: `idf.py -C 0103-atoms3r-m12-native-quickjs build`,
  - binary size: `0x16d070`,
  - app partition free: `0x292f90` bytes, 64% free.
- Hardware flash/boot passed on AtomS3R by-id USB Serial/JTAG path.
- Boot log showed `0103_http_ns: fetch worker start`.
- Hardware `/healthz` fetch smoke passed:
  - eval returned `[object Promise]` quickly,
  - then the worker-settled callbacks printed `fetch status=200 ok=true` and `fetch body=ok` at the prompt.
- Delayed endpoint validation passed against workstation `192.168.4.119:18081`:
  - slow fetch returned after about `3329 ms`,
  - a separate `js eval "print(\"responsive2=\"+(2+3))"` ran during the pending fetch and printed `responsive2=5` before the slow response completed.
- Reset during pending fetch passed:
  - pending Promise printed `reset fetch err=fetch cancelled by QuickJS reset`,
  - namespace reinstall succeeded,
  - `typeof fetch + " " + typeof http + " " + http.status().running` returned `function object true`.
- Queue/pending saturation passed:
  - six delayed fetches produced immediate `er4:too many pending fetches` and `er5:too many pending fetches`,
  - first four delayed requests completed as `ok0`, `ok1`, `ok2`, `ok3`.

### What didn't work

- The first host smoke after editing hit a glibc heap assertion from an old build artifact. A clean host rebuild fixed it, and ASan also passed the first route dispatch case.
- A long console one-liner for delayed fetch validation was truncated into invalid JavaScript and produced `SyntaxError: unexpected token in expression: ''`. A shorter one-liner validated the intended behavior.
- The current host fake async adapter settles immediately rather than using a desktop thread/event loop. It still exercises the shared `fetch_async` Promise-capability path; firmware provides the true delayed worker-backed path.

### What I learned

- Once `fetch()` returns a Promise that is settled by a later posted job, console output from `.then()` callbacks can appear after the `js eval` result and prompt. This is expected for worker-backed settlement and is now documented.
- A four-slot pending table is enough to validate backpressure and keeps resolver ownership auditable.
- Generation invalidation is the simplest guard against stale worker completions after `js reset`.

### What was tricky to build

- The worker cannot hold or free any `JSValue`. The implementation solves this by copying only `FetchRequest` into `FetchWork` and only `FetchResult`/error text into `FetchCompletion`. Resolver values stay in the pending table and are only touched by owner-task jobs.
- Reset ordering matters. `js reset` first calls `clear_http_namespace_state()`, which now rejects and frees active fetch Promises, increments the generation, deletes the runtime wrapper, and then lets `qjs_service_reset()` destroy/recreate QuickJS. Later worker completions carry the old generation and are ignored.
- Promise settlement is not enough by itself. The settlement job also drains pending QuickJS Promise jobs so `.then()`/`.catch()` callbacks run without waiting for an unrelated future eval.

### What warrants a second pair of eyes

- Review `http_namespace.cpp::op_fetch_async()` and `settle_fetch_job()` for resolver ownership and all cleanup paths.
- Review the behavior when `qjs_service_post()` fails after a worker completes. The current implementation retries briefly and logs an error; a pathological queue failure could leave a pending Promise until reset.
- Review whether four pending fetches and one worker remain the right default after real scripts begin composing fetches from async routes.
- Review whether the public `HostOps.fetch_async` signature should remain QuickJS-aware or whether a later abstraction should hide Promise capability details behind a core-owned pending object.

### What should be done in the future

- Add a true desktop delayed async adapter if host-side timing behavior needs to be tested without hardware.
- Consider per-fetch cancellation once JavaScript gets an AbortController-like API.
- Consider one additional diagnostic counter in `http.status().limits` or `js status` for pending worker fetches if scripts need observability.
- Keep HTTPS/TLS out until memory and certificate handling are deliberately designed.

### Code review instructions

- Start with `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.h` and `http_namespace_core.cpp::js_fetch()` to understand the optional async contract.
- Then review `0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp` in this order:
  - pending structs and globals,
  - `op_fetch()` as the reused blocking worker I/O helper,
  - `op_fetch_async()` allocation/backpressure,
  - `fetch_worker_task()`,
  - `settle_fetch_job()`,
  - `invalidate_pending_fetches()`.
- Validate with:
  - `0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh`,
  - `idf.py -C 0103-atoms3r-m12-native-quickjs build`,
  - hardware delayed fetch plus immediate `js eval`,
  - `js reset` during a pending delayed fetch,
  - six delayed fetches to confirm bounded rejection.

### Technical details

- Worker task name: `qjs_fetch`.
- Worker stack: `6144` words.
- Worker priority: `6`.
- Pending fetch capacity: `4`.
- Worker queue capacity: `4` pointers.
- Settlement job timeout: `1000 ms`.
- Promise drain cap after settlement: `64` jobs.
- Firmware validation IP: `192.168.4.22`.
- Workstation delayed-test IP: `192.168.4.119:18081`.
