# Tasks

## TODO

- [x] Add tasks here

- [ ] Refactor main.go into Glazed root with serve command preserving current behavior
- [ ] Add render CLI verb with TypeObjectFromFile layout input, YAML/JSON support, PNG/bitmap outputs, and debug artifacts
- [ ] Add print CLI verb that renders once and posts bitmap to ESP32 printer with configurable feed lines
- [ ] Add inspect CLI verb for DOM metrics, selector validation, and cutoff debugging
- [ ] Refactor renderer to accept base URL, render options, selector, viewport, threshold, and capture CSS
- [ ] Add ephemeral localhost static server for one-shot CLI rendering
- [ ] Align default Go layout generation with frontend Almanach Studio schema
- [ ] Update devctl plugin and README to use the new CLI verbs
- [x] Phase 0.1: Add detailed phased task breakdown and update diary before code work
- [ ] Phase 1.1: Align layout.go data structs with frontend DEFAULTS/RENDERERS schema (title.text, word.part, history.items, did.items)
- [ ] Phase 1.2: Update fetchers to populate the aligned schema and keep default layout block types frontend-valid
- [ ] Phase 1.3: Fix empty HTTP request bodies so /api/render builds default live layout instead of passing an empty layout string
- [ ] Phase 1.4: Add layout schema/default tests for YAML/JSON-compatible structures and frontend-valid block types
- [ ] Phase 2.1: Introduce RenderOptions, RenderMetrics, and selector/threshold/viewport defaults
- [ ] Phase 2.2: Refactor Chrome render flow to accept BaseURL and selector instead of hardcoded localhost/.paper-shell
- [ ] Phase 2.3: Apply clipping-safe capture CSS and wait for almanachReady, fonts, and animation frames
- [ ] Phase 2.4: Collect DOM metrics for paper-shell, paper-body, canvas, workspace, and app; write debug artifacts when requested
- [ ] Phase 2.5: Preserve HTTP /api/render and /api/render-and-print behavior on top of the refactored renderer
- [ ] Phase 3.1: Add one-shot ephemeral 127.0.0.1:0 static server helper for CLI render/inspect/print
- [ ] Phase 3.2: Add Glazed dependencies and root command wiring with logging/help and backwards-compatible serve default
- [ ] Phase 3.3: Implement serve command flags mapped to Config while preserving Docker/devctl server behavior
- [ ] Phase 4.1: Implement render command with TypeObjectFromFile layout input, raw/wrapped layout support, PNG output, and metadata rows
- [ ] Phase 4.2: Extend render command with bitmap format, threshold flag, debug-dir artifacts, and validation errors
- [ ] Phase 4.3: Implement inspect command that emits selector metrics through Glazed output formats
- [ ] Phase 4.4: Implement print command with printer-ip/printer-url/feed-lines/dry-run and render debug options
- [ ] Phase 5.1: Update README with server and CLI workflows plus YAML layout examples
- [ ] Phase 5.2: Update devctl plugin to call CLI verbs for render/print while keeping up/down service management
- [ ] Phase 5.3: Run smoke tests: go test, render YAML to PNG, inspect metrics, bitmap output, dry-run print, and optional physical print
