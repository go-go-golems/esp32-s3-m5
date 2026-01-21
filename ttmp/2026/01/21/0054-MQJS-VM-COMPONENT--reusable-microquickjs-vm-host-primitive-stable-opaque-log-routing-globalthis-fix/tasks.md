# Tasks

## TODO

- [x] Create `esp32-s3-m5/components/mqjs_vm/` (CMakeLists + include + implementation)
- [x] Implement stable opaque + log routing (`WriteFunc`) + deadline (`InterruptHandler`)
- [x] Implement `PrintValue` and `GetExceptionString` helpers used by consumers
- [x] Migrate 0048:
- [x] Replace local printing/timeout/log wiring in `esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp`
- [x] Keep 0048 bootstraps intact; only change VM hosting/invariants
- [x] Refactor `JsEvaluator` internally (no API change):
- [x] Keep `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.h` unchanged
- [x] Update `esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp` to use `MqjsVm`
- [x] Regression: ensure `print({a:1})` and `print(ev)` do not crash in callback-driven use cases
