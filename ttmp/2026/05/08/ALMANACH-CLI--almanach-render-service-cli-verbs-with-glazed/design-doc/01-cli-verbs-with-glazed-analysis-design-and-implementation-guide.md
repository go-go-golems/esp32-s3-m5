---
Title: CLI Verbs with Glazed — Analysis, Design, and Implementation Guide
Ticket: ALMANACH-CLI
Status: active
Topics:
    - almanach
    - go
    - console
    - rendering
    - thermal-printer
    - tooling
DocType: design-doc
Intent: long-term
Owners: []
RelatedFiles:
    - Path: stoms3r/cmd/almanach-render-service/layout.go
      Note: Default layout structs and frontend schema alignment target
    - Path: stoms3r/cmd/almanach-render-service/main.go
      Note: Current server-only entry point that should become Glazed root plus serve default
    - Path: stoms3r/cmd/almanach-render-service/printer.go
      Note: Bitmap POST client reused by print CLI verb
    - Path: stoms3r/cmd/almanach-render-service/renderer.go
      Note: Chrome headless screenshot flow to refactor with selector
    - Path: stoms3r/cmd/almanach-render-service/static.go
      Note: SPA static serving to reuse in ephemeral one-shot CLI server
    - Path: stoms3r/web/almanach/src/almanach-studio.jsx
      Note: Frontend source of truth for layout schema
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/bitmap.go
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/config.go
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/layout.go
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/main.go
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/plugins/almanach-render.py
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/printer.go
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/renderer.go
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/server.go
    - Path: ttmp/2026/05/stoms3r/cmd/almanach-render-service/static.go
    - Path: ttmp/2026/05/stoms3r/web/almanach/src/almanach-studio.jsx
ExternalSources:
    - /home/manuel/code/wesen/corporate-headquarters/glazed/pkg/doc/tutorials/05-build-first-command.md
    - /home/manuel/code/wesen/corporate-headquarters/glazed/cmd/examples/field-types/main.go
    - /home/manuel/code/wesen/corporate-headquarters/glazed/cmd/glaze/main.go
Summary: Design and implementation guide for adding one-shot Glazed CLI verbs to almanach-render-service, including YAML/JSON layout input via TypeObjectFromFile, preview/debug control, printing, and an ephemeral Chrome render server.
LastUpdated: 2026-05-08T06:08:00-04:00
WhatFor: Use this as the intern-facing technical plan for implementing CLI verbs in stoms3r/cmd/almanach-render-service without losing the existing HTTP server mode.
WhenToUse: Read before refactoring main.go, adding Glazed commands, changing renderer capture behavior, or debugging clipped thermal almanac output.
---


# CLI Verbs with Glazed — Analysis, Design, and Implementation Guide

## Executive Summary

The Almanach Render Service currently runs as a long-lived HTTP server. It serves the Almanach Studio React SPA at `/almanach`, accepts layout JSON through `/api/render`, drives Chrome headless through `chromedp`, captures a PNG screenshot of the paper preview, converts that PNG into a 1-bit thermal-printer bitmap, and optionally posts the bitmap to the ESP32 printer firmware.

That design works well for automated scheduling and network use, but it is awkward for local iteration. When a printed page is cut off or a selector is wrong, the developer must start a server, send HTTP requests, manage output files manually, and infer what Chrome saw. The goal of this ticket is to add **first-class CLI verbs** to the same binary using the **Glazed command framework**, while preserving the existing server mode.

The target user experience is:

```bash
# Existing server behavior remains available.
almanach-render-service serve --port 8199 --printer-ip 192.168.0.126

# One-shot PNG preview from JSON or YAML layout.
almanach-render-service render \
  --layout ./layout.yaml \
  --out /tmp/almanach.png \
  --selector .paper-body \
  --debug-dir /tmp/almanach-debug

# One-shot raw bitmap output.
almanach-render-service render \
  --layout ./layout.yaml \
  --format bitmap \
  --out /tmp/almanach.bin

# One-shot print.
almanach-render-service print \
  --layout ./layout.yaml \
  --printer-ip 192.168.0.126 \
  --selector .paper-body

# Selector and clipping diagnostics.
almanach-render-service inspect \
  --layout ./layout.yaml \
  --debug-dir /tmp/almanach-debug \
  --output yaml
```

The most important input requirement is that `--layout` must use Glazed's `fields.TypeObjectFromFile` so the command can read both JSON and YAML layout objects. The command should not make callers pre-convert YAML to JSON.

The most important rendering requirement is that the CLI must be able to debug and control capture. The current implementation hardcodes `.paper-shell`, fixed sleeps, a 1200x2000 viewport, threshold 128, and a partial CSS override. The CLI should expose selector, viewport, threshold, debug artifact, and inspection controls so a developer can quickly determine whether content is clipped because the wrong selector was captured, because an ancestor has `overflow: hidden`, or because the layout itself exceeds practical paper size.

## Problem Statement

### Current state

The current binary has a single entry point in `stoms3r/cmd/almanach-render-service/main.go`. Running the binary always starts an HTTP server. The server registers routes in `server.go`:

- `GET /health` returns service status.
- `GET /almanach` serves the SPA host page.
- `GET /almanach/bundle.js` serves the React bundle.
- `POST /api/render` renders a page and returns JSON, PNG, or raw bitmap based on `Accept`.
- `POST /api/render-and-print` renders and posts the bitmap to the ESP32 printer.
- `GET/POST/DELETE /api/schedule` is a stub for future scheduler work.

Rendering is implemented in `renderer.go`. The flow is:

1. Create a `chromedp` tab using a global allocator.
2. Navigate to `http://localhost:<port>/almanach`.
3. Wait for the body.
4. Evaluate `window.almanachLoadLayout(layoutJSON)` in the page.
5. Inject a `hideChromeJS` stylesheet that hides editor UI.
6. Screenshot `.paper-shell`.
7. Convert the PNG screenshot to a 1-bit bitmap in `bitmap.go`.

The frontend side is implemented in `stoms3r/web/almanach/src/almanach-studio.jsx`. It exposes a headless API:

- `window.almanachReady`
- `window.almanachLoadLayout(json)`
- `window.almanachExportBitmap()`

The render service currently uses `almanachLoadLayout` and `chromedp.Screenshot`; it does not use `almanachExportBitmap` because a previous SVG `foreignObject` based export path could taint the canvas in Chrome headless.

### Developer pain points

A developer who wants to iterate on the printed result faces several problems:

- They must start the long-running server even for a one-shot render.
- They must use `curl` or devctl wrappers instead of a direct CLI.
- They cannot pass YAML layout files directly.
- They cannot easily change `.paper-shell` versus `.paper-body`.
- They cannot request DOM metrics for `.paper-shell`, `.paper-body`, `.canvas`, `.workspace`, and `.almanach-app`.
- They cannot ask the renderer to save intermediate debug artifacts by command-line option.
- The server defaults hide some editor chrome but do not fully remove clipping from `.almanach-app`, `.workspace`, and `.canvas`.
- The Go default layout structs are not fully aligned with the frontend schema, so generated data can silently render incorrectly or be filtered by the frontend parser.

### Why this matters

The target printer is a 58mm thermal printer with a 384px wide 1-bit raster input. Small layout differences matter. If a screenshot contains the outer canvas, box shadow, zigzag edges, editor UI, hidden scroll clipping, or a truncated paper body, the physical print is wrong and wastes paper. The CLI should turn the render loop into a predictable local build artifact workflow: layout file in, PNG/bitmap/debug files out.

## System Overview for a New Intern

This section explains the full system from input data to physical paper. If you are new to the codebase, read this before writing code.

### Big picture

The Almanach system has three cooperating parts:

1. **Almanach Studio React SPA** — a visual paper layout editor and renderer.
2. **Almanach Render Service** — a Go program that drives Chrome headless to render the SPA and produce printer-ready output.
3. **stoms3r ESP32 firmware** — an embedded HTTP printer endpoint that receives packed monochrome bitmaps and sends them to the thermal printer.

```text
           JSON/YAML layout file
                    |
                    v
       +----------------------------+
       | almanach-render-service    |
       | CLI or HTTP server         |
       +-------------+--------------+
                     |
                     | serves /almanach and bundle.js
                     v
       +----------------------------+
       | Chrome headless            |
       | React SPA renders paper    |
       +-------------+--------------+
                     |
                     | screenshot .paper-body/.paper-shell
                     v
       +----------------------------+
       | PNG screenshot             |
       +-------------+--------------+
                     |
                     | threshold + pack MSB-first
                     v
       +----------------------------+
       | 1-bit bitmap               |
       +-------------+--------------+
                     |
                     | POST /api/print/bitmap
                     v
       +----------------------------+
       | ESP32 stoms3r firmware     |
       | printer_drv_print_bitmap   |
       +-------------+--------------+
                     |
                     v
              thermal paper
```

### The React SPA is the rendering source of truth

The frontend file `stoms3r/web/almanach/src/almanach-studio.jsx` defines:

- Theme definitions.
- Default block data (`DEFAULTS`).
- Valid block type metadata (`BLOCK_TYPES`).
- React block renderers (`RENDERERS`).
- Layout import/export helpers.
- The `ThermalPaper` component.
- The headless API used by Go.

The `ThermalPaper` component renders the paper structure:

```text
.paper-shell
  ├── ZigzagEdge top
  ├── .paper-body
  │     ├── paper grain / frame decorations
  │     └── block list
  │           ├── .block-wrap title
  │           ├── .block-wrap date
  │           └── ...
  └── ZigzagEdge bottom
```

The frontend layout parser is `parseLayoutJson`. It validates block types, applies default values, chooses a theme, paper width, font scale, and feed lines. The frontend currently accepts the following block type strings:

```text
title, date, divider, plan, news, weather, note, habits, mood,
reading, reflection, quote, word, history, did
```

Important: the frontend uses `did`, not `did_you_know`.

### The Go server currently wraps the SPA

The Go code does not draw the almanac itself. Instead, it starts Chrome, opens the SPA, injects data, and asks Chrome for a screenshot. That is important: the React/CSS rendering is the canonical visual output. The Go renderer is an automation layer around that visual renderer.

Relevant files:

- `main.go` starts the HTTP server and Chrome allocator.
- `config.go` loads env vars such as port, printer IP, web dir, Chrome path, paper width, font scale, and default theme.
- `server.go` handles HTTP routes.
- `static.go` serves `index.html` and `almanach-bundle.js`.
- `renderer.go` drives Chrome and captures screenshots.
- `bitmap.go` converts PNG to 1-bit MSB-first bitmap data.
- `printer.go` posts bitmap data to ESP32.
- `layout.go` generates default live-data layouts.

### The printer expects a packed monochrome bitmap

The ESP32 firmware endpoint expects:

```text
POST /api/print/bitmap
Content-Type: application/octet-stream
X-Width: 384
X-Height: <rendered height>
X-Feed: 3

<body is packed bitmap bytes>
```

Bitmap packing rule:

- Image is 1 bit per pixel.
- Black pixel means bit `1`.
- White pixel means bit `0`.
- Bits are packed MSB-first.
- Width is padded up to the next multiple of 8 pixels.
- `bytesPerRow = paddedWidth / 8`.

`bitmap.go` implements this conversion by decoding the PNG, computing grayscale luminance, comparing against threshold 128 by default, and setting bits.

## Current Architecture Details

### Server startup

Current startup is roughly:

```go
func main() {
    cfg := loadConfig()
    allocatorCtx, allocatorCancel := newChromeAllocator(cfg)
    srv := &Server{cfg: cfg, allocatorCtx: allocatorCtx}

    mux := http.NewServeMux()
    srv.RegisterRoutes(mux)

    httpServer := &http.Server{Addr: fmt.Sprintf(":%d", cfg.Port), Handler: mux}
    httpServer.ListenAndServe()
}
```

This is simple, but it means the binary cannot do one-shot actions. The new CLI must refactor this into a `serve` command while preserving default behavior for compatibility.

### Rendering path

Current rendering is roughly:

```go
func (s *Server) render(ctx context.Context, layoutOverride io.Reader) (*RenderResult, error) {
    if layoutOverride != nil {
        data := io.ReadAll(layoutOverride)
        layoutJSON = string(data)
    } else {
        layout := buildDefaultLayout(s.cfg)
        layoutJSON = json.Marshal(layout)
    }

    return s.renderWithChrome(ctx, layoutJSON)
}
```

There is a subtle bug here: in an HTTP handler, `r.Body` is usually non-nil even when the request body is empty. Therefore a request such as `curl -X POST /api/render` reads an empty string rather than generating a default layout. This should be fixed while implementing the CLI refactor.

`renderWithChrome` currently hardcodes the local URL, selector, viewport, wait sleeps, capture stylesheet, and threshold. The CLI needs these to become parameters.

### Static file serving

`static.go` registers:

```go
mux.HandleFunc("/almanach", serve index.html)
mux.HandleFunc("/almanach/bundle.js", serve almanach-bundle.js)
```

The CLI should reuse this logic with an **ephemeral local server**. Chrome still needs an HTTP URL because the host page references `/almanach/bundle.js`, and using `file://` would require a separate HTML mode or path rewrite. The ephemeral server is simpler and closer to production.

## Proposed Solution

Add Glazed CLI verbs to the existing binary while preserving the current HTTP service mode.

### Command tree

Recommended command tree:

```text
almanach-render-service
  serve      Start the long-running HTTP API server. Preserves current behavior.
  render     One-shot render to PNG or bitmap file.
  print      One-shot render and send to ESP32 printer.
  inspect    One-shot render setup inspection: DOM metrics, selector bounds, debug artifacts.
  version    Optional plain Cobra or Glazed command for build metadata.
  help       Glazed help system.
```

Recommended compatibility behavior:

- `almanach-render-service serve` starts the server.
- `almanach-render-service` with no subcommand should either:
  - start `serve` for backwards compatibility, or
  - show help if the operator explicitly accepts a breaking change.

For this project, prefer backwards compatibility: no args should behave like `serve`.

### One-shot render without a public server

The CLI render command should start a temporary local-only HTTP server internally:

```text
CLI render command
  |
  | load layout object from YAML/JSON using TypeObjectFromFile
  | marshal layout to JSON
  | net.Listen("tcp", "127.0.0.1:0")
  | registerStaticRoutes(mux, cfg.WebDir)
  | start server in goroutine
  | create Chrome allocator
  | navigate to http://127.0.0.1:<ephemeral>/almanach
  | inject layout
  | apply render capture CSS
  | inspect metrics if requested
  | screenshot selector
  | convert PNG -> bitmap if needed
  | write output artifacts
  | shutdown temp server and Chrome
  v
exit
```

The temporary server is not the product API. It exists only because Chrome needs to load the SPA bundle through HTTP. It should bind to `127.0.0.1`, use port `0` to avoid collisions, and shut down after the command completes.

### Layout input via Glazed TypeObjectFromFile

The layout flag should be defined using Glazed `fields.TypeObjectFromFile`:

```go
fields.New(
    "layout",
    fields.TypeObjectFromFile,
    fields.WithHelp("Layout object file to render. Accepts JSON or YAML."),
)
```

The settings struct should decode it as a map:

```go
type RenderSettings struct {
    Layout map[string]interface{} `glazed:"layout"`
    Out string `glazed:"out"`
    Format string `glazed:"format"`
    Selector string `glazed:"selector"`
    DebugDir string `glazed:"debug-dir"`
}
```

Why `TypeObjectFromFile` matters:

- It lets the caller pass YAML:

  ```bash
  almanach-render-service render --layout daily.yaml --out daily.png
  ```

- It lets the caller pass JSON:

  ```bash
  almanach-render-service render --layout daily.json --out daily.png
  ```

- The command receives an object, not raw text, so the command can validate and normalize it before rendering.

### Accepted layout file forms

The renderer should accept two file shapes.

#### 1. Raw Almanach layout

```yaml
almanach_studio_version: 1
exported_at: "2026-05-08T00:00:00Z"
theme: minimal
paperWidth: 384
bodyScale: 1.6
feedLines: 3
blocks:
  - id: title-1
    type: title
    data:
      text: THE ALMANACH
      subtitle: Friday, May 8, 2026
```

#### 2. Wrapped render request

```yaml
layout:
  theme: minimal
  paperWidth: 384
  bodyScale: 1.6
  feedLines: 3
  blocks:
    - id: title-1
      type: title
      data:
        text: THE ALMANACH
        subtitle: Friday, May 8, 2026
render:
  selector: .paper-body
  threshold: 128
  viewport:
    width: 800
    height: 3000
  debug: true
```

The wrapped form is useful because it lets a layout file also carry render options. Command-line flags should override render options from the file.

### Render options

Introduce a `RenderOptions` struct used by both HTTP and CLI paths:

```go
type RenderOptions struct {
    BaseURL string
    Selector string
    Threshold uint8
    ViewportWidth int
    ViewportHeight int
    WaitAfterLoad time.Duration
    DebugDir string
    CaptureMode string // "body" or "shell" convenience alias, optional
    SavePNG bool
    SaveMetrics bool
}
```

Default values:

```text
Selector:       .paper-body for print-oriented output
Threshold:      128
ViewportWidth:  800
ViewportHeight: 3000
WaitAfterLoad:  250ms after fonts/layout settle
DebugDir:       empty means disabled
```

The HTTP server can keep `.paper-shell` as a compatibility default if desired, but the CLI should default to `.paper-body` because it is the content area without zigzag edges.

### Capture stylesheet

The current capture stylesheet hides topbar/rails but does not fully remove clipping. The refactor should apply a complete render-mode stylesheet before measuring or screenshotting.

Recommended CSS:

```css
html, body, #root {
  margin: 0 !important;
  padding: 0 !important;
  width: fit-content !important;
  height: auto !important;
  min-height: 0 !important;
  overflow: visible !important;
  background: #ffffff !important;
}

.almanach-app {
  background: #ffffff !important;
  height: auto !important;
  min-height: 0 !important;
  overflow: visible !important;
  display: block !important;
}

.almanach-app::before {
  display: none !important;
}

.topbar,
.rail,
.block-controls {
  display: none !important;
}

.workspace {
  display: block !important;
  grid-template-columns: none !important;
  height: auto !important;
  min-height: 0 !important;
  overflow: visible !important;
}

.canvas {
  display: block !important;
  padding: 0 !important;
  margin: 0 !important;
  background: #ffffff !important;
  background-image: none !important;
  width: fit-content !important;
  height: auto !important;
  min-height: 0 !important;
  overflow: visible !important;
}

.paper-shell {
  filter: none !important;
  margin: 0 !important;
  box-shadow: none !important;
}

.block-wrap {
  padding: 4px 0 !important;
  cursor: default !important;
  outline: none !important;
}

.block-wrap::before {
  display: none !important;
}
```

This is the key fix for cutoff debugging. Screenshotting a correct selector still fails if an ancestor clips it. The capture stylesheet must make the document height equal to content height and must remove scroll containers from the render path.

### DOM metrics

The `inspect` command and debug mode should collect metrics for important selectors:

```js
(() => {
  const names = ['.paper-shell', '.paper-body', '.canvas', '.workspace', '.almanach-app'];
  return Object.fromEntries(names.map((sel) => {
    const el = document.querySelector(sel);
    if (!el) return [sel, null];
    const r = el.getBoundingClientRect();
    const cs = getComputedStyle(el);
    return [sel, {
      x: r.x,
      y: r.y,
      width: r.width,
      height: r.height,
      scrollWidth: el.scrollWidth,
      scrollHeight: el.scrollHeight,
      overflow: cs.overflow,
      overflowX: cs.overflowX,
      overflowY: cs.overflowY,
      display: cs.display,
      position: cs.position
    }];
  }));
})()
```

The metrics should be emitted as Glazed rows, and optionally written to `metrics.json` in `--debug-dir`.

## CLI Verb Specifications

### `serve`

Purpose: preserve current long-running HTTP behavior.

Example:

```bash
almanach-render-service serve \
  --port 8199 \
  --web-dir ./web/almanach/dist \
  --printer-ip 192.168.0.126
```

Flags:

| Flag | Type | Default | Description |
|---|---:|---:|---|
| `--port` | integer | env `ALMANACH_PORT` or 8199 | HTTP listen port. |
| `--web-dir` | string | env `ALMANACH_WEB_DIR` or `./web/almanach/dist` | SPA dist directory. |
| `--printer-ip` | string | env `ALMANACH_PRINTER_IP` | ESP32 printer host/IP. |
| `--chrome-path` | string | env `ALMANACH_CHROME_PATH` | Local Chrome executable path. |
| `--chrome-ws-url` | string | env `CHROME_WS_URL` | Remote Chrome websocket URL. |
| `--paper-width` | integer | 384 | Default generated layout width. |
| `--font-scale` | float | 1.6 | Default generated layout font scale. |
| `--feed-lines` | integer | 3 | Default printer feed lines. |
| `--default-theme` | string | minimal | Default generated layout theme. |

Implementation note: `serve` may be a plain Cobra command or a Glazed command. It is long-running and does not need structured output except startup metadata. It should still be under the same root and logging system.

### `render`

Purpose: render a layout once and write a PNG or bitmap file.

Examples:

```bash
# YAML layout to PNG.
almanach-render-service render --layout daily.yaml --out daily.png

# JSON layout to bitmap.
almanach-render-service render \
  --layout daily.json \
  --format bitmap \
  --out daily.bin

# Compare body vs shell capture.
almanach-render-service render --layout daily.yaml --selector .paper-body --out body.png
almanach-render-service render --layout daily.yaml --selector .paper-shell --out shell.png
```

Flags:

| Flag | Glazed type | Default | Description |
|---|---|---:|---|
| `--layout` | `TypeObjectFromFile` | none | JSON/YAML layout object file. If absent, build default live layout. |
| `--out` | `TypeString` | required | Output path for PNG or bitmap. |
| `--format` | `TypeChoice` | `png` | `png` or `bitmap`. |
| `--selector` | `TypeString` | `.paper-body` | CSS selector to screenshot. |
| `--threshold` | `TypeInteger` | 128 | Grayscale threshold for bitmap conversion. |
| `--viewport-width` | `TypeInteger` | 800 | Chrome viewport width. |
| `--viewport-height` | `TypeInteger` | 3000 | Chrome viewport height. |
| `--wait-ms` | `TypeInteger` | 250 | Extra wait after layout injection and fonts. |
| `--debug-dir` | `TypeString` | empty | Directory for screenshot, metrics, layout JSON, and logs. |
| `--web-dir` | `TypeString` | config default | SPA dist directory. |
| `--chrome-path` | `TypeString` | config default | Local Chrome path. |

Output rows:

The command writes the requested artifact to `--out`. It should emit one Glazed row with metadata:

```yaml
artifact: /tmp/almanach.png
format: png
selector: .paper-body
width: 384
height: 1019
bytes: 91402
threshold: 128
rendered_at: 2026-05-08T10:00:00Z
debug_dir: /tmp/almanach-debug
```

Do not emit raw PNG bytes through Glazed stdout by default. That would conflict with structured output and make terminal usage error-prone. Always write image data to `--out`.

### `print`

Purpose: render once and print to the ESP32 device.

Examples:

```bash
almanach-render-service print \
  --layout daily.yaml \
  --printer-ip 192.168.0.126

almanach-render-service print \
  --layout daily.yaml \
  --printer-url http://192.168.0.126/api/print/bitmap \
  --feed-lines 6
```

Flags:

| Flag | Glazed type | Default | Description |
|---|---|---:|---|
| `--layout` | `TypeObjectFromFile` | none | JSON/YAML layout object file. |
| `--printer-ip` | `TypeString` | env | ESP32 host/IP; command builds `http://<ip>/api/print/bitmap`. |
| `--printer-url` | `TypeString` | derived | Full print endpoint, overrides `--printer-ip`. |
| `--feed-lines` | `TypeInteger` | 3 | Lines to feed after print. |
| `--selector` | `TypeString` | `.paper-body` | CSS selector to screenshot. |
| `--threshold` | `TypeInteger` | 128 | Bitmap threshold. |
| `--debug-dir` | `TypeString` | empty | Save render artifacts before printing. |
| `--dry-run` | `TypeBool` | false | Render but do not post to printer. |

Output rows:

```yaml
printed: true
printer_url: http://192.168.0.126/api/print/bitmap
width: 384
height: 1019
bytes: 48912
feed_lines: 3
printer_ok: true
rendered_at: 2026-05-08T10:00:00Z
```

If `--dry-run` is true, `printed` should be false and the row should still report bitmap dimensions.

### `inspect`

Purpose: render setup diagnostics without requiring physical printing.

Examples:

```bash
almanach-render-service inspect --layout daily.yaml --output yaml
almanach-render-service inspect --layout daily.yaml --debug-dir /tmp/almanach-debug
```

Flags:

| Flag | Glazed type | Default | Description |
|---|---|---:|---|
| `--layout` | `TypeObjectFromFile` | none | JSON/YAML layout object file. |
| `--selector` | `TypeString` | `.paper-body` | Selector to validate. |
| `--viewport-width` | `TypeInteger` | 800 | Chrome viewport width. |
| `--viewport-height` | `TypeInteger` | 3000 | Chrome viewport height. |
| `--debug-dir` | `TypeString` | empty | Save metrics JSON and screenshot. |

Output rows should include one row per inspected selector:

```yaml
selector: .paper-body
found: true
x: 0
y: 0
width: 384
height: 1019
scroll_width: 384
scroll_height: 1019
overflow: visible
overflow_x: visible
overflow_y: visible
display: block
position: relative
```

This command is the primary debugging tool for cutoff issues.

## Glazed Implementation Guide

### Required dependencies

Add Glazed and Cobra if not already present:

```bash
go get github.com/go-go-golems/glazed
go get github.com/spf13/cobra
```

Use these import paths:

```go
import (
    "github.com/go-go-golems/glazed/pkg/cli"
    "github.com/go-go-golems/glazed/pkg/cmds"
    "github.com/go-go-golems/glazed/pkg/cmds/fields"
    "github.com/go-go-golems/glazed/pkg/cmds/logging"
    "github.com/go-go-golems/glazed/pkg/cmds/schema"
    "github.com/go-go-golems/glazed/pkg/cmds/values"
    "github.com/go-go-golems/glazed/pkg/help"
    help_cmd "github.com/go-go-golems/glazed/pkg/help/cmd"
    "github.com/go-go-golems/glazed/pkg/middlewares"
    "github.com/go-go-golems/glazed/pkg/settings"
    "github.com/go-go-golems/glazed/pkg/types"
    "github.com/spf13/cobra"
)
```

Common mistake: do not import old or plausible-looking paths such as `glazed/pkg/cmds/parameters/fields`; use `glazed/pkg/cmds/fields`.

### Root command pattern

The root should follow the Glazed conventions:

```go
func NewRootCommand(version string) (*cobra.Command, error) {
    rootCmd := &cobra.Command{
        Use:     "almanach-render-service",
        Short:   "Render and print Almanach Studio thermal pages",
        Version: version,
        PersistentPreRunE: func(cmd *cobra.Command, args []string) error {
            return logging.InitLoggerFromCobra(cmd)
        },
    }

    if err := logging.AddLoggingSectionToRootCommand(rootCmd, "almanach-render-service"); err != nil {
        return nil, err
    }

    helpSystem := help.NewHelpSystem()
    // Later: add embedded docs package if desired.
    help_cmd.SetupCobraRootCommand(helpSystem, rootCmd)

    serveCmd, err := NewServeCobraCommand()
    if err != nil { return nil, err }
    rootCmd.AddCommand(serveCmd)

    renderCmd, err := NewRenderCommand()
    if err != nil { return nil, err }
    renderCobra, err := cli.BuildCobraCommandFromCommand(renderCmd,
        cli.WithParserConfig(cli.CobraParserConfig{
            AppName: "almanach-render-service",
            ShortHelpSections: []string{schema.DefaultSlug},
            MiddlewaresFunc: cli.CobraCommandDefaultMiddlewares,
        }),
    )
    if err != nil { return nil, err }
    rootCmd.AddCommand(renderCobra)

    // Same for print and inspect.
    return rootCmd, nil
}
```

### Render command skeleton

```go
type RenderCommand struct {
    *cmds.CommandDescription
}

type RenderSettings struct {
    Layout map[string]interface{} `glazed:"layout"`
    Out string `glazed:"out"`
    Format string `glazed:"format"`
    Selector string `glazed:"selector"`
    Threshold int `glazed:"threshold"`
    ViewportWidth int `glazed:"viewport-width"`
    ViewportHeight int `glazed:"viewport-height"`
    WaitMS int `glazed:"wait-ms"`
    DebugDir string `glazed:"debug-dir"`
    WebDir string `glazed:"web-dir"`
    ChromePath string `glazed:"chrome-path"`
}

func NewRenderCommand() (*RenderCommand, error) {
    glazedSection, err := settings.NewGlazedSchema()
    if err != nil { return nil, err }

    commandSettingsSection, err := cli.NewCommandSettingsSection()
    if err != nil { return nil, err }

    desc := cmds.NewCommandDescription(
        "render",
        cmds.WithShort("Render an Almanach layout once to PNG or bitmap"),
        cmds.WithLong(`Render a JSON or YAML Almanach Studio layout using Chrome headless.

Examples:
  almanach-render-service render --layout daily.yaml --out daily.png
  almanach-render-service render --layout daily.yaml --format bitmap --out daily.bin
  almanach-render-service render --layout daily.yaml --selector .paper-shell --debug-dir /tmp/almanach-debug
`),
        cmds.WithFlags(
            fields.New("layout", fields.TypeObjectFromFile,
                fields.WithHelp("Layout object file to render. Accepts JSON or YAML.")),
            fields.New("out", fields.TypeString,
                fields.WithHelp("Output artifact path")),
            fields.New("format", fields.TypeChoice,
                fields.WithDefault("png"),
                fields.WithChoices("png", "bitmap"),
                fields.WithHelp("Output format")),
            fields.New("selector", fields.TypeString,
                fields.WithDefault(".paper-body"),
                fields.WithHelp("CSS selector to screenshot")),
            fields.New("threshold", fields.TypeInteger,
                fields.WithDefault(128),
                fields.WithHelp("Grayscale threshold for bitmap conversion")),
            fields.New("viewport-width", fields.TypeInteger,
                fields.WithDefault(800),
                fields.WithHelp("Chrome viewport width")),
            fields.New("viewport-height", fields.TypeInteger,
                fields.WithDefault(3000),
                fields.WithHelp("Chrome viewport height")),
            fields.New("wait-ms", fields.TypeInteger,
                fields.WithDefault(250),
                fields.WithHelp("Extra wait after loading layout")),
            fields.New("debug-dir", fields.TypeString,
                fields.WithDefault(""),
                fields.WithHelp("Directory for debug artifacts")),
            fields.New("web-dir", fields.TypeString,
                fields.WithDefault("./web/almanach/dist"),
                fields.WithHelp("SPA dist directory")),
            fields.New("chrome-path", fields.TypeString,
                fields.WithDefault(""),
                fields.WithHelp("Chrome/Chromium executable path")),
        ),
        cmds.WithSections(glazedSection, commandSettingsSection),
    )

    return &RenderCommand{CommandDescription: desc}, nil
}

func (c *RenderCommand) RunIntoGlazeProcessor(
    ctx context.Context,
    vals *values.Values,
    gp middlewares.Processor,
) error {
    s := &RenderSettings{}
    if err := vals.DecodeSectionInto(schema.DefaultSlug, s); err != nil {
        return err
    }

    if s.Out == "" {
        return fmt.Errorf("--out is required")
    }

    layoutJSON, err := layoutJSONFromObjectOrDefault(ctx, s.Layout, loadConfig())
    if err != nil { return err }

    rr, err := renderOneShot(ctx, OneShotRenderRequest{
        LayoutJSON: layoutJSON,
        WebDir: s.WebDir,
        ChromePath: s.ChromePath,
        Options: RenderOptions{
            Selector: s.Selector,
            Threshold: uint8(s.Threshold),
            ViewportWidth: s.ViewportWidth,
            ViewportHeight: s.ViewportHeight,
            WaitAfterLoad: time.Duration(s.WaitMS) * time.Millisecond,
            DebugDir: s.DebugDir,
        },
    })
    if err != nil { return err }

    switch s.Format {
    case "png":
        err = os.WriteFile(s.Out, rr.PNG, 0644)
    case "bitmap":
        err = os.WriteFile(s.Out, rr.Bitmap.Data, 0644)
    default:
        err = fmt.Errorf("unsupported format %q", s.Format)
    }
    if err != nil { return err }

    return gp.AddRow(ctx, types.NewRow(
        types.MRP("artifact", s.Out),
        types.MRP("format", s.Format),
        types.MRP("selector", s.Selector),
        types.MRP("width", rr.Bitmap.Width),
        types.MRP("height", rr.Bitmap.Height),
        types.MRP("bytes", len(rr.PNG)),
        types.MRP("threshold", s.Threshold),
        types.MRP("rendered_at", rr.RenderedAt),
        types.MRP("debug_dir", s.DebugDir),
    ))
}
```

### Layout object conversion

`TypeObjectFromFile` gives the command a parsed object. The renderer still needs a JSON string to pass to `window.almanachLoadLayout`. Implement a conversion helper:

```go
func layoutJSONFromObjectOrDefault(ctx context.Context, obj map[string]interface{}, cfg Config) (string, error) {
    if len(obj) == 0 {
        layout, err := buildDefaultLayout(cfg)
        if err != nil { return "", err }
        b, err := json.Marshal(layout)
        if err != nil { return "", err }
        return string(b), nil
    }

    // Accept wrapped render request: { layout: {...}, render: {...} }
    if layoutObj, ok := obj["layout"]; ok {
        b, err := json.Marshal(layoutObj)
        if err != nil { return "", err }
        return string(b), nil
    }

    // Accept raw Almanach layout object.
    b, err := json.Marshal(obj)
    if err != nil { return "", err }
    return string(b), nil
}
```

Also validate that the final layout has a `blocks` array. Do not reject all unknown fields because the frontend can ignore harmless metadata, but do fail fast when `blocks` is absent or not an array in a non-empty file.

### One-shot ephemeral server pseudocode

```go
type OneShotRenderRequest struct {
    LayoutJSON string
    WebDir string
    ChromePath string
    ChromeWSURL string
    Options RenderOptions
}

func renderOneShot(ctx context.Context, req OneShotRenderRequest) (*RenderResult, error) {
    mux := http.NewServeMux()
    registerStaticRoutes(mux, req.WebDir)

    ln, err := net.Listen("tcp", "127.0.0.1:0")
    if err != nil { return nil, err }
    defer ln.Close()

    server := &http.Server{Handler: mux}
    go func() {
        if err := server.Serve(ln); err != nil && err != http.ErrServerClosed {
            log.Printf("ephemeral render server error: %v", err)
        }
    }()
    defer server.Shutdown(context.Background())

    cfg := loadConfig()
    cfg.Port = 0
    cfg.WebDir = req.WebDir
    cfg.ChromePath = req.ChromePath
    cfg.ChromeWSURL = req.ChromeWSURL

    allocCtx, allocCancel := newChromeAllocatorWithViewport(cfg, req.Options.ViewportWidth, req.Options.ViewportHeight)
    defer allocCancel()

    renderer := &Renderer{allocatorCtx: allocCtx}
    req.Options.BaseURL = "http://" + ln.Addr().String()
    return renderer.RenderLayout(ctx, req.LayoutJSON, req.Options)
}
```

This pseudocode intentionally separates a reusable renderer from the HTTP server struct. The current `Server.renderWithChrome` method should become a method on a smaller renderer type, or a function that accepts allocator context and options. That reduces coupling between one-shot CLI rendering and HTTP request handling.

## Renderer Refactor Design

### New core types

```go
type ChromeRenderer struct {
    allocatorCtx context.Context
}

type RenderOptions struct {
    BaseURL string
    Selector string
    Threshold uint8
    ViewportWidth int
    ViewportHeight int
    WaitAfterLoad time.Duration
    DebugDir string
    CaptureCSS string
    CollectMetrics bool
}

type RenderMetrics map[string]*ElementMetrics

type ElementMetrics struct {
    X float64 `json:"x"`
    Y float64 `json:"y"`
    Width float64 `json:"width"`
    Height float64 `json:"height"`
    ScrollWidth int `json:"scrollWidth"`
    ScrollHeight int `json:"scrollHeight"`
    Overflow string `json:"overflow"`
    OverflowX string `json:"overflowX"`
    OverflowY string `json:"overflowY"`
    Display string `json:"display"`
    Position string `json:"position"`
}
```

### New render flow

```text
RenderLayout(ctx, layoutJSON, opts)
  validate opts and apply defaults
  new Chrome tab
  navigate opts.BaseURL + "/almanach"
  wait for window.almanachReady === true
  evaluate window.almanachLoadLayout(layoutObject)
  wait for React + fonts + two animation frames
  inject capture CSS
  collect metrics before screenshot
  ensure selector exists and has non-zero size
  screenshot opts.Selector
  save debug screenshot and metrics if requested
  convert PNG to bitmap using opts.Threshold
  return RenderResult{PNG, Bitmap, Metrics, LayoutJSON}
```

Use this wait instead of fixed sleeps when possible:

```js
new Promise(async (resolve) => {
  if (document.fonts && document.fonts.ready) await document.fonts.ready;
  requestAnimationFrame(() => requestAnimationFrame(resolve));
})
```

In `chromedp`, that can be done with `chromedp.Evaluate` returning after the promise resolves.

### Safer layout injection

Current code does:

```go
chromedp.Evaluate(fmt.Sprintf(`window.almanachLoadLayout(%s)`, layoutJSON), nil)
```

This works if `layoutJSON` is valid JavaScript object literal text, but it is safer to pass it as a quoted JSON string or use `json.Marshal` for the argument:

```go
arg, err := json.Marshal(layoutJSON)
// arg is a JSON string containing the layout JSON text
expr := fmt.Sprintf(`window.almanachLoadLayout(%s)`, arg)
```

Or parse before passing:

```go
expr := fmt.Sprintf(`window.almanachLoadLayout(JSON.parse(%s))`, arg)
```

This avoids accidental JavaScript syntax issues when the layout contains characters that are valid JSON but awkward in an inline expression.

## Schema Alignment Work

Before or during CLI implementation, align Go-generated default layouts with the frontend schema. This matters because the CLI default path should generate the same kind of data that the UI exports.

### Current mismatch examples

| Current Go shape | Frontend expects | Effect |
|---|---|---|
| `TitleData{Title}` | `data.text` | Title text may not appear as intended. |
| `WordData{PartOfSpeech}` | `data.part` | Part of speech may be blank. |
| `HistoryData{Year, Event}` | `data.items: [{year,event}]` | Renderer expects a list. |
| type `did_you_know` | type `did` | Frontend filters out unknown block type. |
| `DidYouKnowData{Text}` | `data.items: []string` | Renderer expects a list of facts. |
| `NewsItem{Summary}` | `time` in frontend default | Summary is unused; time may be blank. |

### Recommended structs

```go
type TitleData struct {
    Text string `json:"text"`
    Subtitle string `json:"subtitle"`
}

type WeatherData struct {
    Temp string `json:"temp"`
    High string `json:"high"`
    Low string `json:"low"`
    Condition string `json:"condition"`
    Sunrise string `json:"sunrise,omitempty"`
    Sunset string `json:"sunset,omitempty"`
}

type NewsData struct {
    Label string `json:"label"`
    Items []NewsItem `json:"items"`
}

type NewsItem struct {
    Headline string `json:"headline"`
    Source string `json:"source"`
    Time string `json:"time,omitempty"`
}

type WordData struct {
    Label string `json:"label"`
    Word string `json:"word"`
    Phonetic string `json:"phonetic,omitempty"`
    Part string `json:"part"`
    Definition string `json:"definition"`
    Example string `json:"example,omitempty"`
}

type HistoryData struct {
    Label string `json:"label"`
    Items []HistoryItem `json:"items"`
}

type HistoryItem struct {
    Year string `json:"year"`
    Event string `json:"event"`
}

type DidData struct {
    Label string `json:"label"`
    Items []string `json:"items"`
}
```

### Default layout rules

- Use `theme: minimal` unless configured otherwise.
- Use `paperWidth: 384` by default.
- Use `bodyScale: 1.6` by default.
- Use `feedLines: 3` by default.
- Generate block types exactly as frontend expects.
- Keep default content short enough to fit a reasonable thermal page.

## File-Level Implementation Plan

### 1. `main.go`

Refactor from direct server startup to root command execution.

Before:

```go
func main() {
    cfg := loadConfig()
    // start server immediately
}
```

After:

```go
func main() {
    rootCmd, err := NewRootCommand(Version)
    cobra.CheckErr(err)
    cobra.CheckErr(rootCmd.Execute())
}
```

If preserving no-arg server behavior, add this in root setup:

```go
rootCmd.RunE = func(cmd *cobra.Command, args []string) error {
    return runServe(cmd.Context(), loadConfig())
}
```

### 2. `cmd_root.go` or `cmds/root.go`

Add Glazed/Cobra root setup:

- `logging.AddLoggingSectionToRootCommand`
- `logging.InitLoggerFromCobra`
- `help.NewHelpSystem`
- `help_cmd.SetupCobraRootCommand`
- register `serve`, `render`, `print`, `inspect`

Recommended directory layout for clarity:

```text
cmd/almanach-render-service/
  main.go
  cmd_root.go
  cmd_serve.go
  cmd_render.go
  cmd_print.go
  cmd_inspect.go
  render_oneshot.go
  renderer.go
  layout.go
  ...existing files...
```

Because this is already a `main` package in a small command directory, keeping files in the same package is acceptable. If the code grows, later extract to `internal/render` and `internal/cli`.

### 3. `cmd_serve.go`

Move the current server startup code into:

```go
func runServe(ctx context.Context, cfg Config) error
```

The `serve` command decodes flags and env defaults into `Config`, then calls `runServe`.

### 4. `cmd_render.go`

Implement the Glazed `RenderCommand` with `TypeObjectFromFile` layout input. It should:

1. Decode settings.
2. Convert layout object to JSON or build default layout.
3. Run `renderOneShot`.
4. Write output file.
5. Emit metadata row.

### 5. `cmd_print.go`

Implement `PrintCommand` similarly, but always converts to bitmap and calls `sendBitmapToPrinter` unless `--dry-run` is true.

### 6. `cmd_inspect.go`

Implement `InspectCommand` to run the same browser setup and capture CSS but focus on metrics. It should not require `--out`. It should emit rows for each selector.

### 7. `render_oneshot.go`

Add ephemeral server and one-shot renderer orchestration. Keep this separate from `server.go` so the HTTP API code stays readable.

### 8. `renderer.go`

Refactor `renderWithChrome` to accept:

- base URL
- selector
- threshold
- viewport dimensions
- debug directory
- metrics flag

Do not keep hardcoded `http://localhost:%d/almanach` inside the core renderer.

### 9. `config.go`

Add helpers for config merging:

```go
func loadConfigFromEnv() Config
func (c Config) WithCLIOverrides(s CommonSettings) Config
```

Avoid scattering `envStr` calls across commands.

### 10. `README.md`

Document both modes:

- Server mode.
- One-shot CLI mode.
- Layout YAML examples.
- Debugging cutoff output.

### 11. `plugins/almanach-render.py`

Update devctl custom commands to call the new CLI verbs directly where appropriate:

- `devctl render` can call `./almanach-render-service render --out /tmp/almanach-render.png` instead of `curl`.
- `devctl print` can call `./almanach-render-service print --printer-ip ...`.
- `devctl up` should still use `serve` for long-running supervised service.

## Testing and Validation Plan

### Unit-level tests

Add tests for layout conversion:

```text
TestLayoutJSONFromObject_RawJSONShape
TestLayoutJSONFromObject_WrappedRequestShape
TestLayoutJSONFromObject_EmptyBuildsDefault
TestLayoutJSONFromObject_RejectsMissingBlocks
```

Add tests for render option defaults:

```text
TestRenderOptionsDefaults
TestRenderOptionsValidateSelectorRequired
TestRenderOptionsThresholdRange
```

Add tests for bitmap conversion if not already present:

```text
TestPngToBitmap_PadsWidthToMultipleOf8
TestPngToBitmap_BlackIsOneWhiteIsZero
TestPngToBitmap_ThresholdBoundary
```

### CLI smoke tests

Create a tiny `testdata/layout-minimal.yaml`:

```yaml
almanach_studio_version: 1
theme: minimal
paperWidth: 384
bodyScale: 1.6
feedLines: 3
blocks:
  - id: t1
    type: title
    data:
      text: TEST ALMANACH
      subtitle: CLI smoke test
  - id: d1
    type: date
    data:
      date: May 8, 2026
      day: Friday
```

Run:

```bash
go build -o almanach-render-service .

./almanach-render-service render \
  --layout testdata/layout-minimal.yaml \
  --out /tmp/almanach-cli.png \
  --debug-dir /tmp/almanach-cli-debug \
  --output yaml

identify /tmp/almanach-cli.png
ls -la /tmp/almanach-cli-debug

./almanach-render-service inspect \
  --layout testdata/layout-minimal.yaml \
  --output yaml

./almanach-render-service print \
  --layout testdata/layout-minimal.yaml \
  --printer-ip 192.168.0.126 \
  --dry-run \
  --output yaml
```

Expected results:

- PNG width is 384 when selector is `.paper-body` or `.paper-shell` with 384 paper width.
- PNG contains only black/white content on white background after conversion path.
- Inspect shows `.paper-body` found with non-zero height.
- `.canvas`, `.workspace`, and `.almanach-app` report `overflow: visible` after render CSS injection.
- Dry-run print reports bitmap dimensions but does not contact the printer.

### Physical printer smoke test

Only after PNG preview looks correct:

```bash
./almanach-render-service print \
  --layout testdata/layout-minimal.yaml \
  --printer-ip 192.168.0.126 \
  --feed-lines 6 \
  --debug-dir /tmp/almanach-print-debug \
  --output yaml
```

Validation checklist:

- Top of page is not clipped.
- Bottom of page is not clipped.
- No editor rails/topbar are printed.
- No drop shadow is printed.
- Text is black on white, no gray background.
- Width matches printer paper without horizontal clipping.
- Feed lines leave enough paper for tear-off.

## Design Decisions

### Decision 1: Use Glazed commands instead of ad-hoc `flag` parsing

Glazed provides consistent command metadata, typed fields, structured outputs, shell-friendly formats, and a help system. It also provides `TypeObjectFromFile`, which is the key feature for accepting both YAML and JSON layout files without writing custom file parsing for each command.

### Decision 2: Use `TypeObjectFromFile` for `--layout`

The layout is an object, not a string. Reading it as an object lets the command validate and normalize it. It also means YAML works naturally. This is better than `TypeStringFromFile`, which would force every command to parse JSON/YAML manually.

### Decision 3: Start an ephemeral localhost server for CLI rendering

Chrome still needs to load the React SPA and bundle. The current HTML references `/almanach/bundle.js`, which expects HTTP. An ephemeral `127.0.0.1:0` server is simpler and safer than creating file-mode HTML rewrites. It is invisible to the user and exits with the command.

### Decision 4: Default CLI capture selector should be `.paper-body`

For thermal printing, the body content is what matters. `.paper-shell` includes decorative zigzag edges. Keeping `--selector` makes both available, but `.paper-body` is the better print default.

### Decision 5: Keep server mode

The service still needs HTTP mode for automation, scheduling, Docker Compose, devctl supervision, and remote integrations. CLI mode supplements server mode; it does not replace it.

### Decision 6: Emit metadata rows, not binary bytes, through Glazed

Glazed output is structured data. PNG and bitmap data should be written to files. The command emits rows describing artifacts. This avoids accidental binary output in terminals and keeps `--output json|yaml|table` useful.

## Alternatives Considered

### Alternative: Use `curl` wrappers only

This is what devctl currently does. It avoids Go refactoring but does not solve local iteration pain. It still requires a long-running server and cannot easily expose render internals.

### Alternative: Use `file://` instead of an ephemeral server

This would remove even the internal localhost server, but it requires rewriting the host HTML or changing bundle paths. It is more fragile than serving the same assets the HTTP server already serves.

### Alternative: Reimplement rendering directly in Go

This would avoid Chrome, but it would duplicate the React/CSS renderer and likely diverge from the UI. The browser is the correct rendering engine because the SPA is the source of truth.

### Alternative: Use `window.almanachExportBitmap()` again

The frontend headless API includes `almanachExportBitmap`, but prior attempts using SVG `foreignObject` and canvas hit tainting problems in Chrome headless. The current screenshot-based path is more reliable. We can revisit direct frontend bitmap export later, but the CLI should build on the working screenshot path.

## Implementation Order

Implement in this order to keep the code reviewable:

1. **Schema alignment patch**: update Go layout structs and default block types to match frontend expectations.
2. **Renderer options patch**: introduce `RenderOptions`, selector, threshold, capture CSS, and metrics collection while preserving HTTP behavior.
3. **Ephemeral render helper**: add `renderOneShot` and verify it can render a PNG from a hardcoded layout in a temporary test or small helper.
4. **Glazed root and serve command**: refactor `main.go` so existing behavior is still available.
5. **Render command**: add `TypeObjectFromFile` layout loading and PNG/bitmap artifact writing.
6. **Inspect command**: expose DOM metrics and debug artifacts.
7. **Print command**: reuse render command internals and call `sendBitmapToPrinter`.
8. **Documentation and devctl update**: update README and devctl plugin.
9. **End-to-end smoke test**: render YAML to PNG, inspect metrics, dry-run print, then physical print.

## Cutoff Debugging Playbook

When the print is cut off, use this sequence:

1. Render `.paper-body`:

   ```bash
   almanach-render-service render --layout daily.yaml --selector .paper-body --out /tmp/body.png --debug-dir /tmp/body-debug
   ```

2. Render `.paper-shell`:

   ```bash
   almanach-render-service render --layout daily.yaml --selector .paper-shell --out /tmp/shell.png --debug-dir /tmp/shell-debug
   ```

3. Inspect DOM metrics:

   ```bash
   almanach-render-service inspect --layout daily.yaml --output yaml
   ```

4. Compare:

   - If `.paper-body.height` is smaller than expected, the layout itself is not rendering all content.
   - If `.paper-body.scrollHeight` is greater than `.paper-body.height`, the body is clipping internally.
   - If `.canvas` or `.workspace` still has `overflow: hidden`, capture CSS is not applied correctly.
   - If `.paper-shell.width` is not 384, the layout did not apply `paperWidth` correctly.
   - If `.paper-shell` is taller than `.paper-body` by about the zigzag edge heights, that is expected.

5. Only print after PNG preview is correct.

## API References

### Existing HTTP render API

```bash
curl -X POST http://localhost:8199/api/render \
  -H 'Content-Type: application/json' \
  -H 'Accept: image/png' \
  --data @layout.json \
  -o page.png
```

`Accept` controls response format:

- `image/png` returns PNG screenshot.
- `application/octet-stream` returns packed bitmap with `X-Width` and `X-Height` headers.
- default returns JSON metadata.

### Existing printer API

```bash
curl -X POST http://192.168.0.126/api/print/bitmap \
  -H 'Content-Type: application/octet-stream' \
  -H 'X-Width: 384' \
  -H 'X-Height: 1019' \
  -H 'X-Feed: 3' \
  --data-binary @page.bin
```

### New CLI APIs

```bash
almanach-render-service render --layout layout.yaml --out page.png
almanach-render-service render --layout layout.yaml --format bitmap --out page.bin
almanach-render-service inspect --layout layout.yaml --output yaml
almanach-render-service print --layout layout.yaml --printer-ip 192.168.0.126
```

## Example YAML Layout

```yaml
almanach_studio_version: 1
exported_at: "2026-05-08T10:00:00Z"
theme: minimal
paperWidth: 384
bodyScale: 1.6
feedLines: 3
blocks:
  - id: title-1
    type: title
    data:
      text: THE ALMANACH
      subtitle: Friday, May 8, 2026

  - id: date-1
    type: date
    data:
      date: May 8, 2026
      day: Friday

  - id: weather-1
    type: weather
    data:
      temp: 22°C
      condition: Partly cloudy
      high: 24°C
      low: 15°C
      sunrise: "05:47"
      sunset: "20:32"

  - id: news-1
    type: news
    data:
      label: Top News
      items:
        - headline: Local CLI workflow replaces repeated curl commands.
          source: Almanach Dev Notes
          time: now
        - headline: Selector inspection makes clipping bugs visible.
          source: Render Lab
          time: now

  - id: quote-1
    type: quote
    data:
      label: Quote of the Day
      text: Simplicity is prerequisite for reliability.
      author: Edsger W. Dijkstra
```

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| Glazed dependency changes module graph significantly | Add dependency in one commit; run `go mod tidy`; keep command code isolated. |
| Binary output accidentally goes to stdout | Require `--out` for `render`; only emit structured metadata rows. |
| CLI breaks existing server users | Keep no-arg behavior or document `serve`; ensure Docker still starts server. |
| YAML object decoding produces `map[interface{}]interface{}` style data | Test actual Glazed `TypeObjectFromFile` behavior; normalize via JSON marshal/unmarshal into `map[string]interface{}` if needed. |
| Chrome viewport still clips content | Apply full capture CSS and collect metrics before screenshot. |
| Frontend schema drift continues | Document frontend `DEFAULTS` as source of truth and add fixture tests based on exported layouts. |
| Debug artifacts leak into repo | Default debug dir should be outside repo or user-specified; add patterns to `.gitignore` if needed. |

## Definition of Done

This ticket is complete when:

- `almanach-render-service serve` starts the existing HTTP API server.
- Existing Docker and devctl server workflows still work.
- `almanach-render-service render --layout layout.yaml --out page.png` produces a PNG without starting a persistent server.
- `--layout` accepts both JSON and YAML through Glazed `TypeObjectFromFile`.
- `almanach-render-service render --format bitmap` writes a valid packed bitmap.
- `almanach-render-service print --layout layout.yaml --printer-ip 192.168.0.126` renders and prints.
- `almanach-render-service inspect --layout layout.yaml --output yaml` prints DOM metrics for paper and clipping containers.
- Debug mode writes layout JSON, screenshot PNG, metrics JSON, and possibly bitmap output to `--debug-dir`.
- The default print-oriented selector is `.paper-body`, and `.paper-shell` is available by flag.
- The renderer no longer clips due to `.almanach-app`, `.workspace`, or `.canvas` overflow constraints.
- README documents the CLI workflows.

## References

- `stoms3r/cmd/almanach-render-service/main.go` — current server-only entry point.
- `stoms3r/cmd/almanach-render-service/server.go` — current HTTP route registration and render handlers.
- `stoms3r/cmd/almanach-render-service/renderer.go` — Chrome headless rendering and screenshot logic.
- `stoms3r/cmd/almanach-render-service/layout.go` — default layout generation and schema structs.
- `stoms3r/cmd/almanach-render-service/static.go` — static SPA serving logic to reuse for ephemeral server.
- `stoms3r/cmd/almanach-render-service/bitmap.go` — PNG to packed bitmap conversion.
- `stoms3r/cmd/almanach-render-service/printer.go` — ESP32 bitmap POST client.
- `stoms3r/web/almanach/src/almanach-studio.jsx` — frontend layout schema, renderers, and headless API.
- Glazed tutorial: `/home/manuel/code/wesen/corporate-headquarters/glazed/pkg/doc/tutorials/05-build-first-command.md`.
- Glazed field type example: `/home/manuel/code/wesen/corporate-headquarters/glazed/cmd/examples/field-types/main.go`.
- Glazed root command example: `/home/manuel/code/wesen/corporate-headquarters/glazed/cmd/glaze/main.go`.
