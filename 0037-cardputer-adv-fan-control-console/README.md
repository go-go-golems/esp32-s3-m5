# Tutorial 0037 — Cardputer-ADV Fan Control Console (GPIO3 relay) (ESP-IDF 5.4.x)

This firmware uses an interactive `esp_console` REPL over **USB Serial/JTAG** to control a relay-driven fan on **GPIO3**.

## Pins

- Fan relay: `GPIO3` (active-low by default; verify your wiring)

## Console

Console backend is **USB Serial/JTAG** (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`).

## Commands

- `help`
- `fan` (prints help)
- `fan help`
- `fan status`
- `fan on` / `fan off` / `fan toggle`
- `fan stop` (stop animations and restore last manual state)
- `fan blink [on_ms] [off_ms]` (default `500 500`)
- `fan strobe [ms]` (fast blink; default `100`)
- `fan tick [on_ms] [period_ms]` (default `50 1000`)
- `fan beat` (heartbeat pattern: `100-100-100-700`)
- `fan burst [count] [on_ms] [off_ms] [pause_ms]` (default `3 75 75 1000`)

## Build / Flash / Monitor

If you already have ESP-IDF sourced:

```bash
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Recommended tmux workflow:

```bash
./build.sh tmux-flash-monitor
```
