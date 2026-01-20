# Changelog

## 2026-01-20

- Initial workspace created


## 2026-01-20

Scaffold 0046 firmware+web UI, add minimal /api/led endpoints; bump httpd max_uri_handlers to prevent 404s

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/app_main.c — Starts LED task + Wi-Fi and starts HTTP server on got-IP
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/http_server.c — Registers /api/led/* endpoints and sets max_uri_handlers
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/web/src/store/led.ts — UI client calling /api/led/*

