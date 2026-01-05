# Changelog

## 2026-01-05

- Initial workspace created


## 2026-01-05

Implement WiFi esp_console REPL (status/set/connect/disconnect/clear/scan) with NVS-backed credentials and auto-connect (commit b2650c3).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_console.c — New console command implementation.
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_sta.c — NVS credential storage + scan + state tracking.


## 2026-01-05

Ticket closed


## 2026-01-05

Fix: start esp_console REPL on whichever backend is selected in sdkconfig (USB Serial/JTAG, USB CDC, or UART) (commit 05cb0ad).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0029-mock-zigbee-http-hub/main/wifi_console.c — Backend selection to avoid 'console disabled' when sdkconfig uses UART.

