---
Title: Camera Streaming Protocol and Console Requirements
Ticket: MO-001-ATOMS3R-WEBSOCKET-STREAMING
Status: active
Topics:
    - firmware
    - esp32
    - camera
    - wifi
    - analysis
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0017-atoms3r-web-ui/sdkconfig.defaults
      Note: USB Serial/JTAG console baseline
    - Path: 0029-mock-zigbee-http-hub/main/wifi_console.c
      Note: esp_console WiFi CLI pattern
    - Path: 0029-mock-zigbee-http-hub/main/wifi_sta.c
      Note: STA + NVS credential persistence pattern
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/ARCHITECTURE.md
      Note: Legacy Python port references (mismatch risk)
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/README-GO.md
      Note: Endpoint and port summary for Go server
    - Path: 0040-atoms3r-cam-streaming/esp32-camera-stream/server.go
      Note: Defines the WebSocket protocol and FFmpeg pipeline
    - Path: docs/01-websocket-over-wi-fi-esp-idf-playbook.md
      Note: ESP-IDF WebSocket and console best practices
ExternalSources: []
Summary: Protocol analysis of server.go and requirements for real ESP32 camera + esp_console WiFi/target config.
LastUpdated: 2026-01-14T09:16:33.309419274-05:00
WhatFor: Define the exact wire protocol and console configuration needed for a real ATOMS3R camera client.
WhenToUse: When implementing the ESP32 camera client and console-based configuration.
---


# Camera Streaming Protocol and Console Requirements

This analysis focuses on `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go` and the required device-side protocol. It also pulls console/WiFi configuration patterns from existing ticket documentation (notably tutorial `0029`) and repo playbooks to define how the ATOMS3R firmware should expose `esp_console` for setting WiFi credentials and the target streaming server address.

## 1. Scope

Primary sources:

- `0040-atoms3r-cam-streaming/esp32-camera-stream/server.go` (Go server, definitive protocol)
- `0040-atoms3r-cam-streaming/esp32-camera-stream/README-GO.md` (endpoint summary)
- `0040-atoms3r-cam-streaming/esp32-camera-stream/ARCHITECTURE.md` (legacy Python server overview)
- `docs/01-websocket-over-wi-fi-esp-idf-playbook.md` (ESP-IDF WebSocket patterns + console reminders)
- `0029-mock-zigbee-http-hub/main/wifi_console.c` + `wifi_sta.c` (esp_console WiFi CLI with NVS persistence)
- `0029-mock-zigbee-http-hub/README.md` (runtime console instructions)
- `0017-atoms3r-web-ui/sdkconfig.defaults` (USB Serial/JTAG console baseline)

## 2. Server.go behavior (authoritative protocol)

### 2.1 Endpoints

`server.go` serves HTTP and WebSocket on port 8080:

- HTTP:
  - `GET /` -> `client.html` (H.264 MSE viewer)
  - `GET /mjpeg` -> `client_mjpeg.html` (MJPEG viewer)
  - `GET /status` -> JSON with connection counts
- WebSocket:
  - `ws://HOST:8080/ws/camera` -> camera input
  - `ws://HOST:8080/ws/viewer` -> viewer output

Important: this differs from the Python server README (port 8765/8766). The device should follow `server.go` and target port 8080.

### 2.2 WebSocket camera protocol (what the ESP32 must send)

The camera connection is handled by `handleCameraWebSocket`:

- Accepts WebSocket upgrade.
- Each incoming binary WebSocket message is treated as one JPEG frame.
- The server writes the raw bytes directly into FFmpeg stdin.
- FFmpeg is configured as:

```text
ffmpeg -f image2pipe -c:v mjpeg -i - \
       -c:v libx264 -preset ultrafast -tune zerolatency \
       -pix_fmt yuv420p -r 30 -g 60 -f mpegts -
```

Implication for device protocol:

- Each WebSocket binary message must be a complete JPEG byte stream (SOI to EOI).
- Do not prepend custom headers, sizes, or metadata.
- Do not send multiple JPEG frames in one WS message.
- Do not split a JPEG across multiple WS messages.

If the stream includes non-JPEG bytes, the FFmpeg image2pipe demuxer will likely fail or desync.

### 2.3 Viewer protocol (server-to-client)

Viewer connections to `/ws/viewer` receive a stream of H.264 data as raw binary WebSocket messages:

- FFmpeg outputs MPEG-TS (`-f mpegts -`).
- The server writes each chunk read from stdout directly to viewers.
- Viewer clients must interpret the byte stream as MPEG-TS with H.264.

The server also buffers the last 2 MB of H.264 output so new viewers can start quickly.

Implications:

- Viewer clients might connect mid-GOP; they must wait until the next keyframe.
- GOP size is 60 at 30 fps (approx 2 seconds).
- Buffering is best-effort and not keyframe-aligned.

### 2.4 Connection lifecycle

Server behavior:

- FFmpeg starts on first camera connect.
- FFmpeg stops when no camera clients remain.
- Multiple camera connections are not coordinated; interleaved frames would corrupt the stream.

Device requirement: only one camera should stream at a time.

## 3. Real camera device requirements (ATOMS3R firmware)

This server protocol expects a real JPEG camera source, not a synthetic mock.

### 3.1 Camera capture pipeline

The ESP32 firmware should capture JPEG frames using `esp_camera`:

- Configure the sensor for `PIXFORMAT_JPEG` if supported.
- If the sensor delivers RGB, convert with `frame2jpg()` (quality tradeoff).
- Send each JPEG frame as a single WebSocket binary message.

Pseudo-flow:

```text
connect websocket ws://HOST:8080/ws/camera
loop:
  fb = esp_camera_fb_get()
  if fb->format == JPEG:
    jpeg = fb->buf, len=fb->len
  else:
    jpeg = frame2jpg(fb, quality)
  ws_send_binary(jpeg, len)
  esp_camera_fb_return(fb)
```

### 3.2 Throughput and sizing

Constraints to keep in mind:

- WiFi STA throughput on ATOMS3R is limited; larger frames can spike latency.
- The Go server expects 30 fps; the device can send fewer frames without issue.
- QVGA/VGA at quality 10-15 is typically sustainable; FHD will likely drop frames.

### 3.3 Error handling

Device should:

- Reconnect if WebSocket closes.
- Drop frames if the send queue is full (do not block camera capture indefinitely).
- Log send errors for diagnosis.

## 4. esp_console requirements (WiFi + target host)

The camera firmware needs runtime configuration for:

- WiFi SSID/password (STA mode)
- Streaming server host/IP + port (dest endpoint)

The repo already includes a proven console pattern in `0029-mock-zigbee-http-hub`:

- `wifi` command with `status`, `scan`, `set`, `connect`, `disconnect`, `clear`
- NVS-backed credentials (`wifi` namespace, `ssid` and `pass` keys)
- `esp_console` REPL with USB Serial/JTAG backend preferred

### 4.1 Console backend (ESP32-S3 default)

The project should align with the repo-wide guidance (see `AGENTS.md` and `docs/01-websocket-over-wi-fi-esp-idf-playbook.md`):

```
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
# CONFIG_ESP_CONSOLE_UART is not set
```

If UART is used, document which pins are reserved; otherwise prefer USB Serial/JTAG so peripheral UARTs stay free.

### 4.2 Console command set (recommended)

Use the `0029` pattern as a base and add stream-target configuration:

```
cam> wifi status
cam> wifi scan [max]
cam> wifi set <ssid> <password> [save]
cam> wifi connect
cam> wifi disconnect
cam> wifi clear

cam> stream status
cam> stream set host <ip-or-hostname> [port]
cam> stream set port <port>
cam> stream start
cam> stream stop
```

Persistence: store WiFi creds + stream target in NVS. Suggested NVS layout:

- namespace: `wifi` -> keys: `ssid`, `pass`
- namespace: `stream` -> keys: `host`, `port`

### 4.3 Source patterns to reuse

From `0029-mock-zigbee-http-hub`:

- `wifi_console.c` (command parsing + REPL start)
- `wifi_sta.c` (STA wrapper, NVS persistence, scan)

From `docs/01-websocket-over-wi-fi-esp-idf-playbook.md`:

- WebSocket client/server gotchas
- Advice to keep heavy work out of handlers
- Explicit reminders about USB Serial/JTAG console on ESP32-S3

## 5. Protocol summary (device-side)

Wire protocol required by `server.go`:

- Transport: WebSocket over TCP
- Endpoint: `ws://<server-ip>:8080/ws/camera`
- Frame type: WebSocket binary
- Payload: exactly one JPEG file per message (raw bytes)
- Ordering: sequential frames; no interleaving from multiple cameras

Viewer path (for reference):

- `ws://<server-ip>:8080/ws/viewer` delivers H.264 in MPEG-TS chunks

## 6. Key risks and gotchas

- Port mismatch: Python docs mention 8765/8766, but Go server uses 8080.
- Binary framing: sending partial JPEGs or metadata will break FFmpeg.
- Multi-camera: the server does not serialize multiple camera streams; use a single active camera.
- Console backend: if UART console is selected, USB Serial/JTAG REPL will not appear; ensure `sdkconfig.defaults` or explicit documentation.
- NVS persistence: do not require recompilation for WiFi/host changes; use console and NVS.
