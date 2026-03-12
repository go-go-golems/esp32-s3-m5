# Changelog

## 2026-03-11

- Ported the `0067` timer scheduler pattern into `0074-m5dial-web-remote`, exposing working `setTimeout`, `clearTimeout`, `every`, and `cancel` support for the Lain OS MicroQuickJS runtime.
- Added a project-local JavaScript API guide and timer-based sample scripts under `0074-m5dial-web-remote/`.
- Added a Go server `script-eval` CLI verb that auto-selects the only connected device from `/api/status` and posts `/api/script-eval`.
- Clarified the runtime syntax constraints after hardware validation:
  - use `function () { ... }`
  - use `var`
  - avoid arrow functions, `let`, and `const`
- Improved firmware remote-script status reporting so `remote status` distinguishes lifetime submissions from current pending queue depth.

## 2026-03-11

- Added `reference/02-script-memory-lifecycle.md` to document the real remote `script_eval` memory path, ownership handoff, and the two stack-overflow bugs discovered during hardware validation.

## 2026-03-11

- Ticket workspace created for adding MicroQuickJS scripting to the 0074 Lain OS stack.
- Current-state analysis completed for:
  - `0074-m5dial-web-remote` runtime boundaries
  - `0048-cardputer-js-web` QuickJS service pattern
- Primary design doc and implementation diary written.
- Delivery package prepared for reMarkable upload.
- Task scope narrowed to backend, protocol, and JS runtime ownership; browser editor and visual UX work moved out of system implementation scope.
- Backend implementation landed in 0074:
  - firmware now builds with `mquickjs`, `mqjs_service`, and an app-command bus shared by websocket and JS runtime output
  - Go hub now routes `script_eval` and broadcasts `script_result` / `script_console` / `script_event`
  - remote console now supports enabling and disabling remote script execution

## 2026-03-11

Current-state analysis completed for 0074 and 0048; detailed design package written for a MicroQuickJS Lain DSL and websocket script transport.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0048-cardputer-js-web/main/js_service.cpp — Reference QuickJS service implementation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0074-m5dial-web-remote/firmware/main/app_main.cpp — Current runtime architecture evidence
