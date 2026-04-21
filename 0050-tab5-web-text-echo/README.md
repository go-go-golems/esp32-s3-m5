# Tab5 web text echo

A minimal Tab5-hosted web server that accepts text from a browser, echoes the current text back in the UI, and lets you configure persistent Wi-Fi from the USB serial console.

## Build

```bash
./build.sh
```

## Flash + monitor

```bash
./build.sh flash monitor
```

## Console Wi-Fi setup

The firmware starts an `esp_console` REPL on the USB serial/JTAG port. Useful commands:

```bash
wifi status
wifi scan
wifi set "YourSSID" "YourPassword" save
wifi connect
wifi clear
```

The saved SSID/password are stored in NVS, so the board will try to rejoin your network on the next boot.

## What it does

- Starts a Wi-Fi SoftAP named `Tab5-Text-Echo` for recovery/setup
- Tries to join a saved Wi-Fi network as STA when credentials exist
- Serves a tiny browser page at `http://192.168.4.1/` on the AP side
- Also serves the web UI from the STA IP once the board joins your network
- Accepts text via `POST /api/text`
- Returns the current text via `GET /api/state`

## Notes

- The firmware keeps the text in RAM only.
- Rebooting the board clears the current text, but Wi-Fi credentials persist in NVS.
- The implementation is intentionally small so it can be used as a teaching example.
