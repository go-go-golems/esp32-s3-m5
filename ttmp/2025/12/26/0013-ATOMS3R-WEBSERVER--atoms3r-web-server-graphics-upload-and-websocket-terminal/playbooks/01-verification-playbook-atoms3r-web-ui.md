---
Title: 'Verification playbook: AtomS3R Web UI (PNG upload + display render + WebSocket UART + button)'
Ticket: 0013-ATOMS3R-WEBSERVER
Status: active
Topics:
    - esp32s3
    - esp-idf
    - atoms3r
    - web
    - websocket
    - uart
    - fatfs
    - display
DocType: playbook
Intent: long-term
Owners: []
RelatedFiles:
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/build.sh
      Note: One-command build helper (sources ESP-IDF if needed)
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp
      Note: Asset serving + REST endpoints + /ws WebSocket
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/storage_fatfs.cpp
      Note: RW FATFS mount (/storage) and graphics directory
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/display_app.cpp
      Note: PNG-from-file render path
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/uart_terminal.cpp
      Note: UART RX task + WS bridge
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/main/button_input.cpp
      Note: Button debounce + WS event broadcast
    - Path: /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0017-atoms3r-web-ui/web/src/app.tsx
      Note: Browser UI (upload + terminal + button events)
ExternalSources: []
Summary: "End-to-end verification steps for the MVP: SoftAP + embedded web UI + PNG upload/render + WebSocket button events + WebSocket UART RX/TX + persistence across reboot."
LastUpdated: 2025-12-26
WhatFor: "Confirm that ticket 0013 acceptance criteria are met on real hardware."
WhenToUse: "After any significant change to HTTP/WS/storage/display/UART/button code, or before declaring the ticket done."
---

# Verification playbook: AtomS3R Web UI (PNG upload + display render + WebSocket UART + button)

## Success criteria (MVP)

- Device boots and starts **SoftAP**.
- Browser can load **web UI** from the device (`/`, plus `/assets/app.js` and `/assets/app.css`).
- Browser can **upload a PNG**; device stores it on FATFS and **renders it to the display**.
- Browser receives **button events** over **WebSocket**.
- Browser can do **UART RX/TX** via **WebSocket** (binary frames).
- Uploaded graphics **persist across reboot**.

## Prereqs

- AtomS3R connected via USB (for flashing + logs).
- A client device (laptop/phone) that can connect to WiFi SoftAP.
- For UART test (pick one):
  - **Loopback**: a way to short **GROVE UART TX ↔ RX** at the GROVE header.
  - **USB-UART adapter** wired to GROVE TX/RX/GND (3.3V).

## Config assumptions / defaults

Defaults come from `0017-atoms3r-web-ui/sdkconfig.defaults`:

- SoftAP SSID: `AtomS3R-WebUI`
- Upload size limit: 262144 bytes (256 KiB) via `CONFIG_TUTORIAL_0017_MAX_UPLOAD_BYTES`
- UART: `UART1`, `115200`, TX=GPIO1, RX=GPIO2
- Button GPIO: 41 (active-low)

If your board revision differs, adjust via `idf.py menuconfig`:

- `Tutorial 0017 → UART terminal`
- `Tutorial 0017 → Button input`

## Build + flash

From the project directory:

```bash
cd esp32-s3-m5/0017-atoms3r-web-ui

# Optional: rebuild frontend assets if you changed web code:
cd web
npm ci
npm run build
cd ..

# Build (auto-sources ESP-IDF if needed):
./build.sh build

# Flash + monitor (choose correct PORT):
./build.sh -p /dev/ttyACM0 flash monitor
```

## Step 1: Confirm SoftAP + base HTTP

1. In serial logs, confirm it prints SoftAP SSID and IP (typically `192.168.4.1`).
2. On your client device, connect to **`AtomS3R-WebUI`**.
3. In a browser, open:
   - `http://192.168.4.1/api/status`
   - Expect JSON with `"ok":true`.

## Step 2: Confirm UI loads from embedded assets

1. Open:
   - `http://192.168.4.1/`
2. Confirm the page renders and the UI shows:
   - “Device status” JSON
   - “WS: connected” (or connect button works)
3. (Optional) In browser devtools Network tab, confirm:
   - `GET /assets/app.js` returns 200 and `Content-Type: application/javascript`
   - `GET /assets/app.css` returns 200 and `Content-Type: text/css`

## Step 3: PNG upload → FATFS + display render

1. Prepare a small PNG (<256 KiB). Recommended: 128×128.
2. In the UI “Upload PNG”, select the file and click **Upload**.
3. Verify:
   - UI refreshes “Graphics” list and the uploaded filename appears.
   - The AtomS3R display updates (auto-render newest upload).
4. Confirm by direct API:
   - `GET http://192.168.4.1/api/graphics` includes the filename.
   - `GET http://192.168.4.1/api/graphics/<name>` downloads the file.

### Negative test: too-large upload

1. Try uploading a PNG > `CONFIG_TUTORIAL_0017_MAX_UPLOAD_BYTES`.
2. Expect:
   - HTTP error (likely 413)
   - File does **not** appear in `/api/graphics`

## Step 4: Button events over WebSocket

1. Press the AtomS3R button several times.
2. Verify in UI “Button events” list:
   - New entries appear (last 10).

If no events appear:

- Confirm `CONFIG_TUTORIAL_0017_BUTTON_GPIO` is correct for your revision.
- Confirm the button is not conflicting with any other pin usage.

## Step 5: UART RX/TX over WebSocket (binary frames)

### Option A: TX↔RX loopback at GROVE header (fastest)

1. Wire GROVE UART **TX pin to RX pin** (and ensure common GND).
2. In UI “Terminal (UART)”, type `hello` and press Enter.
3. Expect:
   - `hello` appears back in the terminal output (UART TX looped into UART RX).

If you see nothing:

- Your GROVE mapping may be swapped (GPIO1/GPIO2 ambiguity). Try swapping TX/RX in menuconfig, rebuild + reflash.

### Option B: USB-UART adapter

1. Wire:
   - Atom TX → adapter RX
   - Atom RX → adapter TX
   - Atom GND → adapter GND
2. Use any method to generate bytes into Atom RX (adapter TX), e.g. a serial terminal on the host.
3. Verify bytes appear in the UI terminal.
4. Type in the UI and verify bytes are visible on the host side.

## Step 6: Persistence across reboot

1. Upload a PNG and confirm it appears in “Graphics”.
2. Reset/power-cycle the device.
3. Reconnect to SoftAP and reload the UI.
4. Verify:
   - `GET /api/graphics` still lists the uploaded file
   - Download still works
   - (Optional) re-render by re-uploading or by a future “select” endpoint if added

## Exit criteria

All of these are true in one run:

- UI loads from the device
- Upload succeeds and renders
- Button events stream via WS
- UART round-trip works via WS (loopback or external)
- Uploaded file persists across reboot


