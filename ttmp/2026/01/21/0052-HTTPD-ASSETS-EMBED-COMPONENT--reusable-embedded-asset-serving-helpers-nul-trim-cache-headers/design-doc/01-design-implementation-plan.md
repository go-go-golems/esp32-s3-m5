---
Title: Design + implementation plan
Ticket: 0052-HTTPD-ASSETS-EMBED-COMPONENT
Status: active
Topics:
    - esp-idf
    - http
    - webserver
    - assets
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: Embedded asset routes (no trim today)
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: send_embedded helper
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: embedded_txt_len NUL-trim + asset routes
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:36:21.471536966-05:00
WhatFor: ""
WhenToUse: ""
---


# Design + implementation plan

## Executive Summary

Create a small reusable component, `components/httpd_assets_embed`, that encapsulates:

- serving embedded frontend assets via `esp_http_server`,
- trimming a trailing NUL terminator from assembler-emitted blobs (fixes `U+0000` parse errors),
- setting consistent content type and cache-control headers.

Then migrate existing firmwares that serve embedded assets (`0017-atoms3r-web-ui`, `0029-mock-zigbee-http-hub`, `0048-cardputer-js-web`) to use it.

## Problem Statement

We have repeated, slightly different “serve embedded asset” implementations:

- 0017: `root_get` uses `(end - start)` directly.
- 0048: introduced `embedded_txt_len(...)` that trims a trailing `0` byte to fix browser errors:
  - `Uncaught SyntaxError: illegal character U+0000`
- 0029: has a local `send_embedded(...)` helper.

This is exactly the kind of “small bug fix” that gets lost when copied forward to a new firmware.

## Proposed Solution

Create component:

- `esp32-s3-m5/components/httpd_assets_embed/`
  - `include/httpd_assets_embed.h`
  - `httpd_assets_embed.c` (or `.cpp` if we want C++ helpers; C is fine)
  - `CMakeLists.txt`

### API

```c
typedef struct {
  const char* content_type;   // e.g. "text/html; charset=utf-8"
  const char* cache_control;  // e.g. "no-store" or "public, max-age=31536000, immutable"
  bool trim_trailing_nul;     // default true for text assets
} httpd_assets_embed_opts_t;

size_t httpd_assets_embed_len(const uint8_t* start, const uint8_t* end, bool trim_trailing_nul);
esp_err_t httpd_assets_embed_send(httpd_req_t* req,
                                 const uint8_t* start,
                                 const uint8_t* end,
                                 const httpd_assets_embed_opts_t* opts);
```

### NUL trimming rule (the important part)

```text
len = end - start
if trim && len > 0 && start[len-1] == 0:
  len--
send exactly len bytes
```

## Design Decisions

1) Default to `trim_trailing_nul=true` because the failure mode is nasty and non-obvious.
2) Keep the component small: it is a helper, not a routing framework.

## Alternatives Considered

1) Let each firmware implement its own helper.
   - Rejected: guarantees rediscovering `U+0000` bugs.

2) Serve assets from SPIFFS/FATFS instead of embedding.
   - Rejected for these projects: we explicitly want deterministic embedded assets and no filesystem dependency.

## Implementation Plan

1) Implement `httpd_assets_embed_send(...)` and `httpd_assets_embed_len(...)`.
2) Migrate 0048:
   - Replace `embedded_txt_len(...)` usage in:
     - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp` (`root_get`, `asset_app_js_get`, `asset_app_css_get`)
3) Migrate 0017:
   - Update:
     - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` (`root_get` and asset endpoints)
4) Migrate 0029 (optional but recommended):
   - Replace local `send_embedded(...)` in:
     - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`

## Open Questions

1) Should we add ETag generation helpers?
   - Not needed for current use; keep `cache-control: no-store` defaults.

## References

- 0048 fix site:
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp` (`embedded_txt_len`)
- Example asset servers:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c` (`send_embedded`)
