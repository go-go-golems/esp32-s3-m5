---
Title: Tasks
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
DocType: tasks
Intent: operational
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: Phased implementation checklist for Promise-aware async dynamic route handlers.
LastUpdated: 2026-06-25T20:55:00-07:00
WhatFor: Track implementation of async http.get handler support.
WhenToUse: Use while implementing or reviewing ATOMS3R-M12-QUICKJS-ASYNC-ROUTES.
---

# Tasks

## AR0 — Ticket setup and design

- [x] **AR0.1 — Create ticket workspace.** Create `ATOMS3R-M12-QUICKJS-ASYNC-ROUTES`.
- [x] **AR0.2 — Gather current-state evidence.** Inspect `qjs_service`, `http_namespace_core`, `http_namespace`, `http_server`, and `js_command`.
- [x] **AR0.3 — Write intern-facing design guide.** Document architecture, API, decisions, pseudocode, phases, tests, and risks.
- [x] **AR0.4 — Upload design bundle to reMarkable.** Publish guide and diary for review.

## AR1 — Host-core Promise route support

- [ ] **AR1.1 — Add Promise detection helper.** Detect Promise/thenable route return values in the shared core.
- [ ] **AR1.2 — Add settlement capture.** Attach fulfillment/rejection callbacks that capture resolved response values on the owner context.
- [ ] **AR1.3 — Drain pending jobs inside dispatch.** Drain until route Promise settles, times out, rejects, or exceeds job cap.
- [ ] **AR1.4 — Preserve synchronous behavior.** Existing `{json}` and `{text}` handlers must remain unchanged.
- [ ] **AR1.5 — Add host smoke coverage.** Validate Promise-resolving and Promise-rejecting handlers in `host/native-http`.

## AR2 — Firmware integration

- [ ] **AR2.1 — Pass dispatch timeout/options.** Ensure firmware route dispatch passes bounded settlement settings.
- [ ] **AR2.2 — Map async errors to HTTP responses.** Route miss must still fall back to static; rejection/timeout must return error responses.
- [ ] **AR2.3 — Build firmware.** Run `idf.py -C 0103-atoms3r-m12-native-quickjs build`.
- [ ] **AR2.4 — Validate reset safety.** Confirm `js reset` clears routes and no pending Promise state survives.

## AR3 — Hardware validation

- [ ] **AR3.1 — Validate Promise.resolve route.** Register and curl `/async-ok`.
- [ ] **AR3.2 — Validate async function route.** Register and curl a route using `async function` and `await`.
- [ ] **AR3.3 — Validate rejection route.** Confirm rejected Promise maps to deterministic HTTP error.
- [ ] **AR3.4 — Validate timeout/job-cap route.** Confirm long Promise chains or never-settling handlers fail safely.

## AR4 — Documentation and examples

- [ ] **AR4.1 — Add async route examples.** Add checked-in `/scripts` examples.
- [ ] **AR4.2 — Update firmware README.** Document async route support and limitations.
- [ ] **AR4.3 — Record diary and close ticket.** Update changelog/diary and close when validated.
