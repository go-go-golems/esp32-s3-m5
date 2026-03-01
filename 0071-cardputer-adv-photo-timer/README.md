# Tutorial 0071 - Cardputer-ADV Photo Development Timer

Firmware for Cardputer-ADV that runs film-development timers with encoder navigation,
boot-loaded preset JSON config, and a small device-hosted web UI for preset upload.

## Features

- Chain Encoder (U207 over Grove UART) navigation
- LVGL on Cardputer-ADV display
- Step-based timer engine (start/pause/resume/next/reset)
- SPIFFS-backed preset config (`/spiffs/presets.json`)
- Wi-Fi + HTTP API + embedded minimal web page for preset upload/edit
- `esp_console` over USB Serial/JTAG (`wifi` commands via shared `wifi_console`)

## Build

```bash
./build.sh build
```

## Flash + Monitor

```bash
./build.sh -p /dev/ttyACM0 flash monitor
```

## First-boot behavior

If `/spiffs/presets.json` does not exist, firmware seeds default presets (including a C41 CineStill-style preset) and sets it active.

## HTTP endpoints

- `GET /api/status`
- `GET /api/presets`
- `POST /api/presets`
- `POST /api/control`

Open the device root page (`/`) after Wi-Fi connect to edit/upload preset JSON.
