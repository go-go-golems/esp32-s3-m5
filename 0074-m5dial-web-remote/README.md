# 0074 M5Dial Web Remote

This project has three parts:

- `firmware/`: ESP-IDF firmware for the M5Dial. It uses `esp_console` on USB Serial/JTAG, `wifi_mgr`, and an outbound WebSocket client.
- `server/`: Go HTTP/WebSocket server. Devices connect on `/ws/device`, browsers on `/ws/browser`, and `/api/status` exposes the current snapshot.
- `web/`: React/Vite dashboard. Production assets are copied into `server/static/` so the Go server serves the built UI.

Operational runbook:

- `PLAYBOOK.md`: tmux-based workflow for server, Vite, firmware build/flash, monitor, and restart flows.

## Firmware

Build:

```bash
cd 0074-m5dial-web-remote/firmware
. $HOME/esp/esp-idf-5.4.1/export.sh
idf.py set-target esp32s3
idf.py build
```

Flash:

```bash
idf.py -p /dev/ttyACM0 flash
```

On the console:

- `wifi scan`
- `wifi join <index> <password> --save`
- `remote set-url ws://HOST:8080/ws/device`
- `remote status`
- `remote connect`

## Server

Run:

```bash
cd 0074-m5dial-web-remote/server
go run . -listen 127.0.0.1:8080
```

## Web

Develop:

```bash
cd 0074-m5dial-web-remote/web
npm install
npm run dev
```

Rebuild the embedded server bundle:

```bash
cd 0074-m5dial-web-remote/web
npm run build
rm -rf ../server/static/*
cp -R dist/. ../server/static/
```
