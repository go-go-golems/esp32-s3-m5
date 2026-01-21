---
Title: Diary
Ticket: 0053-HTTPD-WS-HUB-COMPONENT
Status: active
Topics:
    - esp-idf
    - http
    - webserver
    - websocket
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: 0017 WS integration + binary RX hook
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: 0048 WS integration + wrappers
    - Path: components/httpd_ws_hub/httpd_ws_hub.c
      Note: FD tracking + broadcast implementation
    - Path: components/httpd_ws_hub/include/httpd_ws_hub.h
      Note: Public hub API
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T16:40:32.320476933-05:00
WhatFor: ""
WhenToUse: ""
---


# Diary

## Goal

<!-- What is the purpose of this reference document? -->

## Context

<!-- Provide background context needed to use this reference -->

## Quick Reference

<!-- Provide copy/paste-ready content, API contracts, or quick-look tables -->

## Usage Examples

<!-- Show how to use this reference in practice -->

## Related

<!-- Link to related documents or resources -->
# Diary

## Goal

Extract reusable WebSocket client tracking + async broadcast logic for `esp_http_server` into a shared component so multiple firmwares can:
- register a WS handler that tracks connected fds,
- broadcast text/binary frames best-effort via `httpd_ws_send_data_async`,
- optionally process inbound WS frames via callbacks,
without duplicating the mutex/snapshot/free-callback pattern.

## Step 1: Add `httpd_ws_hub` + migrate 0048/0017

This step moves the “FD set + snapshot + async broadcast” pattern out of individual HTTP servers into a component and then rewires 0048 and 0017 to use it while preserving their existing exported wrapper symbols.

The hub keeps the critical concurrency invariant: never hold the client-list mutex while calling `httpd_ws_send_data_async` (we snapshot first), and always heap-own the payload for async sends so the frame payload remains valid until the send completes.

**Commit (code):** 414307d — "httpd_ws_hub: extract WebSocket hub and migrate 0017/0048"

### What I did
- Added `esp32-s3-m5/components/httpd_ws_hub/`:
  - `include/httpd_ws_hub.h` with:
    - `httpd_ws_hub_t` (server handle, mutex, fixed-size fd array, RX callbacks)
    - `httpd_ws_hub_init`, `httpd_ws_hub_deinit`
    - `httpd_ws_hub_handle_req` (shared WS URI handler logic)
    - `httpd_ws_hub_broadcast_text`, `httpd_ws_hub_broadcast_binary`
    - `httpd_ws_hub_set_text_rx_cb`, `httpd_ws_hub_set_binary_rx_cb`
  - `httpd_ws_hub.c` implementing fd tracking, snapshotting, async send + free callback, and inbound frame dispatch
- Migrated 0048 (`esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`):
  - Replaced local `s_ws_clients`/mutex code with a `static httpd_ws_hub_t s_ws_hub`
  - Preserved `http_server_ws_broadcast_text(...)` as a wrapper calling `httpd_ws_hub_broadcast_text(...)`
  - Updated `ws_handler` to delegate to `httpd_ws_hub_handle_req(...)`
  - Initialized the hub after `httpd_start` via `httpd_ws_hub_init(&s_ws_hub, s_server)`
- Migrated 0017 (`esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`):
  - Replaced local ws client tracking + async broadcast code with `httpd_ws_hub_t`
  - Preserved wrapper symbols:
    - `http_server_ws_broadcast_text`
    - `http_server_ws_broadcast_binary`
    - `http_server_ws_set_binary_rx_cb` (now calls `httpd_ws_hub_set_binary_rx_cb`)
  - Added `httpd_ws_hub` to `0017-atoms3r-web-ui/main/CMakeLists.txt` `PRIV_REQUIRES`
- Built both firmwares:
  - 0048: `idf.py -B build_esp32s3_wifi_mgr_comp2 build`
  - 0017: `idf.py -B build build`

### Why
- This repo had multiple near-identical WS implementations (mutex + fd list + snapshot + async send). Keeping them separate invites drift and subtle concurrency regressions.
- A single component makes the WS behavior consistent across firmwares and reduces repeated code in each `http_server` module.

### What worked
- Both firmwares build with the shared hub and keep their external API stable via wrappers.
- The hub supports both broadcast directions and optional inbound dispatch (binary/text callbacks), covering 0017’s “binary RX hook” use case.

### What didn't work
- N/A (no functional blockers encountered during extraction/migration).

### What I learned
- A small, “data-structure + handler + broadcast” component is the right granularity for reuse: firmwares keep ownership of routing/logging, while the hub owns the correctness-sensitive mutex/snapshot/send-lifetime details.

### What was tricky to build
- Ensuring async send payload lifetime: the component must allocate/copy payloads and free them in the `httpd_ws_send_data_async` completion callback.
- Maintaining the invariant of not holding the mutex across async calls required keeping the snapshot helper in the component rather than pushing it to each caller.

### What warrants a second pair of eyes
- `esp32-s3-m5/components/httpd_ws_hub/httpd_ws_hub.c`: verify fd removal logic in broadcast loops and ensure no deadlocks or missed removals when clients disconnect.
- The fixed-size limits (`HTTPD_WS_HUB_MAX_CLIENTS`, `HTTPD_WS_HUB_MAX_RX_LEN`) are appropriate for current workloads; verify that future event streams won’t exceed the RX size cap.

### What should be done in the future
- Migrate 0029 to reuse the hub (per ticket tasks).
- Run on-device WS smoke tests (connect, disconnect, broadcast, RX) for 0017/0048 (and 0029 once migrated).

### Code review instructions
- Start at `esp32-s3-m5/components/httpd_ws_hub/include/httpd_ws_hub.h` then `esp32-s3-m5/components/httpd_ws_hub/httpd_ws_hub.c`.
- Review the two migrations:
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
- Build validation:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `cd esp32-s3-m5/0048-cardputer-js-web && idf.py -B build_esp32s3_wifi_mgr_comp2 build`
  - `cd esp32-s3-m5/0017-atoms3r-web-ui && idf.py -B build build`

### Technical details
- Hub handler (`httpd_ws_hub_handle_req`) adds the client fd and:
  - returns immediately on handshake `HTTP_GET`
  - reads a frame header + payload and dispatches to callbacks (if set)
  - rejects frames larger than `HTTPD_WS_HUB_MAX_RX_LEN` by returning `ESP_FAIL` (socket closes)
