---
Ticket: ATOMS3R-M12-QUICKJS-STORAGE
Title: Tasks
Status: active
Topics:
  - atoms3r
  - esp32s3
  - quickjs
  - javascript
  - firmware
  - storage
DocType: tasks
---

# Tasks

## Phase 0 — Ticket and guide

- [x] **S0.1 — Create storage ticket workspace.** Create `ATOMS3R-M12-QUICKJS-STORAGE`.
- [x] **S0.2 — Write intern-facing analysis/design/implementation guide.** Explain FatFs, virtual roots, QuickJS ownership, limits, console recovery, and validation.
- [x] **S0.3 — Relate files, update changelog, run doctor, upload to reMarkable, and commit docs.** Uploaded `AtomS3R QuickJS Storage Guide.pdf` to `/ai/2026/06/25/ATOMS3R-M12-QUICKJS-STORAGE`.

## Phase 1 — Implementation status imported from 0103

- [x] **S1.1 — Implement bounded storage module.** Added `storage_namespace.{h,cpp}` in commit `521d5a2`.
- [x] **S1.2 — Add console diagnostics.** `storage status`, `mount`, `list`, `read`, `write` exist.
- [x] **S1.3 — Add QuickJS namespace.** `storage.status/list/stat/readText/writeText` exists and reinstalls after `js reset`.
- [x] **S1.4 — Validate on hardware.** Explicit dev-format, console read/write, JS read/write/list/stat, reset persistence, and board-reset persistence passed.

## Phase 2 — Follow-up work

- [ ] **S2.1 — Add `js run <virtual-path>`.** Evaluate scripts from `/scripts` with filename-aware errors and timeout.
- [ ] **S2.2 — Add repeated storage soak.** Run repeated read/write/reset cycles and track heap drift.
- [ ] **S2.3 — Decide casing behavior.** Document or normalize FatFs uppercase 8.3 listing names.
