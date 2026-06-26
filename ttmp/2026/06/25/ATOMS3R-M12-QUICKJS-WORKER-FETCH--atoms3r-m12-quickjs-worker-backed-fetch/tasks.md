---
Title: Tasks
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
DocType: tasks
Intent: operational
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Phased implementation checklist for worker-backed firmware fetch.
LastUpdated: 2026-06-25T20:55:00-07:00
WhatFor: Track implementation of worker-backed fetch that keeps network I/O off the QuickJS owner task.
WhenToUse: Use while implementing or reviewing ATOMS3R-M12-QUICKJS-WORKER-FETCH.
---

# Tasks

## WF0 — Ticket setup and design

- [x] **WF0.1 — Create ticket workspace.** Create `ATOMS3R-M12-QUICKJS-WORKER-FETCH`.
- [x] **WF0.2 — Gather current-state evidence.** Inspect current blocking fetch, FetchRequest/FetchResult, qjs_service Promise draining, and fetch examples.
- [x] **WF0.3 — Write intern-facing design guide.** Document architecture, API, decisions, pseudocode, phases, tests, and risks.
- [x] **WF0.4 — Upload design bundle to reMarkable.** Publish guide and diary for review.

## WF1 — Shared-core async fetch contract

- [ ] **WF1.1 — Add optional async HostOps fetch.** Extend `HostOps` while preserving synchronous fallback.
- [ ] **WF1.2 — Add Promise capability helper.** Create pending Promise plus resolve/reject handles on the owner context.
- [ ] **WF1.3 — Preserve host synchronous fetch.** Existing host smoke must still pass unchanged.
- [ ] **WF1.4 — Add host fake async adapter test.** Validate delayed resolve and rejection through the shared core.

## WF2 — Firmware pending operation model

- [ ] **WF2.1 — Define pending fetch table.** Include id, generation, resolver values, native request/result, and status.
- [ ] **WF2.2 — Add bounded allocation/backpressure.** Reject immediately when capacity is exhausted.
- [ ] **WF2.3 — Enforce owner-task-only JSValue access.** Worker code must not call `JS_*` APIs.
- [ ] **WF2.4 — Add reset invalidation.** Reject/free pending resolver values before runtime reset and ignore stale completions.

## WF3 — Worker task and settlement jobs

- [ ] **WF3.1 — Add worker task and queue.** Single worker first; no worker pool yet.
- [ ] **WF3.2 — Move esp_http_client I/O to worker.** Reuse current caps and cleanup paths.
- [ ] **WF3.3 — Post owner-task settlement job.** Resolve/reject Promise on owner task only.
- [ ] **WF3.4 — Drain jobs after settlement.** Ensure `.then()`/`.catch()` callbacks run.

## WF4 — Validation

- [ ] **WF4.1 — Build firmware.** Run `idf.py -C 0103-atoms3r-m12-native-quickjs build`.
- [ ] **WF4.2 — Validate fetch smoke.** Confirm `/healthz` fetch still prints status/body.
- [ ] **WF4.3 — Validate delayed endpoint.** Confirm owner-task responsiveness during a slow request.
- [ ] **WF4.4 — Validate reset during pending fetch.** Confirm no crash and stale completion is ignored.
- [ ] **WF4.5 — Validate queue saturation.** Confirm excess fetch calls reject cleanly.

## WF5 — Documentation and examples

- [ ] **WF5.1 — Update README and examples.** Document worker-backed behavior and limits.
- [ ] **WF5.2 — Record diary and close ticket.** Update changelog/diary and close when validated.
