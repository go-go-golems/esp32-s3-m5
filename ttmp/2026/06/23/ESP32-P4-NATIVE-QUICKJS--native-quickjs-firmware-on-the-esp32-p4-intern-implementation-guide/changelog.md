# Changelog

## 2026-06-23

- Initial workspace created


## 2026-06-23

Created native ESP32-P4 QuickJS ticket and wrote intern-facing analysis/design/implementation guide, with evidence from mqjs_service, 0067, 0100, and 0099.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/mqjs_service/include/mqjs_service.h — Reference API for proposed qjs_service
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/design/01-native-quickjs-on-esp32-p4-analysis-design-and-implementation-guide.md — Primary design and implementation guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Investigation diary Step 1


## 2026-06-23

Expanded native QuickJS ticket task list into implementation-grade phases T0-T7 and recorded diary Step 2 before starting source changes.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Diary Step 2 records task expansion and implementation plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/tasks.md — Detailed implementation task list


## 2026-06-23

Completed T1.1-T1.4 and T2.1-T2.4: vendored native QuickJS, added 0101 ESP32-P4 smoke firmware, fixed ESP-IDF portability issues, and got idf.py build passing (binary 0xb4f00).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0101-esp32-p4-native-quickjs/main/app_main.cpp — Native QuickJS smoke app
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/quickjs_native/CMakeLists.txt — QuickJS native component build
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/quickjs_native/quickjs_espidf_compat.h — ESP-IDF compatibility shim
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Step 3 records build failures and fixes


## 2026-06-23

Completed T2.5: flashed 0101 to ESP32-P4 and verified native QuickJS boot smoke; qjs ready in 6 ms, print(1+2) printed 3, sum10k completed.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0101-esp32-p4-native-quickjs/main/app_main.cpp — Boot-time native QuickJS smoke verified on hardware
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Step 4 records flash and monitor output


## 2026-06-23

Completed T3.1-T3.6 and service hardware smoke: added components/qjs_service owner-task API/implementation, switched 0101 to service-backed eval/status/reset smoke, built (0xb84d0), flashed, and verified print, benchmark, exception, and reset output on ESP32-P4.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0101-esp32-p4-native-quickjs/main/app_main.cpp — Service smoke harness
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/qjs_service/include/qjs_service.h — Public service API
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/qjs_service/qjs_service.cpp — Owner-task service implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Step 5 records service build and hardware validation


## 2026-06-23

Completed T4.1-T4.6 and T5.4: added interactive UART console commands (js status/eval/reset/gc/bench), fixed benchmark lexical redeclaration, raised qjs owner stack to 32 KiB for fib20, reduced eval timeout to 1000 ms, and verified timeout interruption on hardware.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0101-esp32-p4-native-quickjs/main/app_main.cpp — REPL startup and qjs service stack sizing
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0101-esp32-p4-native-quickjs/main/js_command.cpp — Console command implementation and benchmark scripts
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Step 6 records console validation and device-only fixes


## 2026-06-23

Completed remaining Phase 5 validation: verified js reset clears globals, repeated allocation plus gc stress completes, allocation benchmark completes, and design doc now compares native 0101 measurements against 0100 QuickJS-WASM baseline.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/design/01-native-quickjs-on-esp32-p4-analysis-design-and-implementation-guide.md — Updated native-vs-WAMR benchmark comparison
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/23/ESP32-P4-NATIVE-QUICKJS--native-quickjs-firmware-on-the-esp32-p4-intern-implementation-guide/reference/01-investigation-diary.md — Step 7 records reset

