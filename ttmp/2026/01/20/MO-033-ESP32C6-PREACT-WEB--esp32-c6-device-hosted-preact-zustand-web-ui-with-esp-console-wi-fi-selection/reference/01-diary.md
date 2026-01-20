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
RelatedFiles:
    - Path: 0045-xiao-esp32c6-preact-web/build.sh
      Note: Helper for one-command builds
    - Path: 0045-xiao-esp32c6-preact-web/main/app_main.c
      Note: Firmware entrypoint wiring Wi-Fi -> start HTTP server and start console
    - Path: 0045-xiao-esp32c6-preact-web/main/http_server.c
      Note: Static asset serving and /api/status
    - Path: 0045-xiao-esp32c6-preact-web/main/wifi_console.c
      Note: esp_console command parser for wifi scan/join/set/connect
    - Path: 0045-xiao-esp32c6-preact-web/main/wifi_mgr.c
      Note: Wi-Fi STA manager with NVS credentials and scan/connect
    - Path: 0045-xiao-esp32c6-preact-web/web/src/app.tsx
      Note: Counter UI + fetch /api/status
    - Path: 0045-xiao-esp32c6-preact-web/web/vite.config.ts
      Note: Deterministic Vite output targeting firmware assets
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-20T15:21:22.944938179-05:00
WhatFor: Track the step-by-step implementation of MO-033, including commands, failures, decisions, and review instructions.
WhenToUse: Use while implementing MO-033 to keep a reliable narrative and validation trail.
---


# Diary

## Goal

Capture the research and implementation journey for MO-033: an ESP32‑C6 firmware that uses `esp_console` to select/join Wi‑Fi networks and serves an embedded, deterministic Vite (Preact + Zustand) counter UI bundle from `esp_http_server`.

## Step 1: Ticket setup + initial docs scaffold

This step turns the “idea” into a concrete workspace: a docmgr ticket with an analysis doc, design doc, playbook, tasks, and this diary. The goal is to make implementation steps incremental and reviewable.

**Commit (docs):** e0a5e29 — "docs: add MO-033 ticket docs"

### What I did
- Created the docmgr ticket `MO-033-ESP32C6-PREACT-WEB` with analysis/design/playbook docs and a diary.
- Added an initial task list for implementation.

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
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- Start implementation in a dedicated ESP32-C6 tutorial directory (console + Wi-Fi manager + HTTP server + web bundle).

### Code review instructions
- Review the ticket docs:
  - `../analysis/01-codebase-analysis-existing-patterns-for-esp-console-wi-fi-selection-embedded-preact-zustand-assets.md`
  - `../design-doc/01-design-esp32-c6-console-configured-wi-fi-device-hosted-preact-zustand-counter-ui-embedded-assets.md`
  - `../playbook/01-playbook-build-flash-connect-verify-web-ui-esp32-c6.md`

### Technical details
- Ticket docs:
  - analysis: `analysis/01-codebase-analysis-existing-patterns-for-esp-console-wi-fi-selection-embedded-preact-zustand-assets.md`
  - design: `design-doc/01-design-esp32-c6-console-configured-wi-fi-device-hosted-preact-zustand-counter-ui-embedded-assets.md`
  - playbook: `playbook/01-playbook-build-flash-connect-verify-web-ui-esp32-c6.md`

## Step 2: Scaffold ESP32‑C6 implementation project (0045)

This step creates the working “home” for implementation: a new ESP32‑C6 firmware directory that composes the proven patterns (Wi‑Fi console + deterministic embedded assets) into a minimal tutorial. The immediate goal is correctness and clarity rather than polish: get console commands working, connect to Wi‑Fi, start HTTP server, and serve a basic embedded page.

**Commit (code):** 0e698aa — "feat(0045): add ESP32-C6 wifi console + embedded web ui scaffold"

### What I did
- Created `0045-xiao-esp32c6-preact-web/` with:
  - firmware: `wifi_mgr` (STA + NVS creds + scan/connect), `wifi_console` (`wifi` REPL command), `http_server` (static assets + `/api/status`)
  - frontend: `web/` (Vite + Preact + Zustand counter app) with deterministic bundling rules targeting `main/assets/`
  - helper: `build.sh` with `web` and `all` commands
- Added placeholder embedded assets in `0045-xiao-esp32c6-preact-web/main/assets/` to keep firmware buildable before running Vite.

### Why
- We want a minimal end-to-end demo on ESP32‑C6: console selects Wi‑Fi, then device hosts the UI.
- This repo already proved the core patterns on other targets/projects; MO‑033 should reuse them.

### What worked
- The console UX can be kept simple (`wifi scan` → `wifi join <index> ...`) without any extra host tooling.
- Deterministic Vite outputs match the firmware’s “3 fixed routes” serving model.

### What didn't work
- N/A (build/flash/asset build validation is deferred to the next step).

### What I learned
- The repo already uses numbered tutorial folders as “real code”, so it’s reasonable to include placeholder assets and replace them with real build outputs later.

### What was tricky to build
- Ensuring the tutorial number didn’t collide with an existing project (this implementation uses `0045` because `0044` is already used).

### What warrants a second pair of eyes
- `wifi_mgr.c` event handler sequencing and retry logic (disconnect storms, scan while connecting, etc.).
- `http_server.c` JSON encoding size limits and content-type/header correctness.

### What should be done in the future
- Run `npm ci && npm run build` in `0045-xiao-esp32c6-preact-web/web` and commit the resulting embedded assets in `0045-xiao-esp32c6-preact-web/main/assets/`.
- Run `idf.py set-target esp32c6 build` and document any ESP32‑C6-specific tweaks required.

### Code review instructions
- Start with firmware entrypoint: `0045-xiao-esp32c6-preact-web/main/app_main.c`.
- Review Wi‑Fi flow: `0045-xiao-esp32c6-preact-web/main/wifi_mgr.c` and `0045-xiao-esp32c6-preact-web/main/wifi_console.c`.
- Review HTTP routes: `0045-xiao-esp32c6-preact-web/main/http_server.c`.
- Review bundling determinism: `0045-xiao-esp32c6-preact-web/web/vite.config.ts`.

### Technical details
- Key deterministic Vite knobs:
  - `outDir: '../main/assets'`
  - `inlineDynamicImports: true`
  - `entryFileNames: 'assets/app.js'` + CSS mapped to `assets/app.css`

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
