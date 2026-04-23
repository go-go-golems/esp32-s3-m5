# Changelog

## 2026-04-22

- Built and flashed `esp32-s3-m5/0051-tab5-boot-logo`; confirmed that the new image now compiles, fits in a 2 MB factory partition, and runs far enough to enter `display_app_init()`.
- Captured the current hardware failure on the Tab5: the app powers the MIPI DSI PHY, then stalls in the ST7123 / MIPI DSI short-packet read path and triggers the watchdog.
- Built the original `M5Tab5-UserDemo/platforms/tab5` reference firmware successfully as a sanity check on the local environment and source checkout.
- Added a new detailed bug report and display bring-up failure analysis for intern onboarding and next-step debugging.
- Reworked `display_app.c` to follow the factory board-preparation order (`I2C -> IO expander -> reset -> bsp_display_start_with_config`) and eliminated the original display-init hang.
- Verified a successful runtime sequence through display initialization, touch initialization, backlight enable, logo render, Wi-Fi bring-up, console startup, HTTP server startup, and final `ready` log.
- Investigated the later display flutter / edge-bar symptom and matched it to DSI display underrun errors caused by insufficient external-memory throughput.
- Compared the tutorial project against the original firmware configuration, discovered that 200 MHz PSRAM is gated by `CONFIG_IDF_EXPERIMENTAL_FEATURES`, enabled that path, and verified `I esp_psram: Speed: 200MHz` in the monitor log.
- Added numbered ticket-local scripts under `scripts/` for flash/log capture, config comparison, and PSRAM tuning.
- Added a new detailed resolution report documenting both the architectural repair and the follow-up display stability tuning.
- Wrote a new Obsidian vault article-style project report on the display bring-up difficulties and the Tab5 display architecture, then copied it into the ticket for reference.
- Uploaded the updated ticket bundle to reMarkable at `/ai/2026/04/21/ESP-49-TAB5-BOOTLOGO/ESP-49 Tab5 Boot Logo Firmware Analysis`.

## 2026-04-21

- Initial workspace created
