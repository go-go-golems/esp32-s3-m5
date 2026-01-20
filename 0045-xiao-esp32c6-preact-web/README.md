# ESP32-C6 (XIAO) 0045 — esp_console Wi-Fi selection + embedded Preact/Zustand web UI

This firmware is a small “device-hosted UI” demo for **ESP32-C6** (default: **Seeed XIAO ESP32C6**):

- starts an interactive **`esp_console` REPL over USB Serial/JTAG**
  - scan networks
  - join a network
  - persist credentials in NVS
- once connected, starts **`esp_http_server`** and serves an embedded web UI bundle:
  - `/` (index.html)
  - `/assets/app.js`
  - `/assets/app.css`

## Build web assets (Vite → embedded assets)

The Vite build outputs directly into `main/assets/` (deterministic filenames).

```bash
cd 0045-xiao-esp32c6-preact-web/web
npm ci
npm run build
```

Or with the helper:

```bash
cd 0045-xiao-esp32c6-preact-web
./build.sh web
```

## Build firmware

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd 0045-xiao-esp32c6-preact-web

idf.py set-target esp32c6
idf.py build
```

Or with the helper:

```bash
./build.sh all
```

## Flash + Monitor

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Use the console to connect Wi‑Fi

At the `c6>` prompt:

```text
wifi scan
wifi join 0 mypassword --save
wifi status
```

Then browse to the printed URL: `http://<sta-ip>/`.
