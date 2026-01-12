---
Title: 'Research Diary: Device-Hosted UI Pattern'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - wifi
    - http
    - websocket
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: HTTP server with REST API and WebSocket
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_softap.cpp
      Note: SoftAP initialization
    - Path: esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c
      Note: Protobuf HTTP API with event bus integration
    - Path: esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_sta.c
      Note: STA mode with NVS credential storage
    - Path: esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_stream.c
      Note: Event bus to WebSocket bridge
ExternalSources: []
Summary: 'Research diary documenting investigation of the device-hosted UI pattern across multiple projects.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Capture research trail for the device-hosted UI guide.'
WhenToUse: 'Reference when writing or updating device-hosted UI documentation.'
---

# Research Diary: Device-Hosted UI Pattern

## Goal

Document the research process for understanding the "device-hosted UI" pattern used across multiple ESP32-S3 projects. This pattern involves the device running its own web server (WiFi SoftAP or STA), serving a web UI, and streaming events via WebSocket.

## Step 1: Identify the pattern across projects

Started by reading READMEs for the three projects mentioned in the playbook guide.

### What I did
- Read `0017-atoms3r-web-ui/README.md` — graphics upload + WebSocket terminal
- Read `0021-atoms3-memo-website/README.md` — memo recorder web UI
- Read `0029-mock-zigbee-http-hub/README.md` — protobuf API + event streaming

### What I learned

The pattern appears in three variants:

| Project | WiFi Mode | API Format | WebSocket Purpose |
|---------|-----------|------------|-------------------|
| 0017 | SoftAP (default) | JSON | UART terminal + button events |
| 0021 | STA (with fallback) | JSON | (minimal) |
| 0029 | STA | Protobuf | Event bus observability |

Common elements:
1. Device creates network connectivity (SoftAP or STA)
2. `esp_http_server` serves embedded assets and API endpoints
3. WebSocket provides real-time streaming
4. File uploads go to FATFS storage

## Step 2: Deep dive into 0017 HTTP server

Read `0017-atoms3r-web-ui/main/http_server.cpp` (567 lines) to understand the HTTP server architecture.

### Key findings

**Embedded asset serving:**
Assets are linked into the firmware binary using `asm()` labels:

```c
extern const uint8_t assets_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t assets_index_html_end[] asm("_binary_index_html_end");
```

The root handler serves the HTML directly from flash:

```c
static esp_err_t root_get(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    const size_t len = (size_t)(assets_index_html_end - assets_index_html_start);
    return httpd_resp_send(req, (const char *)assets_index_html_start, len);
}
```

**Route registration pattern:**

```c
esp_err_t http_server_start(void) {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;  // Enable /* patterns
    
    httpd_start(&s_server, &cfg);
    
    // Static routes
    httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    httpd_register_uri_handler(s_server, &root);
    
    // Wildcard routes for REST resources
    httpd_uri_t put = {.uri = "/api/graphics/*", .method = HTTP_PUT, .handler = graphics_put};
    httpd_register_uri_handler(s_server, &put);
    
    // WebSocket endpoint
    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true,
        .handle_ws_control_frames = true,
    };
    httpd_register_uri_handler(s_server, &ws);
}
```

**WebSocket client tracking:**

The server maintains a list of connected WebSocket clients:

```c
static int s_ws_clients[8] = {};
static size_t s_ws_clients_n = 0;

static void ws_client_add(int fd) { /* ... */ }
static void ws_client_remove(int fd) { /* ... */ }
static size_t ws_clients_snapshot(int *out, size_t max_out) { /* ... */ }
```

Broadcast sends to all clients asynchronously:

```c
esp_err_t http_server_ws_broadcast_binary(const uint8_t *data, size_t len) {
    int fds[8];
    const size_t n = ws_clients_snapshot(fds, ...);
    for (size_t i = 0; i < n; i++) {
        httpd_ws_frame_t frame = {.type = HTTPD_WS_TYPE_BINARY, .payload = copy, .len = len};
        httpd_ws_send_data_async(s_server, fds[i], &frame, ws_send_free_cb, copy);
    }
}
```

**File upload flow:**

```c
static esp_err_t graphics_put(httpd_req_t *req) {
    // 1. Validate filename
    // 2. Check content length
    // 3. Open file for writing
    FILE *f = fopen(path, "wb");
    
    // 4. Stream body to file
    char buf[1024];
    while (remaining > 0) {
        int n = httpd_req_recv(req, buf, to_read);
        fwrite(buf, 1, n, f);
        remaining -= n;
    }
    fclose(f);
    
    // 5. Trigger display update
    display_app_png_from_file(path);
    
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}
```

## Step 3: WiFi mode selection (0017 Kconfig)

Read `0017-atoms3r-web-ui/main/Kconfig.projbuild` to understand WiFi configuration.

### Key findings

**Three WiFi modes supported:**

```
choice TUTORIAL_0017_WIFI_MODE
    prompt "WiFi mode"
    default TUTORIAL_0017_WIFI_MODE_SOFTAP

config TUTORIAL_0017_WIFI_MODE_SOFTAP
    bool "SoftAP (device-hosted network)"

config TUTORIAL_0017_WIFI_MODE_STA
    bool "STA (join existing network, DHCP)"

config TUTORIAL_0017_WIFI_MODE_APSTA
    bool "AP+STA (SoftAP + join existing network)"
endchoice
```

**STA fallback to SoftAP:**

```
config TUTORIAL_0017_WIFI_STA_FALLBACK_TO_SOFTAP
    bool "If STA fails, fall back to SoftAP"
    default y
    depends on TUTORIAL_0017_WIFI_MODE_STA
```

This is a critical UX feature: if the device can't connect to the configured network, it falls back to SoftAP so you can still access it for debugging.

## Step 4: SoftAP initialization (0017)

Read `0017-atoms3r-web-ui/main/wifi_softap.cpp` to understand SoftAP setup.

### Key findings

**SoftAP startup sequence:**

```c
esp_err_t wifi_softap_start(void) {
    // 1. Initialize NVS (required for WiFi)
    nvs_flash_init();
    
    // 2. Initialize network interface and event loop
    esp_netif_init();
    esp_event_loop_create_default();
    
    // 3. Create AP network interface
    s_ap_netif = esp_netif_create_default_wifi_ap();
    
    // 4. Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    // 5. Configure AP parameters
    wifi_config_t ap_cfg = {};
    strncpy((char *)ap_cfg.ap.ssid, CONFIG_WIFI_SOFTAP_SSID, ...);
    strncpy((char *)ap_cfg.ap.password, CONFIG_WIFI_SOFTAP_PASSWORD, ...);
    ap_cfg.ap.authmode = password[0] ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    
    // 6. Start WiFi
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    esp_wifi_start();
    
    // DHCP server starts automatically
    ESP_LOGI(TAG, "browse: http://192.168.4.1/");
}
```

Default AP IP is always `192.168.4.1` — this is the ESP-IDF default for SoftAP.

## Step 5: STA mode with NVS credentials (0029)

Read `0029-mock-zigbee-http-hub/main/wifi_sta.c` to understand runtime credential management.

### Key findings

**Credential sources (priority order):**
1. NVS (saved credentials from previous session)
2. Kconfig defaults (compile-time)

**NVS storage pattern:**

```c
static esp_err_t save_credentials_to_nvs(const char *ssid, const char *password) {
    nvs_handle_t nvs = 0;
    nvs_open("wifi", NVS_READWRITE, &nvs);
    nvs_set_str(nvs, "ssid", ssid);
    nvs_set_str(nvs, "pass", password);
    nvs_commit(nvs);
    nvs_close(nvs);
}

static esp_err_t load_saved_credentials(void) {
    nvs_handle_t nvs = 0;
    nvs_open("wifi", NVS_READWRITE, &nvs);
    nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    nvs_get_str(nvs, "pass", pass, &pass_len);
    nvs_close(nvs);
}
```

**Console-based credential management:**

The 0029 project exposes WiFi commands via `esp_console`:

```
hub> wifi scan
hub> wifi set <ssid> <password> save
hub> wifi connect
hub> wifi clear
```

This is a powerful pattern for development: credentials don't need to be compiled in.

## Step 6: Protobuf HTTP API (0029)

Read `0029-mock-zigbee-http-hub/main/hub_http.c` (821 lines) to understand the protobuf API pattern.

### Key findings

**Content-Type: application/x-protobuf:**

All API endpoints use protobuf encoding:

```c
static esp_err_t devices_list_get(httpd_req_t *req) {
    // Build protobuf message
    hub_v1_DeviceList list = hub_v1_DeviceList_init_zero;
    for (...) {
        hub_pb_fill_device(&list.devices[list.devices_count], &snap[i]);
        list.devices_count++;
    }
    
    // Encode to buffer
    uint8_t buf[1024];
    pb_ostream_t s = pb_ostream_from_buffer(buf, sizeof(buf));
    pb_encode(&s, hub_v1_DeviceList_fields, &list);
    
    // Send response
    httpd_resp_set_type(req, "application/x-protobuf");
    return httpd_resp_send(req, (const char *)buf, s.bytes_written);
}
```

**Command-response via event bus:**

HTTP handlers don't modify state directly. They post commands to an internal event bus and wait for a reply:

```c
static esp_err_t device_set_post(httpd_req_t *req) {
    // Parse protobuf body
    hub_v1_CmdDeviceSet in = hub_v1_CmdDeviceSet_init_zero;
    pb_decode(&s, hub_v1_CmdDeviceSet_fields, &in);
    
    // Create reply queue
    QueueHandle_t q = xQueueCreate(1, sizeof(hub_reply_status_t));
    
    // Post command to event bus
    hub_cmd_device_set_t cmd = {...};
    cmd.hdr.reply_q = q;
    esp_event_post_to(loop, HUB_EVT, HUB_CMD_DEVICE_SET, &cmd, ...);
    
    // Wait for reply
    hub_reply_status_t rep = {0};
    xQueueReceive(q, &rep, pdMS_TO_TICKS(250));
    vQueueDelete(q);
    
    // Send response
    return httpd_resp_send(req, ...);
}
```

This decouples HTTP handlers from state management and enables clean event streaming.

## Step 7: WebSocket event streaming (0029)

Read `0029-mock-zigbee-http-hub/main/hub_stream.c` to understand the event-to-WebSocket bridge.

### Key findings

**Bridge architecture:**

```
┌────────────┐      ┌────────────┐      ┌────────────┐
│ Event Bus  │─────►│  Queue     │─────►│ Stream     │
│ (hub loop) │      │ (ISR-safe) │      │ Task       │
└────────────┘      └────────────┘      └─────┬──────┘
                                              │
                                              ▼
                                        ┌────────────┐
                                        │ WebSocket  │
                                        │ Broadcast  │
                                        └────────────┘
```

**Event handler enqueues without blocking:**

```c
static void on_any_event_enqueue(void *arg, esp_event_base_t base, int32_t id, void *data) {
    // Skip if no clients connected (optimization)
    if (hub_http_events_client_count() == 0) return;
    
    // Copy event data to queue item
    hub_stream_item_t it = {.id = id};
    memcpy(&it.payload, data, payload_size_for_id(id));
    
    // Non-blocking send (drops if queue full)
    if (xQueueSend(s_q, &it, 0) != pdTRUE) {
        s_drops++;
    }
}
```

**Stream task encodes and broadcasts:**

```c
static void stream_task(void *arg) {
    while (true) {
        xQueueReceive(s_q, &it, portMAX_DELAY);
        
        // Build protobuf event
        hub_v1_HubEvent ev = hub_v1_HubEvent_init_zero;
        hub_pb_build_event(it.id, &it.payload, &ev);
        
        // Encode
        pb_ostream_t s = pb_ostream_from_buffer(buf, sizeof(buf));
        pb_encode(&s, hub_v1_HubEvent_fields, &ev);
        
        // Broadcast to all WebSocket clients
        hub_http_events_broadcast_pb(buf, s.bytes_written);
    }
}
```

This design keeps the event loop task fast (no encoding in handler) and allows backpressure (queue can fill up).

## Step 8: Synthesis

The device-hosted UI pattern has these core components:

1. **Network layer** — SoftAP (device is AP) or STA (device joins network)
2. **HTTP server** — `esp_http_server` with embedded assets and REST API
3. **WebSocket endpoint** — Real-time bidirectional communication
4. **Storage** — FATFS for uploads and persistent data
5. **Event bridge** (optional) — Connect internal events to WebSocket stream

### Pattern variations

| Aspect | Simple (0017) | Advanced (0029) |
|--------|---------------|-----------------|
| API format | JSON | Protobuf |
| State management | Direct | Event bus |
| WebSocket | Terminal passthrough | Event stream |
| Credential storage | Kconfig only | NVS + runtime |

## Quick Reference

### ESP-IDF APIs used

| API | Purpose |
|-----|---------|
| `esp_netif_create_default_wifi_ap()` | Create SoftAP interface |
| `esp_netif_create_default_wifi_sta()` | Create STA interface |
| `esp_wifi_set_mode()` | Set WiFi mode (AP/STA/APSTA) |
| `httpd_start()` | Start HTTP server |
| `httpd_register_uri_handler()` | Register route handler |
| `httpd_ws_send_data_async()` | Send WebSocket frame |
| `nvs_set_str()` / `nvs_get_str()` | Persist credentials |

### Default endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/` | GET | Serve main HTML page |
| `/assets/*` | GET | Serve CSS/JS |
| `/api/status` | GET | Device status JSON |
| `/api/graphics` | GET | List uploaded files |
| `/api/graphics/*` | PUT | Upload file |
| `/ws` | GET | WebSocket upgrade |
