---
Title: Design + implementation plan
Ticket: 0053-HTTPD-WS-HUB-COMPONENT
Status: active
Topics:
    - esp-idf
    - http
    - webserver
    - websocket
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/main/http_server.cpp
      Note: WS client tracking + binary RX callback
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: WS shared buffer + refcount broadcaster
    - Path: 0048-cardputer-js-web/main/http_server.cpp
      Note: WS client tracking + http_server_ws_broadcast_text
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-21T15:36:21.743654457-05:00
WhatFor: ""
WhenToUse: ""
---


# Design + implementation plan

## Executive Summary

Extract the repeated WebSocket client tracking + async broadcast pattern into `components/httpd_ws_hub`.

This component centralizes the “FD snapshot + `httpd_ws_send_data_async` + free callback” pattern used in:

- `0017-atoms3r-web-ui/main/http_server.cpp`
- `0029-mock-zigbee-http-hub/main/hub_http.c`
- `0048-cardputer-js-web/main/http_server.cpp`

## Problem Statement

Multiple firmwares independently implement:

- tracking WebSocket client fds in a small fixed array,
- pruning stale/non-upgraded connections,
- broadcasting text/binary frames by allocating payload copies and sending via `httpd_ws_send_data_async`.

This is subtle to implement correctly because:

- `httpd_ws_send_data_async` expects the payload pointer to remain valid until the free callback.
- websockets can be in “HTTP handshake not upgraded yet” state (`HTTPD_WS_CLIENT_HTTP`).
- failing to prune invalid fds accumulates dead clients and wastes memory.

## Proposed Solution

Create component:

- `esp32-s3-m5/components/httpd_ws_hub/`
  - `include/httpd_ws_hub.h`
  - `httpd_ws_hub.c` (or `.cpp`)
  - `CMakeLists.txt`

### API

```c
typedef struct httpd_ws_hub httpd_ws_hub_t;

typedef void (*httpd_ws_hub_binary_rx_cb_t)(const uint8_t* data, size_t len, void* ctx);
typedef void (*httpd_ws_hub_text_rx_cb_t)(const char* text, size_t len, void* ctx);

httpd_ws_hub_t* httpd_ws_hub_create(httpd_handle_t server, size_t max_clients);
void httpd_ws_hub_destroy(httpd_ws_hub_t* hub);

// Called by the WS URI handler:
void httpd_ws_hub_on_connect(httpd_ws_hub_t* hub, int fd);
void httpd_ws_hub_on_close(httpd_ws_hub_t* hub, int fd);

void httpd_ws_hub_set_binary_rx_cb(httpd_ws_hub_t* hub, httpd_ws_hub_binary_rx_cb_t cb, void* ctx);
void httpd_ws_hub_set_text_rx_cb(httpd_ws_hub_t* hub, httpd_ws_hub_text_rx_cb_t cb, void* ctx);

esp_err_t httpd_ws_hub_broadcast_text(httpd_ws_hub_t* hub, const char* text);
esp_err_t httpd_ws_hub_broadcast_binary(httpd_ws_hub_t* hub, const uint8_t* data, size_t len);
```

### Broadcast internals (pseudocode)

```text
broadcast_text(text):
  snapshot fds under mutex
  for fd in fds:
    if httpd_ws_get_fd_info(server, fd) != WEBSOCKET:
      remove fd
      continue
    copy = malloc(len); memcpy(copy,text,len)
    frame = {type=TEXT, payload=copy, len=len}
    httpd_ws_send_data_async(server, fd, &frame, free_cb, copy)
```

Note: we intentionally keep `max_clients` small and fixed-size to avoid heap churn.

## Design Decisions

1) **Prefer correctness and small memory use over maximal throughput.**
   - JSON telemetry frames are small; per-client copy is fine.

2) **Support binary RX callback because 0017 uses it.**
   - 0048 ignores inbound frames; 0017 consumes binary frames.

3) **Do not bake in application schemas.**
   - The hub only transports bytes.

## Alternatives Considered

1) Keep local hub implementations per firmware.
   - Rejected: guaranteed drift and repeated mistakes.

2) Use a single shared buffer + refcount for all broadcasts.
   - This is valuable for large payloads (0029 protobuf events), but is more complex.
   - We can add an optional “shared buffer broadcast” later if needed; start with per-client copy.

## Implementation Plan

1) Implement `httpd_ws_hub` with:
   - fd list + mutex
   - snapshot helper
   - async send + free callback
   - optional RX callbacks invoked from handler
2) Migrate 0048:
   - Replace WS client list in:
     - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
   - Keep `http_server_ws_broadcast_text(...)` as a wrapper that calls the hub.
3) Migrate 0017:
   - Replace WS client list in:
     - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
   - Keep:
     - `http_server_ws_broadcast_text(...)`
     - `http_server_ws_broadcast_binary(...)`
     - `http_server_ws_set_binary_rx_cb(...)`
     as wrappers around the hub.
4) Migrate 0029:
   - Consider keeping the existing refcounted binary broadcast in place initially.
   - Optionally refactor `hub_http_events_broadcast_pb(...)` to use the hub for client tracking.

## Open Questions

1) Do we need “keepalive/ping” support?
   - Not required for current LAN usage; document if needed later.

## References

- 0048 WS hub code:
  - `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp` (`ws_client_add/remove`, `http_server_ws_broadcast_text`)
- 0017 WS hub code:
  - `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` (`http_server_ws_broadcast_binary/text`, `http_server_ws_set_binary_rx_cb`)
- 0029 WS broadcast prior art:
  - `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c` (`hub_http_events_broadcast_pb`, shared buffer + refcount)
