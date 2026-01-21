---
Title: Diary
Ticket: 0052-HTTPD-ASSETS-EMBED-COMPONENT
Status: active
Topics:
    - esp-idf
    - http
    - webserver
    - assets
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: Uses helper for UI assets
    - Path: 0046-xiao-esp32c6-led-patterns-webui/main/http_server.c
      Note: Uses helper for UI assets
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: Uses helper for UI assets
    - Path: components/httpd_assets_embed/httpd_assets_embed.c
      Note: Length + send implementation
    - Path: components/httpd_assets_embed/include/httpd_assets_embed.h
      Note: Public helper API
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T16:32:45.318516184-05:00
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

Create a reusable helper component for serving embedded frontend assets via `esp_http_server` that (a) computes correct lengths, (b) optionally trims a trailing NUL byte (fixing browser `U+0000` issues when using `EMBED_TXTFILES`), and (c) standardizes setting `Content-Type` and (optionally) `Cache-Control`.

## Step 1: Add `httpd_assets_embed` + migrate asset handlers

This step consolidates the “embedded asset length + send” logic scattered across HTTP servers into a single component. It explicitly captures the subtle ESP‑IDF behavior where `EMBED_TXTFILES` adds a trailing `\\0`, and naive `(end - start)` length calculations can serve that byte to the browser.

The result is a small, easy-to-audit interface (`httpd_assets_embed_len`, `httpd_assets_embed_send`) that each firmware can reuse without re-implementing NUL trimming or header boilerplate.

**Commit (code):** da25e21 — "httpd_assets_embed: send embedded assets safely"

### What I did
- Added new component `esp32-s3-m5/components/httpd_assets_embed/`:
  - `include/httpd_assets_embed.h`:
    - `size_t httpd_assets_embed_len(const uint8_t *start, const uint8_t *end, bool trim_trailing_nul)`
    - `esp_err_t httpd_assets_embed_send(httpd_req_t *req, const uint8_t *start, const uint8_t *end, const char *content_type, const char *cache_control, bool trim_trailing_nul)`
  - `httpd_assets_embed.c` implementing the helpers
- Migrated asset handlers to use the component:
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`: replaced local `embedded_txt_len(...)` + manual `httpd_resp_send` calls with `httpd_assets_embed_send(..., trim=true)`
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/http_server.c`: replaced local `embedded_txt_len/embedded_bin_len` + manual sends for `index.html`, `app.js`, `app.css`, `app.js.map.gz` with `httpd_assets_embed_send(...)` (binary map uses `trim=false`)
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`: replaced `(end-start)` length sends with `httpd_assets_embed_send(..., trim=true)`
- Updated build wiring:
  - 0048 and 0046: added the component path to `EXTRA_COMPONENT_DIRS` and added `httpd_assets_embed` to `PRIV_REQUIRES`
  - 0017: added `httpd_assets_embed` to `PRIV_REQUIRES` (component discovery already uses `../components`)
- Built affected firmwares:
  - 0048: `idf.py -B build_esp32s3_wifi_mgr_comp2 build`
  - 0046: `idf.py -B build_esp32c6_wifi_mgr_comp2 build`
  - 0017: `idf.py -B build build`

### Why
- Browser-side `Uncaught SyntaxError: illegal character U+0000` can be caused by serving a trailing NUL byte at the end of JavaScript/CSS embedded via `EMBED_TXTFILES`.
- This mistake is easy to reintroduce when copying a “send embedded file” snippet; extracting it into a component turns it into a one-line, consistent call site.

### What worked
- The helper cleanly replaces multiple per-firmware implementations while keeping each firmware’s caching policy:
  - 0048 still uses `"no-store"` for the frontend assets.
  - 0046 continues to use its `CONFIG_MO033_HTTP_CACHE_NO_STORE` gate (now passed into the helper per request).
- All three updated firmwares build successfully.

### What didn't work
- Building 0017 caused its tracked `sdkconfig` and `dependencies.lock` to update (new component configs appearing). I committed those updates as part of this step to keep the working tree clean for future builds.

### What I learned
- `EMBED_TXTFILES` and `EMBED_FILES` behave differently with respect to trailing NULs; helpers need to allow opting out of trim for true binary payloads (e.g. `.gz`).

### What was tricky to build
- The “same feature” appears in both C and C++ HTTP servers across this repo; the helper had to be C ABI friendly and work from both `http_server.c` and `http_server.cpp` call sites.
- Some projects discover components via broad `../components` (0017), while others must list each component explicitly (0046/0048) due to unrelated components with external deps.

### What warrants a second pair of eyes
- `esp32-s3-m5/components/httpd_assets_embed/httpd_assets_embed.c`: confirm the trimming logic is correct and cannot underflow when `end == start`.
- Call sites for binary assets: ensure `.gz`/binary content uses `trim_trailing_nul=false`.

### What should be done in the future
- Run a real browser validation against each firmware UI to confirm the `U+0000` issue is resolved and assets still load correctly.

### Code review instructions
- Start at `esp32-s3-m5/components/httpd_assets_embed/include/httpd_assets_embed.h` then `esp32-s3-m5/components/httpd_assets_embed/httpd_assets_embed.c`.
- Review migrations:
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
  - `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/http_server.c`
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
- Validate builds:
  - `source /home/manuel/esp/esp-idf-5.4.1/export.sh`
  - `cd esp32-s3-m5/0048-cardputer-js-web && idf.py -B build_esp32s3_wifi_mgr_comp2 build`
  - `cd esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui && idf.py -B build_esp32c6_wifi_mgr_comp2 build`
  - `cd esp32-s3-m5/0017-atoms3r-web-ui && idf.py -B build build`

### Technical details
- `httpd_assets_embed_len(...)` returns 0 for invalid pointers or reversed ranges.
- `httpd_assets_embed_send(...)`:
  - optionally calls `httpd_resp_set_type(req, content_type)`
  - optionally calls `httpd_resp_set_hdr(req, "cache-control", cache_control)`
  - sends `start..end` with optional trailing-NUL trim.
