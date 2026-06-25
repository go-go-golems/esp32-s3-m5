# Changelog

## 2026-06-25

- Initial workspace created


## 2026-06-25

Created AtomS3R M12 native QuickJS ticket, intern design guide, and buildable 0103 ESP32-S3 PSRAM console firmware scaffold

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs — New AtomS3R M12 native QuickJS firmware
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md — Intern-facing design guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/reference/01-investigation-diary.md — Initial implementation diary


## 2026-06-25

Uploaded initial AtomS3R M12 native QuickJS design bundle to reMarkable at /ai/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md — Uploaded design guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/reference/01-investigation-diary.md — Uploaded diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/tasks.md — Uploaded task plan


## 2026-06-25

Validated 0103 on AtomS3R M12 hardware: by-id flash succeeded, ESP32-S3-PICO-1 with 8MB PSRAM detected, QuickJS initialized in 9 ms, and js status/eval/exception/reset/bench smoke passed

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/app_main.cpp — Boot identity and memory baseline logging validated
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/js_command.cpp — Console smoke commands validated
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/reference/01-investigation-diary.md — Step 2 hardware validation diary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/tasks.md — Phase 1 and initial Phase 2 task updates


## 2026-06-25

Completed AtomS3R QuickJS memory characterization: 20k numeric array succeeded, oversized allocation failed cleanly as InternalError: out of memory, reset restored baseline, and the 1 MiB cap remains the default

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md — Updated with hardware and memory-characterization results
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/reference/01-investigation-diary.md — Step 3 records memory stress evidence
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/tasks.md — Phase 2 completion and cap decision


## 2026-06-25

Added and validated read-only QuickJS system namespace for 0103 (commit 690972c): metadata is non-extensible/read-only and is restored after js reset

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/app_main.cpp — Installs system namespace after QuickJS startup
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/js_command.cpp — Reinstalls system namespace after js reset
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/system_namespace.cpp — Read-only JavaScript system metadata installer
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/reference/01-investigation-diary.md — Step 4 namespace implementation and validation diary


## 2026-06-25

Designed future 0103 storage and WiFi JavaScript namespaces: bounded virtual-rooted FatFs storage, native ESP32-S3 request/status WiFi API, no password exposure, and no blocking scans/connects on the QuickJS owner task

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/README.md — Firmware README points to future namespace plan
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/design-doc/01-atoms3r-m12-native-quickjs-analysis-design-and-implementation-guide.md — Detailed storage and WiFi namespace contracts
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/reference/01-investigation-diary.md — Step 5 design diary


## 2026-06-25

Implemented bounded FatFs script storage for 0103 (commit 521d5a2): explicit mount/format console command, virtual-rooted QuickJS storage namespace, reset-safe reinstall, hardware smoke, and board-reset persistence

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/app_main.cpp — Startup non-format mount and namespace install
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/js_command.cpp — Reinstalls storage namespace after js reset
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0103-atoms3r-m12-native-quickjs/main/storage_namespace.cpp — Storage implementation and QuickJS bindings
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/ATOMS3R-M12-NATIVE-QUICKJS--atoms3r-m12-native-quickjs-firmware-with-psram/reference/01-investigation-diary.md — Step 6 storage implementation diary

