# ESP32-C6 (XIAO) 0046 — LED patterns control web UI (MO-032/0044) + esp_console Wi-Fi

This firmware combines:

- the **WS281x pattern engine + console semantics** from `0044-xiao-esp32c6-ws281x-patterns-console/`, and
- the **device-hosted embedded web UI** pattern from `0045-xiao-esp32c6-preact-web/`.

Target: **ESP32-C6** (default: **Seeed XIAO ESP32C6**).

- Starts an interactive **`esp_console` REPL over USB Serial/JTAG** (Wi‑Fi STA selection via console).
- Starts **`esp_http_server`** and serves an embedded web UI bundle:
  - `/` (index.html)
  - `/assets/app.js`
  - `/assets/app.css`
  - `/assets/app.js.map` (sourcemap, served gzipped; optional for debugging)

## Build web assets (Vite → embedded assets)

The Vite build outputs directly into `main/assets/` (deterministic filenames).

```bash
cd 0046-xiao-esp32c6-led-patterns-webui/web
npm ci
npm run build
```

Or with the helper:

```bash
cd 0046-xiao-esp32c6-led-patterns-webui
./build.sh web
```

## Build firmware

```bash
source ~/esp/esp-idf-5.4.1/export.sh
cd 0046-xiao-esp32c6-led-patterns-webui

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
