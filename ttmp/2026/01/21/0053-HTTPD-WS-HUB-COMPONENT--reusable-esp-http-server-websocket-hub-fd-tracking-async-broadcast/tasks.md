# Tasks

## TODO

- [x] Create `esp32-s3-m5/components/httpd_ws_hub/` (CMakeLists + include + implementation)
- [x] Implement fd tracking (fixed-size) + mutex + snapshot helper
- [x] Implement `httpd_ws_hub_broadcast_text` and `httpd_ws_hub_broadcast_binary` using `httpd_ws_send_data_async` + free callback
- [x] Implement optional RX callbacks invoked from WS handler (binary + text)
- [x] Migrate 0048:
- [x] Replace WS client list in `esp32-s3-m5/0048-cardputer-js-web/main/http_server.cpp`
- [x] Keep `http_server_ws_broadcast_text` symbol as wrapper for existing call sites (`js_service.cpp`, `encoder_telemetry.cpp`)
- [x] Migrate 0017:
- [x] Replace WS client list in `esp32-s3-m5/0017-atoms3r-web-ui/main/http_server.cpp`
- [x] Preserve wrapper symbols: `http_server_ws_broadcast_text`, `http_server_ws_broadcast_binary`, `http_server_ws_set_binary_rx_cb`
- [ ] Migrate 0029:
  - [ ] At minimum, reuse the hub for WS client tracking; keep existing `hub_http_events_broadcast_pb` refcount logic initially
- [ ] Build + run WS smoke tests on 0017/0048/0029
- [x] Build 0048 after migration (compile/link only)
- [x] Build 0017 after migration (compile/link only)
