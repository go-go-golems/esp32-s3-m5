# Almanach Render Service

A Go HTTP server that renders Almanach Studio pages using Chrome headless and sends them to a thermal printer via the stoms3r ESP32 firmware.

## Quick Start

### Docker Compose (recommended)

```bash
ALMANACH_PRINTER_IP=192.168.0.126 docker compose up
```

This starts two containers:
- **chrome** — `chromedp/headless-shell` (~310 MB, maintained by the chromedp team)
- **render** — the Go server (~12 MB binary)

### Docker (single container)

```bash
docker build -t almanach-render-service .
docker run -p 8199:8199 \
  -e ALMANACH_PRINTER_IP=192.168.0.126 \
  almanach-render-service
```

### Local Development

```bash
# Requires Chrome/Chromium installed on the host
make run
```

## API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `GET /health` | GET | Health check |
| `GET /almanach` | GET | Almanach Studio SPA (for Chrome) |
| `GET /almanach/bundle.js` | GET | React bundle |
| `POST /api/render` | POST | Render an almanac page |
| `POST /api/render-and-print` | POST | Render + send to ESP32 printer |

### Render

```bash
# Auto-generate from live data
curl -X POST http://localhost:8199/api/render

# Custom layout
curl -X POST http://localhost:8199/api/render \
  -H "Content-Type: application/json" \
  -d '{"theme":"minimal","paperWidth":384,"blocks":[...]}'

# Get raw bitmap
curl -X POST http://localhost:8199/api/render \
  -H "Accept: application/octet-stream" \
  -o page.bin
```

### Render and Print

```bash
curl -X POST http://localhost:8199/api/render-and-print
```

## Configuration

| Environment Variable | Default | Description |
|---------------------|---------|-------------|
| `ALMANACH_PORT` | `8199` | HTTP listen port |
| `ALMANACH_WEB_DIR` | `./web/almanach/dist` | SPA static files directory |
| `ALMANACH_PRINTER_IP` | *(empty)* | ESP32 stoms3r device IP |
| `ALMANACH_CHROME_PATH` | *(auto)* | Chrome binary path (local mode) |
| `CHROME_WS_URL` | *(empty)* | WebSocket URL for remote Chrome (Docker mode) |
| `ALMANACH_PAPER_WIDTH` | `384` | Paper width in pixels |
| `ALMANACH_FONT_SCALE` | `1.6` | Body font scale multiplier |
| `ALMANACH_DEFAULT_FEED` | `3` | Feed lines after printing |
| `ALMANACH_DEFAULT_THEME` | `minimal` | Default theme key |
| `ALMANACH_LOG_LEVEL` | `info` | Log verbosity |

## Cross-compile for Raspberry Pi

```bash
make build-pi
# produces almanach-render-service-arm64
```
