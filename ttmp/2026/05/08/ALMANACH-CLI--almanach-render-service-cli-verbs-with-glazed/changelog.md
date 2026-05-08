# Changelog

## 2026-05-08

- Initial workspace created


## 2026-05-08

Created intern-facing design and implementation guide for Glazed CLI verbs, YAML/JSON layout input via TypeObjectFromFile, one-shot ephemeral rendering, print/inspect commands, and cutoff debugging workflow.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/08/ALMANACH-CLI--almanach-render-service-cli-verbs-with-glazed/design-doc/01-cli-verbs-with-glazed-analysis-design-and-implementation-guide.md — Main ALMANACH-CLI design guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/08/ALMANACH-CLI--almanach-render-service-cli-verbs-with-glazed/reference/01-implementation-diary.md — Chronological implementation diary


## 2026-05-08

Uploaded the ALMANACH-CLI design bundle to reMarkable at /ai/2026/05/08/ALMANACH-CLI and verified it is listed in cloud storage.


## 2026-05-08

Phase 0.1: Added detailed phased implementation tasks and recorded the first diary step.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/08/ALMANACH-CLI--almanach-render-service-cli-verbs-with-glazed/reference/01-implementation-diary.md — Diary step for phased planning
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/08/ALMANACH-CLI--almanach-render-service-cli-verbs-with-glazed/tasks.md — Phased ALMANACH-CLI implementation tasks


## 2026-05-08

Phase 1: Aligned Go layout/fetcher schema with the React frontend, fixed empty HTTP body default rendering, and added schema tests (commit c3708df).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/layout.go — Schema alignment
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/layout_test.go — Schema tests
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/renderer.go — Empty-body behavior


## 2026-05-08

Phase 2: Refactored Chrome rendering around RenderOptions, clipping-safe capture CSS, metrics collection, debug artifacts, and safer layout injection while preserving HTTP behavior (commit 4ec7ee6).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/renderer.go — Renderer refactor


## 2026-05-08

Phases 3-4: Added Glazed/Cobra root, backwards-compatible serve mode, one-shot ephemeral renderer, and render/inspect/print CLI verbs using TypeObjectFromFile for YAML/JSON layouts (commit 81fe310).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_inspect.go — Inspect command
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_print.go — Print command
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_render.go — Render command
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_root.go — Root command
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/render_oneshot.go — Ephemeral render server

