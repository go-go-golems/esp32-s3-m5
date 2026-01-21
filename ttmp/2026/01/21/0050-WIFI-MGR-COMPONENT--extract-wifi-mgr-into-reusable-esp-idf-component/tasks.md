# Tasks

## TODO

- [x] Create component skeleton at `esp32-s3-m5/components/wifi_mgr/` (CMakeLists, Kconfig, include/)
- [x] Copy baseline implementation from `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c` into `components/wifi_mgr/wifi_mgr.c`
- [x] Standardize Kconfig: introduce `CONFIG_WIFI_MGR_MAX_RETRY` and `CONFIG_WIFI_MGR_AUTOCONNECT_ON_BOOT`
- [x] Replace config references in component code (`CONFIG_TUTORIAL_0048_WIFI_*` → `CONFIG_WIFI_MGR_*`)
- [x] Migrate `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/` to component
- [x] Migrate `esp32-s3-m5/0048-cardputer-js-web/` to component
- [ ] Build + smoke-test 0046: confirm `wifi scan/join/status` works and autoconnect respects `CONFIG_WIFI_MGR_*`
- [ ] Build + smoke-test 0048: confirm `/api/status` still reports correct fields and autoconnect respects `CONFIG_WIFI_MGR_*`

## Migration details (do not hand-wave)

- 0046:
  - Remove: `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.c`, `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/wifi_mgr.h`
  - Update: `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/CMakeLists.txt` (add dependency on `wifi_mgr`)
  - Update: `esp32-s3-m5/0046-xiao-esp32c6-led-patterns-webui/main/Kconfig.projbuild` (rename `CONFIG_MO033_WIFI_*` → `CONFIG_WIFI_MGR_*`)

- 0048:
  - Remove: `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.c`, `esp32-s3-m5/0048-cardputer-js-web/main/wifi_mgr.h`
  - Update: `esp32-s3-m5/0048-cardputer-js-web/main/CMakeLists.txt` (add dependency on `wifi_mgr`)
  - Update: `esp32-s3-m5/0048-cardputer-js-web/main/Kconfig.projbuild` (rename `CONFIG_TUTORIAL_0048_WIFI_*` → `CONFIG_WIFI_MGR_*`)
- [x] Build 0046 after migration (compile/link only)
- [x] Build 0048 after migration (compile/link only)
