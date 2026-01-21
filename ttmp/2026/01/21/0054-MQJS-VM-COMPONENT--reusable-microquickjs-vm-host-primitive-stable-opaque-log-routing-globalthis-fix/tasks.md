# Tasks

## TODO

- [ ] Create `esp32-s3-m5/components/mqjs_vm/` (CMakeLists + include + implementation)
- [ ] Implement stable opaque + log routing (`WriteFunc`) + deadline (`InterruptHandler`)
- [ ] Implement `PrintValue` and `GetExceptionString` helpers used by consumers
- [ ] Migrate 0048:
  - [ ] Replace local printing/timeout/log wiring in `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
  - [ ] Keep 0048 bootstraps intact; only change VM hosting/invariants
- [ ] Refactor `JsEvaluator` internally (no API change):
  - [ ] Keep `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h` unchanged
  - [ ] Update `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` to use `MqjsVm`
- [ ] Regression: ensure `print({a:1})` and `print(ev)` do not crash in callback-driven use cases
