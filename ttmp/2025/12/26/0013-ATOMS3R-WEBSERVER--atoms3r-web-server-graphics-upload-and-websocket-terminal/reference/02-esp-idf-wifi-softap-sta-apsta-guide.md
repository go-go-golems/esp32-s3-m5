---
Title: 'ESP-IDF WiFi Guide: SoftAP vs STA(DHCP) vs AP+STA (with a worked example from Tutorial 0017)'
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - wifi
    - softap
    - sta
    - dhcp
    - esp_netif
    - esp_event
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/wifi_app.cpp
      Note: Worked example (mode selection + bring-up)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: /api/status reports actual AP/STA IPs
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/Kconfig.projbuild
      Note: Menuconfig options for WiFi mode and credentials
ExternalSources: []
Summary: "A senior-embedded-friendly guide to ESP-IDF WiFi fundamentals (SoftAP, STA DHCP, AP+STA), including the key ESP-IDF APIs, lifecycle/events, failure modes, and how Tutorial 0017 implements mode selection."
LastUpdated: 2025-12-26
WhatFor: "Enable a new-to-ESP32 engineer to confidently configure, debug, and extend WiFi on ESP-IDF projects."
WhenToUse: "When adding WiFi to an ESP-IDF firmware, switching between SoftAP and STA, or debugging connectivity/IP problems."
---

# ESP-IDF WiFi Guide: SoftAP vs STA(DHCP) vs AP+STA (with a worked example from Tutorial 0017)

## Executive summary

If you already know “embedded networking” conceptually but you’re new to ESP-IDF, the main adjustment is that ESP32 WiFi is not “just a driver + socket calls.” It’s a coordinated system involving:

- the **WiFi driver** (`esp_wifi`) that manages the radio and association/authentication,
- the **network interface layer** (`esp_netif`) that binds WiFi to lwIP (TCP/IP stack) and runs DHCP (client or server),
- the **event loop** (`esp_event`) that delivers state transitions like “connected” and “got IP,” and
- your application logic (HTTP server, WebSocket, etc.) that should treat “WiFi is usable” as a state, not a boolean.

This guide explains the three common modes—**SoftAP**, **STA (DHCP)**, and **AP+STA**—and then maps them directly to the concrete code used in tutorial `esp32-s3-m5/0017-atoms3r-web-ui`.

## Mental model (what exists at runtime)

Before diving into code, it helps to name the “layers” you’ll see in ESP-IDF:

- **Radio role (WiFi mode)**: what the ESP32 is doing on the air:
  - `WIFI_MODE_AP`: act as an access point (SoftAP)
  - `WIFI_MODE_STA`: act as a station (client) that joins an AP
  - `WIFI_MODE_APSTA`: do both concurrently

- **Network interface (`esp_netif_t`)**: the “IP side” of each role:
  - an **AP netif** has a **DHCP server** and typically a fixed IP (common default: `192.168.4.1`)
  - a **STA netif** usually runs a **DHCP client** and obtains an IP lease from the upstream network

- **Event-driven lifecycle**:
  - WiFi association/auth events come from **`WIFI_EVENT`**
  - IP acquisition events come from **`IP_EVENT`** (e.g. `IP_EVENT_STA_GOT_IP`)

The key practical implication is: **“connected to WiFi” and “has an IP” are different milestones**. Most application protocols (HTTP/WebSocket) only become reachable once you have an IP on at least one netif.

## SoftAP (device-hosted network)

SoftAP is the most “self-contained” mode. The device creates a WiFi network and your phone/laptop connects to it.

### What it buys you

- **Predictable access**: you don’t need existing infrastructure.
- **Deterministic IP** (usually): the AP-side IP is commonly `192.168.4.1` and the DHCP server hands out leases to clients.
- Great for **field setup**, **recovery**, and **first-time provisioning**.

### What you give up

- Your device is not automatically on the same LAN as your development machine unless you connect to the SoftAP.
- Many clients have no internet while connected (unless you implement forwarding/NAT, which is non-trivial and not in scope here).

### Core ESP-IDF ingredients

- Create AP netif: `esp_netif_create_default_wifi_ap()`
- Set WiFi mode: `esp_wifi_set_mode(WIFI_MODE_AP)`
- Configure AP parameters: `esp_wifi_set_config(WIFI_IF_AP, &cfg)`
- Start WiFi: `esp_wifi_start()`
- Read IP: `esp_netif_get_ip_info(ap_netif, &ip)`

## STA (join existing WiFi) with DHCP

STA mode is what you want when the device should appear on an existing LAN and get an IP via DHCP.

### What it buys you

- Device becomes reachable from other hosts on the LAN (depending on network policy).
- Works naturally for “dev workflow”: browser on laptop accesses device on the same WiFi.

### What you give up (or must handle)

- Dependency on external infrastructure and credentials.
- More failure modes:
  - bad password
  - AP out of range
  - DHCP server unavailable
  - network isolation (client isolation / firewall) preventing access from your host

### Core ESP-IDF ingredients

- Create STA netif: `esp_netif_create_default_wifi_sta()`
- (Optional) set hostname: `esp_netif_set_hostname(sta_netif, "...")`
- Register event handlers:
  - `WIFI_EVENT_STA_START` → call `esp_wifi_connect()`
  - `WIFI_EVENT_STA_DISCONNECTED` → decide reconnect policy
  - `IP_EVENT_STA_GOT_IP` → now the device has a usable IP
- Configure STA credentials: `esp_wifi_set_config(WIFI_IF_STA, &cfg)`
- Start WiFi: `esp_wifi_start()`

### A common pattern: “wait for IP”

Your app usually needs a synchronization point: “don’t print the URL until the device has an IP.” A simple pattern is:

- create an `EventGroup`
- set a bit in the `IP_EVENT_STA_GOT_IP` handler
- block with a timeout waiting for that bit

That’s exactly what Tutorial 0017 implements.

## AP+STA (dual-mode)

AP+STA is a pragmatic “best of both worlds” mode:

- You keep the **SoftAP** around for discovery and recovery.
- You also join an upstream network as **STA** for normal operation.

In practice, AP+STA helps a lot during development because you can still reach the device at `192.168.4.1` even if the STA credentials are wrong or DHCP fails.

The trade-off is increased complexity (two interfaces, and the radio must service both roles). For many small web-UIs, this is fine.

## ESP-IDF API walk-through (in the order you typically call things)

This section is intentionally “didactic”: it explains not just what to call, but why the order matters.

### 1) Persistent storage (NVS)

ESP-IDF WiFi uses NVS (Non-Volatile Storage) for calibration data and other internals. If NVS isn’t initialized, WiFi init will fail.

- API:
  - `nvs_flash_init()`
  - sometimes you need to erase/retry if you changed NVS partition layout:
    - `nvs_flash_erase()` then `nvs_flash_init()`

### 2) Network stack scaffolding (esp_netif + event loop)

You need:

- `esp_netif_init()` to set up the TCP/IP stack glue
- `esp_event_loop_create_default()` so WiFi/IP events can be delivered

Both functions may return `ESP_ERR_INVALID_STATE` if they were already called; robust code treats that as fine.

### 3) Create netifs (AP and/or STA)

Creating the default WiFi netifs does two useful things:

- it creates an `esp_netif_t` instance
- it attaches it to lwIP with sane defaults (including DHCP client/server behavior)

AP netif:

- `esp_netif_create_default_wifi_ap()`

STA netif:

- `esp_netif_create_default_wifi_sta()`

### 4) Initialize WiFi driver (`esp_wifi_init`)

This initializes the WiFi driver. Typical code uses:

- `wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();`
- `esp_wifi_init(&cfg)`

### 5) Register event handlers (so you can react to state changes)

At minimum, STA needs:

- `WIFI_EVENT_STA_START`: trigger `esp_wifi_connect()`
- `WIFI_EVENT_STA_DISCONNECTED`: decide reconnect policy
- `IP_EVENT_STA_GOT_IP`: declare “network usable”

AP mode doesn’t require much event handling for basic usage, but it can still be useful for observing client connections.

### 6) Configure mode + credentials

- choose a mode: `esp_wifi_set_mode(WIFI_MODE_AP | WIFI_MODE_STA | WIFI_MODE_APSTA)`
- set configs:
  - `esp_wifi_set_config(WIFI_IF_AP, &ap_cfg)`
  - `esp_wifi_set_config(WIFI_IF_STA, &sta_cfg)`

### 7) Start WiFi (`esp_wifi_start`) and (for STA) connect

`esp_wifi_start()` boots the driver; then:

- in STA mode you call `esp_wifi_connect()` (often from the STA_START event)
- you wait for `IP_EVENT_STA_GOT_IP` to know DHCP succeeded

### 8) Read back IP for logging or API responses

Use:

- `esp_netif_get_ip_info(netif, &ip_info)`

This is the correct way to populate status pages like `/api/status` without hardcoding IP assumptions.

## Worked example: Tutorial 0017’s `wifi_app_start()`

Tutorial 0017 implements the above patterns in a single module: `main/wifi_app.cpp`.

### Menuconfig surface area (what you configure)

The WiFi options live under `Tutorial 0017: WiFi` and include:

- **Mode choice**:
  - `CONFIG_TUTORIAL_0017_WIFI_MODE_SOFTAP`
  - `CONFIG_TUTORIAL_0017_WIFI_MODE_STA`
  - `CONFIG_TUTORIAL_0017_WIFI_MODE_APSTA`
- **SoftAP params**:
  - `CONFIG_TUTORIAL_0017_WIFI_SOFTAP_SSID`
  - `CONFIG_TUTORIAL_0017_WIFI_SOFTAP_PASSWORD`
  - `CONFIG_TUTORIAL_0017_WIFI_SOFTAP_CHANNEL`
  - `CONFIG_TUTORIAL_0017_WIFI_SOFTAP_MAX_CONN`
- **STA params** (only meaningful in STA/AP+STA):
  - `CONFIG_TUTORIAL_0017_WIFI_STA_SSID`
  - `CONFIG_TUTORIAL_0017_WIFI_STA_PASSWORD`
  - `CONFIG_TUTORIAL_0017_WIFI_STA_HOSTNAME`
  - `CONFIG_TUTORIAL_0017_WIFI_STA_CONNECT_TIMEOUT_MS`
  - `CONFIG_TUTORIAL_0017_WIFI_STA_FALLBACK_TO_SOFTAP` (STA-only)

### High-level flow (pseudocode)

This pseudocode mirrors the control flow in `wifi_app_start()`:

```text
wifi_app_start():
  init_nvs()
  esp_netif_init()
  esp_event_loop_create_default()

  create_event_group()

  esp_wifi_init()
  register(WIFI_EVENT, any, wifi_event_handler)
  register(IP_EVENT, STA_GOT_IP, wifi_event_handler)

  if mode == SOFTAP:
    ap_netif = create_default_wifi_ap()
    esp_wifi_set_mode(AP)
    esp_wifi_set_config(AP, ap_cfg)
    esp_wifi_start()
    log("SoftAP IP ...")

  if mode == STA:
    sta_netif = create_default_wifi_sta()
    esp_wifi_set_mode(STA)
    esp_wifi_set_config(STA, sta_cfg)
    esp_wifi_start()
    wait_for(IP_EVENT_STA_GOT_IP, timeout)
    if timeout and fallback_enabled:
      create_default_wifi_ap()
      esp_wifi_set_mode(APSTA)
      esp_wifi_set_config(AP, ap_cfg)
      log("SoftAP IP ...")

  if mode == APSTA:
    ap_netif = create_default_wifi_ap()
    sta_netif = create_default_wifi_sta()
    esp_wifi_set_mode(APSTA)
    esp_wifi_set_config(AP, ap_cfg)
    esp_wifi_set_config(STA, sta_cfg)
    esp_wifi_start()
    log("SoftAP IP ...")
    wait_for(IP_EVENT_STA_GOT_IP, timeout)  # best effort
```

### Event handling (what happens asynchronously)

The event handler implements a minimal (but effective) STA policy:

- On `WIFI_EVENT_STA_START`:
  - log “connecting”
  - call `esp_wifi_connect()`
- On `WIFI_EVENT_STA_DISCONNECTED`:
  - log and immediately retry (`esp_wifi_connect()`)
  - note: this is intentionally simple; production code usually adds backoff
- On `IP_EVENT_STA_GOT_IP`:
  - set an event-group bit so `wifi_app_start()` can unblock
  - log the obtained IP

### How the rest of the system finds the IP

Tutorial 0017 exposes the created netifs:

- `wifi_app_get_ap_netif()`
- `wifi_app_get_sta_netif()`

`/api/status` uses these to call `esp_netif_get_ip_info()` and return a small JSON payload containing:

- the current mode (`softap|sta|apsta`)
- `ap_ip` and `sta_ip` strings (or `0.0.0.0` if not present)

This is a useful diagnostic pattern because it lets a browser UI discover which interface it’s currently talking to.

## Debugging and “real world” gotchas

### When STA connects but you can’t reach the device

This is a very common situation for new ESP32 developers:

- STA associates fine, DHCP gives an IP, but your laptop can’t reach it.

Common causes:

- **Client isolation** on the WiFi network blocks peer-to-peer traffic.
- **Firewall policy** on the AP/router blocks inbound connections.
- Your dev host is on a different VLAN/subnet than the STA lease.

Practical tips:

- Use serial logs to confirm the IP and subnet.
- Try ping from a host on the same network.
- If AP+STA is enabled, verify the SoftAP path still works as a “known good” access method.

### DHCP is slow or fails

DHCP failure can come from:

- wrong SSID/password (you’ll typically see repeated disconnects)
- weak signal causing retries
- upstream network policy rejecting new clients

Tutorial 0017 has a pragmatic STA-only option:

- `CONFIG_TUTORIAL_0017_WIFI_STA_FALLBACK_TO_SOFTAP=y`

This is a good development UX feature: you can still connect to `192.168.4.1` even when STA is misconfigured.

### Security notes

- SoftAP is configured as:
  - **open** if password is empty
  - **WPA2-PSK** otherwise
- STA config sets:
  - minimum auth threshold to WPA2 (`WIFI_AUTH_WPA2_PSK`)
  - PMF as “capable but not required”

For production, you’ll likely want to revisit:

- credential provisioning (store credentials in NVS and provide a UI to update them)
- stronger auth requirements if your network supports it

## Resources (official docs to read next)

- ESP-IDF WiFi API reference: [`esp_wifi`](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/network/esp_wifi.html)
- ESP-IDF network interface layer: [`esp_netif`](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/network/esp_netif.html)
- ESP-IDF event loop: [`esp_event`](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/system/esp_event.html)


