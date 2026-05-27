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


## 2026-05-08

Phase 5: Documented CLI workflows, updated devctl render/print to call CLI verbs, verified devctl plan/render and command smoke tests (commit df08cca).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/README.md — CLI documentation
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/plugins/almanach-render.py — devctl integration


## 2026-05-08

Phase 6: Added six YAML layout examples, rendered PNG previews, inspect metadata, and a contact sheet; validated dimensions and visible overflow metrics (commit 26dfedc).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/examples/layouts/README.md — Example layout guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/examples/rendered/README.md — Rendered preview validation summary
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/examples/rendered/contact-sheet.png — Rendered visual overview


## 2026-05-08

Phase 7: Replaced unreliable post-bitmap ESC feed with baked trailing blank raster rows in the host print path; user confirmed physical feed worked (commit 6debc0e).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_print.go — Reports padded printer bitmap dimensions
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/printer.go — Bakes trailing blank rows into bitmap and sends X-Feed 0
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/printer_test.go — Unit tests for baked feed rows


## 2026-05-08

Phase 8: Added embedded Glazed help entries for layout authoring: getting started, user guide, DSL reference, and two tutorials; verified help pages load (commit 1117575).

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/cmd_root.go — Loads embedded docs into Glazed help system
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/doc/doc.go — Embedded help loader
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/doc/layout-dsl-reference.md — Full DSL reference
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/doc/layouts-getting-started.md — Getting-started layout tutorial
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/doc/layouts-user-guide.md — Layout authoring user guide
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/doc/tutorial-daily-briefing.md — Daily briefing tutorial
- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/stoms3r/cmd/almanach-render-service/doc/tutorial-knowledge-strip.md — Knowledge strip tutorial


## 2026-05-08

Added standalone repository reorganization and packaging design for moving firmware, service, web, examples, docs, scripts, Docker, packaging, and CI into a product-ready repository.

### Related Files

- /home/manuel/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/ttmp/2026/05/08/ALMANACH-CLI--almanach-render-service-cli-verbs-with-glazed/design-doc/02-repository-reorganization-and-packaging-design.md — Repository packaging design document


## 2026-05-08

Step 9: extracted Almanach render service into sibling almanach repository with cmd/internal Go layout, Glazed root/verbs, and Dagger/pnpm web asset bundling.

### Related Files

- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/cmd/almanach-render-service/main.go — Standalone binary entrypoint
- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/cmd/build-web/main.go — Dagger-first web build
- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/internal/web/embed.go — go:embed web bundle path


## 2026-05-08

Step 10: moved AtomS3R ESP-IDF firmware into almanach/firmware/atoms3r, made build helper portable, and validated an ESP-IDF 5.4.2 esp32s3 build.

### Related Files

- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/README.md — Documents firmware directory
- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/firmware/atoms3r/build.sh — Portable firmware build helper
- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/firmware/atoms3r/main/CMakeLists.txt — Firmware source/embed manifest


## 2026-05-10

Step 11: set up standalone almanach devctl plugin with build/up/health/render helpers, firmware helper commands, .envrc defaults, and validated devctl up/render/down.

### Related Files

- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/.devctl.yaml — devctl config
- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/.envrc — local environment setup
- /home/manuel/workspaces/2026-05-08/extract-almanach/almanach/plugins/almanach-render.py — devctl protocol implementation

