---
Title: 'Device-Hosted UI Pattern: SoftAP, HTTP Server, and WebSocket Streaming'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
    - wifi
    - http
    - websocket
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/0017-atoms3r-web-ui/
      Note: Graphics upload + WebSocket terminal example
    - Path: esp32-s3-m5/0021-atoms3-memo-website/
      Note: Memo recorder with STA mode
    - Path: esp32-s3-m5/0029-mock-zigbee-http-hub/
      Note: Protobuf API with event bus integration
ExternalSources:
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-reference/protocols/esp_http_server.html
      Note: ESP-IDF HTTP Server documentation
    - URL: https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32s3/api-guides/wifi.html
      Note: ESP-IDF WiFi documentation
Summary: 'Complete guide to building device-hosted web UIs with WiFi, HTTP server, and WebSocket streaming.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Help developers add web-based configuration and monitoring UIs to their ESP32 devices.'
WhenToUse: 'Reference when building device-hosted web interfaces.'
---

# Building Device-Hosted Web UIs on ESP32-S3

One of the most powerful patterns in embedded development is the device-hosted web UI: your ESP32 runs its own web server, serves its own HTML/CSS/JavaScript, and provides real-time updates via WebSocket—all without any external infrastructure. Users connect their phone or laptop directly to the device (or through a local network), open a browser, and interact with a modern web interface.

This pattern appears throughout our codebase in different forms: a graphics upload tool with WebSocket terminal (`0017`), a memo recorder (`0021`), and a simulated smart home hub with protobuf APIs (`0029`). In this guide, we'll extract the common architecture, understand the key decisions, and walk through implementation in detail.

## The Pattern at a Glance

Every device-hosted UI shares these components:

```
┌────────────────────────────────────────────────────────────────────┐
│                        USER'S DEVICE                               │
│                     (phone, laptop, etc.)                          │
│                                                                    │
│   ┌─────────────┐                                                 │
│   │  Browser    │                                                 │
│   │             │                                                 │
│   │  ┌───────┐  │                                                 │
│   │  │ HTML  │  │ ◄────────── HTTP GET /                          │
│   │  │ CSS   │  │ ◄────────── HTTP GET /assets/*                  │
│   │  │ JS    │  │                                                 │
│   │  └───────┘  │                                                 │
│   │      │      │                                                 │
│   │      ▼      │                                                 │
│   │  ┌───────┐  │                                                 │
│   │  │ REST  │  │ ────────── HTTP GET/POST/PUT /api/*             │
│   │  │ API   │  │                                                 │
│   │  └───────┘  │                                                 │
│   │      │      │                                                 │
│   │      ▼      │                                                 │
│   │  ┌───────┐  │                                                 │
│   │  │  WS   │  │ ◄══════════ WebSocket /ws (bidirectional)       │
│   │  │Stream │  │                                                 │
│   │  └───────┘  │                                                 │
│   └─────────────┘                                                 │
│          │                                                        │
└──────────┼────────────────────────────────────────────────────────┘
           │
    WiFi   │  (SoftAP: device creates network)
    Link   │  (STA: device joins existing network)
           │
           ▼
┌────────────────────────────────────────────────────────────────────┐
│                        ESP32-S3 DEVICE                             │
│                                                                    │
│   ┌─────────────────────────────────────────────────────────────┐ │
│   │                     esp_http_server                          │ │
│   │                                                              │ │
│   │  Routes:                                                     │ │
│   │  GET  /           → serve embedded HTML                      │ │
│   │  GET  /assets/*   → serve embedded CSS/JS                    │ │
│   │  GET  /api/status → return JSON status                       │ │
│   │  PUT  /api/files/* → receive upload, write to FATFS          │ │
│   │  GET  /ws         → WebSocket upgrade + stream               │ │
│   └─────────────────────────────────────────────────────────────┘ │
│                             │                                      │
│                             ▼                                      │
│   ┌────────────┐    ┌────────────┐    ┌────────────┐              │
│   │  FATFS     │    │  Display   │    │  Sensors   │              │
│   │  Storage   │    │  Hardware  │    │  Actuators │              │
│   └────────────┘    └────────────┘    └────────────┘              │
└────────────────────────────────────────────────────────────────────┘
```

The beauty of this approach is self-containment. The device needs no cloud infrastructure, no internet connection, and no app installation. Just WiFi and a browser.

---

## Networking: SoftAP vs STA

The first architectural decision is how the device gets network connectivity. ESP-IDF supports three modes, each with distinct tradeoffs.

### SoftAP Mode: The Device Is the Network

In SoftAP (Software Access Point) mode, the ESP32 creates its own WiFi network. Clients connect to the device directly, just like connecting to a regular WiFi router.

```
┌──────────────┐                    ┌──────────────┐
│    Phone     │      WiFi          │   ESP32-S3   │
│              │  ◄──────────────►  │   (SoftAP)   │
│  Browser     │   "AtomS3R-WebUI"  │              │
└──────────────┘                    │  192.168.4.1 │
                                    └──────────────┘
```

**How to set up SoftAP:**

```c
esp_err_t wifi_softap_start(void) {
    // 1. Initialize prerequisites
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 2. Create the AP network interface
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    
    // 3. Initialize WiFi subsystem
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 4. Configure AP parameters
    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = "AtomS3R-WebUI",
            .ssid_len = strlen("AtomS3R-WebUI"),
            .password = "",  // Empty = open network
            .channel = 6,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,  // Or WIFI_AUTH_WPA2_PSK if password set
        },
    };
    
    // 5. Start the access point
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // DHCP server starts automatically
    // Device is always at 192.168.4.1
    ESP_LOGI(TAG, "SoftAP started: http://192.168.4.1/");
    return ESP_OK;
}
```

**When to use SoftAP:**

- **Field deployment** — Device operates without existing infrastructure
- **Initial setup** — User connects to configure WiFi credentials
- **Guaranteed access** — Device is always reachable at known IP
- **Simple demos** — No network configuration needed

**SoftAP limitations:**

- Only one network at a time — client disconnects from the internet
- Fixed IP (192.168.4.1) — predictable but not discoverable
- Limited range — ESP32 RF power isn't optimized for AP mode

### STA Mode: The Device Joins an Existing Network

In STA (Station) mode, the ESP32 connects to an existing WiFi network like any other client device. It receives an IP address via DHCP and becomes accessible on the local network.

```
┌──────────────┐                    ┌──────────────┐
│   Router     │                    │              │
│  (Your WiFi) │ ◄─────────────────►│   ESP32-S3   │
│              │                    │    (STA)     │
└──────┬───────┘                    │  192.168.1.x │
       │                            └──────────────┘
       │                                   ▲
       │                                   │
       ▼                                   │
┌──────────────┐                           │
│    Phone     │       Same network        │
│              │ ◄─────────────────────────┘
│  Browser     │
└──────────────┘
```

**How to set up STA mode:**

```c
esp_err_t wifi_sta_start(const char *ssid, const char *password) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handlers for connection status
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                                &wifi_event_handler, sta_netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                                &wifi_event_handler, sta_netif));
    
    wifi_config_t sta_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_OPEN,
            .pmf_cfg = {.capable = true, .required = false},
        },
    };
    strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)sta_cfg.sta.password, password, sizeof(sta_cfg.sta.password));
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Connection happens asynchronously - check for IP_EVENT_STA_GOT_IP
    ESP_LOGI(TAG, "STA connecting to %s...", ssid);
    return ESP_OK;
}

static void wifi_event_handler(void *arg, esp_event_base_t base, 
                                int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Disconnected, retrying...");
        esp_wifi_connect();  // Auto-reconnect
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}
```

**When to use STA mode:**

- **Home/office deployment** — Device on existing network
- **Internet access needed** — Device needs to reach external services
- **Multiple devices** — All on same network, can discover each other
- **Development** — More convenient than switching networks

**STA challenges:**

- Credentials required — How does the user configure them?
- IP discovery — mDNS or display the IP on screen
- Connection failures — What happens if WiFi is unavailable?

### The Hybrid Approach: APSTA Mode

ESP-IDF supports running both modes simultaneously. The device creates its own AP while also connecting to an existing network. This is useful for initial setup: the user connects to the device's AP to configure the STA credentials.

```c
ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
ESP_ERROR_CHECK(esp_wifi_start());
```

### The Fallback Pattern

A robust approach combines STA with SoftAP fallback:

```c
// Try to connect to configured network
esp_err_t err = wifi_sta_connect(ssid, password, TIMEOUT_MS);

if (err != ESP_OK) {
    ESP_LOGW(TAG, "STA connection failed, falling back to SoftAP");
    wifi_softap_start();  // At least the user can still access the device
}
```

This ensures the device is always accessible—either on the configured network or via its own AP.

---

## Storing WiFi Credentials

Hardcoding credentials in your firmware is problematic: you can't ship devices with customer-specific WiFi passwords. The solution is NVS (Non-Volatile Storage)—a key-value store in flash that persists across reboots.

### Saving Credentials

```c
esp_err_t wifi_save_credentials(const char *ssid, const char *password) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    
    err = nvs_set_str(nvs, "ssid", ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(nvs, "pass", password ? password : "");
    }
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    
    nvs_close(nvs);
    return err;
}
```

### Loading Credentials on Boot

```c
esp_err_t wifi_load_credentials(char *ssid, size_t ssid_len, 
                                 char *password, size_t pass_len) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi", NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;
    
    err = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    if (err == ESP_OK) {
        nvs_get_str(nvs, "pass", password, &pass_len);  // Optional
    }
    
    nvs_close(nvs);
    return err;
}
```

### Runtime Configuration via Console

The `0029` project exposes WiFi configuration through `esp_console`:

```
hub> wifi scan
  MyNetwork        -45 dBm  ch 6  WPA2
  Neighbor_5G      -67 dBm  ch 36 WPA2

hub> wifi set MyNetwork SuperSecret save
Credentials saved to NVS

hub> wifi connect
Connecting to MyNetwork...
Connected! IP: 192.168.1.105

hub> wifi clear
Credentials cleared from NVS
```

This is invaluable during development—no need to recompile for different networks.

---

## The HTTP Server

ESP-IDF's `esp_http_server` is a lightweight, single-threaded HTTP server that handles requests synchronously. It's sufficient for device-hosted UIs where you expect a handful of concurrent clients, not thousands.

### Starting the Server

```c
static httpd_handle_t s_server = NULL;

esp_err_t http_server_start(void) {
    if (s_server) return ESP_OK;  // Already running
    
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.uri_match_fn = httpd_uri_match_wildcard;  // Enable /* patterns
    cfg.max_uri_handlers = 16;
    cfg.stack_size = 8192;  // Increase if handlers do heavy work
    
    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(err));
        return err;
    }
    
    // Register routes (see below)
    register_routes();
    
    ESP_LOGI(TAG, "HTTP server started on port %d", cfg.server_port);
    return ESP_OK;
}
```

### Route Registration

Each route maps a URI pattern and HTTP method to a handler function:

```c
static void register_routes(void) {
    // Serve the main page
    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &root);
    
    // Serve static assets
    httpd_uri_t assets = {
        .uri = "/assets/*",
        .method = HTTP_GET,
        .handler = assets_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &assets);
    
    // REST API endpoints
    httpd_uri_t status = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &status);
    
    // File upload (wildcard for filename)
    httpd_uri_t upload = {
        .uri = "/api/files/*",
        .method = HTTP_PUT,
        .handler = upload_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &upload);
}
```

### Wildcard Matching

By setting `cfg.uri_match_fn = httpd_uri_match_wildcard`, you enable patterns like `/api/files/*`. The handler can then parse the actual path from `req->uri`:

```c
static esp_err_t upload_handler(httpd_req_t *req) {
    // req->uri might be "/api/files/image.png"
    const char *filename = req->uri + strlen("/api/files/");
    ESP_LOGI(TAG, "Uploading: %s", filename);
    // ...
}
```

---

## Embedding Assets in Firmware

Rather than serving files from flash storage, you can embed HTML, CSS, and JavaScript directly into the firmware binary. This ensures they're always present and makes deployment simpler.

### The CMakeLists.txt Setup

In your component's `CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "http_server.c" "main.c"
    INCLUDE_DIRS "."
    EMBED_FILES "assets/index.html" "assets/app.js" "assets/app.css"
)
```

The `EMBED_FILES` directive includes the files as binary blobs in the firmware.

### Accessing Embedded Files in Code

ESP-IDF creates symbols for the start and end of each embedded file:

```c
// Symbols generated by build system
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t app_js_start[]     asm("_binary_app_js_start");
extern const uint8_t app_js_end[]       asm("_binary_app_js_end");
extern const uint8_t app_css_start[]    asm("_binary_app_css_start");
extern const uint8_t app_css_end[]      asm("_binary_app_css_end");
```

### Serving Embedded Files

```c
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    const size_t len = index_html_end - index_html_start;
    return httpd_resp_send(req, (const char *)index_html_start, len);
}

static esp_err_t assets_handler(httpd_req_t *req) {
    const char *uri = req->uri;
    
    if (strcmp(uri, "/assets/app.js") == 0) {
        httpd_resp_set_type(req, "application/javascript");
        return httpd_resp_send(req, (const char *)app_js_start,
                               app_js_end - app_js_start);
    }
    
    if (strcmp(uri, "/assets/app.css") == 0) {
        httpd_resp_set_type(req, "text/css");
        return httpd_resp_send(req, (const char *)app_css_start,
                               app_css_end - app_css_start);
    }
    
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_OK;
}
```

---

## REST API Handlers

For API endpoints, you'll typically parse request bodies, interact with device state, and return JSON or protobuf responses.

### A Simple Status Endpoint

```c
static esp_err_t status_handler(httpd_req_t *req) {
    char buf[256];
    
    // Gather device status
    uint32_t uptime_ms = esp_timer_get_time() / 1000;
    uint32_t free_heap = esp_get_free_heap_size();
    
    // Format as JSON
    snprintf(buf, sizeof(buf),
             "{\"ok\":true,\"uptime_ms\":%lu,\"free_heap\":%lu}",
             (unsigned long)uptime_ms,
             (unsigned long)free_heap);
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
}
```

### Handling POST/PUT Bodies

For endpoints that receive data:

```c
static esp_err_t config_handler(httpd_req_t *req) {
    // Check content length
    if (req->content_len <= 0 || req->content_len > 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad content length");
        return ESP_OK;
    }
    
    // Read body
    char buf[1024];
    int received = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (received <= 0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Recv failed");
        return ESP_OK;
    }
    buf[received] = '\0';
    
    // Parse JSON (use cJSON or similar)
    // Apply configuration
    // ...
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}
```

---

## File Uploads

For uploading files (images, configurations, etc.) to flash storage, you need to stream the request body to FATFS.

### Chunked Receive Pattern

```c
static esp_err_t upload_handler(httpd_req_t *req) {
    const char *filename = extract_filename_from_uri(req->uri);
    
    // Validate
    if (!filename || req->content_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
        return ESP_OK;
    }
    
    if (req->content_len > CONFIG_MAX_UPLOAD_SIZE) {
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "File too large");
        return ESP_OK;
    }
    
    // Build path
    char path[128];
    snprintf(path, sizeof(path), "/storage/uploads/%s", filename);
    
    // Open file
    FILE *f = fopen(path, "wb");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Open failed");
        return ESP_OK;
    }
    
    // Stream body to file
    char buf[1024];
    int remaining = req->content_len;
    
    while (remaining > 0) {
        int to_read = (remaining > sizeof(buf)) ? sizeof(buf) : remaining;
        int received = httpd_req_recv(req, buf, to_read);
        
        if (received <= 0) {
            fclose(f);
            unlink(path);  // Clean up partial file
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Recv failed");
            return ESP_OK;
        }
        
        if (fwrite(buf, 1, received, f) != received) {
            fclose(f);
            unlink(path);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
            return ESP_OK;
        }
        
        remaining -= received;
    }
    
    fclose(f);
    
    // Success response
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"ok\":true}");
}
```

### FATFS Setup

Before uploading files, you need to mount the FATFS partition:

```c
esp_err_t storage_init(void) {
    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
    };
    
    static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
    
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(
        "/storage",           // Mount path
        "storage",            // Partition label
        &mount_cfg,
        &s_wl_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "FATFS mount failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Create upload directory if needed
    mkdir("/storage/uploads", 0755);
    
    ESP_LOGI(TAG, "FATFS mounted at /storage");
    return ESP_OK;
}
```

---

## WebSocket Streaming

WebSockets enable real-time bidirectional communication. The device can push updates to the browser without polling, and the browser can send commands instantly.

### Enabling WebSocket Support

In `sdkconfig.defaults`:

```
CONFIG_HTTPD_WS_SUPPORT=y
```

### Registering a WebSocket Endpoint

```c
static esp_err_t ws_handler(httpd_req_t *req);

static void register_websocket(void) {
    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true,
    };
    httpd_register_uri_handler(s_server, &ws);
}
```

### Tracking Connected Clients

The HTTP server doesn't automatically track WebSocket clients. You need to maintain a list:

```c
static SemaphoreHandle_t s_ws_mutex = NULL;
static int s_ws_clients[8] = {0};
static size_t s_ws_client_count = 0;

static void ws_add_client(int fd) {
    xSemaphoreTake(s_ws_mutex, portMAX_DELAY);
    if (s_ws_client_count < 8) {
        s_ws_clients[s_ws_client_count++] = fd;
    }
    xSemaphoreGive(s_ws_mutex);
}

static void ws_remove_client(int fd) {
    xSemaphoreTake(s_ws_mutex, portMAX_DELAY);
    for (size_t i = 0; i < s_ws_client_count; i++) {
        if (s_ws_clients[i] == fd) {
            s_ws_clients[i] = s_ws_clients[--s_ws_client_count];
            break;
        }
    }
    xSemaphoreGive(s_ws_mutex);
}
```

### The WebSocket Handler

```c
static esp_err_t ws_handler(httpd_req_t *req) {
    int fd = httpd_req_to_sockfd(req);
    
    // HTTP_GET with is_websocket=true means this is the upgrade request
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket connected: fd=%d", fd);
        ws_add_client(fd);
        return ESP_OK;
    }
    
    // Subsequent calls are for WebSocket data frames
    httpd_ws_frame_t frame = {0};
    
    // First call with len=0 to get frame info
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0);
    if (err != ESP_OK) return err;
    
    if (frame.len > 1024) {
        ESP_LOGW(TAG, "Frame too large");
        ws_remove_client(fd);
        return ESP_FAIL;
    }
    
    // Allocate buffer and receive payload
    uint8_t buf[1024];
    frame.payload = buf;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err != ESP_OK) return err;
    
    // Handle frame type
    if (frame.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "WebSocket closed: fd=%d", fd);
        ws_remove_client(fd);
        return ESP_OK;
    }
    
    if (frame.type == HTTPD_WS_TYPE_BINARY) {
        // Process binary message
        process_ws_message(buf, frame.len);
    }
    
    return ESP_OK;
}
```

### Broadcasting to All Clients

```c
esp_err_t ws_broadcast(const uint8_t *data, size_t len) {
    if (!s_server) return ESP_ERR_INVALID_STATE;
    
    xSemaphoreTake(s_ws_mutex, portMAX_DELAY);
    
    for (size_t i = 0; i < s_ws_client_count; i++) {
        int fd = s_ws_clients[i];
        
        // Check if still connected
        if (httpd_ws_get_fd_info(s_server, fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
            continue;
        }
        
        // Allocate copy (freed by callback)
        uint8_t *copy = malloc(len);
        if (!copy) continue;
        memcpy(copy, data, len);
        
        httpd_ws_frame_t frame = {
            .type = HTTPD_WS_TYPE_BINARY,
            .payload = copy,
            .len = len,
        };
        
        // Async send with cleanup callback
        httpd_ws_send_data_async(s_server, fd, &frame, ws_send_done, copy);
    }
    
    xSemaphoreGive(s_ws_mutex);
    return ESP_OK;
}

static void ws_send_done(esp_err_t err, int fd, void *arg) {
    free(arg);  // Free the copied data
}
```

---

## Event Bus to WebSocket Bridge

For more complex applications, you might have an internal event bus (using `esp_event`) that manages device state. You can bridge this to WebSocket to give the browser real-time visibility into all device events.

### The Bridge Architecture

```
┌────────────────┐      ┌────────────────┐      ┌────────────────┐
│  Event Bus     │      │  Queue         │      │  Stream Task   │
│  Handlers      │─────►│  (thread-safe) │─────►│  (encodes +    │
│  post events   │      │                │      │   broadcasts)  │
└────────────────┘      └────────────────┘      └───────┬────────┘
                                                        │
                                                        ▼
                                                ┌────────────────┐
                                                │  WebSocket     │
                                                │  Clients       │
                                                └────────────────┘
```

This design keeps the event handlers fast (just enqueue) and moves encoding/sending to a dedicated task.

### Implementation

```c
static QueueHandle_t s_stream_queue = NULL;
static TaskHandle_t s_stream_task = NULL;

typedef struct {
    int32_t event_id;
    uint8_t payload[256];
    size_t payload_len;
} stream_item_t;

// Called from event handlers
static void on_device_event(void *arg, esp_event_base_t base, 
                            int32_t id, void *data) {
    if (ws_client_count() == 0) return;  // Optimization: skip if no listeners
    
    stream_item_t item = {.event_id = id};
    memcpy(item.payload, data, get_payload_size(id));
    item.payload_len = get_payload_size(id);
    
    // Non-blocking send (drop if queue full)
    xQueueSend(s_stream_queue, &item, 0);
}

// Dedicated task for encoding and broadcasting
static void stream_task(void *arg) {
    stream_item_t item;
    uint8_t buf[512];
    
    while (true) {
        if (xQueueReceive(s_stream_queue, &item, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        
        // Encode event (JSON or protobuf)
        size_t len = encode_event(&item, buf, sizeof(buf));
        if (len == 0) continue;
        
        // Broadcast to all WebSocket clients
        ws_broadcast(buf, len);
    }
}

esp_err_t stream_bridge_start(esp_event_loop_handle_t loop) {
    s_stream_queue = xQueueCreate(64, sizeof(stream_item_t));
    xTaskCreate(stream_task, "stream", 4096, NULL, 5, &s_stream_task);
    
    // Register for all events
    esp_event_handler_register_with(loop, MY_EVENTS, ESP_EVENT_ANY_ID,
                                    on_device_event, NULL);
    return ESP_OK;
}
```

---

## Troubleshooting

### "Can't connect to device WiFi"

**SoftAP issues:**
- Check if the AP is visible in your phone's WiFi list
- Verify the SSID in menuconfig matches what you're looking for
- Some phones auto-disconnect from networks without internet

**STA issues:**
- Check credentials are correct (case-sensitive!)
- Verify the network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check boot log for connection errors

### "Page won't load"

- Verify the HTTP server started (check log for "starting http server")
- Confirm you're using the correct IP (192.168.4.1 for SoftAP)
- Check that routes are registered before server start

### "WebSocket keeps disconnecting"

- The HTTP server has a timeout for idle connections
- Send periodic ping frames from the browser
- Check for memory leaks in your handlers

### "File upload fails partway through"

- Check FATFS partition size and free space
- Verify `CONFIG_MAX_UPLOAD_SIZE` is large enough
- Monitor memory usage during upload

### "Credentials not persisting"

- Verify NVS partition exists in partition table
- Check NVS initialization on boot
- Ensure you're calling `nvs_commit()` after writes

---

## Example Projects

### 0017 — Graphics Upload + WebSocket Terminal

Simple device-hosted UI with:
- SoftAP networking (configurable)
- Image upload to FATFS → display on LCD
- WebSocket bridge to UART terminal
- Button event streaming

### 0029 — Mock Zigbee Hub

Advanced pattern with:
- STA networking with NVS credentials
- Protobuf HTTP API
- Event bus architecture
- WebSocket event streaming
- Console-based WiFi configuration

---

## Quick Reference

### Essential Kconfig

```
# Enable WebSocket support in HTTP server
CONFIG_HTTPD_WS_SUPPORT=y

# WiFi mode
CONFIG_MY_WIFI_MODE_SOFTAP=y   # or STA or APSTA

# SoftAP settings
CONFIG_MY_WIFI_AP_SSID="MyDevice"
CONFIG_MY_WIFI_AP_PASSWORD=""  # Empty = open

# STA settings (if applicable)
CONFIG_MY_WIFI_STA_SSID="HomeNetwork"
CONFIG_MY_WIFI_STA_PASSWORD="secret"
```

### Key ESP-IDF APIs

| Function | Purpose |
|----------|---------|
| `esp_netif_create_default_wifi_ap()` | Create SoftAP interface |
| `esp_netif_create_default_wifi_sta()` | Create STA interface |
| `esp_wifi_set_mode()` | Set AP/STA/APSTA mode |
| `httpd_start()` | Start HTTP server |
| `httpd_register_uri_handler()` | Register route |
| `httpd_ws_send_data_async()` | Send WebSocket frame |
| `nvs_set_str()` / `nvs_get_str()` | Persist credentials |

### Default URLs

| Mode | Device IP | Access |
|------|-----------|--------|
| SoftAP | 192.168.4.1 | Connect to device's WiFi network |
| STA | DHCP-assigned | On same network as device |
