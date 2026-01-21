# Changelog

## 2026-01-21

- Initial workspace created


## 2026-01-21

Implement mqjs_vm host primitive + migrate 0048/js_service and mqjs-repl/JsEvaluator; fix mqjs-repl explicit component deps for IDF 5.4 (commit b426db0).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Use MqjsVm for PrintValue/GetExceptionString and deadlines
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_vm/mqjs_vm.cpp — New stable ctx->opaque/log routing/deadline implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/imports/esp32-mqjs-repl/mqjs-repl/main/eval/JsEvaluator.cpp — Use MqjsVm without changing JsEvaluator.h

