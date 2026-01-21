# Tasks

## TODO

- [ ] Create `esp32-s3-m5/components/mqjs_service/` (CMakeLists + include + implementation)
- [ ] Implement VM owner task + inbound queue + eval request path
- [ ] Implement generic “job on VM task” path for callback/bindings work
- [ ] Integrate with `MqjsVm` (ticket 0054): deadlines + safe printing helpers
- [ ] Migrate 0048 incrementally:
  - [ ] Keep public 0048 wrappers: `js_service_eval_to_json`, `js_service_post_encoder_click`, `js_service_update_encoder_delta`
  - [ ] Internally route those wrappers into `mqjs_service_eval` / `mqjs_service_run`
  - [ ] Ensure `emit(...)` + `__0048_take_lines(...)` flush still works
- [ ] Build + smoke-test 0048 with REST eval + encoder callbacks + WS events
