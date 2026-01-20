# Changelog

## 2026-01-20

- Initial workspace created


## 2026-01-20

Step 1: create ticket docs (commit e0a5e29)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/01/20/MO-033-ESP32C6-PREACT-WEB--esp32-c6-device-hosted-preact-zustand-web-ui-with-esp-console-wi-fi-selection/index.md — Ticket overview and links
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/vocabulary.yaml — Add esp32c6/wifi/preact/zustand/webserver topics


## 2026-01-20

Step 2: scaffold 0045 ESP32-C6 wifi console + embedded web UI (commit 0e698aa)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/main/http_server.c — Serve embedded index/app.js/app.css and /api/status
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/main/wifi_console.c — esp_console wifi commands incl. join-by-index
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/web/vite.config.ts — Deterministic Vite bundle outputs into main/assets


## 2026-01-20

Step 3: build + embed real web bundle (commit a64dc8f)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/main/assets/index.html — Vite-built entry HTML served at /
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/main/assets/assets/app.js — Vite-built JS bundle served by firmware


## 2026-01-20

Step 4: idf.py build OK on esp32c6; harden build.sh python selection (commit e7f279c)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/build.sh — Prepend IDF python venv to PATH to avoid esp_idf_monitor import issues
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0045-xiao-esp32c6-preact-web/sdkconfig — Captured ESP32-C6 sdkconfig baseline (force-added)
