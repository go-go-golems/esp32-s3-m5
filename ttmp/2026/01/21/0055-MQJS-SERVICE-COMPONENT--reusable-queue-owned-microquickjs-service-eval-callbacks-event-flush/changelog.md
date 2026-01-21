# Changelog

## 2026-01-21

- Initial workspace created


## 2026-01-21

Implement mqjs_service (queue-owned VM task) and refactor 0048 js_service to use it (commit 3e2e671).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Wrapper layer using mqjs_service
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_service.cpp — New reusable service implementation


## 2026-01-21

Fix boot crash: add task-start handshake and early NULL-queue guard in mqjs_service task loop (commit 1cbd51f).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/mqjs_service.cpp — Service task now signals readiness before blocking on queue

