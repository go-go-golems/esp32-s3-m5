---
Ticket: ATOMS3R-M12-QUICKJS-HTTP
Title: Tasks
Status: active
Topics:
  - atoms3r
  - esp32s3
  - quickjs
  - javascript
  - firmware
  - http
DocType: tasks
---

# Tasks

## Phase 0 — Ticket and guide

- [x] **H0.1 — Create HTTP ticket workspace.** Create `ATOMS3R-M12-QUICKJS-HTTP`.
- [x] **H0.2 — Inspect go-go-goja Express/static asset patterns.** Read `modules/express` and `modules/fs/http.go` for API inspiration.
- [x] **H0.3 — Write intern-facing analysis/design/implementation guide.** Explain ESP-IDF HTTP serving, route tables, static assets, QuickJS dispatch, and limits.
- [x] **H0.4 — Relate files, update changelog, run doctor, upload to reMarkable, and commit docs.** Uploaded `AtomS3R QuickJS HTTP Guide.pdf` to `/ai/2026/06/25/ATOMS3R-M12-QUICKJS-HTTP`.

## Phase 1 — HTTP host service

- [x] **H1.1 — Add HTTP server module.** Add `http_namespace.{h,cpp}` or `http_server.{h,cpp}` with `esp_http_server` lifecycle.
- [x] **H1.2 — Add console diagnostics.** Implement `http status`, `http start [port]`, and `http stop`.
- [x] **H1.3 — Add `/healthz`.** Serve a firmware-owned health route without QuickJS.
- [x] **H1.4 — Validate over WiFi.** Use `curl` against the AtomS3R IP.

## Phase 2 — Static assets

- [ ] **H2.1 — Add static mount table.** Map URL prefixes to storage virtual roots.
- [ ] **H2.2 — Stream files from storage.** Serve chunks directly from FatFs, outside QuickJS.
- [ ] **H2.3 — Add MIME detection.** Support html/js/css/json/png/jpeg/svg/text.
- [ ] **H2.4 — Validate `/static/index.html`.** Write asset through storage, mount it, and fetch it with curl.

## Phase 3 — Express-like QuickJS API

- [ ] **H3.1 — Install global `http` namespace.** Use `qjs_service_run()` and reinstall/clear safely after `js reset`.
- [ ] **H3.2 — Add `http.status/start/stop/static`.** Keep lifecycle host-owned.
- [ ] **H3.3 — Add dynamic route registration.** Implement `http.get()` and route handler dispatch with request/response DTOs.
- [ ] **H3.4 — Validate JSON/text dynamic routes.** Keep request body and response limits small.

## Phase 4 — Script workflow

- [ ] **H4.1 — Add or depend on `js run <virtual-path>`.** Server scripts should live under `/scripts`.
- [ ] **H4.2 — Add example `/scripts/server.js`.** Register static and dynamic routes.
- [ ] **H4.3 — Decide autoload policy.** Do not autoload without console disable/recovery controls.
