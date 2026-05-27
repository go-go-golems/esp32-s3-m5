# Changelog

## 2026-05-27

- Initial workspace created


## 2026-05-27

Step 1: Codebase reconnaissance — mapped Tab5 projects (0050/0051/UserDemo), diagnosed UserDemo crash, identified 0051 as fork base, wrote design doc

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/main/display_app.c — Primary fork base for screen viewer


## 2026-05-27

Step 2: Found and fixed real wifi_app.c crash — apply_sta_config() before configure_apsta_mode() causes ESP_ERR_WIFI_MODE when NVS has saved creds. Reordered in both 0050 and 0051. Board now boots successfully: display + WiFi + HTTP + console all working.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0051-tab5-boot-logo/main/wifi_app.c — Fixed wifi_app_start() init ordering bug


## 2026-05-27

Step 3: Implemented screen viewer firmware (Phases 1-3). Forked 0051, rewrote display_app.c with LVGL 9 SPIRAM image, added upload/clear/screen API endpoints, wrote browser drag-drop UI. Fixed LVGL assertion (need lock), HTTP timeout (30s), LVGL 9 API migration. All endpoints verified on hardware. Commit 2101b62.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0093-tab5-ui-screen-viewer/main/display_app.c — LVGL 9 SPIRAM image buffer + lock-safe invalidation


## 2026-05-27

Step 4: Browser upload verified via Playwright. Red/blue test patterns uploaded successfully (1.8MB each). Clear endpoint works. Board stable after USB reconnect. Awaiting visual RGB565 byte-order confirmation from user.


## 2026-05-27

Step 5: Fixed JS syntax error (two causes: UTF-8 chars corrupted by assembly embedding, and NUL terminator from EMBED_TXTFILES). Added display_app_clear/has_image. Browser file picker now works, full upload pipeline verified. Commits 875ff11, 9c9a484.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0093-tab5-ui-screen-viewer/main/display_app.c — added display_app_clear() and has_image tracking
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0093-tab5-ui-screen-viewer/main/http_server.c — NUL stripping in asset handlers

