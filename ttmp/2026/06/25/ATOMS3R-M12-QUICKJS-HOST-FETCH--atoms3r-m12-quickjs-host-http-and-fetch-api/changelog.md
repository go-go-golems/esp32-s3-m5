# Changelog

## 2026-06-25

- Initial workspace created


## 2026-06-25

Created host/fetch ticket, intern-facing design guide, investigation diary, and phased task plan for shared QuickJS HTTP namespace plus bounded fetch API

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/design-doc/01-analysis-design-and-implementation-guide.md — Analysis/design/implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/reference/01-investigation-diary.md — Investigation diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/tasks.md — Phased task plan


## 2026-06-25

Uploaded host HTTP and fetch design bundle to reMarkable at /ai/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/design-doc/01-analysis-design-and-implementation-guide.md — Uploaded primary guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/reference/01-investigation-diary.md — Uploaded diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/tasks.md — Marked delivery task complete


## 2026-06-25

Implemented Phase 1 shared QuickJS HTTP core and desktop native host; host smoke validates http.static/http.get direct dispatch and bounded fetch (commit 3737dfd)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/host/native-http/tests/run-smoke.sh — Passing host smoke
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_namespace_core.cpp — Shared HTTP/fetch binding implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/reference/01-investigation-diary.md — Phase 1 diary entry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/tasks.md — Marked HF1.1-HF1.4 complete


## 2026-06-25

Implemented Phase 2 firmware wrapper for the shared QuickJS HTTP core: boot install, pre-reset clear, post-reset reinstall, and native HTTP status bridge; build passed without device (commit acae5fb)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp — Firmware HTTP namespace wrapper
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/js_command.cpp — Reset-safe HTTP namespace handling
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/reference/01-investigation-diary.md — Phase 2 diary entry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/tasks.md — Marked HF2.1-HF2.3 complete


## 2026-06-25

Implemented Phase 3 firmware dynamic GET route dispatch: esp_http_server wildcard requests call the shared QuickJS route table through qjs_service_run before static fallback; build passed without device (commit a0009cf)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp — QuickJS owner-task dispatch bridge
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_server.cpp — Dynamic-first HTTP wildcard dispatch
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/reference/01-investigation-diary.md — Phase 3 diary entry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/tasks.md — Marked HF3.1-HF3.3 complete


## 2026-06-25

Implemented Phase 4 bounded firmware fetch adapter using esp_http_client; host fetch smoke still passes and firmware build passes without device (commit faf621d)

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/CMakeLists.txt — esp_http_client dependency
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp — Firmware fetch adapter
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/reference/01-investigation-diary.md — Phase 4 diary entry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/tasks.md — Marked HF4.1-HF4.3 complete


## 2026-06-25

Hardware-validated AtomS3R QuickJS HTTP/fetch: /healthz, static /static/index.html, dynamic /api/hello, firmware fetch Promise callbacks, and reset route clearing all passed; added qjs_service Promise-job draining (commit 05c8bc6).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_namespace.cpp — Firmware fetch validation path
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/http_server.cpp — Dynamic/static hardware validation path
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/qjs_service/qjs_service.cpp — Promise job draining after eval
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/reference/01-investigation-diary.md — Hardware validation diary entry
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-QUICKJS-HOST-FETCH--atoms3r-m12-quickjs-host-http-and-fetch-api/tasks.md — Marked HF2.4

