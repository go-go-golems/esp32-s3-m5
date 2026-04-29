---
title: Diary
doc_type: reference
status: active
intent: long-term
topics:
  - esp32-s3
  - http-server
  - react
  - static-embed
  - thermal-printer
ticket: ALMANACH-STUDIO
created: 2026-04-29
---

# Diary

## Goal

Create a comprehensive design and implementation guide for hosting Almanach Studio (a React SPA) on the AtomS3R ESP32-S3 device, served at `/almanach` using static-compile (esbuild) with assets embedded in firmware.

## Step 1: Analysis and Design Document Creation

Write 1–2 short prose paragraphs here (required). Explain intent, what changed at a high level, and what it unlocked.

### Prompt Context

**User prompt (verbatim):** "Create a new docmgr ticket to create a local almanach creator. Use docmgr import file --file '/home/manuel/Downloads/almanach-studio(1).jsx' --ticket XXX, then read it, and study how to host it locally, look for example at: /home/manuel/code/wesen/2026-03-29--serve-claude-experiments. Ideally I would like to do a static compile, use babel in the browser to minimize asset size, and serve it from the atoms3r under /almanach. You can prob look at other firmwares to see how to host static files if that is not already done. Create a detailed analysis / design / implementation guide that is very detailed for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet points and pseudocode and diagrams and api references and file references. It should be very clear and detailed. Store in the ticket and then upload to remarkable."

**Assistant interpretation:** Create a docmgr ticket, import the JSX source, study the existing JSX-hosting approach from serve-claude-experiments and the AtomS3R firmware's static asset embedding pattern, then write a comprehensive design/implementation guide.

**Inferred user intent:** Get a thorough, intern-ready guide that covers everything needed to implement Almanach Studio hosting on the AtomS3R, including the JSX component architecture, the ESP32 HTTP server, the build pipeline, and the integration plan.

### What I did

- Created docmgr ticket `ALMANACH-STUDIO` with title "Almanach Studio — Local Almanach Creator for AtomS3R"
- Imported `almanach-studio(1).jsx` into the ticket's sources directory
- Read and analyzed the full JSX source (~2100 lines):
  - 15 block types with renderers and editors
  - 6 themes (Classic, Minimal, Botanical, Notebook, Vintage Ledger, Space Age)
  - PNG export via SVG foreignObject
  - JSON save/load for layout persistence
  - All styling is inline CSS-in-JS
  - Dependencies: React 18 + Lucide React icons only
- Studied `serve-claude-experiments`:
  - JSX host page pattern with import maps and Babel fallback
  - Precompiled vs. runtime JSX transform
  - Auto-mount via `jsx.BuildModuleSource()`
- Studied `0017-atoms3r-web-ui` firmware:
  - EMBED_TXTFILES pattern for embedding web assets
  - `httpd_assets_embed` reusable component
  - URI handler registration for static routes
  - Vite build → main/assets/ → EMBED_TXTFILES pipeline
- Wrote comprehensive design document (15 sections, ~41 KB)

### Why

The user wants to host Almanach Studio on an AtomS3R device. This requires understanding both the React/JSX side and the ESP32 embedded HTTP server side, and bridging them with a build pipeline.

### What worked

- The `serve-claude-experiments` project provides an excellent reference for the JSX hosting pattern
- The `0017-atoms3r-web-ui` firmware demonstrates exactly how to embed and serve static assets
- The `httpd_assets_embed` component is already reusable — no need to create a new embedding helper
- esbuild provides the perfect build tool: fast, zero-config JSX transform, tree-shaking, minification

### What didn't work

- N/A (analysis phase only)

### What I learned

- The Almanach Studio JSX file is self-contained (single file, no external CSS/JS dependencies beyond React and Lucide)
- Google Fonts are the main offline concern — they're loaded via `@import url(...)` which won't work in SoftAP mode
- The AtomS3R's 8 MB flash gives plenty of room for the SPA (~150 KB minified)
- EMBED_TXTFILES adds a trailing NUL byte — `httpd_assets_embed_send` already handles trimming it

### What was tricky to build

- The EMBED_TXTFILES symbol naming convention (dots and slashes → underscores) can be surprising. The exact symbol names must be verified after the first build.

### What warrants a second pair of eyes

- The estimated asset sizes (React + Lucide + component = ~150 KB minified) should be verified with an actual build
- The font fallback strategy (system fonts vs. embedded WOFF2) needs a visual quality check
- The `asm()` symbol names in the C++ code need to match exactly what ESP-IDF generates

### What should be done in the future

- Test on a real AtomS3R device (browser at http://<device-ip>/almanach)
- Implement font embedding for offline beauty (WOFF2 subsets)
- Add thermal printer integration (print PNG directly from ESP32)
- Add data API endpoints for live weather/news/quotes

### Code review instructions

- Read the design document: `design-doc/01-almanach-studio-design-implementation-guide.md`
- Compare the EMBED_TXTFILES pattern with the existing `0017-atoms3r-web-ui/main/CMakeLists.txt`
- Verify the esbuild build command produces a valid IIFE bundle

### Technical details

- Ticket path: `ttmp/2026/04/29/ALMANACH-STUDIO--almanach-studio-local-almanach-creator-for-atoms3r/`
- Source JSX imported to: `sources/local/almanach-studio(1).jsx`
- Design doc: `design-doc/01-almanach-studio-design-implementation-guide.md`
- 8 related files linked to the ticket index
- 7 tasks created to track implementation phases

---

## Step 2: Implementation in stoms3r/ (the correct firmware)

The design document was originally written referencing `0017-atoms3r-web-ui/` as the target, but the actual target firmware is `stoms3r/` (SToMS3R — the thermal printer firmware). This step implements the full Almanach Studio hosting in the correct project.

### Prompt Context

**User prompt (verbatim):** (same as Step 1 — continuation)

**User prompt (verbatim):** "oops, which subproject are you even working in? we need it in stoms3r/"

**Assistant interpretation:** I had been implementing in `0017-atoms3r-web-ui/` by mistake. The correct target is `stoms3r/` — the SToMS3R thermal printer firmware. Revert the wrong changes and redo everything in the right project.

**Inferred user intent:** Correct the implementation target and get the Almanach Studio SPA working in the actual thermal printer firmware.

**Commit (code):** 7924e1b — "feat(almanach): host Almanach Studio SPA at /almanach"

### What I did

1. **Reverted `0017-atoms3r-web-ui/`** — cherry-picked only the web build pipeline commit (no firmware changes to that project)
2. **Studied `stoms3r/`** — it's a C (not C++) firmware with:
   - `web_server.c` — already serves `index.html` at `/` and has print APIs
   - `EMBED_TXTFILES` for `index.html` (11 KB)
   - 4 MB factory partition, 1 MB storage FAT
   - No external component dependencies (clean ESP-IDF-only build)
3. **Copied web build pipeline** to `stoms3r/web/almanach/` and rebuilt (210.9 KB)
4. **Embedded assets** in `main/assets/almanach/` via CMakeLists EMBED_TXTFILES
5. **Added URI handlers** in `web_server.c`:
   - `GET /almanach` → serves `almanach.html` (host page)
   - `GET /almanach/bundle.js` → serves `almanach-bundle.js` (IIFE bundle)
6. **Built firmware** — success, 1.01 MB binary, 74% partition free

### Why

The `stoms3r/` firmware is the actual thermal printer firmware — it makes sense to host Almanach Studio here since it already has the printer integration API (`/api/print/bitmap`, `/api/print/text`).

### What worked

- `stoms3r/` builds cleanly with no external component issues (unlike `0017-atoms3r-web-ui/` which has broken shared component deps)
- The esbuild IIFE bundle embeds flawlessly via EMBED_TXTFILES
- 210.9 KB minified bundle fits in the 4 MB factory partition with room to spare
- The `max_uri_handlers` was easily increased from 16 to 20

### What didn't work

1. **Wrong project initially** — implemented in `0017-atoms3r-web-ui/` instead of `stoms3r/`. Had to revert and redo.
2. **EMBED_TXTFILES symbol name collision** — `assets/almanach/index.html` collides with `index.html` (ESP-IDF uses basename only, no path). Fixed by renaming to `almanach.html`.
3. **Symbol names were wrong** — initially used `_binary_index_html_start_2` (guessing dedup suffix) but ESP-IDF doesn't do that. The actual symbols are derived from the filename: `_binary_almanach_html_start`.

### What I learned

- **EMBED_TXTFILES uses the basename only** — `assets/almanach/index.html` generates `_binary_index_html_start`, which collides with the top-level `index.html`. Always use unique filenames across ALL embedded files.
- **`nm` is your friend** — `xtensa-esp32s3-elf-nm libmain.a | grep _binary_` shows exactly what symbols are defined vs. undefined.
- **The `0017-atoms3r-web-ui/` firmware has broken build** due to shared component dependencies (`mqjs_service` → `mquickjs`, `screenshot_qoi` → `M5Unified`). Not related to my changes.

### What was tricky to build

The symbol naming was the main trap. I first guessed that ESP-IDF would deduplicate by appending `_2`, but it doesn't. Instead, it silently creates one symbol and the other file's content is simply lost. The fix was to give the almanach HTML a unique name (`almanach.html`) so its symbols are distinct.

### What warrants a second pair of eyes

- The `asm()` symbol names must exactly match what ESP-IDF generates — verified via `nm` on `libmain.a`
- The almanach HTML references `almanach-bundle.js` as a relative path — this works because the route is `/almanach/bundle.js` and the browser resolves relative URLs from `/almanach`

### What should be done in the future

- Flash and test on real hardware
- Test the SPA actually renders in a browser (no runtime errors)
- Verify PNG export works (the bundle inlines fonts as data URLs)
- Wire the "Print" button to call `/api/print/bitmap` with the rendered paper

### Code review instructions

- Start at `stoms3r/main/web_server.c` — search for `almanach`
- Check `stoms3r/main/CMakeLists.txt` — the EMBED_TXTFILES list
- Verify `stoms3r/main/assets/almanach/almanach.html` references `almanach-bundle.js`
- Build: `cd stoms3r && ./build.sh build`

### Technical details

- **Firmware binary**: 1,059,088 bytes (1.01 MB) in 4 MB partition (74% free)
- **Almanach bundle**: 215,993 bytes (210.9 KB minified IIFE)
- **Almanach HTML**: 435 bytes
- **New routes**: `GET /almanach`, `GET /almanach/bundle.js`
- **max_uri_handlers**: increased from 16 → 20 (now 12 of 20 used)
- **Commit**: 7924e1b

---

## Step 3: Monochrome, dithering selector, font scale slider

### Prompt Context

**User prompt (verbatim):** "is the dithering in the main webpage configurable? because I uploaded a 384 pixel wide image and it still got dithered?"

**User prompt (verbatim):** "Background color in the almanach app still says #fbfbfb for the background, at least in the demo setup. I think this might be due to the loaded widgets? the printer is monochrome, so everything should be black or white, grey is not going to work"

**User prompt (verbatim):** "every font except title needs to be at least 1.3x as big. Can we maybe add a slider?"

**Inferred user intent:** Make the Almanach Studio output truly monochrome (pure B/W, no gray) for thermal printing. Make font sizes adjustable via a slider since 1× is too small for thermal output. Fix the stoms3r web UI to not dither images that are already B/W.

### What I did

1. **Dithering selector** (stoms3r `index.html`): Added dropdown with Floyd-Steinberg / None / Auto modes. Auto detects pure B/W images and skips dithering.
2. **Monochrome conversion** (almanach-studio.jsx):
   - All 6 themes: `ink`/`muted`/`accent`/`rule` → `#000000`, `grain` → `0`
   - Removed all `opacity` values in rendered content (dividers 0.6, botanical corners 0.7, done items 0.7, date strip 0.95)
   - Removed space theme radial-gradient star pattern (would create gray dots)
   - Removed notebook lined-paper transparency
   - Verified via `page.evaluate()`: 0 gray elements in printed content
3. **Font scale slider**: Added `bodyScale` state (default 1.3), `theme.fs(base)` helper that multiplies non-title font sizes. Python script replaced 43 hardcoded `fontSize: N` with `fontSize: theme.fs(N)` in renderers. Title block's main `theme.titleSize` stays unscaled. Slider in right rail (1.0×–2.0×). Persisted in JSON save/load.
4. **Default width**: Changed from 380px → 384px (thermal printer dot width)

### What worked

- Using `page.evaluate()` to scan all computed styles in the paper-body element was extremely effective for verifying monochrome compliance
- The Python regex replacement script was the right call for 43 mechanical fontSize edits — doing it by hand would be error-prone
- Attaching `fs` to the theme object means all renderers can access it via `theme.fs(N)` without changing their prop signatures

### What didn't work

- Initially set `isAlreadyBW = true` in the auto-detect code which was backwards (should default false, set true only if all pixels pass). Fixed before commit.

### What was tricky to build

- The `EMBED_TXTFILES` symbol naming collision from Step 2 was a precursor to the theme.color issues here — everything in embedded firmware must be thought about from the "what actually gets rendered as pixels" perspective
- Distinguishing editor chrome (buttons, controls) from printed content when verifying monochrome — the `el.closest('.block-controls')` filter was essential

### What warrants a second pair of eyes

- The font scale slider at 2.0× may cause text to overflow the 384px paper width — no overflow handling was added
- The `theme.fs()` function rounds to 0.1 precision which could cause slight inconsistencies at extreme scale values

### What should be done in the future

- Rebuild the firmware bundle with these JSX changes and flash to device
- Test with real thermal printer at various font scale values
- Consider adding a "print" button that calls `/api/print/bitmap` directly from the Almanach Studio

### Technical details

- **Commits**: 2494359 (dithering + white paper), 101ffed (monochrome + 384px), baee459 (font scale slider)
- **Verified**: 0 gray pixels in printed content (96 content elements checked)
- **Font scale**: 43 renderer fontSize values wrapped with `theme.fs()`, title excluded
- **bodyScale default**: 1.3 (range 1.0–2.0, step 0.1, persisted in JSON)

---

## Step 4: Project Report — Textbook-Style Technical Article

### Prompt Context

**User prompt (verbatim):** "Create a detailed project report that explains how the almanach creation studio works, its implementation, its features, as a deep technical report written in the style of a textbook (see skills) and store it in the obsidian vault and then use `cp` to copy it over into this ticket."

**Inferred user intent:** Produce a comprehensive, durable technical article (not a changelog or summary dump) that teaches a future reader how the entire Almanach Studio system works, following the textbook-authoring skill's Peter Norvig style.

### What I did

1. Read the `textbook-authoring` skill and `obsidian-vault-writing` skill for style guidance
2. Read the existing vault article on thermal printers for cross-referencing
3. Read the existing ARTICLE exemplar (`Go Wasm Browser Playbook`) for formatting conventions
4. Wrote a ~26 KB article with:
   - YAML frontmatter (tags, aliases, repo, type: article)
   - Mermaid architecture diagrams (system overview, HTTP server, PNG export pipeline)
   - Prose paragraphs explaining *why* decisions were made, not just *what*
   - Code snippets with inline commentary
   - Tables comparing block types, asset sizes, dithering modes
   - Concrete failure modes with debug techniques
   - Working rules for future development
5. Stored in Obsidian vault at `Projects/2026/04/29/ARTICLE - Almanach Studio - ...`
6. Copied into ticket workspace at `reference/02-almanach-studio-project-report.md`
7. Related both files in the ticket index

### What worked

- The textbook-authoring skill's emphasis on "foundational first" prose produced a report that explains the system from concepts to implementation, not the other way around
- The Mermaid diagrams (system architecture, HTTP server routes, PNG export flow) replaced pages of prose that would have been harder to follow
- Cross-linking to the existing thermal printer article creates a knowledge graph: printer hardware → this article → the build pipeline

### What was tricky to build

- Balancing depth vs. length. The full JSX source is 2100 lines — documenting every renderer would make the report a code dump. Instead, I focused on the architecture, the data model, one representative renderer, and the non-obvious parts (monochrome enforcement, PNG export, EMBED_TXTFILES collision).

### What should be done in the future

- Upload to reMarkable for offline reading
- Add the "Print" button integration and update the report accordingly
- When the data API endpoints are added, document the live-data flow

---

## Step 5: Direct Print Button

### Prompt Context

**User prompt (verbatim):** "add direct print"

**Inferred user intent:** Replace the `window.print()` browser print with a button that renders the paper, binarizes it, and POSTs the raw 1-bit bitmap directly to the firmware's `/api/print/bitmap` endpoint.

### What I did

Replaced the `handlePrint` callback (which just called `window.print()`) with a function that:

1. Renders the paper to an off-screen canvas via SVG `foreignObject` (same technique as PNG export, but at 1× scale not 2×)
2. Fills white background, strips zigzag edges and block controls
3. Reads all pixels, converts to grayscale, thresholds at 128 → 1-bit B/W
4. Packs into MSB-first bitmap with width padded to next multiple of 8
5. POSTs to `/api/print/bitmap` with `X-Width` and `X-Height` headers
6. Shows spinner during render+send, success/error toast on completion

The Print button now shares the `exporting` state with the PNG button, so they can't run concurrently.

### What worked

- The existing PNG export code provided the SVG `foreignObject` rasterization pattern — just needed to change scale from 2× to 1× and skip the PNG encoding
- The `index.html` bitmap packing code was a direct reference for the MSB-first format
- Reusing `getInlineFontCss()` ensures fonts render correctly in the off-screen clone

### What was tricky

- Width must be divisible by 8 for the K118 printer. The paper width is 384 which divides evenly, but the padding logic handles any width correctly.
- The `handlePrint` dependency array needed `theme.paper` added for the zigzag-stripping logic.

### What warrants a second pair of eyes

- The 1× scale (384px canvas) means the print resolution is exactly 384 dots wide = full printer width. This is correct for the K118 but would need adjustment for different printers.
- No error recovery if the POST starts but the printer buffer overflows mid-print. The firmware handles flow control via UART CTS, so this should be safe.

### Technical details

- **Commit**: 99e0016
- **Bitmap format**: 1-bit packed, MSB first, `bytesPerRow = ceil(W/8) * 8 / 8`
- **API**: `POST /api/print/bitmap`, headers `X-Width`, `X-Height`, body = raw bitmap
- **Threshold**: grayscale < 128 → black (1), else white (0)
