# Tasks

## TODO

- [ ] Create component skeleton at `esp32-s3-m5/components/wifi_console/` (CMakeLists, Kconfig, include/)
- [ ] Implement `wifi_console_start(const wifi_console_config_t*)` with extension hook
- [ ] Port `cmd_wifi` implementation (scan/join/set/status) from `esp32-s3-m5/0048-cardputer-js-web/main/wifi_console.c`
- [ ] Ensure the component depends on `wifi_mgr` and only uses its public API
- [ ] Migrate 0046 to the component:
  - [ ] Replace local `wifi_console_start()` with wrapper configuring `prompt="c6> "` and `register_extra=led_console_register_commands`
  - [ ] Update `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/CMakeLists.txt` to depend on `wifi_console`
- [ ] Migrate 0048 to the component:
  - [ ] Replace local `wifi_console_start()` with wrapper configuring `prompt="0048> "` and `register_extra=js_console_register_commands`
  - [ ] Update `esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt` to depend on `wifi_console`
- [ ] Manual validation (both boards): `help`, `wifi scan`, `wifi join`, `wifi status` and confirm extra commands still exist (`led` for 0046, `js` for 0048)
