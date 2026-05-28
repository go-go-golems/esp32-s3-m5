---
Title: Investigation Diary
Ticket: "0097"
Status: active
Topics:
    - esp32
    - m5dial
    - 3d-rendering
    - dithering
    - software-rendering
DocType: reference
Intent: long-term
Owners: []
RelatedFiles:
    - Path: 0096-m5dial-dithered-3d/main/app_main.cpp
      Note: Backend dispatch between poster and planet3d renderers
    - Path: 0096-m5dial-dithered-3d/main/console_commands.cpp
      Note: Interactive backend
    - Path: 0096-m5dial-dithered-3d/main/renderer3d.cpp
      Note: Implemented first firmware 80x80 16-bit-Z planet renderer (commit 66ec490)
    - Path: 0096-m5dial-dithered-3d/main/renderer3d.h
      Note: Renderer3D public contract and stats
    - Path: ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-first.png
      Note: First hardware framebuffer capture for visual review
    - Path: ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-spherical.png
      Note: Updated hardware capture after spherical geometry/color correction
    - Path: ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/planet-80-z16-spherical.png
      Note: Host preview after spherical geometry/color correction
    - Path: ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/design-doc/01-proper-3d-planet-renderer-analysis-and-implementation-guide.md
      Note: |-
        Primary design and implementation guide produced in this ticket
        Primary design deliverable
    - Path: ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/01-host-planet-renderer-prototype.py
      Note: |-
        Host-side executable prototype for the proposed coarse 3D planet renderer
        Host-side experiment recorded in diary
ExternalSources: []
Summary: Chronological investigation diary for ticket 0097, focused on designing a proper coarse-framebuffer 3D planet renderer for the M5Dial.
LastUpdated: 2026-05-28T00:00:00Z
WhatFor: Record research steps, host experiments, design decisions, failures, and validation evidence for the 0097 renderer design.
WhenToUse: Resume or review the proper 3D planet renderer implementation plan.
---




# Investigation Diary

## Goal

Design the next-stage renderer for the M5Dial dithered scene viewer: a proper software 3D backend that starts with the `PLANET` scene from the original `m5dial.jsx`, uses the hardware constraints learned in ticket 0096, and remains compatible with the existing 2-bit framebuffer, palette, console, and screenshot tooling.

## Step 1: Ticket Setup and Scope Selection

The user asked to create a new ticket and write a detailed analysis/design/implementation guide for returning to the original JSX and implementing a proper 3D renderer. The user explicitly allowed host-side experiments because they provide faster feedback and easier visual inspection.

### Prompt Context

**User prompt (verbatim):** "alright, let's do that then. Create a new ticket, and create a detailed analysis and implementation guide. Feel free to run host side experiments because it's easier to get feedback and inspect the results, as well. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet point sand pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and the nupload to remarkable."

**Assistant interpretation:** Create ticket 0097, investigate the original JSX and existing firmware, run a host prototype, write a detailed intern-oriented implementation guide, validate ticket docs, and upload the documentation bundle to reMarkable.

**Inferred user intent:** Move from the 0096 poster-renderer milestone to a real mesh-transform/raster/Z-buffer renderer, starting with one scene and preserving documentation quality.

### What I did

- Created ticket `0097` with title `M5Dial Proper 3D Planet Renderer Design`.
- Created the primary design document:
  - `ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/design-doc/01-proper-3d-planet-renderer-analysis-and-implementation-guide.md`
- Created this investigation diary:
  - `ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/reference/01-investigation-diary.md`
- Loaded the ticket-research, textbook-authoring, and reMarkable upload skill instructions.
- Selected `PLANET` as the first proper 3D target instead of `TERRAIN`.

### Why

Planet is the right first target because it is bounded, centered, and recognizable with low polygon counts. Terrain is visually important, but it requires a grid mesh, a more demanding camera composition, and many triangles. A planet sphere tests transform, projection, triangle rasterization, Z-buffering, color interpolation, and Bayer quantization without the additional burden of a terrain heightfield.

### What worked

- `docmgr ticket create-ticket` created the ticket workspace under the current date path:
  - `ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design`
- `docmgr doc add` created the design and diary documents.
- The previous 0096 work provided strong reusable foundations: 2-bit framebuffer, `dumpfb`, host PNG reconstruction, console controls, and a working display path.

### What didn't work

- No failure in ticket creation.

### What I learned

- The proper 3D renderer should be treated as a new backend, not a replacement for the poster renderer. Keeping the poster renderer intact preserves the working device demo while the new renderer matures.

### What was tricky to build

The key scope decision was avoiding a direct Three.js port. The correct embedded target is not the browser scene graph; it is the final visual pipeline: coarse pixels, Bayer ordered dithering, four palette indices, circular mask, and solid UI overlay.

### What warrants a second pair of eyes

- Confirm that `PLANET` remains the preferred first scene for the proper 3D backend.
- Confirm that the first milestone should exclude ring and moon until the sphere path is correct.

### What should be done in the future

- Implement the 3D backend in the firmware after reviewing the design document.
- Add measurement commands (`heap`, `allocprobe`) before allocating renderer buffers.

### Code review instructions

- Review ticket setup artifacts first: `index.md`, `tasks.md`, `changelog.md`, the design doc, and this diary.

## Step 2: Evidence Gathering from JSX and Existing Firmware

I gathered line-anchored evidence from the original JSX and the current firmware. The goal was to make the design document concrete rather than speculative.

### What I did

- Inspected the original dither shader in:
  - `ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/m5dial.jsx` lines 41–112.
- Inspected the original planet scene in:
  - `m5dial.jsx` lines 298–365.
- Inspected the existing 2-bit framebuffer contract in:
  - `0096-m5dial-dithered-3d/main/framebuffer.h` lines 6–19 and 30–42.
- Inspected the current app render loop and LCD transfer path in:
  - `0096-m5dial-dithered-3d/main/app_main.cpp` lines 130–188.
- Inspected the poster renderer's dither, pixel-size, aperture, and solid text helpers in:
  - `0096-m5dial-dithered-3d/main/terrain_poster.cpp` lines 1–100.
- Inspected the `dumpfb` and `fps` command implementation in:
  - `0096-m5dial-dithered-3d/main/console_commands.cpp` lines 246–281.
- Inspected the old experimental triangle renderer in:
  - `0096-m5dial-dithered-3d/main/renderer.cpp` lines 1–80 and 220–310.

### Why

The design must be tied to files the intern can open. The existing firmware already solves many hard problems. The new renderer should reuse those solutions and replace only the middle of the pipeline: the scene renderer.

### What worked

The evidence strongly supports the design:

- The original GLSL shader already uses pixel blocks before Bayer dithering.
- The current firmware already has a 2-bit indexed framebuffer and scanline RGB565 expansion.
- The current `dumpfb` command gives a validation loop for any new renderer output.
- The old `renderer.cpp` provides useful projection/rasterization reference code but should not be reused directly because of the scanline-Z limitation.

### What didn't work

The old triangle renderer is not a final architecture. It has a scanline Z-buffer reset inside triangle drawing, which cannot provide correct inter-triangle occlusion for a planet and ring.

### What I learned

The new renderer should use a full logical Z-buffer. Because the logical target is only 80×80 initially, full-frame Z-buffering is cheaper and simpler than scanline-Z tricks.

### What was tricky to build

The nuance is that the 4-color framebuffer solves color storage but not visibility. We still need a Z-buffer for proper 3D. The Z-buffer can be small if the 3D renderer is coarse.

### What warrants a second pair of eyes

- Check whether the final firmware should use a UV sphere for easier latitude coloring or an icosphere for better triangle uniformity.

### What should be done in the future

- Add direct links/relations from the design doc to the evidence files using `docmgr doc relate`.

### Code review instructions

- In review, verify every major design claim against the file references above.

## Step 3: Host-Side Planet Renderer Prototype

I wrote a host-side executable prototype to validate the proposed renderer shape before firmware implementation.

### What I did

Created:

```text
ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/01-host-planet-renderer-prototype.py
```

The script implements:

- low-resolution logical render targets (`80×80`, `120×120`, etc.);
- UV sphere generation with red/blue latitude coloring;
- cheap deterministic pseudo-noise for surface variation;
- camera projection into logical coordinates;
- triangle rasterization with a full logical Z-buffer;
- direct Bayer 4×4 four-color quantization;
- scale-up to 240×240 PNG;
- solid `PLANET` title overlay.

Ran:

```bash
python3 ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/01-host-planet-renderer-prototype.py \
  --logical 80 --zbits 16 \
  --out ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/planet-80-z16.png

python3 ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/01-host-planet-renderer-prototype.py \
  --logical 80 --zbits 8 \
  --out ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/planet-80-z8.png

python3 ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/01-host-planet-renderer-prototype.py \
  --logical 120 --zbits 16 \
  --out ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/planet-120-z16.png
```

Observed for 80×80 / 16-bit:

```text
logical_pixels: 6400
zbuffer_bytes: 12800
physical_framebuffer_bytes_2bpp: 14400
sphere_vertices: 532
sphere_triangles: 952
ring_vertices: 192
ring_triangles: 192
planet_pixels: 2066
ring_pixels: 0
pixel_scale: 3
```

### Why

Host-side rendering is faster to inspect than firmware iteration. It validates the memory model and the data flow before C++ implementation.

### What worked

- The script generated valid 240×240 PNGs.
- The 80×80/3× scaled output clearly demonstrates the intended coarse-pixel architecture.
- The Z-buffer memory numbers are small and match the design: 12.8 KB for 80×80 16-bit and 6.4 KB for 80×80 8-bit.
- The prototype confirms that direct four-color quantization is viable without an RGB framebuffer.

### What didn't work

- The first ring experiment did not produce visible ring fragments (`ring_pixels: 0`). This is acceptable evidence for the design: ring should not be part of the first firmware milestone.

### What I learned

The sphere path is enough for the first milestone. Ring geometry adds a separate set of problems: thin triangles, two-sided rendering, low-resolution aliasing, and occlusion composition. Those should be handled after the planet body is correct.

### What was tricky to build

Getting the prototype's inside-triangle test and backface culling to behave consistently required care because screen-space `y` is inverted by projection. This is another reason to keep the first firmware milestone narrow.

### What warrants a second pair of eyes

- The prototype uses a UV sphere, not an icosphere. Review whether UV poles are acceptable at the target resolution.
- The ring path should be revisited separately.

### What should be done in the future

- Add a second prototype pass for a split front/back ring after firmware sphere rendering works.
- Optionally generate C arrays from the Python mesh once the mesh density is chosen.

### Code review instructions

- Review `01-host-planet-renderer-prototype.py` as executable pseudocode, not as production code.
- Compare its output and stats with the design document's proposed memory tables.

## Step 4: Design Document Authoring

I wrote the primary design document for an intern-level implementation.

### What I did

Updated:

```text
ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/design-doc/01-proper-3d-planet-renderer-analysis-and-implementation-guide.md
```

The document includes:

- executive summary;
- problem statement and scope;
- file-backed evidence from JSX and current firmware;
- hardware and memory model;
- memory budget tables;
- 16-bit vs 8-bit Z-buffer guidance;
- proposed architecture and data flow diagram;
- renderer API sketch;
- mesh strategy;
- projection math;
- rasterization pseudocode;
- quantization pseudocode;
- ring and moon plan;
- host experiment results;
- phased implementation plan;
- testing strategy;
- risks, alternatives, open questions, and file references.

### Why

The next person implementing the firmware should not have to rediscover the memory constraints, rendering pipeline, or scope decisions. The document is meant to be sufficient for a new intern to start implementation safely.

### What worked

The existing 0096 system gave the design a solid foundation. The guide can point to actual code for framebuffer layout, display transfer, console screenshot export, and poster renderer behavior.

### What didn't work

No writing failure, but the design intentionally leaves ring/moon details as later phases because the host experiment showed ring work needs separate attention.

### What I learned

The proper 3D backend should be introduced as an optional backend, not as a destructive rewrite. That gives the team a working UI and screenshot loop throughout implementation.

### What was tricky to build

The hardest part of the guide was separating three layers that are easy to conflate:

1. Physical LCD framebuffer: 240×240, 2-bit packed.
2. Logical 3D render target: initially 80×80.
3. UI overlay: solid 240×240 text/status after the 3D pass.

The design is clear only if these layers stay separate.

### What warrants a second pair of eyes

- Confirm whether runtime backend switching (`backend poster|planet3d`) should be added before or after the first sphere render.
- Confirm whether generated mesh data should be built at boot or stored as `const` tables.

### What should be done in the future

- Run `docmgr doctor` and address vocabulary/frontmatter issues.
- Upload the design bundle to reMarkable.

### Code review instructions

- Read the design doc top to bottom before implementing.
- Pay special attention to the phased plan; it intentionally delays ring and moon.

## Step 5: Validation and reMarkable Upload

After writing the design guide and diary, I validated the ticket with `docmgr doctor`, fixed vocabulary warnings, and uploaded the bundle to reMarkable.

### What I did

- Ran:

```bash
docmgr doctor --ticket 0097 --stale-after 30
```

- The first doctor run reported unknown topic vocabulary entries for:
  - `3d-rendering`
  - `dithering`
  - `m5dial`
  - `software-rendering`
- Added those topics to the docmgr vocabulary:

```bash
docmgr vocab add --category topics --slug 3d-rendering --description "Software 3D rendering, projection, rasterization, and depth buffering"
docmgr vocab add --category topics --slug dithering --description "Ordered dithering and palette quantization"
docmgr vocab add --category topics --slug m5dial --description "M5Stack M5Dial device projects"
docmgr vocab add --category topics --slug software-rendering --description "CPU-based rendering pipelines without GPU acceleration"
```

- Re-ran doctor. It passed:

```text
## Doctor Report (1 findings)

### 0097

- ✅ All checks passed
```

- Uploaded the design bundle to reMarkable:

```bash
remarquee upload bundle \
  ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/design-doc/01-proper-3d-planet-renderer-analysis-and-implementation-guide.md \
  ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/reference/01-investigation-diary.md \
  --name "0097 M5Dial Proper 3D Planet Renderer Design" \
  --remote-dir "/ai/2026/05/28/0097" \
  --toc-depth 2 \
  --non-interactive
```

- Upload succeeded:

```text
OK: uploaded 0097 M5Dial Proper 3D Planet Renderer Design.pdf -> /ai/2026/05/28/0097
```

### Why

The user explicitly asked to store the guide in the ticket and upload it to reMarkable. The doctor pass ensures the ticket metadata is valid before delivery.

### What worked

- Vocabulary additions resolved the only doctor warning.
- The reMarkable bundle upload succeeded in one command.

### What didn't work

- No upload failure occurred.

### What I learned

The 0097 topic set needed vocabulary entries because this ticket introduces a more specific rendering-design vocabulary than the older benchmark/display tickets.

### What was tricky to build

No technical issue in this step. The main detail was following the current reMarkable upload workflow and not running unnecessary account/status checks.

### What warrants a second pair of eyes

- Review the design PDF on reMarkable for readability and diagram rendering.
- Confirm whether the implementation should begin immediately in the existing 0096 firmware directory or be forked into a fresh 0097 firmware copy.

### What should be done in the future

- Start firmware implementation with measurement commands: `heap` and `allocprobe`.
- Then add the `renderer3d` skeleton and first 80×80 16-bit-Z sphere render.

### Code review instructions

- Validate that `docmgr doctor --ticket 0097 --stale-after 30` still passes after any future edits.
- Confirm the reMarkable remote path before replacing or uploading updated versions.

## Step 6: Buffer Configuration Comparison and JSX-Matching Revision

After the first host comparison, the user pointed out that the generated planet did not really look like the original JSX version. That was correct. The first prototype validated the coarse Z-buffer and four-color raster path, but it omitted the most recognizable part of the JSX planet scene: the ring crossing the planet body.

### Prompt Context

**User prompt (verbatim):** "compare different buffer configurations on the host side, since we have a lot of memory, save screenshots in the ticket, and write a md report with thoe screenshots and writeup, and open with md-view view ... when done"

**Assistant interpretation:** Generate a host-side matrix of logical framebuffer/Z-buffer/mesh-density screenshots, save the artifacts into ticket 0097, write a Markdown report with screenshots and analysis, and open it with `md-view view`.

**Inferred user intent:** Choose the firmware buffer configuration from rendered evidence rather than theoretical memory tables alone.

**User prompt (verbatim):** "this doesn't look like the jsx version really, make it match more closely"

**Assistant interpretation:** Revise the host prototype so the comparison resembles the original JSX planet scene composition more closely, especially the ringed planet shape.

**Inferred user intent:** Do not optimize for a generic red/blue sphere; the comparison should target the actual JSX visual direction.

### What I did

- Added batch comparison script:

```text
ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/scripts/02-compare-buffer-configs.py
```

- Generated the first comparison set under:

```text
artifacts/buffer-config-comparison/
```

- Created montages for:
  - resolution at 16-bit Z
  - 8-bit versus 16-bit Z
  - mesh density at 80×80
- Inspected the generated montages with `read`.
- Wrote the first comparison report:

```text
design-doc/02-buffer-configuration-comparison-report.md
```

- Revised `01-host-planet-renderer-prototype.py` after visual review because the first output did not match the JSX planet closely enough.
- Added a split JSX-style ring composition to the host prototype:
  1. draw back half of ring
  2. rasterize planet body
  3. draw front half of ring
  4. draw solid title UI
- Regenerated the comparison set under:

```text
artifacts/buffer-config-comparison-v2/
```

- Inspected the updated montage with `read`; it now shows a ringed planet composition much closer to the JSX scene.
- Wrote the revised report:

```text
design-doc/03-jsx-matched-buffer-configuration-report.md
```

### Why

The purpose of the host comparison is not only to compare memory costs. It must compare configurations using a visual target that resembles the scene we actually want to implement. The original planet scene is a ringed planet. Without the ring, the output looked like separated red/blue hemispheres and produced misleading visual conclusions.

### What worked

- The batch comparison script generated 17 screenshots plus a CSV of metrics.
- The v2 ringed screenshots are much closer to the JSX target.
- The comparison clarified two different targets:
  - `80×80` is the best first firmware target.
  - `120×120` is the closer JSX visual match because JSX defaults to `pixelSize = 2`.
- 8-bit and 16-bit Z still look equivalent for the sphere body, but the report correctly warns that this does not prove 8-bit is safe for a future true 3D ring.

### What didn't work

- The first report's screenshots were not visually close enough to the JSX planet scene.
- The earlier ring mesh path still did not produce useful visible fragments. The v2 host prototype uses a split ellipse composition instead of a true 3D ring mesh to match the visual target.

### What I learned

The ring is not optional for evaluating the planet scene. It changes the visual read of the whole composition and affects which resolution feels acceptable. At 80×80, the ringed planet is recognizable and firmware-friendly. At 120×120, the result is closer to the original JSX default.

### What was tricky to build

The important distinction is between a renderer architecture prototype and a visual target prototype. The sphere-only renderer was architecturally useful but visually incomplete. The split-ring update is less geometrically pure than a true 3D torus strip, but it provides a better target for the first firmware ring milestone.

### What warrants a second pair of eyes

- Decide whether the firmware should implement the ring first as a split composition, as recommended by the v2 report, or continue trying for a true 3D ring strip immediately.
- Review whether `80×80` is acceptable as the first on-device visual milestone, or whether the closer `120×120` target is worth attempting earlier.

### What should be done in the future

- Open the revised report with `md-view view` for visual review.
- Upload the revised comparison report to reMarkable if desired after user review.
- Use `resolution-L80-Z16-lat18-lon28.png` as the first firmware screenshot target and `resolution-L120-Z16-lat18-lon28.png` as the quality target.

### Code review instructions

- Review `scripts/01-host-planet-renderer-prototype.py` for the split ring functions `draw_reference_ring()` and `draw_reference_moon()`.
- Review `scripts/02-compare-buffer-configs.py` for the comparison matrix.
- Review `design-doc/03-jsx-matched-buffer-configuration-report.md` for updated recommendations.

## Step 7: First On-Device Proper 3D Planet Firmware Backend

This step moved ticket 0097 from host-side design into the M5Dial firmware. I kept the stable poster renderer as the default backend, then added an opt-in `planet3d` backend so the new coarse Z-buffer path can be tested from `esp_console` without losing the known-good scenes.

The first hardware capture is now available and recognizable: a 240×240 dumpfb screenshot of a ringed red/blue dithered planet with solid `PLANET` text. The implementation uses the conservative `80×80` logical target from the report, a 16-bit Z-buffer, a 2-bit output framebuffer, and a split analytic ring matching the JSX-oriented host prototype.

### Prompt Context

**User prompt (verbatim):** "start firmware, commit at appropriate intervals"

**Assistant interpretation:** Begin implementing the proper 3D planet renderer in firmware and make focused commits at stable milestones.

**Inferred user intent:** Turn the 0097 design/report findings into working M5Dial code while preserving reviewable history.

**User prompt (verbatim):** "m5dial is connected, tmux idf.py monitor has been closed though"

**Assistant interpretation:** The board is ready on USB Serial/JTAG and the serial port should be free for flashing/probing.

**Inferred user intent:** Proceed with live device validation, but avoid serial contention.

**Commit (code):** 6b3fa95179a955aec3a161924f127173de8a5850 — "feat(0096): add renderer backend and heap diagnostics"

**Commit (code):** 66ec490f8d70b02d4c672d29100c299fcff03353 — "feat(0096): render proper 3d planet backend"

### What I did

- Committed the previously generated 0097 host comparison reports, screenshots, and printed almanach artifacts first:
  - `87de29f1444766ef5ad0a870dc16d5ea003b9a99` — `docs(0097): add planet renderer comparison artifacts`
- Added firmware console controls and diagnostics:
  - `backend [poster|planet3d]`
  - `heap`
  - `allocprobe <bytes>`
  - `r3dstats`
- Added `render_backend_t` and `render_params_t.backend`, defaulting to `RENDER_BACKEND_POSTER` so existing poster behavior remains the boot default.
- Added `renderer3d.h/cpp` with:
  - `R3D_W = 80`
  - `R3D_H = 80`
  - `R3D_PIXEL_SCALE = 3`
  - `R3D_Z_BITS = 16`
  - static `uint16_t` Z-buffer: 12,800 bytes
  - static logical color buffer: 6,400 bytes
  - generated `lat18/lon28` UV sphere: 532 vertices, 952 triangles
  - ordered 4×4 Bayer quantization into the existing four color indices
  - split back/front analytic ring pass
  - small moon billboard pass
  - solid, non-dithered `PLANET` title overlay
- Wired `app_main.cpp` to dispatch either `poster_render_scene()` or `renderer3d_render_planet()` based on the selected backend.
- Built with:

```text
cd 0096-m5dial-dithered-3d
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build
```

- Checked serial ownership before flashing:

```text
fuser -v /dev/ttyACM0
```

- Flashed the connected M5Dial with:

```text
idf.py -p /dev/ttyACM0 flash
```

- Captured first device framebuffer output with:

```text
python3 ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/03-capture-dumpfb.py \
  --port /dev/ttyACM0 \
  --setup "backend planet3d" \
  --setup "debug off" \
  --setup "angle 0" \
  --out ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-first.png \
  --transcript ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-first.txt \
  --prompt-timeout 15 --command-timeout 10 --dump-timeout 20
```

- Queried device stats after selecting the backend:

```text
R3D: 80x80 scale=3 z=16-bit render=21032 us (47.5 FPS render-only)
Buffers: z=12800 bytes color=6400 bytes fb=14400 bytes
Mesh: 532 vertices, 952 triangles
Triangles: 952 submitted, 533 drawn
Pixels: planet=2080 ring=369 moon=0

Frame time: 33920 us (29.5 FPS)
Mode: triangle
Triangles: 952 submitted, 533 drawn
Pixels written: 2449

Heap INTERNAL|8BIT free=283248 largest=212992 minimum=275296
Heap DMA free=275488 largest=212992
Heap DEFAULT free=283248 largest=212992
```

### Why

The 0097 report recommended starting firmware at `80×80` with 16-bit Z because it is visually recognizable and small enough for the no-PSRAM M5Dial. The new backend implements exactly that first target while preserving the poster renderer as a fallback.

The heap and allocation probes are intentionally part of the first firmware step. They make SRAM decisions measurable on the live board instead of relying on ESP32-S3 spec-sheet memory numbers.

### What worked

- The firmware builds successfully on ESP-IDF 5.4.2.
- Flashing over `/dev/ttyACM0` succeeded after confirming no serial holder.
- `backend planet3d` switches into the new renderer at runtime.
- `dumpfb` capture reconstructed a valid PNG on the host.
- The first device screenshot visually matches the host target closely enough for a first firmware milestone: solid title, red upper hemisphere, blue lower hemisphere, and split ring.
- The render-only time is about 21 ms and the full frame time including display transfer is about 34 ms, leaving significant headroom at the 80×80 target.
- Internal heap remains healthy after boot and backend rendering: largest internal 8-bit block was 212,992 bytes in the measured session.

### What didn't work

- Opening a new pyserial session resets the board, so a later `r3dstats` command initially reported zeroed stats because the device had returned to the default `poster` backend after reset. I reran `backend planet3d` and `angle 0` in the same serial session before querying `r3dstats`, `fps`, and `heap`.
- The moon pass is implemented, but at the captured angle it reported `moon=0`; the first screenshot validates the sphere and ring, not the moon visibility envelope.
- The build still emits existing warnings for ESP-IDF console struct missing-field initializers and the legacy PCNT driver. These warnings predate the renderer work and did not block the build.

### What I learned

The conservative memory plan is viable on hardware. The new static renderer buffers add about 19.2 KB for logical color + Z, plus projection and mesh storage, while the board still reports more than 280 KB free internal 8-bit heap and a largest block above 200 KB.

The host report's `80×80` recommendation was correct for first firmware. The on-device capture is blocky, but it is recognizable, fast, and stable enough to become the baseline for future quality work.

### What was tricky to build

The main integration issue was preserving a safe fallback while adding a second renderer. The solution was to add a backend field to `render_params_t`, default it to `poster`, and make `backend planet3d` opt in to the new path. This keeps boot behavior stable and makes serial testing reversible.

The other tricky part was serial-session behavior. The capture helper and ad-hoc stats probe both open USB Serial/JTAG; opening the port can reset the board. That means stats must be collected after reapplying setup commands in the same serial session, otherwise the observed backend may not be the backend that produced the previous screenshot.

### What warrants a second pair of eyes

- Review `renderer3d.cpp` rasterization math and backface-culling sign against the host prototype.
- Review whether the split analytic ring should remain the first firmware ring implementation or be replaced with a true 3D strip once sphere behavior is stable.
- Review whether the static renderer buffers should stay in BSS or move behind explicit heap allocation once `120×120` experiments begin.
- Verify moon placement at several angles; the angle-0 capture did not draw it.

### What should be done in the future

- Capture a small angle sweep with `backend planet3d` to verify rotation, ring stability, and moon visibility.
- Compare `device-planet3d-first.png` against `artifacts/buffer-config-comparison-v2/resolution-L80-Z16-lat18-lon28.png` side by side.
- Try a `120×120` branch only after the 80×80 path is reviewed and stable.
- Consider adding a runtime `r3dres` or compile-time profile for 80×80 versus 120×120 experiments.

### Code review instructions

- Start with `0096-m5dial-dithered-3d/main/renderer3d.h` for the public constants and stats contract.
- Then review `0096-m5dial-dithered-3d/main/renderer3d.cpp` in this order:
  1. static buffer declarations,
  2. `build_sphere()`,
  3. `project_vertex()`,
  4. `rasterize_sphere()`,
  5. `draw_reference_ring()`,
  6. `expand_to_framebuffer()`.
- Review `0096-m5dial-dithered-3d/main/app_main.cpp` for backend dispatch and renderer stats recording.
- Review `0096-m5dial-dithered-3d/main/console_commands.cpp` for operator controls.
- Validate with:

```text
cd 0096-m5dial-dithered-3d
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/ttyACM0 flash
```

- Then capture:

```text
python3 ttmp/2026/05/27/0096--m5dial-dithered-3d-scene-viewer/scripts/03-capture-dumpfb.py \
  --port /dev/ttyACM0 \
  --setup "backend planet3d" \
  --setup "angle 0" \
  --out ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-first.png
```

### Technical details

- Firmware binary after adding `renderer3d`: `0x5a8e0` bytes; app partition still has about 88% free.
- First renderer3d runtime stats:

```text
80x80 logical pixels = 6,400 pixels
16-bit Z-buffer = 12,800 bytes
2-bit physical framebuffer = 14,400 bytes
logical color buffer = 6,400 bytes
sphere mesh = 532 vertices, 952 triangles
render-only time = 21,032 us
full frame including display transfer = 33,920 us
```

- First hardware capture:

```text
ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-first.png
```

## Step 8: Correct the Planet Silhouette to Read as a Sphere

The first firmware capture was recognizable, but the planet body looked pinched and lumpy. The important correction is that Z-buffer precision was not the cause. Z precision decides which surface wins at a pixel; it does not make the projected geometry smoother or more spherical.

I corrected the geometry and color model so the mesh remains a true sphere and the visible dithered pixels fill the disk more evenly. Noise now affects surface color only, not vertex radius, and every point on the planet gets a small base red/blue density so the equator and limb do not quantize away to black.

### Prompt Context

**User prompt (verbatim):** "one would the planet not be a sphere? it looks oddly misshapen, even at high resolution of z"

**Assistant interpretation:** Explain why the planet appeared misshapen and fix the renderer so the planet reads as a sphere rather than a noisy/lumpy body.

**Inferred user intent:** The PLANET scene should look like a round planet; increasing Z precision should not be treated as the solution to a silhouette/color problem.

### What I did

- Updated firmware `renderer3d.cpp`:
  - removed radial noise displacement from sphere vertices
  - kept vertex positions on `kPlanetRadius`
  - retained noise only as color/speckle texture
  - added base color density across the full sphere so the equator/limb remain visible after Bayer quantization
- Updated the host prototype with the same geometry/color changes.
- Generated a host preview:

```text
ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/planet-80-z16-spherical.png
```

- Built and flashed the firmware again:

```text
cd 0096-m5dial-dithered-3d
source /home/manuel/esp/esp-idf-5.4.2/export.sh
idf.py build
idf.py -p /dev/ttyACM0 flash
```

- Captured the updated hardware framebuffer:

```text
ttmp/2026/05/28/0097--m5dial-proper-3d-planet-renderer-design/artifacts/device-planet3d-spherical.png
```

- Rechecked renderer stats after a render completed:

```text
R3D: 80x80 scale=3 z=16-bit render=23147 us (43.2 FPS render-only)
Buffers: z=12800 bytes color=6400 bytes fb=14400 bytes
Mesh: 532 vertices, 952 triangles
Triangles: 952 submitted, 573 drawn
Pixels: planet=2095 ring=367 moon=0

Frame time: 36018 us (27.8 FPS)
Mode: triangle
Triangles: 952 submitted, 573 drawn
Pixels written: 2462
```

### Why

The misshapen look came from two non-Z sources. First, the prototype used procedural noise as radial displacement, which made the actual mesh lumpy. Second, the latitude-only color model left low-density areas near the equator and limb; after thresholded 4-color dithering those areas disappeared, making the visible body look pinched.

### What worked

- The updated host and device captures both show a much more spherical planet body.
- The firmware still builds and flashes successfully.
- Runtime remains acceptable: about 23 ms render-only and 36 ms including display transfer in the measured run.

### What didn't work

- Querying `r3dstats` immediately after changing backend/angle once showed `render=0 us` while pixel counters were already populated. This was a race with the render task; querying again after a short delay produced the valid render time above.

### What I learned

High Z precision is not a cure for silhouette or shading artifacts. In this renderer, geometry, logical resolution, mesh density, and quantization thresholds determine whether the object reads as round. Z precision only determines depth ordering once pixels are being rasterized.

### What was tricky to build

The body was geometrically and visually wrong in different ways. Removing radius displacement fixed the mesh, but it was not sufficient by itself because the quantizer could still erase weakly colored body pixels. The final fix needed both spherical geometry and a color-density floor.

### What warrants a second pair of eyes

- Review whether the new base density is too solid compared with the JSX dithered aesthetic.
- Review whether the equatorial blue/neutral band is desirable or whether the color ramp should be tuned further.

### What should be done in the future

- Capture angle sweeps to confirm the silhouette stays round during rotation.
- Tune the color ramp after comparing on the physical LCD, not only the dumpfb PNG.

### Code review instructions

- Review `build_sphere()` in `0096-m5dial-dithered-3d/main/renderer3d.cpp`.
- Compare `device-planet3d-first.png` with `device-planet3d-spherical.png`.
- Validate with `backend planet3d`, `angle 0`, `dumpfb`, and `r3dstats` after waiting for a redraw.

### Technical details

The corrected geometry line is conceptually:

```text
position = unit_sphere_position * radius
color = latitude_gradient + procedural_texture + base_density
```

not:

```text
position = unit_sphere_position * (radius + procedural_noise)
```
