# Cardputer ADV 0066 — LED chain GFX simulator (console-driven)

MVP goal: render a **virtual 50-LED WS281x chain** on the Cardputer display and drive patterns via `esp_console` over **USB Serial/JTAG** (no network protocol in the MVP).

## Console

The REPL prompt is `sim>`.

Try:

- `help`
- `sim help`
- `sim status`

## Build / Flash

```bash
cd /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0066-cardputer-adv-ledchain-gfx-sim
idf.py build flash monitor
```

