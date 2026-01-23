# Cardputer ADV 0066 — LED chain GFX simulator (console-driven)

MVP goal: render a **virtual 50-LED WS281x chain** on the Cardputer display and drive patterns via `esp_console` over **USB Serial/JTAG** (no network protocol in the MVP).

## Console

The REPL prompt is `sim>`.

Try:

- `help`
- `sim help`
- `sim status`
- `js help`
- `js eval sim.status()`
- `js eval sim.setBrightness(100)`
- `js eval sim.setPattern("rainbow")`
- `js eval sim.setRainbow(5,100,10)`

Enter the JS REPL (nested prompt):

- `js repl` (type `.exit` to return)

## Wi-Fi + Web

If you connect Wi‑Fi from the console, an HTTP server starts after STA gets an IP.

- `wifi scan`
- `wifi join <index> <password> --save`
- `wifi status` (prints the IP)

Endpoints:

- `GET /api/status`
- `POST /api/apply`

## Build / Flash

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim
idf.py set-target esp32s3
idf.py build flash monitor
```
