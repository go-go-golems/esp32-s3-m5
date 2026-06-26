---
Title: Investigation Diary
Ticket: ATOMS3R-M12-QUICKJS-HOST-FETCH
Status: active
Topics:
    - atoms3r
    - esp32s3
    - quickjs
    - javascript
    - firmware
    - http
    - tooling
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0103-atoms3r-m12-native-quickjs/host/native-http/src/host_http_ops.cpp
      Note: Desktop HostOps adapter with in-memory server state and POSIX http:// fetch (commit 3737dfd)
    - Path: 0103-atoms3r-m12-native-quickjs/host/native-http/src/main.cpp
      Note: Desktop QuickJS host runner
    - Path: 0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh
      Note: Host smoke test for http.get dispatch and fetch (commit 3737dfd)
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp
      Note: Shared JavaScript http/fetch binding core
    - Path: 0103-atoms3r-m12-native-quickjs/main/http_namespace_core.h
      Note: |-
        Uncommitted Phase 1 implementation seed created before this ticket request
        Portable QuickJS HTTP/fetch core interface and HostOps callback boundary (commit 3737dfd)
    - Path: ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/design-doc/01-analysis-design-and-implementation-guide.md
      Note: Primary intern-facing design guide for host HTTP and fetch work
ExternalSources: []
Summary: Chronological diary for the shared host/firmware QuickJS HTTP namespace and fetch API work.
LastUpdated: 2026-06-25T19:15:00-07:00
WhatFor: Use to resume implementation of the desktop host, firmware HTTP namespace, dynamic routes, and fetch API.
WhenToUse: Read before modifying `http_namespace_core`, host native HTTP tooling, or firmware reset/dispatch paths.
---



# Diary

## Goal

Capture the design and implementation path for a shared host/firmware QuickJS HTTP API for `0103-atoms3r-m12-native-quickjs`, including a desktop host that works while the AtomS3R is disconnected and a bounded `fetch()` API.

## Step 1: Create the host/fetch ticket and intern guide

This step creates a dedicated ticket for the host-testable HTTP namespace and `fetch()` API. The design is deliberately separate from the earlier HTTP static-serving ticket because it introduces a new portability boundary: the same QuickJS binding core should compile in ESP-IDF firmware and in a desktop native host.

The guide uses the 0102 native-host pattern as the primary precedent. The firmware already has host-owned HTTP server and storage streaming primitives; this ticket explains how to expose those primitives to JavaScript without losing owner-task safety, reset safety, or the ability to run tests while the device is disconnected.

### Prompt Context

**User prompt (verbatim):**

> Create a new docmgr ticket for this work, and Create  a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable.
>
>
> We also want to implement a fetch() API.

**Assistant interpretation:** Create a new docmgr ticket for the shared desktop-host and firmware QuickJS HTTP work, write a detailed intern-facing design/implementation guide, include `fetch()` in the design, and upload the result to reMarkable.

**Inferred user intent:** Continue implementation without the disconnected AtomS3R by making the HTTP/Fetch API testable on the host, while preserving the same firmware binding code path for later device validation.

**Commit (code):** N/A — documentation/design step only.

### What I did

- Created ticket `ATOMS3R-M12-QUICKJS-HOST-FETCH`.
- Added a design document: `design-doc/01-analysis-design-and-implementation-guide.md`.
- Added this investigation diary: `reference/01-investigation-diary.md`.
- Replaced the generated task list with a phased implementation plan covering:
  - shared host/firmware QuickJS core,
  - desktop native host,
  - firmware `http` namespace,
  - dynamic `http.get()` routes,
  - bounded `fetch()` API,
  - script workflow.
- Inspected the 0102 native host precedent:
  - `0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/README.md`
  - `0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/Makefile`
  - `0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/src/pico_native_api.hpp`
- Inspected the current 0103 firmware boundaries:
  - `components/qjs_service/include/qjs_service.h`
  - `0103-atoms3r-m12-native-quickjs/main/http_server.{h,cpp}`
  - `0103-atoms3r-m12-native-quickjs/main/storage_namespace.h`
  - `0103-atoms3r-m12-native-quickjs/main/js_command.cpp`
  - `0103-atoms3r-m12-native-quickjs/main/app_main.cpp`

### Why

- The AtomS3R is disconnected, so a host executable is the fastest safe way to keep developing JavaScript API behavior.
- The HTTP namespace needs shared code because duplicated host mocks drift from firmware behavior.
- `fetch()` has enough ownership, timeout, memory, and Promise semantics that it deserves design before implementation.

### What worked

- `docmgr` created the ticket workspace and both documents successfully.
- The 0102 host pattern provides a clear precedent: portable QuickJS binding code plus host-only executable glue.
- The 0103 HTTP/static server already exposes enough native operations to become a `HostOps` adapter.

### What didn't work

- N/A for the ticket-creation step.

### What I learned

- The right abstraction is not a host-only mock. It is a portable QuickJS binding core with host operation callbacks.
- The first `fetch()` should be a bounded subset. Full browser Fetch would exceed the current milestone's memory, API, and runtime complexity.
- Dynamic routes and `fetch()` are related because both store or settle QuickJS values across native host work and must respect reset behavior.

### What was tricky to build

- The design had to balance two needs that pull in different directions. Host testing wants simple synchronous calls. Firmware correctness wants all QuickJS access serialized through `qjs_service_run()` and may eventually need a worker-backed `fetch()` path. The guide resolves this by keeping JavaScript API conversion in a shared core and allowing firmware adapters to evolve from bounded blocking fetch to worker-backed Promise settlement without changing scripts.
- Another tricky part is reset safety. Stored route callbacks are `JSValue`s tied to a specific runtime. The design therefore requires explicit route clearing around `js reset` and namespace reinstallation after reset.

### What warrants a second pair of eyes

- Review whether the first dynamic route API should use return-object handlers or an Express-like mutable response object.
- Review whether firmware `fetch()` may start as bounded blocking work or must be worker-backed from the beginning.
- Review Promise-job draining in `qjs_service_eval()` before documenting `await fetch(...)` as a console smoke command.

### What should be done in the future

- Implement `http_namespace_core.{h,cpp}` and host native HTTP tooling.
- Add firmware wrapper `http_namespace.{h,cpp}` and reset wiring.
- Add dynamic route dispatch through `qjs_service_run()`.
- Add bounded `fetch()` host adapter first, then firmware adapter.

### Code review instructions

- Start with the design document's file layout and API sketches.
- Compare the proposed host split against `0102-esp32-p4-visual-quickjs-repl/js/tools/native-host/`.
- Before reviewing firmware route dispatch, verify that no HTTP server task calls QuickJS directly.
- Before reviewing `fetch()`, verify body/header/timeout caps and reset behavior for pending work.

### Technical details

- Ticket path: `ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/`.
- Design doc: `design-doc/01-analysis-design-and-implementation-guide.md`.
- Existing uncommitted implementation seed at the time of ticket creation:
  - `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.h`
- The design guide treats that file as the intended start of Phase 1, but this step does not commit code.

## Step 2: Implement the shared QuickJS HTTP core and desktop native host

This step turns the host/fetch design into a working Phase 1 implementation. The shared core now installs a JavaScript `http` object and a bounded Promise-returning `fetch()` function without including ESP-IDF headers, and a desktop native host compiles the same core against upstream QuickJS so the API can be tested while the AtomS3R is disconnected.

The milestone proves the key portability boundary. JavaScript API behavior for `http.static()`, `http.get()`, direct route dispatch, and a small `fetch()` request can be exercised on the host before firmware wrapper code is added.

### Prompt Context

**User prompt (verbatim):** "go ahead. Keep a detailed diary as you work, and commit at appropriate intervals"

**Assistant interpretation:** Start implementing the host/fetch ticket, keep chronological docmgr diary notes, validate incrementally, and make focused commits.

**Inferred user intent:** Continue progress without the disconnected device by building host-testable shared code first, while preserving reviewable checkpoints.

**Commit (code):** `3737dfd` — "0103: add host-testable QuickJS HTTP core"

### What I did

- Implemented `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.h`.
- Implemented `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp`.
- Added host-native HTTP tooling under `0103-atoms3r-m12-native-quickjs/host/native-http/`:
  - `Makefile`
  - `README.md`
  - `src/main.cpp`
  - `src/host_http_ops.{h,cpp}`
  - `examples/server.js`
  - `examples/fetch.js`
  - `tests/run-smoke.sh`
- Implemented JavaScript globals in the shared core:
  - `http.status()`
  - `http.start(port)`
  - `http.stop()`
  - `http.static(prefix, virtualRoot)`
  - `http.clearStatic()`
  - `http.get(path, handler)`
  - `fetch(url, options)`
- Implemented route storage with duplicated QuickJS handler values and destructor cleanup.
- Implemented direct host dispatch for registered `GET` handlers.
- Implemented response conversion for primitive text, `{text}`, `{status,text}`, and `{json}` handler returns.
- Implemented a host `fetch()` adapter using POSIX sockets for bounded `http://` requests.
- Added a host smoke test that validates both dynamic route dispatch and fetch.
- Fixed the host Makefile object mapping so shared-core objects stay under `host/native-http/build/core/` instead of creating an accidental nested project directory.
- Removed generated build artifacts before committing.
- Committed the focused Phase 1 code as `3737dfd`.
- Marked tasks HF1.1 through HF1.4 complete.

### Why

- The AtomS3R is disconnected, so the desktop host is the only practical way to validate JavaScript-facing API behavior immediately.
- The shared core keeps JavaScript semantics in one place. Firmware and host adapters should differ only in their native I/O implementations.
- Host smoke tests reduce risk before adding firmware wrappers, reset hooks, and HTTP server task dispatch.

### What worked

- The native host builds with the vendored upstream QuickJS sources.
- `0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh` passed.
- The route smoke output was:
  - `native-http example boot`
  - `routes=1 mounts=1`
  - `DISPATCH status=200 content-type=application/json; charset=utf-8`
  - `{"ok":true,"method":"GET","path":"/api/hello"}`
- The fetch smoke output was:
  - `fetch status=200 ok=true`
  - `fetch body=ok`
  - `PASS native-http host smoke`
- Runtime teardown was adjusted so the `qjs_http::Runtime` is deleted before `JS_FreeContext(ctx)`, ensuring stored route `JSValue`s are freed while the context is still valid.

### What didn't work

- The first host Makefile mapped `$(MAIN_DIR)/http_namespace_core.cpp` into an object path containing `build/../../../...`, which created an accidental untracked nested path:
  - `0103-atoms3r-m12-native-quickjs/0103-atoms3r-m12-native-quickjs/main/http_namespace_core.o`
- Fix:
  - rewrote the Makefile with an explicit `$(BUILD)/core/http_namespace_core.o` target.
  - removed the accidental nested directory and host build directory.
  - reran the smoke test from a clean build.

### What I learned

- The 0102 native-host pattern transfers cleanly to the HTTP namespace: compile upstream QuickJS directly, keep the host runner small, and put JavaScript-visible semantics in a reusable core.
- Promise-shaped `fetch()` is testable on host as long as the runner explicitly drains QuickJS pending jobs with `JS_ExecutePendingJob()` after eval.
- The core must own JavaScript semantics such as route return conversion and fetch argument validation. Native adapters should only perform platform I/O.

### What was tricky to build

- QuickJS value lifetime is the main sharp edge. Route handlers are duplicated `JSValue`s, so the runtime wrapper must free them before the `JSContext*` disappears. The host runner originally used an automatic `qjs_http::Runtime` object that would have been destroyed after the context if left in declaration order. Allocating it explicitly and deleting it before `JS_FreeContext(ctx)` makes the teardown order clear.
- The `fetch()` API returns Promises even though the first host adapter performs a synchronous socket request. This keeps the JavaScript shape aligned with future firmware behavior, but it requires the host runner to execute pending Promise jobs after script evaluation.
- Response conversion needs to accept small useful forms without becoming a full Express clone. The current core supports string returns, text returns, status/text returns, and JSON returns; streaming responses remain intentionally out of scope.

### What warrants a second pair of eyes

- Review `http_namespace_core.cpp` for QuickJS ownership correctness around `JS_DefinePropertyValueStr`, `JS_SetPropertyUint32`, route handler duplication, and Promise response objects.
- Review whether `fetch()` should reject unsupported options with thrown synchronous exceptions or rejected Promises consistently. The current implementation returns rejected Promises for parse/fetch failures.
- Review host POSIX HTTP parsing. It intentionally supports only simple HTTP/1.0-style responses and bounded bodies, which is enough for smoke tests but not a general client.
- Review whether `http.get()` should allow replacing an existing route, as the current implementation does.

### What should be done in the future

- Add the firmware wrapper `http_namespace.{h,cpp}`.
- Install `http` and `fetch` at boot via `qjs_service_run()`.
- Clear stored route callbacks before `js reset` and reinstall after reset.
- Wire dynamic firmware HTTP dispatch through `qjs_service_run()`.
- Decide whether firmware `fetch()` starts as bounded blocking ESP-IDF HTTP client work or uses a worker-backed Promise settlement path.

### Code review instructions

- Start with `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.h` to understand the portable interface and `HostOps` callback table.
- Then review `0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp`, especially:
  - `Runtime::install_global()`
  - `Runtime::add_get_route()`
  - `Runtime::dispatch_get()`
  - `convert_handler_result()`
  - `parse_fetch_request()`
  - `make_fetch_response()`
- Review host-only glue in `host/native-http/src/main.cpp` and `host/native-http/src/host_http_ops.cpp` after the core.
- Validate with:
  - `0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh`

### Technical details

- Host smoke command:
  - `0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh`
- The smoke script starts a temporary Python `HTTPServer` on `127.0.0.1:18080` for the fetch test.
- The shared core currently supports `http://` fetch URLs only, `GET` and `POST`, 16 headers, 4096-byte request bodies, 16 KiB response bodies, and up to 5000 ms timeout.
- The host runner drains Promise jobs with `JS_ExecutePendingJob()`.
