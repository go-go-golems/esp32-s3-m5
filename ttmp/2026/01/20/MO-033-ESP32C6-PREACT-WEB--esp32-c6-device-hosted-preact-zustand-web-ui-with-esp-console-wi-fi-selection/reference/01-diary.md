---
Title: Diary
Ticket: MO-033-ESP32C6-PREACT-WEB
Status: active
Topics:
    - esp32c6
    - esp-idf
    - wifi
    - console
    - http
    - webserver
    - ui
    - assets
    - preact
    - zustand
DocType: reference
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T15:21:22.944938179-05:00
WhatFor: "Track the step-by-step implementation of MO-033, including commands, failures, decisions, and review instructions."
WhenToUse: "Use while implementing MO-033 to keep a reliable narrative and validation trail."
---

# Diary

## Goal

Capture the research and implementation journey for MO-033: an ESP32‑C6 firmware that uses `esp_console` to select/join Wi‑Fi networks and serves an embedded, deterministic Vite (Preact + Zustand) counter UI bundle from `esp_http_server`.

## Step 1: Ticket setup + initial implementation scaffold

This step turns the “idea” into a concrete workspace: a docmgr ticket with tasks plus the first cut of the implementation scaffold. The goal is to make subsequent steps purely incremental: fill in missing features, build, and verify.

**Commit (code):** TBD — "TBD"

### What I did
- Created the docmgr ticket `MO-033-ESP32C6-PREACT-WEB` with analysis/design/playbook docs and a diary.
- Added an initial task list for implementation.
- Scaffolded a new ESP32‑C6 project directory `0045-xiao-esp32c6-preact-web/`:
  - `esp_console` REPL with `wifi` commands (scan/set/join/connect/disconnect/clear/status)
  - `wifi_mgr` STA manager with NVS credential persistence and blocking scan helper (patterned after tutorial 0029)
  - `esp_http_server` static asset serving (`/`, `/assets/app.js`, `/assets/app.css`) and `/api/status`
  - `web/` Vite + Preact + Zustand counter app with deterministic bundling config
- Added placeholder embedded assets in `0045-xiao-esp32c6-preact-web/main/assets/` so the firmware can build before the real Vite build runs.

### Why
- MO-033 is easiest to implement as “compose existing proven patterns”:
  - Wi‑Fi console UX and NVS persistence already exists (0029).
  - Deterministic Vite bundling + embedded serving already exists (0017 + docs playbook).
  - ESP32‑C6 console defaults already exist (0042/0043).

### What worked
- The repo already had strong reference implementations to copy from (reduces risk).
- The deterministic Vite config (`outDir: ../main/assets`, `inlineDynamicImports: true`, stable `assets/app.js`) keeps firmware routing simple.

### What didn't work
- N/A (no build executed yet in this step).

### What I learned
- The “select network” requirement is best met by adding `wifi join <index>` atop a stored scan list (scan output already prints indexes).

### What was tricky to build
- Keeping the firmware buildable while also wanting Vite-built assets: required checking in placeholder `app.js/app.css` initially and planning for a later “real build outputs” step.

### What warrants a second pair of eyes
- The Wi‑Fi manager is copied/derived from 0029; review for any ESP32‑C6-specific differences (event ordering, scan behavior).
- Cache behavior: deterministic filenames require deliberate `Cache-Control` choices (currently set to `no-store` behind Kconfig).

### What should be done in the future
- Replace placeholder assets with real Vite build outputs (run `npm ci && npm run build`) and commit those embedded artifacts.
- Add a `build.sh` helper (like 0017) to make “build web + build firmware” a one-command flow.

### Code review instructions
- Start with firmware entrypoint: `0045-xiao-esp32c6-preact-web/main/app_main.c`.
- Review Wi‑Fi flow: `0045-xiao-esp32c6-preact-web/main/wifi_mgr.c` and `0045-xiao-esp32c6-preact-web/main/wifi_console.c`.
- Review static serving + status endpoint: `0045-xiao-esp32c6-preact-web/main/http_server.c`.
- Review frontend determinism knobs: `0045-xiao-esp32c6-preact-web/web/vite.config.ts`.

### Technical details
- Ticket docs:
  - analysis: `analysis/01-codebase-analysis-existing-patterns-for-esp-console-wi-fi-selection-embedded-preact-zustand-assets.md`
  - design: `design-doc/01-design-esp32-c6-console-configured-wi-fi-device-hosted-preact-zustand-counter-ui-embedded-assets.md`
  - playbook: `playbook/01-playbook-build-flash-connect-verify-web-ui-esp32-c6.md`

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
