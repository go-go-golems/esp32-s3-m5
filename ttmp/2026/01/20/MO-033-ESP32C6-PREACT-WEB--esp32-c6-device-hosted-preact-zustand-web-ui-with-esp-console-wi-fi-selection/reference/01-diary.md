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
LastUpdated: 2026-01-20T16:58:53-05:00
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
- Flash + run the end-to-end console + HTTP verification playbook on real hardware (and capture logs).
- Decide whether to address `npm audit` findings (policy decision).

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

## Step 3: Build the real web bundle and embed it (Vite → main/assets)

This step replaces the placeholder embedded assets with a real Vite production build of the Preact+Zustand counter UI. The key outcome is that `main/assets/index.html`, `main/assets/assets/app.js`, and `main/assets/assets/app.css` are now the actual bundle the device will serve.

**Commit (code):** a64dc8f — "build(0045): embed built web assets"

### What I did
- Installed frontend deps:
  - `cd 0045-xiao-esp32c6-preact-web/web && npm ci`
- Built the bundle (configured to output directly to firmware assets dir):
  - `cd 0045-xiao-esp32c6-preact-web/web && npm run build`
- Verified outputs landed at:
  - `0045-xiao-esp32c6-preact-web/main/assets/index.html`
  - `0045-xiao-esp32c6-preact-web/main/assets/assets/app.js`
  - `0045-xiao-esp32c6-preact-web/main/assets/assets/app.css`
- Committed those built artifacts (so firmware embedding is deterministic and reproducible).

### Why
- The firmware uses `EMBED_TXTFILES` and hard-coded routes. That model depends on stable filenames and having the built artifacts present at compile time.

### What worked
- `vite build` produced a small bundle (JS ~26 KB) and wrote directly into `main/assets/`.

### What didn't work
- `npm ci` reported: `1 high severity vulnerability`.
  - Not addressed here; this repo may want a separate, intentional dependency refresh pass.

### What I learned
- The “deterministic bundle” approach is token-and-risk efficient for embedded demos: no manifest, no FS partition, minimal routes.

### What was tricky to build
- Ensuring `web/node_modules` stays out of Git while built artifacts remain in Git (handled via `web/.gitignore` and committing only `main/assets/*`).

### What warrants a second pair of eyes
- Whether we should treat `npm audit` as a gating policy for tutorial web bundles in this repo.

### What should be done in the future
- Decide if/when to run `npm audit fix` (and if we accept the resulting dependency churn).

### Code review instructions
- Confirm deterministic build outputs and paths:
  - `0045-xiao-esp32c6-preact-web/web/vite.config.ts`
  - `0045-xiao-esp32c6-preact-web/main/assets/index.html`
  - `0045-xiao-esp32c6-preact-web/main/assets/assets/app.js`

### Technical details
- Build output excerpt:
  - `../main/assets/assets/app.js   26.46 kB │ gzip: 10.43 kB`

## Step 4: Fix build helper Python selection; run ESP32-C6 build

This step makes the build helper robust in this repo’s shell environment and validates that the firmware builds for `esp32c6`. The issue was `idf.py` being invoked under a global Python (pyenv) instead of the ESP-IDF venv, causing `esp_idf_monitor` import failures.

**Commit (code):** e7f279c — "chore(0045): fix build.sh python env + add sdkconfig"

### What I did
- Reproduced the failure:
  - `cd 0045-xiao-esp32c6-preact-web && ./build.sh set-target esp32c6 build`
  - Error: `No module named 'esp_idf_monitor'`
- Verified the ESP-IDF venv Python contains the missing module:
  - `$HOME/.espressif/python_env/idf5.4_py3.11_env/bin/python -c "import esp_idf_monitor"`
- Updated `0045-xiao-esp32c6-preact-web/build.sh` to prepend `${IDF_PYTHON_ENV_PATH}/bin` to `PATH` when available.
- Ran a successful build:
  - `cd 0045-xiao-esp32c6-preact-web && ./build.sh set-target esp32c6 build`
- Captured a baseline `sdkconfig`:
  - repo `.gitignore` ignores new `sdkconfig` files by default (`**/sdkconfig`)
  - added with `git add -f 0045-xiao-esp32c6-preact-web/sdkconfig` to preserve this tutorial’s baseline

### Why
- This repo’s shell environment can have pyenv earlier in `PATH`; explicitly preferring `IDF_PYTHON_ENV_PATH` avoids confusing idf.py failures.
- A committed `sdkconfig` gives a concrete baseline for others to build/flash the same configuration.

### What worked
- `idf.py` build succeeded and produced:
  - `build/xiao_esp32c6_preact_web_0045.bin`

### What didn't work
- The initial build helper relied on ambient `PATH` ordering and picked the wrong Python interpreter.

### What I learned
- For “team tutorial” projects in this repo, build scripts should not assume `python` points at the ESP-IDF venv.

### What was tricky to build
- Reconciling the repo-wide ignore rule for `sdkconfig` with the desire to version a tutorial baseline.

### What warrants a second pair of eyes
- Whether committing `sdkconfig` for new tutorials should be a documented convention (vs relying on only `sdkconfig.defaults`).

### What should be done in the future
- Flash + monitor on real hardware and record boot logs / console interaction in the ticket.

### Code review instructions
- Validate build script behavior:
  - `cd 0045-xiao-esp32c6-preact-web && ./build.sh set-target esp32c6 build`

## Step 5: Small polish fix (log tag)

This is a tiny correctness cleanup: the firmware entrypoint log tag referenced the old tutorial number. It doesn’t change behavior, but it avoids confusion when scanning logs.

**Commit (code):** b562047 — "fix(0045): correct log tag"

### What I did
- Updated the `TAG` string in `0045-xiao-esp32c6-preact-web/main/app_main.c`.

### Why
- Log tags show up in every line; having the wrong suffix is confusing during bring-up.

### What worked
- N/A (one-line change).

### What didn't work
- N/A

### What I learned
- N/A

### What was tricky to build
- N/A

### What warrants a second pair of eyes
- N/A

### What should be done in the future
- N/A

### Code review instructions
- `0045-xiao-esp32c6-preact-web/main/app_main.c`

## Step 6: Fix `U+0000` JS parse error; add (gzipped) sourcemaps

This step addresses a browser runtime error when serving embedded JS:

- Browser error: `Uncaught SyntaxError: illegal character U+0000`
- Cause: ESP-IDF’s `EMBED_TXTFILES` appends a trailing NUL byte (`0x00`) to embedded text blobs, and we were serving `end - start` bytes verbatim (including that NUL).

It also adds a stable sourcemap endpoint for better debugging, while keeping flash usage reasonable by gzipping the map.

**Commit (code):** 1ce953e — "fix(0045): trim embedded NULs + serve sourcemap"

### What I did
- Confirmed the trailing NUL by inspecting the generated assembly for the embedded JS (`build/app.js.S` ends in `... 0x0a, 0x00`).
- Updated the HTTP server to:
  - Trim a trailing `0x00` from embedded *text* assets before sending
  - Serve JS/CSS/JSON with explicit UTF‑8 charsets
  - Serve `/assets/app.js.map` from a gzipped embedded sourcemap (`Content-Encoding: gzip`)
- Updated the web build pipeline to:
  - Enable Vite sourcemap generation (`build.sourcemap: true`)
  - Generate `app.js.map.gz` after `vite build` (Node `zlib` gzip) so the firmware embeds a smaller artifact
- Updated firmware asset embedding:
  - Keep HTML/JS/CSS via `EMBED_TXTFILES`
  - Embed `app.js.map.gz` via `EMBED_FILES`

### Why
- Trailing NUL in JS payload is invalid in the JS grammar, so browsers reject the script.
- Sourcemaps significantly improve debugging of device-hosted web UIs; gzipping the map saves flash space and keeps the app partition from getting too tight.

### What worked
- `./build.sh web` produces both `app.js.map` and `app.js.map.gz`.
- `./build.sh build` succeeds and embeds `app.js.map.gz` (seen as `app.js.map.gz.S` generation during build).

### What didn't work
- N/A (the fix is straightforward once the trailing NUL is identified).

### What I learned
- Treat `EMBED_TXTFILES` as “C string blob”: it commonly includes a terminator. If you’re serving it as a raw HTTP payload, you must send the correct length.

### What warrants a second pair of eyes
- Whether we want a “release build” mode that disables sourcemaps entirely (flash headroom vs. debugging convenience).

### What should be done in the future
- Flash and verify end-to-end on hardware:
  - UI loads without the `U+0000` error
  - Browser devtools successfully requests `/assets/app.js.map`

### Code review instructions
- NUL trimming and headers: `0045-xiao-esp32c6-preact-web/main/http_server.c`
- Sourcemap embedding config: `0045-xiao-esp32c6-preact-web/main/CMakeLists.txt`
- Sourcemap generation: `0045-xiao-esp32c6-preact-web/web/vite.config.ts`
- Build pipeline gzip step: `0045-xiao-esp32c6-preact-web/build.sh`
