---
title: "Face Animation Studio Architecture and Implementation Guide"
doc_type: design-doc
status: active
intent: long-term
topics: [m5stackchan, animation, frontend, esp32, tooling]
---

# Face Animation Studio — Architecture and Implementation Guide

## Executive Summary

Face Animation Studio is a browser-based tool for composing tile-based facial expression animations for the M5StackChan desktop robot. It takes pre-sliced sprite sheets (4×4 grids of expression tiles), lets the user select individual tiles, arrange them into animation sequences with timing, and play back the result in real time. The final output is a JSON animation descriptor that can be loaded on the ESP32 to drive the robot's LCD face.

The tool is a single-page HTML + JavaScript application with no build step required. Image preprocessing (sprite sheet slicing) is done offline with ImageMagick. The app runs entirely in the browser using the Canvas API for rendering and `requestAnimationFrame` for playback.

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [Source Asset Inventory](#2-source-asset-inventory)
3. [System Architecture](#3-system-architecture)
4. [Data Model](#4-data-model)
5. [Tile Sheet Preprocessing](#5-tile-sheet-preprocessing)
6. [Application Layout and UI](#6-application-layout-and-ui)
7. [Tile Browser Component](#7-tile-browser-component)
8. [Timeline Editor Component](#8-timeline-editor-component)
9. [Animation Preview Component](#9-animation-preview-component)
10. [Animation Engine](#10-animation-engine)
11. [Serialization Format](#11-serialization-format)
12. [ESP32 Integration Path](#12-esp32-integration-path)
13. [Implementation Plan](#13-implementation-plan)
14. [Decision Records](#14-decision-records)
15. [File and API Reference](#15-file-and-api-reference)

---

## 1. Problem Statement

The M5StackChan robot needs to display expressive faces on its 135×240 TFT LCD. Creating these animations requires:

1. **Selecting expression frames** from a library of pre-drawn tiles
2. **Sequencing them with timing** (frame duration, transitions)
3. **Previewing the animation** to verify it looks correct
4. **Exporting the sequence** in a format the ESP32 firmware can consume

Currently there is no tool for this. The workflow is manual: draw frames, load them on the device, test, iterate. Face Animation Studio automates the composition, preview, and export steps.

### Requirements

- Load 3 sprite sheets (4×4 grids each, 48 total tiles)
- Select individual tiles by clicking
- Build animation sequences (ordered list of tile references + timing)
- Play animations in real time at the target frame rate
- Adjust per-frame duration with a visual timeline
- Export animations as JSON for ESP32 consumption
- Import previously saved animations for editing
- No server required — runs as a static HTML file

### Non-Goals

- Drawing or editing tiles (use an external image editor)
- Generating tiles programmatically (use AI or manual drawing)
- Real-time device preview (save the JSON, load on device separately)
- Audio synchronization (future feature)

---

## 2. Source Asset Inventory

### Sprite Sheets

Three PNG images, each containing a 4×4 grid of facial expression tiles on a black background:

| Sheet | File | Dimensions | Tile Size | Style |
|-------|------|-----------|-----------|-------|
| Sheet 1 | `assets/sheets/sheet1.png` | 1322×1190 | ~331×298 | Monochrome stippled ink comic |
| Sheet 2 | `assets/sheets/sheet2.png` | 1254×1254 | ~314×314 | Monochrome stippled ink comic |
| Sheet 3 | `assets/sheets/sheet3.png` | 1254×1254 | ~314×314 | Monochrome stippled ink comic |

### Tile Expressions (Identified by VLM)

**Sheet 1** (Row by Row, Left→Right):
| # | Expression | # | Expression |
|---|-----------|---|-----------|
| 00 | Side-eye / Suspicious | 01 | Angry / Glare |
| 02 | Sleepy / Eyes closed | 03 | Stern / Alert |
| 04 | Looking right / Wide-eyed | 05 | Skeptical / Squint |
| 06 | Scowl / Disgust | 07 | Surprised / Wide eyes |
| 08 | Suspicious / Narrowed | 09 | Small "o" mouth / Surprise |
| 10 | Eyes closed / Neutral | 11 | Annoyed side-eye |
| 12 | Drowsy / Half-lidded | 13 | Neutral |
| 14 | Side-eye / Skeptical | 15 | Neutral looking right |

**Sheet 2** (Row by Row, Left→Right):
| # | Expression | # | Expression |
|---|-----------|---|-----------|
| 00 | Neutral | 01 | Yelling / Angry (mouth wide) |
| 02 | Sleepy / Half-closed | 03 | Sad / Downcast |
| 04 | Shocked (big eyes, "O" mouth) | 05 | Pout / Sad |
| 06 | Scowl / Angry | 07 | Stern frown |
| 08 | Screaming | 09 | Skeptical / Side glance |
| 10 | Rage / Gritted teeth | 11 | Playful tongue-out |
| 12 | Annoyed side glance | 13 | Worried / Anxious (large pupils) |
| 14 | Disgust / Sneer | 15 | Shocked / Wide-eyed |

**Sheet 3** (Row by Row, Left→Right):
| # | Expression | # | Expression |
|---|-----------|---|-----------|
| 00 | Neutral | 01 | Big grin / Teeth |
| 02 | Suspicious / Glare | 03 | Curious / Puckered "huh?" |
| 04 | Pout / Sad | 05 | Shocked (wide eyes) |
| 06 | Stern | 07 | Sly / Smirking |
| 08 | Scream / Yell | 09 | Annoyed side-eye |
| 10 | Rage / Gritted teeth | 11 | Goofy tongue-out |
| 12 | Content / Eyes closed | 13 | Worried / Bug-eyes |
| 14 | Wince / One eye squeezed | 15 | Surprised "O" mouth |

### Pre-Sliced Tiles

48 individual PNG files in `assets/tiles/`:
- `sheet1_00.png` through `sheet1_15.png` (331×298 px)
- `sheet2_00.png` through `sheet2_15.png` (314×314 px)
- `sheet3_00.png` through `sheet3_15.png` (314×314 px)

---

## 3. System Architecture

The application is a single HTML file with embedded CSS and JavaScript. No build step, no npm, no framework.

```
┌─────────────────────────────────────────────────────────────────────┐
│  Face Animation Studio (single-page app)                           │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │  index.html                                                    │ │
│  │  ┌─────────────────┐  ┌───────────────────────────────────┐  │ │
│  │  │  Tile Browser    │  │  Animation Preview               │  │ │
│  │  │  (left panel)    │  │  (center panel)                  │  │ │
│  │  │                  │  │                                   │  │ │
│  │  │  ┌─────┐┌─────┐ │  │  ┌─────────────────────────────┐ │  │ │
│  │  │  │ 😐  ││ 😠  │ │  │  │                             │ │  │ │
│  │  │  └─────┘└─────┘ │  │  │    Current Frame             │ │  │ │
│  │  │  ┌─────┐┌─────┐ │  │  │    (Canvas 135×240)         │ │  │ │
│  │  │  │ 😴  ││ 😮  │ │  │  │                             │ │  │ │
│  │  │  └─────┘└─────┘ │  │  └─────────────────────────────┘ │  │ │
│  │  │  ... 48 tiles   │  │                                   │  │ │
│  │  └─────────────────┘  │  ▶ ⏸ ⏹  [1x] [2x] [0.5x]      │  │ │
│  │                       └───────────────────────────────────┘  │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │  Timeline Editor (bottom panel)                         │ │ │
│  │  │                                                          │ │ │
│  │  │  | 😐 | 😠  | 😮  | 😐  | 😴  | 😐  |               │ │ │
│  │  │  |100ms|200ms|150ms|100ms|500ms|100ms|               │ │ │
│  │  │  ◄─────────────────────────────────────────►           │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  │                                                              │ │
│  │  Toolbar: [New] [Open] [Save] [Export JSON] [Export C++]     │ │
│  └──────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘

External files:
  assets/tiles/*.png     — 48 pre-sliced tile images
  assets/sheets/*.png    — 3 original sprite sheets (reference only)
```

### Data Flow

```
1. User clicks tile in Tile Browser
2.  → Tile added to Timeline at cursor position
3.  → Timeline re-renders
4.  → If playing, Animation Engine picks up the new frame
5. User adjusts frame duration slider
6.  → Timeline frame metadata updated
7.  → Animation Engine timing updated
8. User clicks Play
9.  → Animation Engine starts requestAnimationFrame loop
10.  → Engine reads current frame from Timeline
11.  → Engine draws tile to Preview Canvas (scaled to 135×240)
12.  → Engine waits for frame duration
13.  → Engine advances to next frame
14. User clicks Save
15.  → Timeline serialized to JSON
16.  → JSON downloaded as .json file
```

---

## 4. Data Model

### Tile

A Tile represents a single expression image from the sprite sheet.

```javascript
class Tile {
  constructor(sheet, index, label) {
    this.id = `sheet${sheet}_${String(index).padStart(2, '0')}`;  // e.g., "sheet1_03"
    this.sheet = sheet;        // 1, 2, or 3
    this.index = index;        // 0-15
    this.label = label;        // Human-readable expression name
    this.src = `assets/tiles/${this.id}.png`;  // Image file path
    this.image = null;         // HTMLImageElement (loaded lazily)
    this.naturalWidth = 0;     // Original tile width
    this.naturalHeight = 0;    // Original tile height
  }
}
```

### AnimationFrame

An AnimationFrame is a reference to a Tile with timing metadata.

```javascript
class AnimationFrame {
  constructor(tileId, durationMs = 100) {
    this.tileId = tileId;       // References Tile.id
    this.durationMs = durationMs;  // How long to display this frame
  }
}
```

### Animation

An Animation is an ordered sequence of AnimationFrames with metadata.

```javascript
class Animation {
  constructor(name = "untitled") {
    this.name = name;
    this.frames = [];           // Array of AnimationFrame
    this.loop = true;           // Whether to loop
    this.fps = 10;             // Default playback speed
    this.createdAt = new Date().toISOString();
    this.modifiedAt = new Date().toISOString();
  }

  get totalDurationMs() {
    return this.frames.reduce((sum, f) => sum + f.durationMs, 0);
  }

  get frameCount() {
    return this.frames.length;
  }
}
```

### TileLibrary

The TileLibrary holds all loaded tiles and their images.

```javascript
class TileLibrary {
  constructor() {
    this.tiles = new Map();     // tileId → Tile
    this.sheets = new Map();    // sheetNum → Tile[]
  }

  async loadAll() {
    // Load all 48 tiles, create Image objects, wait for them to load
    // Populate this.tiles and this.sheets
  }

  getTile(tileId) {
    return this.tiles.get(tileId);
  }

  getSheet(sheetNum) {
    return this.sheets.get(sheetNum);
  }

  getAllTiles() {
    return Array.from(this.tiles.values());
  }
}
```

### Relationship Diagram

```
TileLibrary
├── tiles: Map<string, Tile>
│   ├── "sheet1_00" → Tile { id: "sheet1_00", sheet: 1, index: 0, label: "Side-eye" }
│   ├── "sheet1_01" → Tile { id: "sheet1_01", sheet: 1, index: 1, label: "Angry" }
│   ├── ...
│   └── "sheet3_15" → Tile { id: "sheet3_15", sheet: 3, index: 15, label: "Surprised" }
└── sheets: Map<number, Tile[]>
    ├── 1 → [Tile, Tile, ... 16 tiles]
    ├── 2 → [Tile, Tile, ... 16 tiles]
    └── 3 → [Tile, Tile, ... 16 tiles]

Animation
├── name: "happy_greeting"
├── loop: true
├── fps: 10
├── frames: [
│   AnimationFrame { tileId: "sheet3_00", durationMs: 200 },  // neutral
│   AnimationFrame { tileId: "sheet3_01", durationMs: 300 },  // big grin
│   AnimationFrame { tileId: "sheet2_11", durationMs: 200 },  // playful tongue
│   AnimationFrame { tileId: "sheet3_01", durationMs: 400 },  // big grin (hold)
│   AnimationFrame { tileId: "sheet3_00", durationMs: 100 },  // back to neutral
│ ]
└── totalDurationMs: 1200
```

---

## 5. Tile Sheet Preprocessing

The sprite sheets are preprocessed offline using ImageMagick. This step is done once — the sliced tiles are committed to the project.

### Slicing Command

```bash
# Split a 4×4 sprite sheet into 16 individual tiles
convert assets/sheets/sheet1.png -crop 4x4@ +repage assets/tiles/sheet1_%02d.png
convert assets/sheets/sheet2.png -crop 4x4@ +repage assets/tiles/sheet2_%02d.png
convert assets/sheets/sheet3.png -crop 4x4@ +repage assets/tiles/sheet3_%02d.png
```

The `-crop 4x4@` flag tells ImageMagick to divide the image into 4 columns × 4 rows. The `+repage` resets the page geometry so each tile is a standalone image.

### Tile Naming Convention

Tiles are named `{sheet}_{index}.png` where:
- `sheet` is the sheet number (1, 2, or 3)
- `index` is the tile position (00–15), left-to-right, top-to-bottom

Examples: `sheet1_00.png`, `sheet2_07.png`, `sheet3_15.png`

### Tile Size Normalization

The tiles from Sheet 1 are slightly larger (331×298) than those from Sheets 2 and 3 (314×314). The browser handles this naturally — the Canvas API scales images to fit the target viewport. No normalization needed at the preprocessing stage.

### Optional: Transparent Background

If the black background around each face needs to be removed (for overlay compositing), this can be done with:

```bash
# Make the black background transparent
for f in assets/tiles/*.png; do
  convert "$f" -fuzz 10% -transparent black "${f%.png}_transparent.png"
done
```

This is optional and depends on whether the ESP32 firmware supports transparent backgrounds.

---

## 6. Application Layout and UI

The application uses a three-panel layout with a top toolbar:

```
┌─────────────────────────────────────────────────────────────────┐
│  Toolbar: [New] [Open] [Save] [Export▼] [▸ Play] [⏸ Pause]    │
├──────────────┬──────────────────────────────┬───────────────────┤
│              │                              │                   │
│  Tile        │  Animation Preview           │  Properties       │
│  Browser     │                              │  Panel            │
│              │  ┌────────────────────┐      │                   │
│  [Sheet ▼]  │  │                    │      │  Frame: sheet3_01│
│              │  │   Current Frame    │      │  Duration: 200ms │
│  ┌────┐┌────┐│  │   on Canvas       │      │  Label: Grin     │
│  │ 😐 ││ 😠 ││  │                    │      │                   │
│  └────┘└────┘│  │   (scaled to       │      │  ────────────    │
│  ┌────┐┌────┐│  │    135×240)        │      │  Animation:      │
│  │ 😴 ││ 😮 ││  │                    │      │  Name: hello    │
│  └────┘└────┘│  └────────────────────┘      │  Loop: ✓        │
│  ┌────┐┌────┐│                              │  FPS: 10        │
│  │ 🤨 ││ 😲 ││                              │                   │
│  └────┘└────┘│                              │                   │
│  ...         │                              │                   │
│              │                              │                   │
├──────────────┴──────────────────────────────┴───────────────────┤
│  Timeline Editor                                                 │
│                                                                  │
│  | 😐  | 😠  | 😮  | 😐  | 😴  | 😐  |                        │
│  | 200 | 200 | 150 | 100 | 500 | 100 | ms                      │
│                                                                  │
│  [+] Add Frame  [×] Delete  [◄] Move Left  [►] Move Right      │
└──────────────────────────────────────────────────────────────────┘
```

### Responsive Behavior

- **Desktop (>1024px)**: Three-column layout as shown above
- **Tablet (768–1024px)**: Tile browser collapses to a dropdown, two-column layout
- **Mobile (<768px)**: Stacked layout, swipe between panels

### CSS Layout

```css
body {
  display: grid;
  grid-template-rows: auto 1fr auto;
  grid-template-columns: 280px 1fr 240px;
  grid-template-areas:
    "toolbar toolbar toolbar"
    "tiles   preview props"
    "timeline timeline timeline";
  height: 100vh;
  margin: 0;
  font-family: system-ui, sans-serif;
  background: #1a1a2e;
  color: #eee;
}
```

---

## 7. Tile Browser Component

The Tile Browser shows all available expression tiles, organized by sheet.

### Features

- **Sheet selector** — Dropdown to switch between Sheet 1, 2, 3, or "All"
- **Grid display** — 4×4 grid of clickable tile thumbnails
- **Selection** — Click a tile to add it to the timeline at the cursor position
- **Hover preview** — Larger preview on hover
- **Search/filter** — Filter tiles by expression label (e.g., type "happy" to find happy expressions)
- **Visual indicator** — Tiles used in the current animation are highlighted

### Rendering

```javascript
class TileBrowser {
  constructor(container, tileLibrary, onTileSelect) {
    this.container = container;     // DOM element
    this.library = tileLibrary;
    this.onTileSelect = onTileSelect;  // Callback when tile is clicked
    this.currentSheet = 0;          // 0 = all, 1-3 = specific sheet
    this.filterText = '';
  }

  render() {
    // 1. Get tiles for current sheet filter
    const tiles = this.currentSheet === 0
      ? this.library.getAllTiles()
      : this.library.getSheet(this.currentSheet);

    // 2. Apply text filter
    const filtered = this.filterText
      ? tiles.filter(t => t.label.toLowerCase().includes(this.filterText))
      : tiles;

    // 3. Render as CSS grid
    this.container.innerHTML = '';
    const grid = document.createElement('div');
    grid.className = 'tile-grid';

    for (const tile of filtered) {
      const el = document.createElement('div');
      el.className = 'tile-thumb';
      el.style.backgroundImage = `url(${tile.src})`;
      el.title = `${tile.label} (${tile.id})`;
      el.addEventListener('click', () => this.onTileSelect(tile));
      grid.appendChild(el);
    }

    this.container.appendChild(grid);
  }
}
```

### Tile Grid CSS

```css
.tile-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 4px;
  padding: 8px;
}

.tile-thumb {
  aspect-ratio: 1;
  background-size: contain;
  background-repeat: no-repeat;
  background-position: center;
  border: 2px solid transparent;
  border-radius: 4px;
  cursor: pointer;
  transition: border-color 0.15s;
}

.tile-thumb:hover {
  border-color: #e94560;
  transform: scale(1.05);
}

.tile-thumb.selected {
  border-color: #0f3460;
  background-color: #16213e;
}
```

---

## 8. Timeline Editor Component

The Timeline Editor shows the animation as a horizontal strip of frames with per-frame timing controls.

### Features

- **Frame strip** — Horizontal scrollable list of frame thumbnails
- **Duration editing** — Click a frame to select it, adjust duration with a slider or input
- **Drag-and-drop reordering** — Drag frames to rearrange
- **Insert/delete** — Add frames (from tile browser), delete selected frame
- **Playback cursor** — Visual indicator of current playback position
- **Zoom** — Scale the timeline view (compact vs. expanded)

### Frame Rendering

```javascript
class TimelineEditor {
  constructor(container, animation, tileLibrary, onFrameSelect, onFrameUpdate) {
    this.container = container;
    this.animation = animation;
    this.library = tileLibrary;
    this.onFrameSelect = onFrameSelect;
    this.onFrameUpdate = onFrameUpdate;
    this.selectedIndex = -1;
    this.playbackIndex = -1;    // Current frame during playback
    this.dragSourceIndex = -1;  // For drag-and-drop
  }

  render() {
    this.container.innerHTML = '';
    const strip = document.createElement('div');
    strip.className = 'timeline-strip';

    this.animation.frames.forEach((frame, i) => {
      const tile = this.library.getTile(frame.tileId);
      const el = document.createElement('div');
      el.className = 'timeline-frame';
      if (i === this.selectedIndex) el.classList.add('selected');
      if (i === this.playbackIndex) el.classList.add('playing');

      // Thumbnail
      const thumb = document.createElement('div');
      thumb.className = 'frame-thumb';
      thumb.style.backgroundImage = `url(${tile.src})`;

      // Duration label
      const dur = document.createElement('div');
      dur.className = 'frame-duration';
      dur.textContent = `${frame.durationMs}ms`;

      el.appendChild(thumb);
      el.appendChild(dur);

      // Click to select
      el.addEventListener('click', () => {
        this.selectedIndex = i;
        this.onFrameSelect(i);
        this.render();
      });

      // Drag handlers for reordering
      el.draggable = true;
      el.addEventListener('dragstart', (e) => {
        this.dragSourceIndex = i;
        e.dataTransfer.effectAllowed = 'move';
      });
      el.addEventListener('dragover', (e) => {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'move';
      });
      el.addEventListener('drop', (e) => {
        e.preventDefault();
        if (this.dragSourceIndex !== i) {
          this.reorderFrame(this.dragSourceIndex, i);
        }
      });

      strip.appendChild(el);
    });

    this.container.appendChild(strip);
  }

  reorderFrame(fromIndex, toIndex) {
    const [frame] = this.animation.frames.splice(fromIndex, 1);
    this.animation.frames.splice(toIndex, 0, frame);
    this.selectedIndex = toIndex;
    this.onFrameUpdate();
    this.render();
  }
}
```

### Timeline CSS

```css
.timeline-strip {
  display: flex;
  gap: 2px;
  padding: 8px;
  overflow-x: auto;
  align-items: flex-end;
  min-height: 120px;
  background: #16213e;
  border-top: 1px solid #0f3460;
}

.timeline-frame {
  display: flex;
  flex-direction: column;
  align-items: center;
  border: 2px solid transparent;
  border-radius: 4px;
  cursor: pointer;
  transition: border-color 0.15s;
}

.timeline-frame.selected {
  border-color: #e94560;
}

.timeline-frame.playing {
  border-color: #53d769;
  box-shadow: 0 0 8px #53d769;
}

.frame-thumb {
  width: 64px;
  height: 64px;
  background-size: contain;
  background-repeat: no-repeat;
  background-position: center;
}

.frame-duration {
  font-size: 10px;
  color: #888;
  padding: 2px;
}
```

---

## 9. Animation Preview Component

The Preview shows the animation played back on a canvas sized to match the M5StackChan's actual display (135×240 pixels).

### Features

- **Canvas rendering** — Draws the current frame to a 135×240 canvas, scaled up 2–3× for visibility
- **Playback controls** — Play, Pause, Stop buttons
- **Speed control** — 0.25×, 0.5×, 1×, 2× playback speed
- **Frame counter** — "Frame 3/12" display
- **Loop toggle** — Play once or loop continuously
- **Background** — Option to show with black background (matching device) or transparent

### Canvas Setup

```javascript
class AnimationPreview {
  constructor(canvasElement, tileLibrary) {
    this.canvas = canvasElement;
    this.ctx = this.canvas.getContext('2d');
    this.library = tileLibrary;

    // M5StackChan display is 135×240
    this.deviceWidth = 135;
    this.deviceHeight = 240;
    this.scale = 3;  // Display at 3× for visibility

    this.canvas.width = this.deviceWidth * this.scale;
    this.canvas.height = this.deviceHeight * this.scale;
    this.ctx.imageSmoothingEnabled = false;  // Pixel-perfect scaling
  }

  drawFrame(tileId) {
    const tile = this.library.getTile(tileId);
    if (!tile || !tile.image) return;

    this.ctx.fillStyle = '#000000';
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

    // Center the tile image in the device viewport
    const img = tile.image;
    const scale = Math.min(
      this.deviceWidth / img.naturalWidth,
      this.deviceHeight / img.naturalHeight
    );
    const drawW = img.naturalWidth * scale * this.scale;
    const drawH = img.naturalHeight * scale * this.scale;
    const drawX = (this.canvas.width - drawW) / 2;
    const drawY = (this.canvas.height - drawH) / 2;

    this.ctx.drawImage(img, drawX, drawY, drawW, drawH);
  }

  clear() {
    this.ctx.fillStyle = '#000000';
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
  }
}
```

---

## 10. Animation Engine

The Animation Engine manages playback state and frame timing.

### State Machine

```
         ┌─────────┐
         │ STOPPED │◄──────────────────┐
         └────┬────┘                   │
     play()   │                   stop()
              ▼                          │
         ┌─────────┐    pause()    ┌────┴────┐
         │ PLAYING │──────────────►│ PAUSED  │
         └────┬────┘◄──────────────└────┬────┘
              │         play()          │
              │                         │
              ▼                         │
    [requestAnimationFrame]             │
    - Check if frame duration elapsed   │
    - If yes: advance to next frame     │
    - Draw current frame to preview     │
    - Update playback cursor            │
    - If last frame and !loop: stop()   │
    - Else: request next animation frame│
              │                         │
              └─────────────────────────┘
```

### Implementation

```javascript
class AnimationEngine {
  constructor(animation, preview, timeline, tileLibrary) {
    this.animation = animation;
    this.preview = preview;
    this.timeline = timeline;
    this.library = tileLibrary;

    this.state = 'stopped';     // 'stopped', 'playing', 'paused'
    this.currentFrameIndex = 0;
    this.speed = 1.0;          // Playback speed multiplier
    this.lastFrameTime = 0;
    this.rafId = null;
  }

  play() {
    if (this.state === 'paused') {
      this.state = 'playing';
      this.lastFrameTime = performance.now();
      this.rafId = requestAnimationFrame((t) => this.tick(t));
      return;
    }

    this.state = 'playing';
    this.currentFrameIndex = 0;
    this.lastFrameTime = performance.now();
    this.drawCurrentFrame();
    this.rafId = requestAnimationFrame((t) => this.tick(t));
  }

  pause() {
    this.state = 'paused';
    if (this.rafId) {
      cancelAnimationFrame(this.rafId);
      this.rafId = null;
    }
  }

  stop() {
    this.state = 'stopped';
    if (this.rafId) {
      cancelAnimationFrame(this.rafId);
      this.rafId = null;
    }
    this.currentFrameIndex = 0;
    this.preview.clear();
    this.timeline.playbackIndex = -1;
    this.timeline.render();
  }

  tick(timestamp) {
    if (this.state !== 'playing') return;

    const frame = this.animation.frames[this.currentFrameIndex];
    const elapsed = (timestamp - this.lastFrameTime) * this.speed;

    if (elapsed >= frame.durationMs) {
      // Advance to next frame
      this.currentFrameIndex++;
      this.lastFrameTime = timestamp;

      if (this.currentFrameIndex >= this.animation.frames.length) {
        if (this.animation.loop) {
          this.currentFrameIndex = 0;
        } else {
          this.stop();
          return;
        }
      }

      this.drawCurrentFrame();
      this.timeline.playbackIndex = this.currentFrameIndex;
      this.timeline.render();
    }

    this.rafId = requestAnimationFrame((t) => this.tick(t));
  }

  drawCurrentFrame() {
    const frame = this.animation.frames[this.currentFrameIndex];
    if (frame) {
      this.preview.drawFrame(frame.tileId);
    }
  }

  setSpeed(speed) {
    this.speed = speed;
  }
}
```

### Frame Timing Precision

The engine uses `requestAnimationFrame` with manual duration tracking. This provides:
- 60fps display refresh rate (browser-managed)
- Variable frame durations (each animation frame can have a different `durationMs`)
- Speed multiplier support (0.25× to 4×)
- Smooth playback without blocking the UI thread

The timing math is:

```
elapsed_since_last_frame_change = (current_timestamp - last_frame_timestamp) × speed_multiplier
if elapsed >= frame.durationMs:
    advance to next frame
    reset last_frame_timestamp
```

This approach handles variable frame rates correctly — a 100ms frame at 2× speed takes 50ms of real time.

---

## 11. Serialization Format

### JSON Animation File

The primary export format is JSON. This is both the save format (for reloading in the editor) and the interchange format for the ESP32 firmware.

```json
{
  "version": 1,
  "name": "happy_greeting",
  "loop": true,
  "fps": 10,
  "display": {
    "width": 135,
    "height": 240
  },
  "frames": [
    {
      "tileId": "sheet3_00",
      "durationMs": 200,
      "label": "neutral"
    },
    {
      "tileId": "sheet3_01",
      "durationMs": 300,
      "label": "big grin"
    },
    {
      "tileId": "sheet2_11",
      "durationMs": 200,
      "label": "playful tongue"
    },
    {
      "tileId": "sheet3_01",
      "durationMs": 400,
      "label": "big grin"
    },
    {
      "tileId": "sheet3_00",
      "durationMs": 100,
      "label": "neutral"
    }
  ],
  "totalDurationMs": 1200,
  "createdAt": "2026-06-11T19:00:00Z",
  "modifiedAt": "2026-06-11T19:05:00Z"
}
```

### C++ Header Export

For embedding directly in ESP32 firmware:

```cpp
// Auto-generated by Face Animation Studio
// Animation: happy_greeting
// Total duration: 1200ms, Frames: 5, Loop: true

#ifndef ANIM_HAPPY_GREETING_H
#define ANIM_HAPPY_GREETING_H

#include "face_animation.h"

static const FaceFrame anim_happy_greeting[] = {
    { .tileIndex = TILE_SHEET3_00, .durationMs = 200 },  // neutral
    { .tileIndex = TILE_SHEET3_01, .durationMs = 300 },  // big grin
    { .tileIndex = TILE_SHEET2_11, .durationMs = 200 },  // playful tongue
    { .tileIndex = TILE_SHEET3_01, .durationMs = 400 },  // big grin
    { .tileIndex = TILE_SHEET3_00, .durationMs = 100 },  // neutral
};

static const FaceAnimation happy_greeting = {
    .name = "happy_greeting",
    .frames = anim_happy_greeting,
    .frameCount = 5,
    .loop = true,
    .totalDurationMs = 1200,
};

#endif
```

### Tile Index Mapping

For the C++ export, tiles are mapped to enum constants:

```cpp
// Tile index constants (matching assets/tiles/ naming)
enum TileIndex {
    TILE_SHEET1_00 = 0,   // Side-eye / Suspicious
    TILE_SHEET1_01 = 1,   // Angry / Glare
    // ... (48 total)
    TILE_SHEET3_15 = 47,  // Surprised "O" mouth
    TILE_COUNT = 48
};
```

### Save/Load API

```javascript
class AnimationSerializer {
  static toJSON(animation) {
    return JSON.stringify({
      version: 1,
      name: animation.name,
      loop: animation.loop,
      fps: animation.fps,
      display: { width: 135, height: 240 },
      frames: animation.frames.map(f => ({
        tileId: f.tileId,
        durationMs: f.durationMs,
        label: this.library.getTile(f.tileId)?.label || ''
      })),
      totalDurationMs: animation.totalDurationMs,
      createdAt: animation.createdAt,
      modifiedAt: new Date().toISOString()
    }, null, 2);
  }

  static fromJSON(jsonString) {
    const data = JSON.parse(jsonString);
    const anim = new Animation(data.name);
    anim.loop = data.loop;
    anim.fps = data.fps;
    anim.createdAt = data.createdAt;
    anim.frames = data.frames.map(f => new AnimationFrame(f.tileId, f.durationMs));
    return anim;
  }

  static toCppHeader(animation) {
    // Generate C++ header file as shown above
    // ...
  }

  static download(data, filename, mimeType = 'application/json') {
    const blob = new Blob([data], { type: mimeType });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
  }
}
```

---

## 12. ESP32 Integration Path

The animation studio produces JSON (and optionally C++ headers) that describe animation sequences. Here's how they integrate with the M5StackChan firmware:

### Option A: Pre-compiled Animations (Simplest)

1. Design animations in the studio
2. Export as C++ headers
3. Compile into the firmware
4. Firmware plays animations from flash

This requires no runtime parsing. The tile images are also compiled into the firmware as PROGMEM byte arrays.

### Option B: SPIFFS-Based Animations

1. Design animations in the studio
2. Export as JSON files
3. Upload JSON + tile PNGs to SPIFFS
4. Firmware reads JSON at runtime, loads tiles from SPIFFS
5. More flexible — can add new animations without reflashing

### Option C: NVS Key-Value Animation States

For very simple state-based animations (idle, happy, sad, angry):

1. Define a set of emotional states
2. Each state maps to a single tile or short loop
3. Store in NVS as key-value pairs
4. Firmware reads current state, plays the associated animation

### Recommended: Start with Option A

Pre-compiled animations are the simplest path. The studio produces C++ headers that compile directly into the firmware. No filesystem, no JSON parser, no runtime loading. When the project matures, migrate to Option B for flexibility.

### Tile Image Conversion for ESP32

The M5StackChan uses an ST7789 TFT display at 135×240. Tiles need to be converted to RGB565 format for fast rendering:

```bash
# Convert a tile to C array (RGB565, PROGMEM)
python3 scripts/tile_to_cpp.py \
  --input assets/tiles/sheet3_01.png \
  --output src/tiles/sheet3_01.h \
  --width 135 --height 240 \
  --format rgb565
```

The conversion script:
1. Loads the PNG
2. Scales to 135×240 (or whatever the target size is)
3. Centers the face in the frame
4. Converts each pixel to RGB565 (16-bit: R5 G6 B5)
5. Outputs as a C array with PROGMEM attribute

---

## 13. Implementation Plan

### Phase 1: Core App (Day 1-2)

| Step | Task | Files |
|------|------|-------|
| 1.1 | Create `index.html` with layout and CSS | `src/index.html` |
| 1.2 | Implement `TileLibrary` — load 48 tiles | `src/tile-library.js` |
| 1.3 | Implement `TileBrowser` — grid display + click | `src/tile-browser.js` |
| 1.4 | Implement `Animation` + `AnimationFrame` data classes | `src/animation.js` |
| 1.5 | Implement `TimelineEditor` — frame strip | `src/timeline-editor.js` |
| 1.6 | Implement `AnimationPreview` — canvas rendering | `src/animation-preview.js` |
| 1.7 | Implement `AnimationEngine` — play/pause/stop | `src/animation-engine.js` |
| 1.8 | Wire up toolbar buttons (New, Play, Pause, Stop) | `src/app.js` |

### Phase 2: Editing Features (Day 3)

| Step | Task | Files |
|------|------|-------|
| 2.1 | Frame duration editing (slider + input) | `src/timeline-editor.js` |
| 2.2 | Drag-and-drop frame reordering | `src/timeline-editor.js` |
| 2.3 | Delete frame, insert frame | `src/timeline-editor.js` |
| 2.4 | Sheet selector + text filter in Tile Browser | `src/tile-browser.js` |
| 2.5 | Speed control (0.25×, 0.5×, 1×, 2×) | `src/animation-engine.js` |
| 2.6 | Loop toggle | `src/animation.js` |

### Phase 3: Save/Load/Export (Day 4)

| Step | Task | Files |
|------|------|-------|
| 3.1 | Save animation as JSON | `src/animation-serializer.js` |
| 3.2 | Load animation from JSON | `src/animation-serializer.js` |
| 3.3 | Export as C++ header | `src/cpp-exporter.js` |
| 3.4 | File drag-and-drop for loading | `src/app.js` |
| 3.5 | LocalStorage auto-save | `src/app.js` |

### Phase 4: Polish (Day 5)

| Step | Task | Files |
|------|------|-------|
| 4.1 | Keyboard shortcuts (Space=play, Delete=remove frame) | `src/app.js` |
| 4.2 | Undo/redo | `src/undo-manager.js` |
| 4.3 | Animation list (multiple animations per file) | `src/animation.js` |
| 4.4 | Tile label editor | `src/tile-browser.js` |
| 4.5 | Dark theme polish | `src/index.html` |

### Phase 5: ESP32 Export Tooling (Day 6-7)

| Step | Task | Files |
|------|------|-------|
| 5.1 | Python script: tile_to_cpp.py | `scripts/tile_to_cpp.py` |
| 5.2 | Python script: animation_to_cpp.py | `scripts/animation_to_cpp.py` |
| 5.3 | C++ FaceAnimation header template | `src/templates/face_animation.h` |
| 5.4 | ESP32 animation playback example | `examples/animation_playback.ino` |

---

## 14. Decision Records

### DR-001: Single HTML file with no build step

**Context:** The tool could be built as a React SPA, a Go web app with templates, or a single HTML file.

**Options:**
1. Single HTML + inline CSS + inline JS — zero dependencies
2. React SPA with npm build — modern DX but requires Node.js
3. Go backend + HTML templates — more powerful but requires server

**Decision:** Option 1. Single HTML file with separate JS modules loaded via `<script>` tags.

**Rationale:** The tool is for a single user (the developer creating animations). No need for a production-grade frontend. Zero build step means it works by double-clicking the HTML file. Separate JS files keep the code organized without a bundler.

**Consequences:** No TypeScript, no JSX, no hot reload. But the codebase is small enough that this doesn't matter.

### DR-002: Pre-sliced tiles instead of runtime slicing

**Context:** We could slice the sprite sheets at runtime in the browser using Canvas, or pre-slice them offline.

**Options:**
1. Pre-slice with ImageMagick, load individual tiles
2. Load sprite sheets, slice at runtime in browser
3. Load sprite sheets, use CSS background-position to show individual tiles

**Decision:** Option 1. Pre-slice with ImageMagick.

**Rationale:** Pre-sliced tiles are simpler to work with (each tile is a standalone file). They load faster (browser can cache individual tiles). They're easier to export to ESP32 (each tile becomes a separate C array). Runtime slicing adds complexity for no benefit.

**Consequences:** 48 small PNG files instead of 3 large ones. Slightly more disk space but negligible.

### DR-003: Canvas API for preview rendering

**Context:** The preview could use CSS transforms, SVG, or Canvas.

**Options:**
1. Canvas 2D API — direct pixel control, fast rendering
2. CSS transforms on img elements — simpler but less control
3. SVG — resolution-independent but overkill

**Decision:** Option 1. Canvas 2D API.

**Rationale:** Canvas gives direct pixel-level control, which is important for matching the ESP32's exact display output. It handles scaling, centering, and background fill naturally. It's the right tool for a display emulator.

**Consequences:** Need to handle image loading manually (create Image objects, wait for load events). But this is straightforward.

---

## 15. File and API Reference

### Project Structure

```
face-animation-studio/
├── assets/
│   ├── sheets/                     # Original sprite sheets
│   │   ├── sheet1.png              # 1322×1190, 4×4 grid
│   │   ├── sheet2.png              # 1254×1254, 4×4 grid
│   │   └── sheet3.png              # 1254×1254, 4×4 grid
│   └── tiles/                      # Pre-sliced individual tiles
│       ├── sheet1_00.png           # Side-eye / Suspicious
│       ├── sheet1_01.png           # Angry / Glare
│       ├── ...                     # (16 tiles per sheet)
│       ├── sheet3_14.png           # Wince
│       └── sheet3_15.png           # Surprised "O" mouth
├── src/
│   ├── index.html                  # Main application HTML
│   ├── app.js                      # Application entry point
│   ├── tile-library.js             # Tile loading and management
│   ├── tile-browser.js             # Tile grid UI component
│   ├── animation.js                # Animation + AnimationFrame data classes
│   ├── timeline-editor.js          # Timeline strip UI component
│   ├── animation-preview.js        # Canvas preview component
│   ├── animation-engine.js         # Playback state machine
│   ├── animation-serializer.js     # JSON save/load + C++ export
│   └── undo-manager.js             # Undo/redo support
├── scripts/
│   ├── slice_sheets.sh             # ImageMagick tile slicing
│   ├── tile_to_cpp.py              # PNG → C++ RGB565 array
│   └── animation_to_cpp.py         # JSON → C++ header
├── docs/
│   └── expressions.md              # Label reference for all 48 tiles
└── examples/
    └── animation_playback.ino       # ESP32 animation playback example
```

### Key APIs

| API | Purpose | File |
|-----|---------|------|
| `TileLibrary.loadAll()` | Load all 48 tile images | `tile-library.js` |
| `TileLibrary.getTile(id)` | Get tile by ID | `tile-library.js` |
| `TileBrowser.render()` | Render tile grid | `tile-browser.js` |
| `TimelineEditor.render()` | Render frame strip | `timeline-editor.js` |
| `AnimationPreview.drawFrame(tileId)` | Draw frame to canvas | `animation-preview.js` |
| `AnimationEngine.play()` | Start playback | `animation-engine.js` |
| `AnimationEngine.pause()` | Pause playback | `animation-engine.js` |
| `AnimationEngine.stop()` | Stop and reset | `animation-engine.js` |
| `AnimationSerializer.toJSON(anim)` | Serialize to JSON | `animation-serializer.js` |
| `AnimationSerializer.fromJSON(str)` | Deserialize from JSON | `animation-serializer.js` |
| `AnimationSerializer.toCppHeader(anim)` | Export as C++ header | `animation-serializer.js` |

### Browser APIs Used

| API | Purpose |
|-----|---------|
| `HTMLImageElement` | Load tile PNG files |
| `CanvasRenderingContext2D` | Draw tiles to preview canvas |
| `requestAnimationFrame` | Smooth animation playback |
| `DragEvent` | Frame reordering in timeline |
| `Blob` + `URL.createObjectURL` | File download for save/export |
| `FileReader` | Load saved animations from disk |
| `localStorage` | Auto-save current work |

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Space` | Play / Pause |
| `Escape` | Stop |
| `Delete` / `Backspace` | Delete selected frame |
| `←` / `→` | Select previous/next frame |
| `Ctrl+S` | Save animation |
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |
| `+` / `-` | Adjust frame duration |
