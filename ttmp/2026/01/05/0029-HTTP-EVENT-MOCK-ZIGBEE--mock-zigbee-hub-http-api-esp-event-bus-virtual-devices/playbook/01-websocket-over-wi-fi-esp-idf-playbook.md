---
Title: WebSocket over Wi-Fi (ESP-IDF) Playbook
Ticket: 0029-HTTP-EVENT-MOCK-ZIGBEE
Status: active
Topics:
    - esp-idf
    - esp32s3
    - http
    - zigbee
    - backend
    - websocket
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto
      Note: Protobuf schema referenced by WS/HTTP
    - Path: 0029-mock-zigbee-http-hub/main/hub_http.c
      Note: HTTPD + WS endpoint + static UI serving
    - Path: 0029-mock-zigbee-http-hub/main/hub_stream.c
      Note: Queue+task stream bridge and counters
    - Path: 0029-mock-zigbee-http-hub/main/ui/app.js
      Note: Browser WS + protobuf decoder
    - Path: 0029-mock-zigbee-http-hub/main/wifi_sta.c
      Note: Wi-Fi STA bring-up and IP events
    - Path: ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/http_pb_hub.js
      Note: Host protobuf HTTP client
    - Path: ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/ws_hub_events.js
      Note: Host WS smoke tester
ExternalSources: []
Summary: A practical, end-to-end playbook for building and debugging ESP-IDF WebSocket-over-WiFi apps (esp_http_server + FreeRTOS), with proven patterns from tutorial 0029.
LastUpdated: 2026-01-05T17:19:12.99901528-05:00
WhatFor: ""
WhenToUse: ""
---


# WebSocket over Wi-Fi (ESP-IDF) Playbook

## Purpose

This document teaches a new developer how to build reliable **WebSocket-over-Wi‑Fi** applications on ESP-IDF using:

- `esp_wifi` STA mode (DHCP)
- `esp_http_server` for HTTP + WebSocket endpoints
- `esp_event` as an internal application bus
- FreeRTOS tasks/queues for backpressure-safe streaming

It is written as a practical guide for this repo and is “battle tested” by the tutorial `0029-mock-zigbee-http-hub` implementation:

- `0029-mock-zigbee-http-hub/main/hub_http.c` (HTTPD + WS endpoint + broadcaster)
- `0029-mock-zigbee-http-hub/main/hub_stream.c` (bus→queue→task stream bridge)
- `0029-mock-zigbee-http-hub/main/wifi_sta.c` + `0029-mock-zigbee-http-hub/main/wifi_console.c` (Wi‑Fi bring-up + interactive console)

## Audience / prerequisites

You do not need prior WebSocket experience, but you do need:

- basic C and embedded debugging skills
- basic networking concepts (IP, port, client/server)
- a working ESP-IDF toolchain

## Background knowledge (the minimum you need)

### What WebSocket is (and why we use it)

WebSocket is an upgrade of an HTTP connection into a bidirectional byte stream. In practice:

- Client connects with an HTTP `GET` request that includes `Upgrade: websocket`.
- Server replies `101 Switching Protocols`.
- From that point on, data is exchanged in **frames** (binary or text).

Why it’s good for event-driven embedded apps:

- low overhead per message compared to repeated HTTP polling
- preserves ordering
- supports continuous streaming of events/telemetry
- fits the “coordinator” architecture: observe everything happening on the internal bus

### A tiny protocol mental model

You don’t need to implement RFC 6455 yourself on ESP-IDF, but you *do* need a mental model to debug it:

- Frames have an opcode (`TEXT`, `BINARY`, `PING`, `PONG`, `CLOSE`).
- Client-to-server frames are masked (browser/Node handle this; ESP-IDF handles it).
- Control frames exist (PING/PONG/CLOSE) and matter for connection health.

## Architecture pattern (recommended)

### The core idea

Keep the application deterministic by enforcing this invariant:

> **No heavy work in WebSocket handlers or event callbacks.**

Instead:

1. **event bus** publishes internal events (`esp_event`)
2. event handler **copies** a bounded payload into a **queue**
3. a dedicated **stream task** encodes + pushes to WS clients

### Diagram: bus → stream bridge → WebSocket

```text
                      (cheap, bounded)                   (IO / potentially slow)

  +------------------+      +------------------+          +----------------------+
  | App Event Loop    |      | Stream Queue     |          | Stream Task          |
  | (esp_event loop)  | ---> | (xQueueSend)     | -------> | encode + broadcast    |
  |                  |      | drops if full    |          | (WS send per client) |
  +------------------+      +------------------+          +----------------------+
                                                              |
                                                              v
                                                      +------------------+
                                                      | httpd work queue |
                                                      | + socket send    |
                                                      +------------------+
                                                              |
                                                              v
                                                      WS clients (browser/node)
```

### Why this pattern is “fool proof”

It protects you against the most common embedded WS failures:

- handler runs too long → httpd task stalls → watchdog resets / timeouts
- alloc/free in hot path → heap fragmentation / latency spikes
- slow client blocks send → internal event loop is delayed → missed deadlines

## ESP-IDF APIs you need (with gotchas)

### Wi‑Fi STA: connect and get an IP

Relevant APIs:

- `esp_wifi_init`, `esp_wifi_set_mode(WIFI_MODE_STA)`, `esp_wifi_start`
- `esp_wifi_set_config` for STA config
- `esp_netif` events (`IP_EVENT_STA_GOT_IP`)

Gotchas:

- Wi‑Fi may connect, then disconnect, then reconnect during initial association; treat this as normal.
- DHCP can take seconds depending on AP conditions; do not assume IP immediately.
- If a device seems “randomly unstable”, power/USB hubs/cables matter (brownouts, flaky USB data lines).

### HTTP server: start it and register endpoints

Relevant APIs:

- `httpd_start(&handle, &config)` (creates the httpd task)
- `httpd_register_uri_handler(handle, &httpd_uri_t{...})`
- `httpd_uri_match_wildcard` for simple wildcard matching

Gotchas:

- The default `httpd_config_t.max_uri_handlers` can be too small once you add:
  - index page (`/`)
  - JS/CSS assets (`/app.js`, `/style.css`, …)
  - REST endpoints
  - a WS endpoint  
  If you hit it, you will see logs like:
  - `httpd_register_uri_handler: no slots left for registering handler`
  and endpoints later show `404 URI not found`.
- The WS handler registration must succeed, or clients will see 404 on `/v1/events/ws`.

### WebSocket endpoint registration

In `esp_http_server`, WS is enabled per-route:

```c
httpd_uri_t ws = {
  .uri = "/v1/events/ws",
  .method = HTTP_GET,        // required for handshake
  .handler = events_ws_handler,
  .is_websocket = true,      // required
  .handle_ws_control_frames = false, // usually prefer false
};
```

**Critical gotcha:** The handler must return `ESP_OK` on success; returning an error closes the socket.

From `esp_http_server.h` (URI handler comment):

- “This must return `ESP_OK`, or else the underlying socket will be closed.”

### Receiving frames: `httpd_ws_recv_frame`

Pattern:

1. Call `httpd_ws_recv_frame(req, &frame, 0)` to get the length
2. Allocate (or use a bounded stack buffer), set `frame.payload`
3. Call `httpd_ws_recv_frame(req, &frame, frame.len)` to read payload

Gotchas:

- Your endpoint can be “server push only”, but you still might get client frames (pings, app messages).
- If you set `.handle_ws_control_frames = true`, your handler will also receive PING/PONG/CLOSE; this can add noise or create edge cases during idle periods. For simple “stream only” endpoints, keep it `false` and let the server handle control frames internally.

### Sending frames: `httpd_ws_send_data_async` vs `httpd_ws_send_frame`

Relevant APIs:

- `httpd_ws_send_frame(req, &frame)` (called in handler context)
- `httpd_ws_send_data_async(handle, fd, &frame, cb, arg)` (queue work to httpd task)

Recommendations:

- Prefer `httpd_ws_send_data_async` from outside the handler (e.g., from your stream task). It queues work in httpd’s context and avoids doing socket send from arbitrary tasks.

Gotchas:

- `httpd_ws_send_data_async` can return `ESP_OK` even if the actual send later fails (it queues work). If you need per-send success, use the blocking `httpd_ws_send_data` or implement your own acknowledgement strategy.

### Tracking clients: `httpd_ws_get_fd_info`

Useful helper:

- `httpd_ws_get_fd_info(server, fd)` returns:
  - `HTTPD_WS_CLIENT_WEBSOCKET` when handshake is done and socket is active
  - `HTTPD_WS_CLIENT_HTTP` when the session exists but is not upgraded yet
  - `HTTPD_WS_CLIENT_INVALID` when fd is not a valid session

Gotcha:

- Treating `HTTPD_WS_CLIENT_HTTP` as “disconnect” is too aggressive; it can be a transient state during handshake.

## Implementation checklist (recommended sequence)

### Step 1: bring-up “known good” Wi‑Fi + HTTP

1. Ensure the console is interactive (Cardputer / ESP32‑S3):
   - Prefer USB Serial/JTAG console backend (`sdkconfig.defaults` should enforce this in this repo).
2. Verify Wi‑Fi:
   - device gets an IP (`STA IP: ...`)
3. Verify HTTP:
   - `GET /v1/health` returns something stable

### Step 2: add WebSocket endpoint (empty handler)

Start with handshake only:

```c
static esp_err_t events_ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    // handshake completed
    return ESP_OK;
  }
  return ESP_OK; // ignore data frames initially
}
```

Confirm:

- you do not see `URI '/v1/events/ws' not found` in logs
- a host WS client can connect without immediate disconnects

### Step 3: add client bookkeeping

Maintain a small, bounded list of client fds.

Rules:

- always bound the array (e.g., 8 clients)
- do not malloc per connection if you can avoid it
- remove invalid fds when `httpd_ws_get_fd_info` says invalid

### Step 4: add streaming (with backpressure)

Use a queue (bounded) and counters:

- `drops` when queue is full
- `encode_fail` for protobuf encode failures
- `send_fail` for send queue failures (or “work queue enqueue failures”)

Pseudo:

```c
static void on_event(...) {
  if (client_count == 0) return;
  if (xQueueSend(q, &item, 0) != pdTRUE) drops++;
}

static void stream_task(void*) {
  while (xQueueReceive(q, &item, portMAX_DELAY)) {
    encode_protobuf(item, buf);
    broadcast(buf);
  }
}
```

## Commands (repeatable procedures)

### 0) Avoid serial port lock issues (tmux discipline)

If you can’t flash because `/dev/ttyACM0` is locked, kill the monitor tmux session:

```bash
tmux kill-session -t hub-0029 || true
```

### 1) Flash + monitor in tmux (recommended)

```bash
tmux new-session -d -s hub-0029 \
  "source ~/esp/esp-idf-5.4.1/export.sh && idf.py -C 0029-mock-zigbee-http-hub -p /dev/ttyACM0 flash monitor"

tmux capture-pane -pt hub-0029:0 -S -80
```

### 2) Open the embedded UI

Once the device prints its IP (example `192.168.0.18`), open:

- `http://192.168.0.18/`

Then:

- click `Connect WS`
- click `Seed Devices`

Expected:

- the events list fills up (telemetry reports + command/state events)
- clicking an event shows decoded JSON-like data

### 3) Smoke test WS stream from host (repeatable script)

Script saved in the ticket:

- `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/ws_hub_events.js`

Example:

```bash
node ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/ws_hub_events.js \
  --host 192.168.0.18 --duration 12000 --seed --head
```

### 4) Protobuf HTTP API client (repeatable script)

Script saved in the ticket:

- `ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/http_pb_hub.js`

Examples:

```bash
node ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/http_pb_hub.js \
  --host 192.168.0.18 list

node ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/http_pb_hub.js \
  --host 192.168.0.18 add --type plug --name desk --caps onoff,power

node ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/scripts/http_pb_hub.js \
  --host 192.168.0.18 set --id 1 --on true
```

## Exit Criteria (success conditions)

You should consider WS-over-Wi‑Fi “working” when all are true:

- Device connects to Wi‑Fi and obtains an IP reliably.
- `GET /` serves the UI and `GET /app.js` returns JS content.
- A WS client can connect to `/v1/events/ws` and receive binary frames when events occur.
- Streaming does not stall the console and does not crash the device when multiple clients connect.
- When no clients are connected, the stream bridge is gated off (low overhead).

## Failure modes (symptoms → root cause → fix)

### “URI '/v1/events/ws' not found” (404)

Root causes:

- WS handler not registered due to handler-slot exhaustion
- wrong `.method` or `.is_websocket` not set

Fixes:

- increase `cfg.max_uri_handlers`
- ensure route:
  - `.method = HTTP_GET`
  - `.is_websocket = true`

### “Writing to serial is timing out” / REPL not interactive

Root causes:

- wrong console backend (UART vs USB Serial/JTAG)
- port locked by monitor session

Fixes:

- ensure `sdkconfig.defaults` selects USB Serial/JTAG console
- kill the tmux monitor session holding `/dev/ttyACM0`

### WS connects, then immediately disconnects

Root causes:

- handler returned `ESP_FAIL` or other non-OK error
- receiving control frames in handler and treating recv errors as fatal

Fixes:

- always return `ESP_OK` for normal “no data” situations
- prefer `.handle_ws_control_frames = false` for server-push endpoints

### Event stream causes lag/resets

Root causes:

- encoding/sending done inside event loop callbacks
- unbounded allocation or per-event heap churn
- slow client blocks sending

Fixes:

- queue + dedicated stream task (see architecture section)
- bounded buffers and bounded queues
- drop policy + counters

## Security notes (for production)

This demo assumes a trusted LAN. If you productize:

- add authentication (token/cookie) for HTTP and WS
- consider TLS (`wss://`) if you stream anything sensitive
- rate-limit or bound incoming frames (even if you “ignore” them)
- avoid exposing debug endpoints like `/v1/debug/seed`

## Appendix: API reference pointers (in this repo)

- HTTP server + WS endpoint + UI serving:
  - `0029-mock-zigbee-http-hub/main/hub_http.c` (`hub_http_start`, `/v1/events/ws`)
- Stream bridge pattern:
  - `0029-mock-zigbee-http-hub/main/hub_stream.c` (`hub_stream_start`, counters)
- Protobuf encoding:
  - `0029-mock-zigbee-http-hub/main/hub_pb.c` (`hub_pb_build_event`, `hub_pb_fill_device`)
  - `0029-mock-zigbee-http-hub/components/hub_proto/defs/hub_events.proto`
- Wi‑Fi bring-up + console:
  - `0029-mock-zigbee-http-hub/main/wifi_sta.c`
  - `0029-mock-zigbee-http-hub/main/wifi_console.c`

## Environment Assumptions

<!-- What environment or setup is required? -->

## Commands

<!-- List of commands to execute -->

```bash
# Command sequence
```

## Exit Criteria

<!-- What indicates success or completion? -->

## Notes

<!-- Additional context or warnings -->
