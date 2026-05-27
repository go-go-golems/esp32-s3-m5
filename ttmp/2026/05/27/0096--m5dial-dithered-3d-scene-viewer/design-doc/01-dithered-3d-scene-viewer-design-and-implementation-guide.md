# Dithered 3D Scene Viewer — Design and Implementation Guide

## Executive Summary

This document specifies a firmware application for the M5Dial (ESP32-S3FN8, 240×240 GC9A01 circular display, rotary encoder) that renders 3D scenes in real-time with Bayer 4×4 ordered dithering and 4-color palette quantization. The rotary encoder orbits the camera around the scene. The design is ported from a React/Three.js WebGL simulator (`m5dial.jsx`) to bare-metal C with software 3D rasterization — no GPU, no floating-point hardware, no PSRAM.

The firmware targets 5–10 FPS at 240×240. It uses fixed-point arithmetic, precomputed lookup tables, scanline-based triangle rasterization, and a two-pass dither pipeline identical in logic to the GLSL dither shader. esp_console provides runtime control for scene selection, palette switching, and parameter tuning.

## Problem Statement

The original `m5dial.jsx` renders 3D scenes using WebGL with the following pipeline:

1. Three.js renders a scene into a 240×240 offscreen render target
2. A GLSL fragment shader pixelates, applies Bayer 4×4 dithering, and quantizes to 4 colors
3. The result is displayed at 480×480 with nearest-neighbor scaling

This pipeline depends on a GPU for vertex transformation, triangle rasterization, and texture sampling. The M5Dial has no GPU. It has a single-core 240 MHz LX7 processor, 224 KB of free internal RAM, and a SPI-driven display. Every operation — vertex transformation, projection, rasterization, dithering, pixel output — must run in software.

The core challenge: can a 240 MHz microcontroller produce a visually recognizable 4-color dithered 3D image at interactive frame rates, where "interactive" means the rotary encoder produces a visible rotation response within 100–200 ms?

## M5Dial Hardware Profile

| Resource | Specification | Impact on Design |
|----------|--------------|-----------------|
| CPU | ESP32-S3FN8, single-core 240 MHz | No FPU — use fixed-point; no PSRAM — all buffers in internal RAM |
| Flash | 8 MB embedded (DIO mode) | Read-only data (LUTs, sin/cos tables) lives in flash |
| Free RAM | ~224 KB after system reserves | Framebuffer + Z-buffer + geometry must fit in ~200 KB |
| Display | GC9A01, 240×240, SPI at 80 MHz | `pushImage()` for full-frame blit; ~1.5 ms per frame transfer |
| Encoder | GPIO 40/41, interrupt-driven | Camera orbit input; ~1° per detent |
| Touch | FT5x06, I2C | Scene switch on tap (optional) |
| Button | GPIO 42 | Cycle palette or toggle auto-rotate |
| Console | USB Serial/JTAG (`/dev/ttyACM0`) | esp_console for interactive control |

### Memory Budget

The display is 240×240 pixels. A full RGB565 framebuffer requires 240 × 240 × 2 = 115,200 bytes. A 1-byte-per-pixel indexed framebuffer requires 57,600 bytes. A Z-buffer at 16-bit depth requires 115,200 bytes. The total for a single-buffered RGB565 + Z-buffer approach is 230,400 bytes — more than the available 224 KB.

This constraint shapes the entire rendering architecture. The solution is a 2-bit indexed framebuffer (4 colors × 2 bits = 57,600 bytes, but packed to 2 bits per pixel = 14,400 bytes) paired with a scanline Z-buffer that only holds one row at a time (240 × 2 = 480 bytes). The tradeoff is that pixel output requires a 2-bit → RGB565 expansion pass before display.

**Memory allocation plan:**

| Buffer | Size | Location |
|--------|------|----------|
| 2-bit framebuffer (240×240×2 bits) | 14,400 B | `heap_caps_malloc(MALLOC_CAP_8BIT)` |
| Scanline Z-buffer (240 entries × 16-bit) | 480 B | Stack or static |
| Projected vertex buffer (per-mesh) | ~2 KB | Stack |
| Render LUTs (sin, cos, Bayer) | ~2 KB | Flash (const) |
| Color palette (4 × RGB565) | 8 B | Static |
| RGB565 scanline expansion buffer | 480 B | Stack |
| Geometry data (per-scene) | 4–8 KB | Flash (const) or computed on init |
| **Total RAM** | **~17 KB** | **Leaves ~200 KB for stack, heap, system** |

## Architecture

### Pipeline Overview

```
┌─────────────┐    ┌──────────────────┐    ┌─────────────┐    ┌───────────┐
│  Geometry    │───▶│  Vertex          │───▶│  Scanline   │───▶│  Dither + │
│  (flash)    │    │  Transform       │    │  Rasterize  │    │  Quantize │
│  vertices,  │    │  (fixed-point    │    │  (scanline  │    │  (Bayer   │
│  normals,   │    │   model→screen)  │    │   Z-buffer) │    │   4×4 +   │
│  colors     │    │                  │    │             │    │   palette) │
└─────────────┘    └──────────────────┘    └─────────────┘    └─────┬─────┘
                                                                │
                                                                ▼
                                                          ┌───────────┐
                                                          │  2-bit →  │
                                                          │  RGB565   │
                                                          │  expand   │
                                                          └─────┬─────┘
                                                                │
                                                                ▼
                                                          ┌───────────┐
                                                          │  SPI blit │
                                                          │  to GC9A01│
                                                          └───────────┘
```

The pipeline processes one scanline at a time for the rasterization and dither stages. This eliminates the need for a full-screen Z-buffer and keeps peak memory usage low.

### Two Rendering Strategies

**Strategy A: Render-then-dither (simpler, lower quality)**

Render the full scene into a luminance/chroma framebuffer (one byte per pixel: high nibble = luminance 0–15, low nibble = chroma 0 = warm, 1 = cool, 2 = white, 3 = black). Then apply Bayer dithering as a separate pass over the completed framebuffer. This requires 57,600 bytes for the intermediate buffer — feasible but uses more RAM.

**Strategy B: Rasterize-and-dither per scanline (lower memory, this design)**

Rasterize one scanline of the scene into a luminance+chroma buffer (240 bytes). Immediately apply Bayer dithering and quantization to that scanline, writing the 2-bit result into the packed framebuffer. The Z-buffer only needs to span one scanline (480 bytes). Total working memory per scanline: ~720 bytes.

This document adopts Strategy B. The per-scanline approach keeps peak RAM under 20 KB, leaves room for geometry computation, and naturally produces the same visual result as the GLSL shader since the Bayer matrix is position-dependent (it depends on the pixel's `x mod 4, y mod 4` coordinates, which are known per-scanline).

## Fixed-Point Arithmetic

The ESP32-S3 has single-precision floating-point hardware (IEEE 754), but integer multiplication is faster and more deterministic. However, for this project, `float` is acceptable for vertex transformation (the FPU handles it in 1–3 cycles per operation on S3). The performance-critical path — scanline rasterization and dithering — will use integer arithmetic.

**Hybrid approach:**
- Vertex transformation and projection: `float` (hardware FPU on ESP32-S3)
- Scanline rasterization and interpolation: `int32_t` fixed-point with 16.16 format
- Dithering and quantization: pure integer lookup

### Fixed-Point Conventions

```
// 16.16 fixed-point: 1.0 = 0x10000, -1.0 = 0xFFFF0000
typedef int32_t fixed_t;
#define FIXED_SHIFT 16
#define FLOAT_TO_FIXED(f) ((fixed_t)((f) * (1 << FIXED_SHIFT)))
#define FIXED_TO_INT(x) ((x) >> FIXED_SHIFT)
#define FIXED_MUL(a, b) (((int64_t)(a) * (b)) >> FIXED_SHIFT)
```

## Vertex Transformation Pipeline

Each vertex passes through three transformations: model → world → camera → screen.

### 1. Model-World Transform

The scene geometry is defined in model space. For static scenes, the model-world matrix is the identity. For animated objects (rotating torus, orbiting moon), the model-world matrix is computed per-frame from the scene's `update()` function.

The matrix is a 4×3 affine matrix (rotation + translation, no perspective in the model matrix):

```
M_model_world = | R00 R01 R02 Tx |
               | R10 R11 R12 Ty |
               | R20 R21 R22 Tz |
```

### 2. Camera Transform (View Matrix)

The camera orbits the scene origin at a distance `d` and height `h`. The orbit angle `θ` is driven by the rotary encoder.

```
eye = (target_x + d * sin(θ),  target_y + h,  target_z + d * cos(θ))

// Look-at matrix (right-handed)
forward = normalize(target - eye)
right   = normalize(cross(world_up, forward))
up      = cross(forward, right)

V = | right.x    right.y    right.z   -dot(right, eye)   |
    | up.x       up.y       up.z      -dot(up, eye)      |
    | -forward.x -forward.y -forward.z  dot(forward, eye)|
```

The `sin(θ)` and `cos(θ)` values come from a 1024-entry lookup table in flash (4 KB). Each encoder detent (~1°) advances `θ` by `π/180`. The lookup table covers 0–2π in 1024 steps, giving ~0.006 radian resolution.

### 3. Projection

Perspective projection with a 50° vertical field of view:

```
// Screen coordinates (pixel space)
screen_x = (view_x * focal_length / view_z) * scale + screen_cx
screen_y = -(view_y * focal_length / view_z) * scale + screen_cy

// For 240×240 display, 50° FOV:
//   focal_length = 1 / tan(25°) ≈ 2.145
//   scale = 120 (half-resolution)
//   screen_cx = screen_cy = 120 (center)
```

The Z-divider operation (`view_x / view_z`) is the most expensive per-vertex computation. For ESP32-S3's hardware FPU, a single-precision division takes ~14 cycles. With 100–300 vertices per scene, this is ~4,000 cycles — negligible.

## Scene Geometry

The original `m5dial.jsx` defines five scenes. Each scene provides:
- A group of meshes (triangles)
- An `update(time)` function that animates vertex positions, rotations, or other parameters
- Camera parameters: orbit distance, height, look-at target

For the ESP32 port, scenes are simplified to fit in flash and RAM:

### Scene 1: Terrain

**Original:** 80×80 grid (6,400 vertices, 12,288 triangles), vertex-colored blue landscape with red sun.

**ESP32 adaptation:** Reduce to 20×20 grid (400 vertices, 722 triangles). Height computed at init from `noise2d()`. Vertex colors are blue-valley to blue-peak gradient. Red sun drawn as a filled circle in post-processing (no geometry). Animated: sun height oscillates.

### Scene 2: Toroid (Torus Knot)

**Original:** TorusKnotGeometry(2.4, 0.85, 180, 24, 2, 3) — ~8,600 triangles.

**ESP32 adaptation:** Reduce to p=2, q=3 knot with 36 segments × 8 sides = 576 triangles. Vertex colors interpolated red↔blue based on position. Animated: rotation on two axes.

### Scene 3: Ocean

**Original:** 70×70 grid with per-frame vertex displacement, sun, reflection trail.

**ESP32 adaptation:** 16×16 grid (256 vertices, 450 triangles). Per-frame wave displacement computed via `sin()` lookup. Colors: blue base, red heat near sun reflection. Sun drawn as filled circle. Animated: continuous wave motion.

### Scene 4: Planet

**Original:** SphereGeometry(2.6, 80, 60) — ~9,600 triangles, plus moon and ring.

**ESP32 adaptation:** Icosphere subdivision-2 (162 vertices, 320 triangles). Noise displacement at init for surface detail. Colors: red=north, blue=south, speckled. Moon: 12-triangle icosphere at computed position. Ring: 64-segment line strip (drawn as thin triangles). Animated: planet rotation, moon orbit.

### Scene 5: Tunnel

**Original:** 24 torus rings + 6 long bars.

**ESP32 adaptation:** 12 rings × 24 segments = 288 triangles. 4 long bars × 2 faces × 12 segments = 96 triangles. Total: ~384 triangles. Animated: rings fly toward camera in a loop.

### Geometry Storage

All geometry is stored as `const` data in flash:

```c
typedef struct {
    float x, y, z;    // model-space position
    uint8_t cr, cg, cb; // vertex color (0–255)
} scene_vertex_t;

typedef struct {
    uint16_t v0, v1, v2;  // indices into vertex array
} scene_triangle_t;

typedef struct {
    const scene_vertex_t* vertices;
    const scene_triangle_t* triangles;
    uint16_t vertex_count;
    uint16_t triangle_count;
    float camera_distance;
    float camera_height;
    float target[3];
    void (*update)(float time);  // animate vertices in-place
} scene_def_t;
```

For a 20×20 terrain: 400 vertices × 12 bytes + 722 triangles × 6 bytes = 4,800 + 4,332 = 9,132 bytes in flash. This is well within the 8 MB flash budget.

## Scanline Rasterization

The rasterizer processes triangles one scanline at a time, integrating with the dither pass.

### Triangle Setup

For each projected triangle on the current scanline `y`:

1. Compute the edge functions at `y` for the three edges
2. Compute the X intercepts (left and right boundaries)
3. Compute the interpolated depth (Z) and color at the left and right edges
4. For each pixel `x` from left to right:
   a. Compute interpolated Z via incremental stepping
   b. Compare with Z-buffer[y – triangle_top][x] (scanline-local Z-buffer, reset per scanline)
   c. If closer, compute interpolated luminance and chroma
   d. Apply Bayer dither to the luminance using the pixel's `(x, y)` position
   e. Write the 2-bit quantized color into the framebuffer

### Edge Function Interpolation

For a triangle with screen-space vertices `(x0,y0)`, `(x1,y1)`, `(x2,y2)` and attributes `a0`, `a1`, `a2`:

```
// Barycentric coordinates at pixel (px, py)
w0 = edge_function(x1,y1, x2,y2, px,py)
w1 = edge_function(x2,y2, x0,y0, px,py)
w2 = edge_function(x0,y0, x1,y1, px,py)
area = w0 + w1 + w2

// Interpolated attribute
a = (a0*w0 + a1*w1 + a2*w2) / area
```

Incremental computation: advancing `px` by 1 changes each `wi` by a constant. This makes the inner loop purely additive — no multiplication per pixel.

### Z-Buffer

A 16-bit Z-buffer for one scanline (240 entries × 2 bytes = 480 bytes). Before processing each scanline, the buffer is initialized to maximum depth (0x7FFF). After all triangles for that scanline are processed, the buffer is discarded (not needed for the next scanline).

The Z value is the camera-space depth (`view_z`), quantized to 16 bits:

```
z_fixed = (int16_t)(view_z * 256.0f)  // range 0–65535 for view_z 0–255
```

Near triangles have small `z_fixed`, far triangles have large. The Z-buffer test is `z_current < z_buffer[x]`.

## Dithering and Quantization

The dither pipeline is a direct translation of the GLSL `DITHER_FS` shader into integer C. The shader performs three logical steps per pixel:

1. **Pixelation** — snap to pixel grid (already handled by rasterizer)
2. **Contrast enhancement** — S-curve around 0.5
3. **Bayer 4×4 ordered dithering + 4-color quantization**

### Bayer 4×4 Threshold Matrix

The 4×4 Bayer matrix produces 16 threshold levels (0/16 through 15/16):

```c
static const uint8_t BAYER4X4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 },
};
```

The threshold for pixel `(x, y)` is `BAYER4X4[y & 3][x & 3]`. The luminance value from the rasterizer is 0–15 (4 bits). The dither comparison is:

```
if (luminance > bayer_threshold) → use bright color; else → use dark color
```

### 4-Color Quantization Logic

The GLSL shader classifies each pixel into one of four categories based on the original color's red/blue balance:

```
1. isWhite = (r > 0.55 && g > 0.55 && b > 0.55)
2. isWarm  = (r > b + 0.05)         → red-dominant
3. isCool  = (b > r + 0.05)         → blue-dominant
4. Neutral → lean cool
```

For each category, Bayer dithering chooses between two of the four palette colors:

| Category | Dither result | Color |
|----------|--------------|-------|
| White + above threshold | `color_high` | White or accent |
| Warm + above threshold | `color_warm` | Red/warm tone |
| Cool + above threshold | `color_cool` | Blue/cool tone |
| Any + below threshold | Black | Background |

### ESP32 Implementation

The rasterizer produces per-pixel attributes:
- `lum` (4 bits, 0–15): luminance value
- `chroma` (2 bits): 0=warm, 1=cool, 2=white, 3=neutral→cool

The dither decision:

```c
uint8_t bayer = BAYER4X4[y & 3][x & 3];
uint8_t pixel_color = 0;  // default: black

if (chroma == 2 && lum > 8) {
    // White-dominant pixel
    if (lum > bayer) pixel_color = COLOR_HIGH;
} else if (chroma == 0) {
    // Warm-dominant
    if (lum > bayer) pixel_color = COLOR_WARM;
} else {
    // Cool or neutral
    if (lum > bayer) pixel_color = COLOR_COOL;
}

// Write 2-bit result to packed framebuffer
fb_set(x, y, pixel_color);
```

### Circular Mask

The GLSL shader applies a circular mask: pixels outside a radius from center are forced to black. For a 240×240 display with center at (120, 120):

```c
int dx = x - 120;
int dy = y - 120;
if (dx*dx + dy*dy > mask_radius_sq) {
    pixel_color = COLOR_BLACK;
}
```

The `mask_radius_sq` is precomputed from the user-configurable aperture parameter (default: `0.97 * 120 = 116.4`, squared ≈ 13,549).

## Color Palettes

Five palettes from the original, each defining three non-black colors:

```c
typedef struct {
    uint16_t warm;   // RGB565
    uint16_t cool;   // RGB565
    uint16_t high;   // RGB565
    const char* name;
} palette_t;

// Classic
{ 0xF804, 0x185F, 0xFFFF, "CLASSIC" }
//   #ff2940      #3050ff      #ffffff

// Inverted (warm ↔ cool swapped)
{ 0x185F, 0xF804, 0xFFFF, "INVERTED" }

// Red Mono
{ 0xF804, 0x7810, 0xFFFF, "RED MONO" }

// Blue Mono
{ 0x5B5F, 0x185F, 0xFFFF, "BLUE MONO" }

// Amber CRT
{ 0xFD04, 0x5A60, 0xFF20, "AMBER CRT" }
```

The RGB565 values are precomputed from the hex colors in the original. The palette is selected by button press or console command.

## Framebuffer to Display

The 2-bit packed framebuffer (14,400 bytes) must be expanded to RGB565 for the GC9A01 display. This is a two-step process:

### Step 1: 2-bit → RGB565 Expansion

For each scanline `y` (0–239):

```c
uint16_t rgb565_line[240];
for (int x = 0; x < 240; x++) {
    uint8_t idx = fb_get(x, y);  // 0–3
    rgb565_line[x] = palette_colors[idx];  // 0=black, 1=warm, 2=cool, 3=high
}
```

### Step 2: Push to Display

```c
display.startWrite();
display.setAddrWindow(0, 0, 240, 240);
for (int y = 0; y < 240; y++) {
    expand_scanline(y, rgb565_line);
    display.pushColors(reinterpret_cast<uint8_t*>(rgb565_line), 240 * 2);
}
display.endWrite();
```

At 80 MHz SPI with 2 bytes per pixel, the full frame transfer takes:
`240 × 240 × 2 × 8 / 80,000,000 = 11.5 ms`

This is the display bottleneck. The total frame budget at 10 FPS is 100 ms; at 5 FPS, 200 ms. The 11.5 ms SPI transfer leaves 88.5 ms (at 10 FPS) or 188.5 ms (at 5 FPS) for rendering.

## Encoder Input and Camera Orbit

The M5Dial's rotary encoder produces interrupts on GPIO 40/41. The existing `M5DialBoard` class from project 0074 captures encoder deltas and posts them to an input queue as `InputEvent::kEncoderDelta` with a `value` of ±1 per detent.

Camera orbit angle `θ` advances by `π/90` per encoder detent (2° per click):

```c
void handle_encoder_delta(int32_t delta) {
    camera_angle += delta * (31416 / 1800);  // π/90 in fixed-point
    scene_dirty = true;
}
```

The button (GPIO 42) cycles palettes. A long press toggles auto-rotation.

## esp_console Integration

The firmware exposes runtime commands via esp_console over USB Serial/JTAG:

| Command | Arguments | Description |
|---------|-----------|-------------|
| `scene` | `[terrain\|torus\|ocean\|planet\|tunnel]` | Select active scene |
| `palette` | `[classic\|inverted\|red\|blue\|amber]` | Select color palette |
| `rotate` | `<speed>` | Set auto-rotation speed (-1.5 to 1.5 rad/s) |
| `contrast` | `<value>` | Set contrast (0.6–2.5) |
| `aperture` | `<pct>` | Set circular mask radius (40–100%) |
| `fps` | | Show last frame time and FPS |
| `stats` | | Show triangle count, vertex count, render time |
| `wireframe` | `[on\|off]` | Toggle wireframe overlay |
| `pause` | | Pause/resume rendering |

These commands provide the "simulated interaction" layer — they let the developer control the firmware without physical input devices, which is useful for testing and demo.

## Performance Estimates

| Operation | Estimated Time | Basis |
|-----------|---------------|-------|
| Vertex transform (300 vertices) | 0.3 ms | 300 × 4 matrix mults × ~250 ns each |
| Triangle setup (500 triangles) | 0.5 ms | 500 × edge computations × ~1 µs each |
| Scanline rasterization + dither (240 lines × 500 tri/line) | 30 ms | Pessimistic: 500 tri avg × 240 lines × 250 ns/pixel |
| 2-bit → RGB565 expansion | 1 ms | 57,600 pixel lookups × ~17 ns each |
| SPI display transfer | 11.5 ms | 115,200 bytes at 80 MHz |
| **Total** | **~43 ms** | **~23 FPS theoretical** |

The actual FPS will be lower due to:
- Triangle sorting / clipping overhead
- Scanline coherence breaks (multiple triangles per scanline)
- Cache pressure from flash-resident geometry
- Interrupt latency from encoder GPIO

A realistic target is **8–15 FPS** for simple scenes (terrain, tunnel) and **5–8 FPS** for complex scenes (ocean with per-frame vertex updates).

## Implementation Plan

### Phase 1: Foundation (Tasks 1–3)

Set up the project structure, display driver, and console. Render a static test pattern (gradient or bayer matrix) to verify the display pipeline.

### Phase 2: Software Rasterizer (Tasks 4–6)

Implement vertex transformation, triangle setup, and scanline rasterization with Z-buffer. Render a single rotating triangle or cube to verify projection and rasterization.

### Phase 3: Dither Pipeline (Tasks 7–8)

Add Bayer dithering and 4-color quantization to the rasterizer. Add palette selection. Render the rotating cube with dithering.

### Phase 4: Scenes (Tasks 9–11)

Implement terrain, torus, ocean, planet, and tunnel scenes. Each scene provides its geometry, update function, and camera parameters.

### Phase 5: Input and Polish (Tasks 12–14)

Wire encoder, button, and touch input. Add auto-rotation. Add circular mask. Tune performance. Add FPS counter and wireframe overlay.

### File Structure

```
0096-m5dial-dithered-3d/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.cpp            # Entry point, task creation, console init
│   ├── renderer.h              # Rasterizer API
│   ├── renderer.cpp            # Vertex transform, scanline rasterize, dither
│   ├── framebuffer.h           # 2-bit packed framebuffer API
│   ├── framebuffer.cpp         # fb_set, fb_get, expand_scanline
│   ├── scene.h                 # Scene definition, palette, camera
│   ├── scene_terrain.cpp       # Terrain scene
│   ├── scene_torus.cpp         # Torus knot scene
│   ├── scene_ocean.cpp         # Ocean scene
│   ├── scene_planet.cpp        # Planet scene
│   ├── scene_tunnel.cpp        # Tunnel scene
│   ├── scene_manager.h         # Scene selection, animation, update loop
│   ├── scene_manager.cpp       # Scene lifecycle, frame timing
│   ├── input_handler.h         # Encoder/button/touch → camera/palette
│   ├── input_handler.cpp       # Input event processing
│   ├── console_commands.h      # esp_console registration
│   ├── console_commands.cpp    # scene, palette, rotate, contrast, etc.
│   ├── m5dial_board.h          # Board driver (from 0074)
│   ├── m5dial_board.cpp        # Board driver implementation
│   ├── input_events.h          # Input event types (from 0074)
│   ├── fixedpoint.h            # Fixed-point arithmetic macros
│   └── trig_lut.h              # Sin/cos lookup table (const flash data)
└── components/
    └── LovyanGFX/              # Display driver (submodule or copy)
```

## Key Design Decisions

1. **2-bit framebuffer, not RGB565.** A 240×240 RGB565 framebuffer is 115 KB — over half the free RAM. A 2-bit framebuffer is 14.4 KB. The tradeoff is a 1 ms expansion pass before display, which is negligible compared to the 11.5 ms SPI transfer.

2. **Scanline-parallel rasterize + dither, not render-then-dither.** Processing dithering inline with rasterization avoids the need for a full-screen intermediate luminance buffer (another 57.6 KB). The Bayer matrix is position-dependent, so per-scanline processing produces the same visual result as a separate pass.

3. **Float for vertices, int for rasterization.** The ESP32-S3 has a hardware FPU that makes float vertex transformation fast. The rasterization inner loop benefits from integer-only arithmetic to avoid FPU stalls and pipeline bubbles.

4. **Const geometry in flash.** Static geometry (terrain heights, torus knots) is computed at build time and stored in flash. Only animated vertices (ocean waves, tunnel ring positions) are updated per-frame in RAM.

5. **Reused M5DialBoard from 0074.** The board initialization, encoder, touch, and display driver are proven code. Copying them avoids re-debugging GPIO configuration and SPI setup.

6. **No LVGL.** The display is driven directly through LovyanGFX. LVGL's refresh model (dirty-rect tracking, widget tree) adds overhead without benefit for a full-frame 3D application.

## Risks and Open Questions

1. **Triangle count vs. frame rate.** The estimate of 500 triangles per frame may be optimistic for 10 FPS. The rasterization inner loop is the bottleneck. If performance is insufficient, reduce geometry further (10×10 terrain grid = 162 triangles).

2. **Per-frame vertex animation for ocean.** Updating 256 vertex heights per frame using `sin()` is ~2,000 float operations. The hardware FPU handles this in ~5 µs — negligible. But if the scene grows, vertex animation cost increases linearly.

3. **SPI transfer bandwidth.** The 11.5 ms SPI transfer is unavoidable without DMA double-buffering. If the render time is under 30 ms, the total frame time is ~42 ms (24 FPS theoretical), but the SPI transfer blocks the CPU during transmission. LovyanGFX's `startWrite()`/`endWrite()` bracketing acquires the SPI bus, which means no other SPI device can be accessed during the 11.5 ms window.

4. **Wireframe mode.** Drawing triangle edges in addition to (or instead of) filled triangles requires a separate line-drawing pass. This is a debug feature, not a production requirement. The wireframe lines would be drawn in `color_high` (white) directly to the 2-bit framebuffer after the fill pass.

5. **Touch input for scene switching.** The original JSX uses a center-tap gesture. The M5Dial's touch controller can detect tap position. A tap within 60 pixels of center (matching the original) cycles to the next scene. This is optional — the encoder button and console commands provide the same functionality.

## References

- `scripts/m5dial.jsx` — Original React/Three.js simulator (full source in ticket)
- `0074-m5dial-web-remote/firmware/main/m5dial_board.h` — M5Dial board driver
- `0074-m5dial-web-remote/firmware/main/input_events.h` — Input event types
- `0072-m5dial-timer-demo/` — Earlier M5Dial project with encoder + display
- `0095-m5dial-wifi-bench/` — M5Dial memory profile (224 KB free heap)
- LovyanGFX `LGFXBase.hpp` — `pushImage()`, `pushColors()` API
- GC9A01 datasheet — 240×240 SPI display controller
