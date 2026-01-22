# Tasks

## TODO

- [ ] Create `esp32-s3-m5/components/encoder_service/` (CMakeLists + include + implementation)
- [ ] Define driver vtable interface (init/take_delta/take_click_kind)
- [ ] Define sink callback interface (encoder events fan-out) so `encoder_service` has **no direct** web/JS dependencies
- [ ] Implement encoder service task loop with configurable poll/coalesce periods and “only notify on change”
- [ ] Migrate 0048:
  - [ ] Move `esp32-s3-m5/0048-cardputer-js-web/main/chain_encoder_uart.cpp` into `encoder_service` as an internal driver module
  - [ ] Replace task logic in `esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp` with `encoder_service_start(...)` + per-firmware sinks:
    - [ ] WS sink (0048): calls `http_server_ws_broadcast_text(...)` with existing schemas (`type:"encoder"`, `type:"encoder_click"`)
    - [ ] JS sink (0048): calls `js_service_update_encoder_delta(...)` and `js_service_post_encoder_click(...)`
- [ ] Hardware validation: rotate + click, confirm:
  - [ ] WS only sends encoder snapshot when changed
  - [ ] clicks show as WS events and trigger JS callbacks
