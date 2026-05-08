# Changelog

## 2026-05-08

- Initial workspace created


## 2026-05-08

Created design and implementation guide for ZIP layout bundles before implementation.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/08/ALMANACH-ZIP-BUNDLES--support-zip-layout-bundles-for-almanach-renders/design/01-design-and-implementation-guide.md — Design guide


## 2026-05-08

Implemented ZIP layout bundle support for render, inspect, and print dry-run paths; added unit tests and validated a real SQLite animals bundle render (commit 0f2244e).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_inspect.go — Inspect command uses path-based layout loader
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_print.go — Print command uses path-based layout loader
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_render.go — Render command uses path-based layout loader
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/layout_bundle.go — ZIP/standalone layout loader and image asset inlining
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/layout_bundle_test.go — Unit tests for loader and bundle behaviors

