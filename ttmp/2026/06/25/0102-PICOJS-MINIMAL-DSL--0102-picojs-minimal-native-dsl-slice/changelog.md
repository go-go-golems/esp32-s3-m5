# Changelog

## 2026-06-25

- Initial workspace created


## 2026-06-25

Implemented minimal native PicoJS DSL bindings, hello load/frame/dump flow, reset reinstall handling, and by-id UART probe validation

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0102-esp32-p4-visual-quickjs-repl/main/app_main.cpp — Installs PicoJS into qjs_service and adds load hello flow
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/components/picojs_runtime/picojs_runtime.cpp — Native OS/App/Panel/Text QuickJS bindings and text renderer
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/06/25/0102-PICOJS-MINIMAL-DSL--0102-picojs-minimal-native-dsl-slice/scripts/01-minimal-dsl-probe.py — Repeatable UART validation for minimal DSL

