---
Title: 'Playbook: Phase 1 Web IDE smoke test (build, connect, run snippets)'
Ticket: 0048-CARDPUTER-JS-WEB
Status: active
Topics:
    - cardputer
    - esp-idf
    - webserver
    - http
    - rest
    - websocket
    - preact
    - zustand
    - javascript
    - quickjs
    - microquickjs
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/web/vite.config.ts
      Note: Reference asset pipeline for deterministic /assets/app.js and /assets/app.css
    - Path: esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp
      Note: Reference eval semantics the /api/js/eval endpoint should match
    - Path: esp32-s3-m5/ttmp/2026/01/20/0048-CARDPUTER-JS-WEB--cardputer-web-js-ide-preact-microquickjs/design-doc/01-phase-1-design-device-hosted-web-js-ide-preact-zustand-microquickjs-over-rest.md
      Note: Phase 1 architecture and API contract this playbook validates
ExternalSources: []
Summary: 'Repeatable smoke-test procedure for Phase 1: build embedded assets + firmware, flash, connect, validate routes, and run a suite of JavaScript snippets via REST.'
LastUpdated: 2026-01-20T23:08:54.633594833-05:00
WhatFor: A concrete, command-oriented checklist to validate the Phase 1 MVP end-to-end (browser UI served by device + REST eval working).
WhenToUse: Use after implementing Phase 1 endpoints and UI; run before demos or when refactoring the HTTP server / JS runner.
---


# Playbook: Phase 1 Web IDE smoke test (build, connect, run snippets)

## Purpose

Validate the Phase 1 MVP end-to-end:

1) Device serves embedded web UI assets (`/`, `/assets/app.js`, `/assets/app.css`).
2) Device responds to status endpoint (`/api/status`).
3) Device executes JavaScript submitted over REST (`POST /api/js/eval`) and returns formatted output or errors.
4) Browser UI can run snippets and display output.

## Environment Assumptions

- You have an ESP-IDF toolchain installed for ESP32‑S3 and can run `idf.py`.
- You have a Node.js toolchain capable of building the Vite UI (`npm` available).
- The Phase 1 firmware exists in a project directory (placeholder name used below):
  - `esp32-s3-m5/0048-cardputer-js-web/` (TBD)
- The firmware embeds web assets under `main/assets/` with deterministic filenames:
  - `index.html`, `assets/app.js`, `assets/app.css`
- The device is reachable either via:
  - STA (device joins your Wi‑Fi via `esp_console` configuration and prints an IP).

## Commands

```bash
# 0) Build frontend assets into firmware asset folder
cd esp32-s3-m5/0048-cardputer-js-web/web
npm ci
npm run build

# 1) Build + flash firmware
cd ../
idf.py -B build_esp32s3_v2 set-target esp32s3
idf.py -B build_esp32s3_v2 build
idf.py -B build_esp32s3_v2 flash monitor
```

## Exit Criteria

In serial logs you see:

- `esp_console` prompt appears (e.g. `0048>`)
- you can run `wifi scan` and `wifi status`
- after `wifi join ... --save` (or `wifi set ... --save` + `wifi connect`), a browse URL printed, e.g. `http://192.168.1.123/`
- HTTP server starts after STA got-IP (and routes register)

Console commands (example):

```text
0048> wifi scan
0048> wifi join 0 mypassword --save
0048> wifi status
```

From a host machine (replace `<ip>` with the device IP):

```bash
# 1) UI assets served
curl -i "http://<ip>/" | head
curl -I "http://<ip>/assets/app.js"
curl -I "http://<ip>/assets/app.css"

# 2) Status endpoint returns JSON
curl -s "http://<ip>/api/status" | jq .

# 3) JS eval works for a simple expression
curl -sS "http://<ip>/api/js/eval" \
  -H 'Content-Type: text/plain; charset=utf-8' \
  --data-binary '1+1' | jq .

# 4) JS eval reports errors (exception path)
curl -sS "http://<ip>/api/js/eval" \
  -H 'Content-Type: text/plain; charset=utf-8' \
  --data-binary 'does_not_exist' | jq .
```

In the browser:

- Loading `http://<ip>/` renders the editor UI.
- Clicking “Run” produces output that matches the REST response.

Suggested snippet suite (run both via UI and via curl):

```js
1+1
let x = 3; x * 7
({a: 1, b: [2,3]})
```

## Notes

### Failure modes and what they mean

- `GET /assets/app.js` 404:
  - asset pipeline did not output deterministic files into firmware asset directory
  - compare with `esp32-s3-m5/0017-atoms3r-web-ui/web/vite.config.ts`
- `/api/js/eval` returns “out of memory”:
  - JS heap budget too small for current code
  - use `/api/js/reset` (if implemented) and reduce snippet size
- Device becomes unresponsive after running a snippet:
  - likely infinite loop / long-running code without a JS interrupt timeout; implement deadline via `JS_SetInterruptHandler`.
