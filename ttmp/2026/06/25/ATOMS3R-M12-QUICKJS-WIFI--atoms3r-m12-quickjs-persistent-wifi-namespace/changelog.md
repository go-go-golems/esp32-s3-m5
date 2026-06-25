# Changelog

## 2026-06-25

- Initial workspace created


## 2026-06-25

Created WiFi ticket, intern guide, task list, and diary; documented persistent NVS credential plan with password redacted

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI--atoms3r-m12-quickjs-persistent-wifi-namespace/design-doc/01-analysis-design-and-implementation-guide.md — WiFi intern guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI--atoms3r-m12-quickjs-persistent-wifi-namespace/reference/01-investigation-diary.md — WiFi initial diary


## 2026-06-25

Uploaded WiFi guide bundle to reMarkable at /ai/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI; password remains redacted in docs

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI--atoms3r-m12-quickjs-persistent-wifi-namespace/tasks.md — Upload task marked complete


## 2026-06-25

Implemented native STA-only WiFi console with NVS credential persistence (commit badc45a); provisioned Sonic Guest on-device with password redacted, validated autoconnect and STA IP 192.168.4.22

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/wifi_app.c — STA-only WiFi service and NVS credential persistence
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/wifi_command.cpp — Console provisioning and diagnostics
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI--atoms3r-m12-quickjs-persistent-wifi-namespace/reference/01-investigation-diary.md — Step 2 WiFi implementation and validation diary


## 2026-06-25

Added reset-safe QuickJS wifi namespace (commit 8ebff61): status/connect/disconnect/clearCredentials, no password exposure, hardware smoke validated reconnect and js reset persistence

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/app_main.cpp — Installs wifi namespace after QuickJS startup
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/js_command.cpp — Reinstalls wifi namespace after js reset
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/wifi_namespace.cpp — QuickJS wifi namespace implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-WIFI--atoms3r-m12-quickjs-persistent-wifi-namespace/reference/01-investigation-diary.md — Step 3 WiFi namespace diary

