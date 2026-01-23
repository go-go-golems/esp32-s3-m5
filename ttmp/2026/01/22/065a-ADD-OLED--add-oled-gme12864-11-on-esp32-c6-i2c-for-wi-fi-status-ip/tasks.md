# Tasks

## TODO

- [ ] Confirm OLED controller + I2C address (scan)
- [x] Document wiring (XIAO ESP32C6 pin4/pin5 -> GPIO22/GPIO23)
- [x] Integrate esp_lcd SSD1306 driver + 1bpp framebuffer
- [x] Add font rendering strategy (bitmap font + text layout)
- [x] Display Wi-Fi state + IP using wifi_mgr status/events
- [x] Upload ticket docs to reMarkable
- [ ] Flash on hardware + confirm I2C address and readability
- [x] Update reMarkable upload with implementation commit details
- [ ] If OLED blank: verify OLED enabled in menuconfig, enable scan-on-boot, confirm address (0x3C/0x3D) and interface (I2C vs SPI)
- [x] Fix build when sdkconfig is stale (missing CONFIG_MO065_OLED_SCAN_ON_BOOT) + avoid SSID snprintf truncation Werror
