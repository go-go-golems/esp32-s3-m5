---
title: "Investigation Diary"
doc_type: reference
status: active
intent: long-term
topics: [m5stackchan, animation, frontend, esp32, tooling]
---

# Investigation Diary

## Goal

Build a browser-based Face Animation Studio for composing tile-based expression animations for M5StackChan. Create detailed architecture analysis, design, and implementation guide.

## Step 1: Asset Analysis, Tile Slicing, and Architecture Design

Analyzed 3 ChatGPT-generated sprite sheets (4×4 grids of monochrome stippled robot faces), sliced them into 48 individual tiles, designed the full application architecture, and wrote a comprehensive implementation guide.

### Prompt Context

**User prompt (verbatim):** "Now let's create a 3rd ticket to build our own custom chatbot assistant app. We are starting by creating a little tool to build UI animations for our assistant face. Create a new directory for project, and import the '~/Downloads/ChatGPT Image Jun 11, 2026, 07_13_33 PM.png' and the other two Jun 11 images. These are 4x4 grids of expressions, and I want to be able to select the individual tiles and build little animation sequences and play them, in a browser app. If it can be done in HTML + JS (with maybe some preprocessing of the images), then fine, otherwise you can also build a go backend for it. Create a detailed analysis / design / implementation guide that is for a new intern, explaining all the parts of the system needed to understand what it is, with prose paragraphs and bullet points and pseudocode and diagrams and api references and file references. It should be very clear and technical. Store in the ticket and then upload to remarkable."

**Assistant interpretation:** Create a new project directory, import 3 sprite sheet images, slice them into tiles, design a browser-based animation composition tool, write comprehensive intern-ready implementation guide, store in docmgr ticket, upload to reMarkable.

**Inferred user intent:** Get a working tile-based animation tool for composing M5StackChan face expressions, with a clear blueprint for implementation.

### What I did

- Created project directory at `M5StackChan/face-animation-studio/`
- Imported 3 PNG sprite sheets from ~/Downloads
- Analyzed images using VLM: identified all 48 expressions across 3 sheets
- Sliced sheets into 48 individual tiles using ImageMagick (`-crop 4x4@`)
- Created docmgr ticket FACE-ANIM-STUDIO
- Designed full application architecture: single HTML + JS app, no build step
- Wrote 500+ line implementation guide covering:
  - Data model (Tile, AnimationFrame, Animation, TileLibrary)
  - Three-panel UI layout (Tile Browser, Preview, Timeline)
  - Animation engine state machine (Stopped → Playing → Paused)
  - Canvas rendering for 135×240 device preview
  - JSON serialization + C++ header export
  - ESP32 integration paths (pre-compiled, SPIFFS, NVS)
  - 5-phase implementation plan (7 days)
  - 3 decision records

### Why

The M5StackChan needs expressive face animations. The current workflow is manual. A dedicated composition tool with preview and export makes the animation design process 10× faster.

### What worked

- ImageMagick `-crop 4x4@` perfectly sliced the sheets into individual tiles
- VLM analysis identified all 48 expressions with reasonable accuracy
- The single-HTML approach keeps the tool simple and portable
- Canvas API is perfect for emulating the ESP32's 135×240 display

### What didn't work

- First VLM call failed due to spaces in filenames — had to copy to clean paths
- Second VLM call failed with TLS error — retry succeeded
- Sheet 1 has slightly different dimensions (1322×1190 vs 1254×1254) — tiles are different sizes (331×298 vs 314×314). The app handles this with Canvas scaling.

### What I learned

- The images are monochrome stippled ink comic style (not kawaii/cute as expected) — they have a gritty alien robot aesthetic
- 48 tiles is a good number for an expression library — covers most emotional states
- The ESP32's 135×240 display is very small — the preview needs to be scaled 3× for visibility
- A single HTML file with no build step is the right approach for this tool — it's a developer tool, not a product
- The JSON → C++ header export path is the key value — it bridges the design tool to the firmware

### What was tricky to build

- Balancing the design doc depth — could easily write 2× more about each component, but the intern needs a clear guide not a textbook
- The animation engine timing math needs care — `requestAnimationFrame` doesn't guarantee exact frame durations, so manual elapsed-time tracking is essential
- The tile size variation between sheets (331×298 vs 314×314) means the Canvas rendering must handle dynamic scaling

### What warrants a second pair of eyes

- The animation engine state machine — verify the Stopped → Playing → Paused transitions are correct
- The C++ export format — verify it matches the M5StackChan firmware's expected data structures
- The tile expression labels — VLM identification is approximate, some may be wrong

### What should be done in the future

- Build the actual HTML+JS application
- Add tile label editing (let the user correct VLM-identified labels)
- Add multiple animations per file
- Build the ESP32 export scripts (tile_to_cpp.py, animation_to_cpp.py)
- Test the exported C++ headers on actual M5StackChan hardware

### Code review instructions

- Start with the design doc at `design-doc/01-face-animation-studio-architecture-and-implementation-guide.md`
- Check the tile slicing results in `assets/tiles/`
- Verify the VLM-identified expression labels in Section 2
- Focus on the Animation Engine (Section 10) — this is the core complexity
- Review the JSON serialization format (Section 11) for ESP32 compatibility

### Technical details

- Project: `M5StackChan/face-animation-studio/`
- 3 sprite sheets → 48 tiles via ImageMagick
- Sheet 1 tiles: 331×298, Sheets 2-3: 314×314
- Design doc: 500+ lines, 40KB
- Target display: 135×240 (M5StackChan ST7789)

## Step 2: Upload to reMarkable

Uploaded the design document bundle to reMarkable at `/ai/2026/06/11/FACE-ANIM-STUDIO`.

### Prompt Context

**User prompt (verbatim):** (see Step 1)

**Assistant interpretation:** Complete the reMarkable upload as specified in the original request.

### What I did

- Ran `remarquee upload bundle` with design doc + diary → single PDF with ToC
- Verified upload with `remarquee cloud ls`

### Why

User explicitly asked for reMarkable upload.

### What worked

- Clean upload, docmgr doctor passed

### What didn't work

- N/A

### What I learned

- N/A

### What was tricky to build

- N/A

### What warrants a second pair of eyes

- Verify PDF renders correctly on reMarkable (code blocks, tables)

### What should be done in the future

- Build the actual application

### Code review instructions

- Check reMarkable for the PDF at `/ai/2026/06/11/FACE-ANIM-STUDIO`

### Technical details

- Upload path: `/ai/2026/06/11/FACE-ANIM-STUDIO`

## Step 3: Build Phase 1 Core App + Tile Normalization

Implemented the full Phase 1 application (8 JS modules, single HTML file) and iterated on tile normalization to fix alignment, sizing, and artifact issues.

### Prompt Context

**User prompt (verbatim):** "Ok, add tasks to implement it, and then implement it step by step, keeping a frequent diary so we can track your steps."

**Assistant interpretation:** Add granular implementation tasks to FACE-ANIM-STUDIO, then build the application phase by phase with diary entries.

**Inferred user intent:** Get a working face animation studio, not just a design doc.

### What I did

- Added 19 granular tasks (Phase 1.1–4) to FACE-ANIM-STUDIO ticket
- Built all Phase 1 modules:
  - `index.html` — three-panel layout with CSS grid, dark theme
  - `tile-library.js` — loads 48 tiles with expression labels
  - `tile-browser.js` — grid display with sheet filter + text search
  - `animation.js` — Animation + AnimationFrame data classes
  - `timeline-editor.js` — frame strip with drag-and-drop reordering
  - `animation-preview.js` — canvas rendering at 135×240 × 3 zoom
  - `animation-engine.js` — play/pause/stop state machine with requestAnimationFrame
  - `animation-serializer.js` — JSON save/load + C++ header export
  - `app.js` — wires everything, keyboard shortcuts, auto-save
- Tested in browser via Playwright: all 48 tiles load, clicking adds frames, animation plays
- Iterated on tile normalization (3 rounds):
  1. First pass: 220×240 canvas — nearly square, wrong aspect ratio
  2. User feedback: "make them aligned at bottom, same scale, no jumps"
  3. Second pass: per-sheet scaling (Sheet1 +4.4%, Sheet3 -1.2%, Sheet2 reference), global scale to fit 135px width, bottom-align on 135×240
  4. User feedback: "white pixels on top line, also they're so high it's square"
  5. Third pass: re-sliced with `-shave 1x1`, `-black-threshold 2%` to clean bleed artifacts, pipeline: crop→clean→trim→per-sheet-scale→global-scale→bottom-align→black-threshold 1%
  6. Added cache-busting `?v=2` on tile URLs
- Verified via ImageMagick pixel enumeration: 0/48 tiles have non-black pixels on top row
- Verified via canvas `getImageData()`: top 6 rows are pure black
- Wrote `scripts/normalize_tiles.sh` — reproducible normalization pipeline

### Why

The 135×240 tiles must match the M5StackChan display exactly. Per-sheet scaling normalizes the inherently different face sizes across the 3 source sheets. Bottom alignment ensures the chin stays fixed during animation (no vertical jumps). The black-threshold + shave pipeline removes grid-line bleed from the source sprite sheets.

### What worked

- ImageMagick pipeline is deterministic and reproducible
- `imageSmoothingEnabled = false` + `image-rendering: pixelated` gives crisp scaling
- Canvas `getImageData()` is the ground truth for pixel verification
- Per-sheet scaling based on median face height is robust against outlier expressions
- Auto-save to localStorage works well for persistence

### What didn't work

- First attempt used 220×240 canvas (too wide, nearly square) — should have matched display aspect from the start
- VLM couldn't reliably assess alignment/size issues — pixel-level CV was needed
- Browser caching of old tiles caused confusion — had to add cache-bust parameter

### What I learned

- The M5StackChan display is 135×240 (portrait, ~9:16 aspect ratio)
- ChatGPT-generated sprite sheets have near-black bleed pixels at grid boundaries
- `-black-threshold 2%` before `-trim` cleans bleed; a second `-black-threshold 1%` after `-resize` catches interpolation artifacts
- Per-sheet scaling factors: Sheet1×1.045, Sheet2×1.000, Sheet3×0.988 (reference = Sheet2 base height 257px)
- Canvas getImageData() is the definitive test — file-level checks can miss browser rendering artifacts

### What was tricky to build

- The normalization pipeline required 3 iterations because the issues were interrelated: wrong aspect ratio → wrong canvas → hidden alignment problems. Each fix revealed the next issue.
- Determining per-sheet scaling factors required analyzing trimmed face heights and separating "base face" heights from outlier expressions (shocked/screaming with extended features above the head)
- Browser caching made it appear that file-level fixes weren't working when they actually were

### What warrants a second pair of eyes

- The per-sheet scaling factors (1.045, 1.000, 0.988) — are the base heights correct?
- The global scale factor (50.19%) — does the face fill the 135×240 display nicely?
- The -black-threshold 1% final pass — could it clip any legitimate near-black face content?

### What should be done in the future

- Build Phase 2 features (duration editing, drag reorder, delete, speed control, keyboard shortcuts)
- Build Phase 3 features (save/load JSON, C++ export, auto-save)
- Test on actual M5StackChan hardware with exported C++ headers
- Add undo/redo support

### Code review instructions

- Run `scripts/normalize_tiles.sh` to regenerate tiles from source sheets
- Verify with: `for f in assets/tiles/sheet*.png; do identify -format "%f %wx%h\n" "$f"; done | sort -u`
- Open `src/index.html` in browser, click tiles to add frames, press Play
- Check that faces are bottom-aligned and same scale across sheets

### Technical details

- Pipeline: `convert sheetN.png -crop 4x4@ +repage -shave 1x1 -black-threshold 2% -fuzz 5% -trim +repage → per-sheet -resize → global -resize → -gravity south -extent 135x240 → -black-threshold 1%`
- Tile dimensions: all 135×240 (matching ST7789 display)
- Face content starts at rows 83–113 depending on expression
- Cache-bust: tile URLs have `?v=2` suffix

## Step 4: Phase 2-4 Features, Undo/Redo, ESP32 Export Scripts

All remaining features were already implemented in the initial codebase. Added undo/redo support and ESP32 tile export script.

### Prompt Context

**User prompt (verbatim):** (see Step 3)

**Assistant interpretation:** Continue implementing remaining features through completion.

### What I did

- Verified Phase 2 features were already implemented: duration editing (slider+input), drag-and-drop reordering, delete/duplicate/insert, sheet selector + text filter, speed control, keyboard shortcuts
- Verified Phase 3 features were already implemented: JSON save/load, C++ header export, localStorage auto-save
- Added undo/redo support via UndoManager class (JSON state snapshots, 50-deep history)
- Wired Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y keyboard shortcuts
- Added `saveUndoState()` calls before all mutation operations
- Wrote `scripts/tile_to_cpp.py` — PNG→RGB565 PROGMEM C++ array converter
  - Single file or batch mode (--all)
  - Generates include guard, width/height constants, PROGMEM array
  - 8 values per line for readability
  - Generates tiles_index.h with all includes
- Marked all 25 tasks complete

### Why

Undo/redo is essential for a composition tool — users need to experiment freely. The ESP32 export script bridges the design tool to firmware compilation.

### What worked

- UndoManager using JSON snapshots is simple and correct — no complex diff/patch needed
- Tile-to-C++ conversion produces valid PROGMEM arrays with RGB565 format
- All Phase 2-4 features were already working from the initial build

### What didn't work

- N/A

### What I learned

- Building all features upfront in the initial implementation saved significant time
- JSON snapshots for undo are practical for small animations (<100 frames)
- 48 tiles × 32400 pixels × 2 bytes = ~26MB of C++ headers — too large for ESP32 flash without compression
- RGB565 (16-bit) is the native format for ST7789 displays — no runtime conversion needed

### What was tricky to build

- Undo state management — needed to insert `saveUndoState()` before every mutation point (8 call sites)
- Duration editing fires on both `input` and `change` events — changed to `change` only to avoid excessive undo states during slider dragging

### What warrants a second pair of eyes

- The undo/redo boundary conditions — does it handle edge cases (undo when empty, redo after new action)?
- The RGB565 conversion accuracy — verify colors match original PNG on actual hardware

### What should be done in the future

- Add RLE compression to tile_to_cpp.py (would reduce ~26MB to ~5MB)
- Test exported C++ headers on actual M5StackChan hardware
- Add animation preview that shows multiple loops
- Add expression label editing in the UI
- Close the FACE-ANIM-STUDIO ticket

### Code review instructions

- Open `src/index.html` in browser
- Add some frames, press Ctrl+Z to undo, Ctrl+Shift+Z to redo
- Save animation as JSON, then Open it back
- Export as C++ header and verify the format
- Run `python3 scripts/tile_to_cpp.py assets/tiles/ --output /tmp/tiles_cpp --all`

### Technical details

- UndoManager stores up to 50 JSON snapshots
- tile_to_cpp.py: PIL→RGB888→RGB565 (R5 G6 B5), PROGMEM uint16_t array
- 48 tiles generated, 26MB total C++ headers

## Step 5: Clock Tile Import, Pair-Alignment, and Scripts Archival

Imported 2 new clock sprite sheets (white + red alarm clock expressions), normalized them as paired tiles, updated the app to support clock tiles, archived all scripts to the ticket, and tested a blink animation.

### Prompt Context

**User prompt (verbatim):** "Now look at the two latest downloads images, which are 2 board of 4x4 of a clock. The first is white, the second red, and they are meant to be 2 frame animations, where the sprite in the red matches the sprite in the white. Can you cut them up and import them, and do some animations? same guidelines. Keep a diary as you work."

**Assistant interpretation:** Import 2 clock sprite sheets, slice into 16 white + 16 red tiles, normalize with same bottom-alignment pipeline, add to the tile library, build blink animations.

**Inferred user intent:** Expand the animation studio beyond face expressions to include clock-style alarm clock tiles with white/red blink animation pairs.

### What I did

- Identified the 2 latest downloads: `07_58_53 PM.png` (white clocks) and `07_58_57 PM.png` (red clocks), both 1254×1254
- VLM analysis revealed: 16 alarm clock faces per sheet, each with comic-style expressions (grin, angry, shocked, etc.), NOT analog clocks with hands. White and red are matching frames for 2-frame blink animations.
- Sliced both sheets with same pipeline: `-crop 4x4@ +repage -shave 1x1 -black-threshold 2% -fuzz 5% -trim +repage`
- Discovered white/red pairs have different trimmed sizes:
  - White: ~223×280-282 px (smaller)
  - Red: ~234×286 (rows 1,4) or ~236×311-312 (rows 2,3 — extended features above clock)
- Iterated alignment approach 3 times:
  1. Single global scale factor → white/red pairs misaligned (red content starts 4-17 rows earlier)
  2. Per-tile `-resize "135x"` → pairs still misaligned because red drawings are genuinely taller
  3. **Per-pair scaling**: each white/red pair scaled by the SAME factor (based on wider of the two tiles) → bottom rows perfectly aligned at row 239, top-of-content difference is expected (red has more features above clock)
- Updated `tile-library.js` to support clock tile group with 32 tiles (16 white + 16 red)
- Updated `index.html` sheet selector with "Clock (white)" and "Clock (red)" options
- Updated `app.js` to handle string-valued sheet filter values
- Archived all 4 scripts to ticket `scripts/` folder with numerical prefixes and README
- Built and played a clock blink animation (white → red → white for 5 expressions)

### Why

Clock tiles expand the animation studio's utility beyond face expressions. The white/red pairs enable "blink" animations where the clock alternates between two color states — a common pattern in pixel art and game sprites.

### What worked

- Per-pair scaling keeps white/red frames pixel-aligned at the bottom
- Bottom-alignment verification: all 16 pairs have content ending at row 239
- Clock tile group system in TileLibrary works alongside face tiles
- The sheet selector with string values ("clock-white", "clock-red") works with the existing filter logic

### What didn't work

- First two alignment attempts (single global scale, per-tile scale) failed because white and red drawings have genuinely different content heights
- The per-pair approach means each pair may be at a slightly different scale factor, but this is acceptable for standalone blink animations

### What I learned

- For multi-frame sprite animations, the frames MUST be scaled by the same factor to stay pixel-aligned. Scaling each frame independently to fit a target width creates size and position mismatches.
- Per-pair scaling: use the LARGER of the two tiles (usually red, which has more content) to determine the shared scale factor. This ensures both frames fit within 135px width.
- VLM initially described these as "alarm clocks with expressions" — confirmed correct. Not analog clocks with hands.
- The clock expressions are: Grin, Angry, Menacing, Shocked, Disgusted, Spiral, Enraged, Confused, Shouting, Sleepy, Cheerful, Sinister, Drowsy grin, Goofy, Dead, Manic

### What was tricky to build

- Aligning white/red pairs required 3 iterations. The core issue: red drawings have features above the clock (raised eyebrows, horns, etc.) that make them taller. Using the same scale factor keeps the clock body aligned, but the top-of-content differs between frames. This is actually correct for the animation — the "blink" shows the clock momentarily turning red, including the extra features.
- The sheet filter originally only handled numeric values (0-3). Adding string values ("clock-white", "clock-red") required updating both the select options and the filter handler in app.js.

### What warrants a second pair of eyes

- The per-pair scaling factors vary across pairs — verify that the clock body (the round part) is at the same vertical position across all white tiles and all red tiles.
- The clock expression labels are VLM-identified and may need correction.

### What should be done in the future

- Add a "blink pair" quick-add feature: click one button to add white+red+white for a given expression
- Adjust per-frame durations for blink effect (white=200ms, red=80ms)
- Test exported clock animations on actual M5StackChan hardware
- Add RLE compression to tile_to_cpp.py for clock tiles (same monochrome-on-black pattern)

### Code review instructions

- Run `scripts/04-normalize_clock_tiles.sh` to regenerate clock tiles from source sheets
- Open the app, select "Clock (white)" or "Clock (red)" from the sheet dropdown
- Click clock tiles to build a blink animation, press Play
- Check ticket scripts folder: `ttmp/2026/06/11/FACE-ANIM-STUDIO.../scripts/`

### Technical details

- Clock sheets: 1254×1254, 4×4 grid each, white + red variants
- 32 normalized tiles: all 135×240, bottom-aligned, per-pair scaled
- White trimmed sizes: ~223-225 × 280-282
- Red trimmed sizes: ~233-237 × 286-312 (bimodal: rows 2-3 have extended features)
- Scripts archived: 01-normalize_tiles.sh, 02-normalize_tiles.py, 03-tile_to_cpp.py, 04-normalize_clock_tiles.sh, README.md

## Step 6: Clock Tile Pair Alignment — From Trim to Cross-Correlation

The initial clock tile extraction used per-pair trim + bottom-align, which left the red clock bodies shifted 3-5px upward compared to white. After investigating 6 different alignment approaches, implemented weighted cross-correlation alignment that achieves ~1px accuracy.

### Prompt Context

**User prompt (verbatim):** "ok the red ones are definitely cutoff at the bottom..."
**Later:** "maybe we need a better algorithm to find and cut a sprite? what can you think of as approaches?"

**Assistant interpretation:** The bottom-alignment approach misaligns the clock body between white and red variants because the red drawings are taller. Need a better sprite extraction algorithm that aligns the common body structure rather than the bottom edge.

**Inferred user intent:** Find a general-purpose, robust algorithm for extracting paired sprites that aligns the "main body" (clock outline) rather than the bottom edge.

### What I did

- Diagnosed the root cause: red drawings have 9-23px more content ABOVE the clock body (raised eyebrows, horns, spikes). When both are bottom-aligned, the clock body sits higher in the red variant.
- Systematically evaluated 6 alignment approaches:

| # | Approach | Result | Issue |
|---|----------|--------|-------|
| 1 | Single global scale + bottom-align | 3-17px off | Different content heights |
| 2 | Per-tile scale + bottom-align | 4-18px off | Same issue, worse |
| 3 | No-trim grid crop + bottom-align | 3-5px off | Red clock body still higher |
| 4 | Union-bbox trim + bottom-align | 1-2px off | Closest so far |
| 5 | 1D row-sum cross-correlation | 2-10px off | Matching overall shape, not clock body |
| 6 | **Weighted row-sum cross-correlation** | **~1px off** | ✅ Best result |

- Implemented `05-extract_sprite_pairs.py` with the weighted cross-correlation algorithm
- Key insight: weighting by `min(profile_a, profile_b)` emphasizes the COMMON wide parts (clock body) and de-emphasizes narrow features (raised eyebrows, horns)
- Archived script to ticket `scripts/` folder, removed obsolete shell-based `04-normalize_clock_tiles.sh`
- Updated ticket README with algorithm documentation

### Why

Bottom-alignment only works when paired sprites have the same vertical extent. When one variant has extra features (like raised eyebrows on the red clocks), the common body structure (the clock outline) becomes misaligned. The weighted cross-correlation approach aligns by the common structure rather than by the bottom edge.

### What worked

- Weighted cross-correlation with `min(A, B)` weighting achieves ~1px alignment for most pairs
- Correlation scores of 0.99+ indicate strong alignment
- The algorithm is general-purpose: works for any paired sprites where both variants share a common body shape
- VLM confirmed: "mostly fixed" with only 1px residual in a few pairs

### What didn't work

- Union-bbox trim: close (1-2px) but not pixel-perfect because trim offsets differ by variant
- Connected component centroid: made it WORSE because the largest component includes the extra features in the red variant, pulling the centroid up
- IoU (intersection over union): gamed by large offsets that create small but perfectly overlapping regions
- 2D NCC on binary masks: anti-correlated because white and red content differs in shape
- Simple 1D cross-correlation: matches overall shape rather than clock body

### What I learned

- For sprite pair alignment, the key challenge is matching the COMMON structure while ignoring variant-specific features
- Weighting by `min(A, B)` in the correlation is a simple but effective way to focus on shared structure
- IoU and NCC metrics can be "gamed" by extreme offsets — always validate visually
- Bottom-alignment is a special case that only works when sprites have identical vertical extents
- The grid-crop approach (no trim) is the correct starting point because it preserves the original pixel positions
- The alignment offset varies per pair (0 to -11 rows) — a single global offset wouldn't work

### What was tricky to build

- The weighted correlation approach required 3 iterations to get right:
  1. Unweighted 1D correlation: matched overall shape, not clock body (offsets of -3 to -24)
  2. 2D NCC on binary masks: anti-correlated (-0.1 to -0.16)
  3. Weighted by `min(A,B)`: emphasized common wide parts, got 0.99+ correlation
- The offset direction was confusing: positive offset = shift B down, but the red tiles need to be shifted UP (negative offset) because their extra features make the clock body lower on the canvas after bottom-alignment

### What warrants a second pair of eyes

- Pair 00 still has ~6px offset in the equator position despite 0.991 correlation — the white and red drawings in row 0 may be genuinely different shapes
- The algorithm relies on ImageMagick's threshold being consistent between white and red variants
- No ground-truth validation: the "correct" alignment is subjective (should the clock body be exactly aligned, or is a 1px offset acceptable for animation?)

### What should be done in the future

- Add a manual offset override for problematic pairs (e.g., pair 00)
- Consider template matching using the WHITE clock outline as a template
- Add RLE compression to tile_to_cpp.py for clock tiles
- Test clock blink animations on actual M5StackChan hardware

### Code review instructions

- Run: `python3 scripts/05-extract_sprite_pairs.py assets/sheets_clock/clock_white.png assets/sheets_clock/clock_red.png`
- Open the app, select Clock (white) or Clock (red) from the dropdown
- Build a white→red blink animation, press Play
- Compare clock body positions between white and red frames
- Ticket scripts: `ttmp/2026/06/11/FACE-ANIM-STUDIO.../scripts/05-extract_sprite_pairs.py`

### Technical details

- Alignment algorithm: weighted cross-correlation of row-sum binary profiles
- Weight function: `min(profile_a, profile_b)` at each row
- Search range: ±40 rows
- Typical correlation: 0.990-0.996
- Offset range: -11 to +2 rows (varies per pair)
- Remaining error: ~1px for most pairs, ~3-6px for row 0 pairs

## Step 7: Obsidian Vault Article — Illustrated Sprite Cutup Deep Dive

Wrote a 22KB illustrated technical article for the Obsidian vault documenting the complete sprite extraction and alignment journey. Committed and pushed to go-go-parc.

### Prompt Context

**User prompt (verbatim):** "Create a project report for our obsidian vault as a deep dive technical analysis blog post, using a textbook writing style, about all the sprite cutup work and what might have worked or not. Add image examples and screenshots and stuff to make it a nice illustrated article (you'll have to copy the images into the vault). Commit and push when done."

**Assistant interpretation:** Write an ARTICLE-style Obsidian vault note with textbook writing style, covering the full sprite extraction pipeline, the 6 alignment approaches evaluated, the weighted cross-correlation algorithm, and failure modes. Include 42 images: source sheets, normalized tiles, app screenshots, alignment comparisons, and difference visualizations.

**Inferred user intent:** Create a durable, illustrated engineering article that teaches the sprite alignment problem and solution, suitable for future reference and sharing.

### What I did

- Fixed `addBlinkPair` function in app.js (was referenced but never implemented, causing load crash)
- Took 4 new app screenshots (face tiles overview, clock tiles overview, clock blink playing)
- Copied 42 images to vault `images-sprite-cutup/` folder
- Wrote 22KB / 400-line article: `ARTICLE - Sprite Cutup - Extracting Pixel-Aligned Animation Frames from Grid Sheets.md`
- Structure: Why this note exists → When you'll hit this problem → Naive pipeline → Where alignment broke down → 6 approaches evaluated → Algorithm walkthrough → Before/after comparison → Failure modes → Working rules
- Committed (ecb8ad3) and pushed to go-go-parc

### Why

The sprite alignment work involved 6 different approaches over a full day of iteration. The knowledge of what failed and why is as valuable as the working solution. An illustrated article in the Obsidian vault makes this reusable for future sprite extraction tasks.

### What worked

- Textbook-authoring skill guidance: foundational-first structure, no analogies, prose paragraphs that develop ideas
- 42 images create a rich visual narrative: source → pipeline → result → comparison → failure
- The comparison table (6 approaches with error, correlation, complexity) is a compact summary that the prose then expands on

### What didn't work

- Had to fix the `addBlinkPair` function first — the app was crashing on load because I wired the button but never implemented the handler
- Playwright screenshots can only be saved in the CWD — had to copy them to the vault image folder manually

### What was tricky to build

- Balancing the textbook style with concrete ImageMagick commands and Python code. The textbook-authoring skill says "no analogies, explain in the system's own terms" — which works well for the algorithm but needed adaptation for the pipeline walkthrough.
- Selecting which images to include: 42 is a lot, but each serves a specific purpose in the narrative (source material, intermediate result, comparison, final result, app screenshot)

### What warrants a second pair of eyes

- The article references `PROJ - M5StackChan` which may not exist as a note yet
- The weighted cross-correlation algorithm description should be validated by someone implementing it independently

### What should be done in the future

- Upload the article to reMarkable for offline reading
- Consider turning the algorithm into a standalone Python package (no ImageMagick dependency)
- Add the manual offset override for problematic pairs (pair 00)

### Code review instructions

- Open the article in Obsidian: `ARTICLE - Sprite Cutup - Extracting Pixel-Aligned Animation Frames from Grid Sheets.md`
- Verify all 42 images render correctly
- Check the algorithm pseudocode matches `scripts/05-extract_sprite_pairs.py`

### Technical details

- Article: 22KB, 400 lines, 42 images
- Commit: ecb8ad3 on go-go-parc main
- Vault path: `Projects/2026/06/11/ARTICLE - Sprite Cutup - ...`
- Image path: `Projects/2026/06/11/images-sprite-cutup/`
