---
Title: 'Guide: Device-hosted UI pattern (SoftAP/STA + HTTP + WS)'
Ticket: MO-02-ESP32-DOCS
Status: active
Topics:
    - esp32
    - firmware
    - documentation
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles: []
ExternalSources: []
Summary: 'Expanded research and writing guide for device-hosted UI pattern docs.'
LastUpdated: 2026-01-11T19:44:01-05:00
WhatFor: 'Help a ghostwriter draft the device-hosted UI pattern doc.'
WhenToUse: 'Use before writing the SoftAP/STA + HTTP + WS guide.'
---

# Guide: Device-hosted UI pattern (SoftAP/STA + HTTP + WS)

## Purpose

This guide enables a ghostwriter to produce a focused doc about a recurring pattern: ESP32-S3 firmware that hosts a web UI. The doc should cover networking mode selection (SoftAP vs STA), HTTP server setup, WebSocket streaming, and file uploads to storage.

## Assignment Brief

Write a developer guide that explains the device-hosted UI architecture as a reusable pattern. Treat the endpoints as the central contract. Emphasize how the device is both a server and a stateful UI target, and how WebSocket streams provide live telemetry or terminal traffic.

## Environment Assumptions

- ESP-IDF 5.4.x
- AtomS3R or Cardputer hardware
- WiFi STA credentials for testing (optional)

## Source Material to Review

- `esp32-s3-m5/0017-atoms3r-web-ui/README.md`
- `esp32-s3-m5/0021-atoms3-memo-website/README.md`
- `esp32-s3-m5/0029-mock-zigbee-http-hub/README.md`
- `esp32-s3-m5/ttmp/2025/12/26/0013-ATOMS3R-WEBSERVER--atoms3r-web-server-graphics-upload-and-websocket-terminal/reference/01-diary.md`
- `esp32-s3-m5/ttmp/2026/01/05/0029-HTTP-EVENT-MOCK-ZIGBEE--mock-zigbee-hub-http-api-esp-event-bus-virtual-devices/reference/01-diary.md`

## Technical Content Checklist

- SoftAP vs STA startup logic (when to use each)
- `esp_http_server` initialization and routing
- WebSocket endpoint behavior and framing
- Upload flow: POST/PUT -> FATFS write -> device render
- UI boot page endpoint (`GET /`)
- Note if protobuf is used for HTTP payloads (0029)

## Pseudocode Sketch

Use a lightweight request flow to clarify the architecture.

```c
// Pseudocode: server setup
if config.use_sta:
  wifi_sta_connect()
else:
  wifi_softap_start()

http_server.start()
http_server.route("/", serve_ui)
http_server.route("/upload", handle_upload)
http_server.ws("/events", ws_handler)
```

## Pitfalls to Call Out

- Credentials stored in NVS can be stale; include reset steps.
- WebSocket endpoints may require explicit binary framing.
- Large uploads can block or fragment without chunking.

## Suggested Outline

1. Pattern overview (SoftAP/STA + HTTP + WS)
2. Networking setup paths (SoftAP default, STA optional)
3. HTTP endpoints and assets
4. WebSocket event stream pattern
5. Upload path and storage layout
6. Troubleshooting checklist

## Commands

```bash
# AtomS3R web UI demo
cd esp32-s3-m5/0017-atoms3r-web-ui
idf.py set-target esp32s3
idf.py build flash monitor

# Mock Zigbee hub (protos + WS events)
cd ../0029-mock-zigbee-http-hub
idf.py build flash monitor
```

## Exit Criteria

- Doc includes a concise endpoint list and data formats.
- Doc describes SoftAP vs STA flow and how to set credentials.
- Doc includes an upload flow diagram or step list.

## Notes

Keep the scope on the UI pattern; avoid Zigbee domain details beyond what is needed to illustrate HTTP/WS flows.
