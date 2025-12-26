# Tutorial 0017 — AtomS3R Web UI (graphics upload + WebSocket terminal)

This tutorial builds an **ESP-IDF-first** AtomS3R firmware that:
- starts a **WiFi SoftAP**
- serves a **device-hosted web UI**
- accepts **PNG uploads** to FATFS and renders them on the display
- exposes a **WebSocket terminal** (UART RX/TX)
- streams **button events** to the browser

## Build

From this directory:

```bash
idf.py build
```

## Flash

```bash
idf.py flash monitor
```


