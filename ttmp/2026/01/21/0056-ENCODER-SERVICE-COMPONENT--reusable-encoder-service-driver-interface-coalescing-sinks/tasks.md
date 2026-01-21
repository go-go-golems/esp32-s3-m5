# Tasks

## TODO

- [ ] Create `esp32-s3-m5/components/encoder_service/` (CMakeLists + include + implementation)
- [ ] Define driver vtable interface (init/take_delta/take_click_kind)
- [ ] Implement encoder service task loop with configurable poll/broadcast periods
- [ ] Migrate 0048:
  - [ ] Adapt `esp32-s3-m5/0048-cardputer-js-web/main/chain_encoder_uart.cpp` into a driver implementation
  - [ ] Replace task logic in `esp32-s3-m5/0048-cardputer-js-web/main/encoder_telemetry.cpp` with `encoder_service_start(...)` + sinks:
    - [ ] WS sink: calls `http_server_ws_broadcast_text(...)` with existing schemas (`type:"encoder"`, `type:"encoder_click"`)
    - [ ] JS sink: calls `js_service_update_encoder_delta(...)` and `js_service_post_encoder_click(...)`
- [ ] Hardware validation: rotate + click, confirm:
  - [ ] WS only sends encoder snapshot when changed
  - [ ] clicks show as WS events and trigger JS callbacks
