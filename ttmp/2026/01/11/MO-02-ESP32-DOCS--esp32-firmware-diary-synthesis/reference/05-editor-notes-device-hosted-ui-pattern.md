---
Title: 'Editor Notes: Device-Hosted UI Pattern'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: esp32-s3-m5/ttmp/2026/01/11/MO-02-ESP32-DOCS--esp32-firmware-diary-synthesis/playbook/12-device-hosted-ui-pattern-softap-http-server-and-websocket-streaming.md
      Note: The final guide document to review
ExternalSources: []
Summary: 'Editorial review checklist for the device-hosted UI pattern guide.'
LastUpdated: 2026-01-12T12:00:00-05:00
WhatFor: 'Help editors verify the guide is accurate, complete, and useful.'
WhenToUse: 'Use when reviewing the guide before publication.'
---

# Editor Notes: Device-Hosted UI Pattern

## Purpose

Editorial checklist for reviewing the device-hosted UI pattern guide before publication.

---

## Target Audience Check

The guide is written for developers who:
- Have ESP-IDF experience (building, flashing, menuconfig)
- Want to add a web UI to their ESP32-S3 device
- Need to understand networking, HTTP server, and WebSocket patterns

**Review questions:**
- [ ] Does the guide explain when to use SoftAP vs STA clearly enough for someone to make the right choice?
- [ ] Is the HTTP server setup explained step-by-step?
- [ ] Are WebSocket concepts accessible to developers new to real-time web?

---

## Structural Review

### Required Sections

- [ ] **Pattern overview** — What is device-hosted UI and why use it?
- [ ] **Networking modes** — SoftAP vs STA comparison with decision guidance
- [ ] **HTTP server setup** — `esp_http_server` initialization and routing
- [ ] **Asset embedding** — How to include HTML/CSS/JS in firmware
- [ ] **REST API patterns** — Endpoint design and handlers
- [ ] **WebSocket streaming** — Client tracking and broadcast
- [ ] **File uploads** — Chunked receive to FATFS
- [ ] **Credential management** — NVS storage and runtime configuration
- [ ] **Troubleshooting** — Common issues and solutions

### Flow and Readability

- [ ] Does the guide progress from simple (SoftAP + static page) to complex (STA + WebSocket + protobuf)?
- [ ] Are architecture diagrams clear and labeled?
- [ ] Is code properly explained, not just dumped?

---

## Accuracy Checks

### Claims to Verify Against Source Code

| Claim in Guide | Verify In | What to Check |
|----------------|-----------|---------------|
| Default SoftAP IP is 192.168.4.1 | ESP-IDF docs / wifi_softap.cpp | Logged IP matches |
| `httpd_uri_match_wildcard` enables /* patterns | http_server.cpp | `cfg.uri_match_fn` assignment |
| WebSocket frame type `HTTPD_WS_TYPE_BINARY` | http_server.cpp | Used in broadcast function |
| NVS keys are "ssid" and "pass" | wifi_sta.c | `nvs_set_str` calls |
| DHCP server starts automatically for AP | ESP-IDF docs | Verify this claim |
| Embedded assets use `asm()` labels | http_server.cpp | `_binary_*_start` symbols |

- [ ] All claims verified against source code

### Code Snippets

| Snippet | Check |
|---------|-------|
| Route registration pattern | Matches actual `http_server_start()` |
| WebSocket broadcast function | Matches `http_server_ws_broadcast_binary()` |
| SoftAP initialization | Matches `wifi_softap_start()` |
| NVS credential storage | Matches `save_credentials_to_nvs()` |

- [ ] Code snippets match source or are clearly labeled as simplified

---

## Completeness Checks

### Topics That Should Be Covered

| Topic | Covered? | Notes |
|-------|----------|-------|
| When to use SoftAP vs STA | | |
| SoftAP+STA hybrid mode | | |
| Default AP IP (192.168.4.1) | | |
| Fallback to SoftAP if STA fails | | |
| `esp_http_server` initialization | | |
| Route registration with wildcards | | |
| Embedded asset pattern (asm labels) | | |
| Content-Type headers | | |
| WebSocket upgrade handling | | |
| Client tracking for broadcast | | |
| Async send with cleanup callback | | |
| File upload chunking | | |
| FATFS mount and write | | |
| NVS credential persistence | | |
| Console-based WiFi config | | |
| Protobuf vs JSON tradeoffs | | |
| Event bus → WebSocket bridge | | |

- [ ] All essential topics covered adequately

### Potential Missing Information

- [ ] What happens if FATFS partition doesn't exist?
- [ ] Maximum file upload size and configuration
- [ ] WebSocket frame size limits
- [ ] How to debug connectivity issues
- [ ] Memory considerations for concurrent connections

---

## Diagrams to Verify

- [ ] **Data flow diagram** — Request → Handler → Storage/Response flow is accurate
- [ ] **Network architecture** — Client ↔ SoftAP/STA ↔ Device is clear
- [ ] **WebSocket client tracking** — Add/remove/broadcast flow makes sense
- [ ] **Event bridge** — Event bus → Queue → Task → WebSocket is accurate

---

## Tone and Style

- [ ] Consistent use of "you" (not "we" or "one")
- [ ] Active voice preferred
- [ ] Technical terms explained (SoftAP, STA, NVS, FATFS)
- [ ] Warnings use appropriate formatting

---

## Final Review Questions

1. **Could someone build a working device-hosted UI from this guide alone?**

2. **Is the SoftAP vs STA decision clearly explained with concrete recommendations?**

3. **Are the code snippets complete enough to adapt without reading the full source?**

4. **Does the troubleshooting section address the most likely problems?**

5. **Is the protobuf variant explained as an advanced option, not a requirement?**

---

## Editor Sign-Off

| Reviewer | Date | Status | Notes |
|----------|------|--------|-------|
| ________ | ____ | ______ | _____ |

---

## Suggested Improvements

_____________________________________________________________________

_____________________________________________________________________
