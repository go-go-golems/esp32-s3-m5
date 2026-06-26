---
Ticket: ATOMS3R-M12-QUICKJS-HOST-FETCH
Title: Tasks
Status: active
Topics:
  - atoms3r
  - esp32s3
  - quickjs
  - javascript
  - firmware
  - http
  - tooling
DocType: tasks
---

# Tasks

## Phase 0 — Ticket and guide

- [x] **HF0.1 — Create ticket workspace.** Create `ATOMS3R-M12-QUICKJS-HOST-FETCH`.
- [x] **HF0.2 — Inspect existing host pattern and firmware boundaries.** Read the 0102 native host, 0103 HTTP/static server, storage streaming API, and `qjs_service` job API.
- [x] **HF0.3 — Write intern-facing analysis/design/implementation guide.** Explain the shared host/firmware core, QuickJS `http` namespace, route dispatch, and `fetch()` design.
- [x] **HF0.4 — Relate files, update changelog, run doctor, upload to reMarkable, and commit docs.**

## Phase 1 — Shared host/firmware core

- [x] **HF1.1 — Define portable HTTP QuickJS core.** Create `http_namespace_core.{h,cpp}` that owns JavaScript-facing objects and route state without ESP-IDF headers.
- [x] **HF1.2 — Define host operation table.** Use callbacks for start/stop/static/fetch so firmware and desktop adapters share JS binding code.
- [x] **HF1.3 — Add desktop native host.** Add a `host/native-http` build similar to the 0102 native host.
- [x] **HF1.4 — Add host smoke tests.** Run scripts locally without a connected AtomS3R.

## Phase 2 — Firmware `http` namespace

- [x] **HF2.1 — Add ESP-IDF wrapper.** Install global `http` through `qjs_service_run()` and bridge to `http_server_*` functions.
- [x] **HF2.2 — Reinstall/clear on reset.** Ensure `js reset` clears dynamic route references and reinstalls a fresh namespace.
- [x] **HF2.3 — Expose lifecycle/static API.** Implement `http.status()`, `http.start()`, `http.stop()`, `http.static()`, and `http.clearStatic()`.
- [x] **HF2.4 — Validate host and firmware parity.** Confirm scripts that pass on host run on device when reconnected.

## Phase 3 — Dynamic routes

- [x] **HF3.1 — Add `http.get(path, handler)`.** Store duplicated QuickJS callbacks in the shared core.
- [x] **HF3.2 — Add request and response DTOs.** Support bounded path/query/header/body fields and text/json response helpers.
- [x] **HF3.3 — Dispatch through owner task.** Ensure firmware HTTP server tasks call into QuickJS only via `qjs_service_run()`.
- [x] **HF3.4 — Validate dynamic route smoke.** Test `http.get('/api/hello', () => ({ok:true}))` on host first, then on device.

## Phase 4 — `fetch()` API

- [x] **HF4.1 — Define bounded `fetch()` contract.** Decide supported URL schemes, methods, headers, body caps, timeout caps, and response fields.
- [x] **HF4.2 — Implement host adapter.** Implement host-side HTTP fetch for local/off-device tests.
- [x] **HF4.3 — Implement firmware adapter.** Use ESP-IDF HTTP client or a worker task to avoid unsafe QuickJS access from network callbacks.
- [x] **HF4.4 — Validate fetch smoke.** Test `await fetch('http://.../healthz')` or the accepted embedded equivalent.

## Phase 5 — Script workflow

- [x] **HF5.1 — Add `js run <virtual-path>`.** Load server scripts from `/scripts` using the bounded storage API.
- [ ] **HF5.2 — Add examples.** Provide host/device examples for static serving, dynamic routes, and fetch.
- [ ] **HF5.3 — Document recovery policy.** Keep USB Serial/JTAG console as the authoritative recovery path; no autoload until disable controls exist.
