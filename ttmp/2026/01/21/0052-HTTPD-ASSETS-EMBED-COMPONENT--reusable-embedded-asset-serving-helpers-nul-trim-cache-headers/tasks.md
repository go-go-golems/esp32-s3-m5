# Tasks

## TODO

- [x] Create `esp32-s3-m5/components/httpd_assets_embed/` (CMakeLists + include + implementation)
- [x] Implement `httpd_assets_embed_len(...)` with optional trailing-NUL trim
- [x] Implement `httpd_assets_embed_send(...)` that sets content-type + cache-control and sends trimmed length
- [x] Migrate 0048:
- [x] Replace `embedded_txt_len(...)` in `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
- [x] Keep the fix for `Uncaught SyntaxError: illegal character U+0000` covered by default trim
- [x] Migrate 0017:
- [x] Update `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp` asset sends to use `httpd_assets_embed_send(...)`
- [ ] Migrate 0029 (optional):
  - [ ] Replace `send_embedded(...)` in `esp32-s3-m5/0029-mock-zigbee-http-hub/main/hub_http.c`
- [ ] Rebuild affected firmwares and validate the browser loads without `U+0000` errors
- [x] Migrate 0046: use httpd_assets_embed_send for embedded UI assets
- [x] Build 0046 after migration (compile/link only)
- [x] Build 0048 after migration (compile/link only)
- [x] Build 0017 after migration (compile/link only)
