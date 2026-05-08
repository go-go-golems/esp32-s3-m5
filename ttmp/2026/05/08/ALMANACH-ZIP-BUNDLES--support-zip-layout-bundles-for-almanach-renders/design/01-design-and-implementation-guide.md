---
Title: Design and Implementation Guide
Ticket: ALMANACH-ZIP-BUNDLES
Status: active
Topics:
    - almanach
    - rendering
    - assets
DocType: design
Intent: long-term
Owners: []
RelatedFiles:
    - Path: stoms3r/cmd/almanach-render-service/cmd_inspect.go
      Note: Inspect command layout flag contract
    - Path: stoms3r/cmd/almanach-render-service/cmd_print.go
      Note: Print command layout flag contract
    - Path: stoms3r/cmd/almanach-render-service/cmd_render.go
      Note: Render command layout flag contract
    - Path: stoms3r/cmd/almanach-render-service/layout_bundle.go
      Note: Implemented bundle design (commit 0f2244e)
    - Path: stoms3r/cmd/almanach-render-service/layout_bundle_test.go
      Note: Tests for implemented bundle design (commit 0f2244e)
    - Path: stoms3r/cmd/almanach-render-service/render_oneshot.go
      Note: Existing layout JSON normalization helper
ExternalSources: []
Summary: Design for ZIP layout bundles that contain layout YAML/JSON plus relative image assets.
LastUpdated: 2026-05-08T09:55:00-04:00
WhatFor: Use this before changing Almanach CLI layout loading or asset resolution.
WhenToUse: When implementing, reviewing, or extending ZIP bundle support for render/inspect/print commands.
---



# Design and Implementation Guide

## Goal

Almanach layouts currently support images by embedding data URLs in `image.src`. This is portable, but it makes YAML files large and hard to review. ZIP layout bundles should allow a layout to reference local image files by relative path while keeping the command-line render interface self-contained.

The target bundle shape is:

```text
layout.zip
├── layout.yaml          # or layout.yml / layout.json / almanach.yaml / almanach.json
└── images/
    ├── fox.png
    ├── owl.png
    └── hedgehog.png
```

The layout should be able to say:

```yaml
blocks:
  - id: fox
    type: image
    data:
      src: images/fox.png
      height: 56
      fit: contain
      border: false
```

The CLI loader rewrites that relative source to a data URL before passing layout JSON to the browser. The browser-side renderer does not need to know whether the original source came from a ZIP member or an already embedded data URL.

## Non-goals

This ticket does not implement a browser UI for opening ZIP files. It is CLI-focused: `render`, `inspect`, and `print` should accept either a standalone YAML/JSON layout or a ZIP bundle path.

This ticket does not implement asset caching, external filesystem serving, or an asset manifest format. The first implementation should inline image bytes as data URLs because the existing image block already supports them and the headless renderer already waits for images.

This ticket does not add upload-time downscaling or dithering. Those are separate print-quality and layout-size improvements.

## CLI contract

The existing user-visible flag remains `--layout`. It should now accept either:

- no value: build the default live-data layout,
- a standalone `.yaml`, `.yml`, or `.json` file,
- a `.zip` file containing one layout file and referenced assets.

Examples:

```bash
./almanach-render-service render \
  --layout ./examples/layouts/08-image-block.yaml \
  --out /tmp/page.png

./almanach-render-service render \
  --layout ./examples/bundles/sqlite-animals.zip \
  --out /tmp/page.png
```

The same flag should work for:

```bash
./almanach-render-service inspect --layout bundle.zip --output yaml
./almanach-render-service print --layout bundle.zip --dry-run --output yaml
```

## Bundle layout-file discovery

The loader should find the layout member with deterministic rules:

1. Prefer root-level names in this order:
   - `layout.yaml`
   - `layout.yml`
   - `layout.json`
   - `almanach.yaml`
   - `almanach.yml`
   - `almanach.json`
2. If no preferred name exists, choose the single root-level `.yaml`, `.yml`, or `.json` file.
3. If there are zero candidates, return an error.
4. If there are multiple unpreferred candidates, return an error and ask for one of the preferred names.

The first implementation does not need a `manifest.json`.

## Asset rewriting rule

Only image-block `data.src` values are rewritten. The loader should traverse the normalized layout object and find blocks where:

```text
block.type == "image"
block.data.src is a string
```

A `src` value is left unchanged when it is already one of:

- `data:...`
- `http://...`
- `https://...`
- absolute path-like values beginning with `/`

A `src` value is treated as a ZIP-relative asset path when it is a relative string, for example:

```text
images/fox.png
./images/fox.png
```

The path is cleaned and resolved relative to the layout member directory. For example:

```text
layout member: cards/sqlite/layout.yaml
src: ../images/fox.png
resolved member: cards/images/fox.png
```

For the initial bundle contract, root-level `layout.yaml` and `images/foo.png` are sufficient, but supporting relative resolution makes nested layouts predictable.

## ZIP safety rules

The loader must not extract the ZIP archive to disk. It should read members in memory from `archive/zip`.

The loader should reject paths that escape the ZIP namespace after cleaning. In practice:

- normalize separators to `/`,
- strip a leading `./`,
- clean the path with `path.Clean`,
- reject `..`, paths beginning with `../`, and absolute paths.

This prevents ambiguous asset references and avoids path traversal concerns even though no files are extracted.

## MIME type rule

When an asset is inlined, the loader should set an appropriate data URL MIME type.

Use `mime.TypeByExtension` first. If it returns empty, use `http.DetectContentType` on the bytes. The resulting URL is:

```text
data:<mime>;base64,<payload>
```

This supports PNG, JPEG, GIF, WebP, and SVG without adding custom cases.

## Implementation plan

1. Change command settings for `render`, `inspect`, and `print` so `--layout` is a string path rather than `objectFromFile`.
2. Add a loader function that returns:
   - layout JSON string,
   - optional render options from wrapped layouts,
   - metadata about the source kind.
3. Keep `layoutJSONFromObjectOrDefault` for already-decoded objects or refactor it into a helper that accepts `map[string]interface{}`.
4. For standalone YAML/JSON, parse the file into `map[string]interface{}` using `encoding/json` or `gopkg.in/yaml.v3`.
5. For ZIP files:
   - open with `zip.OpenReader`,
   - locate layout member,
   - parse it,
   - rewrite relative image sources to data URLs,
   - pass the resulting object to the same normalization helper.
6. Update help text for `render`, `inspect`, and `print`.
7. Add unit tests for:
   - standalone YAML loading,
   - ZIP layout member discovery,
   - relative image rewrite,
   - preservation of existing data URLs and HTTP URLs,
   - rejection of ambiguous layout members.
8. Validate with a real bundle built from the SQLite animal layout.

## Expected code locations

```text
cmd_render.go       # RenderSettings layout field and help text
cmd_inspect.go      # InspectSettings layout field and help text
cmd_print.go        # PrintSettings layout field and help text
render_oneshot.go   # Existing object-to-layout JSON normalization helper
layout_bundle.go    # New loader for file/ZIP layouts
layout_bundle_test.go
```

## Review checklist

- `--layout existing.yaml` still works.
- `--layout bundle.zip` works for render, inspect, and print dry-run.
- Image sources in ZIP bundles are inlined as data URLs before browser render.
- Existing explicit `data:` and remote `http(s):` image sources are not modified.
- ZIP members are not extracted to disk.
- Ambiguous ZIPs fail with actionable errors.
- Tests cover both success and failure paths.
