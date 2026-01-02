---
Title: M5GFX functionality inventory + demo plan
Ticket: 0021-M5-GFX-DEMO
Status: active
Topics:
    - cardputer
    - m5gfx
    - display
    - ui
    - keyboard
    - esp32s3
    - esp-idf
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp
      Note: Board autodetect; Cardputer ST7789 offsets; backlight PWM
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.h
      Note: M5GFX wrapper API and M5Canvas alias
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp
      Note: Decoder implementations; qrcode; createPng; PNG memory reuse
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp
      Note: Primary API surface for primitives/text/images/DMA
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Button.hpp
      Note: Button widget API
    - Path: ../../../../../../../M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.hpp
      Note: Sprite/canvas API; pushSprite; transforms
    - Path: ../../../../../../../M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp
      Note: Real-world multi-sprite layout and brightness control usage
    - Path: 0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp
      Note: Known-good Cardputer canvas + waitDMA present pattern
ExternalSources: []
Summary: Exhaustive M5GFX/LGFX feature inventory (APIs + file map) and a coverage-driven demo-suite plan for Cardputer.
LastUpdated: 2026-01-01T19:46:13.779610027-05:00
WhatFor: ""
WhenToUse: ""
---




## Purpose and scope

This document is an exhaustive, codebase-grounded inventory of **M5GFX / LovyanGFX** capabilities as vendored in this repo, plus a concrete plan for a **Cardputer firmware** that demonstrates those capabilities in an interactive, “showcase” way.

Scope boundaries (important for correctness):

- This is based on the vendored sources at `M5Cardputer-UserDemo/components/M5GFX` (the exact code ESP-IDF tutorials in `esp32-s3-m5/` are already using).
- The target runtime environment for the demo firmware is **ESP-IDF** (not Arduino). M5GFX contains Arduino-only convenience wrappers (filesystem/HTTP/image loaders); we inventory them, but we call out what is Arduino-gated.
- This is specifically about **graphics + display IO**; it is not a full analysis of the Cardputer “app framework” used in `M5Cardputer-UserDemo` (e.g. Mooncake), except where it shows proven rendering patterns we can reuse.

## Repo entry points (what to read first)

### Known-good Cardputer usage (ESP-IDF tutorials in this repo)

- Bring-up + sprite/canvas + `waitDMA()` pattern:
  - `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp`
- Text rendering + keyboard-driven UI patterns:
  - `esp32-s3-m5/0012-cardputer-typewriter/main/hello_world_main.cpp`
  - `esp32-s3-m5/0015-cardputer-serial-terminal/main/hello_world_main.cpp`

### The vendored library itself

- High-level wrapper (board autodetect, state stack, progress bar, `M5Canvas` alias):
  - `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.h`
  - `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`
- Core graphics API surface (primitives, text/fonts, images, transforms, DMA):
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp`
- Off-screen rendering (sprites) and transforms:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.cpp`
- Basic widget:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Button.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Button.cpp`

### Cardputer “real world” usage (not just the library)

- Cardputer HAL uses multiple sprites for layout + brightness control:
  - `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp`

## M5GFX architecture (what it is, at a glance)

M5GFX is M5Stack’s packaging of LovyanGFX (v1) plus board support (auto-detect, backlight wiring, touch drivers, etc.). In this repo, it is built as an ESP-IDF component and used directly from C++ apps.

### Class hierarchy (diagram)

The public “thing you construct” on device is typically `m5gfx::M5GFX`, which inherits a large drawing API from LovyanGFX:

```text
m5gfx::M5GFX
  └── lgfx::LGFX_Device
        └── lgfx::LovyanGFX
              └── lgfx::LGFXBase          (drawing primitives, text, images, DMA, etc.)
```

Off-screen buffers are `LGFX_Sprite` (M5 wraps it as `M5Canvas`):

```text
m5gfx::M5Canvas (alias/wrapper)
  └── lgfx::LGFX_Sprite
        └── lgfx::LovyanGFX / LGFXBase    (same draw/text APIs, but backed by a RAM buffer)
```

### Device stack (panel / bus / backlight / touch)

At runtime, `LGFX_Device` holds a `Panel_Device` which composes the hardware interface:

```text
LGFX_Device
  └── Panel_Device
        ├── IBus   (e.g., Bus_SPI on ESP32-S3)
        ├── IPanel (chip-specific panel, e.g., Panel_ST7789)
        ├── ILight (backlight control, e.g., PWM or PMIC)
        └── ITouch (optional touch controller)
```

For Cardputer specifically, autodetect configures:

- bus: SPI (SPI3 host)
- panel: ST7789 with visible offset (because the glass is 240×135 visible inside a larger RAM window)
- backlight: PWM on a board-specific pin

## Cardputer bring-up specifics (important for all demos)

The Cardputer display bring-up that our demo firmware should treat as canonical is:

```cpp
m5gfx::M5GFX display;
display.init();          // triggers board autodetect
display.setColorDepth(16);
```

The implementation details live in `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`, within `M5GFX::autodetect(...)`.

### Cardputer ST7789 panel config (from autodetect)

The autodetect path for Cardputer chooses `Panel_ST7789` and configures it approximately as follows:

- `cfg.panel_width = 135`
- `cfg.panel_height = 240`
- `cfg.offset_x = 52`
- `cfg.offset_y = 40`
- `cfg.invert = true`
- `cfg.readable = true`
- rotation is set to `1` for Cardputer (so logical width/height become 240×135)
- backlight uses `_set_pwm_backlight(GPIO_NUM_38, 7, 256, false, 16)` (pin/channel/freq/offset)

Those offsets are what makes “(0,0)” map to the visible top-left window.

### DMA pacing (`waitDMA()` pattern)

The tutorial `esp32-s3-m5/0011-cardputer-m5gfx-plasma-animation/main/hello_world_main.cpp` uses:

- `M5Canvas canvas(&display);`
- `canvas.createSprite(w, h);`
- `canvas.pushSprite(0, 0);`
- `display.waitDMA();`

The important idea is: pushing a full-screen sprite uses DMA; if you start writing the next frame into the sprite buffer too early (or trigger another transfer), you can get visible tearing/flicker. The demo suite should use a consistent “present” path:

```cpp
canvas.pushSprite(0, 0);
display.waitDMA(); // only if the demo is DMA-heavy / full-screen present
```

## Feature inventory (what exists, where it lives, how to demo it)

This section inventories M5GFX/LGFX functionality by capability. For each capability we include:

- what it does
- key API signatures (as they appear in headers)
- where it is implemented
- how we should demo it on Cardputer

### 1) Initialization and device control

#### M5GFX wrapper conveniences (M5GFX.h)

`m5gfx::M5GFX` adds a small amount of convenience functionality on top of `lgfx::LGFX_Device`:

```cpp
class M5GFX : public lgfx::LGFX_Device {
public:
  void clearDisplay(int32_t color = TFT_BLACK);
  void progressBar(int x, int y, int w, int h, uint8_t val);

  void pushState(void);
  void popState(void);

  // Board-specific autodetect init:
  bool init(void);                 // inherited via LGFX_Device

  // Explicit panel init (bypasses autodetect):
  bool init(lgfx::Panel_Device* panel);
};
```

How to demo on Cardputer:

- **`progressBar`**: implement a “loading” demo that animates a progress bar while also showing the underlying primitive calls (it is a good example of “small UI helpers” in M5GFX).
- **`pushState/popState`**: show that text settings (font, size, style, cursor) can be pushed/popped:
  - set a big font/style, print something
  - `pushState()`
  - change style dramatically, print
  - `popState()`
  - confirm style is restored

#### Core device API (LGFX_Device)

From `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`:

```cpp
class LGFX_Device : public LovyanGFX {
public:
  bool init(void);
  bool begin(void);
  bool init_without_reset(void);

  board_t getBoard(void) const;

  void initBus(void);
  void releaseBus(void);
  void setPanel(Panel_Device* panel);

  void sleep(void);
  void wakeup(void);
  void powerSave(bool flg);

  void setBrightness(uint8_t brightness);
  uint8_t getBrightness(void) const;

  uint_fast8_t getTouchRaw(touch_point_t *tp, uint_fast8_t count = 1);
  uint_fast8_t getTouch(touch_point_t *tp, uint_fast8_t count = 1);
};
```

How to demo on Cardputer:

- **Brightness**: reuse the known-good loop from `M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp` (it fades the backlight 0→255→0 while printing the current value).
- **Sleep/Wakeup**: demonstrate quickly toggling sleep mode (backlight to 0 + panel sleep), then wake.
- **Board identity**: print `display.getBoard()` enum (or an interpreted string) to confirm autodetect chooses `board_M5Cardputer`.

### 2.5) Display refresh control (auto display, scanline, busy waits)

Even if most of our demos “just draw”, M5GFX/LGFX exposes panel-level display refresh controls that are useful for performance/debugging.

From `.../lgfx/v1/LGFXBase.hpp`:

```cpp
void display(int32_t x, int32_t y, int32_t w, int32_t h);
void display(void);
void waitDisplay(void);
bool displayBusy(void);
void setAutoDisplay(bool flg);

int32_t getScanLine(void); // raw scanline if supported, else -1
```

How to demo on Cardputer:

- **Scanline**: show `getScanLine()` live (if supported) and explain it is “raw” (not offset/rotation adjusted).
- **Auto display**: if relevant for the configured panel, toggle `setAutoDisplay(false)` and explicitly call `display()` after drawing a frame.

### 2) Write batching, DMA, and performance controls

#### Transaction + DMA API (LGFXBase)

From `.../lgfx/v1/LGFXBase.hpp`:

```cpp
void startWrite(bool transaction = true);
void endWrite(void);

void waitDMA(void);
bool dmaBusy(void);

bool getSwapBytes(void) const;
void setSwapBytes(bool swap);
```

How to demo on Cardputer:

- **Batching**: show a primitive-heavy frame rendered:
  - once without `startWrite()/endWrite()` (slow path),
  - once with them (fast path).
  - Keep the content identical (e.g., 500 rectangles / lines).
- **DMA**: show full-screen sprite push with and without `waitDMA()` and document any tearing.
- **swapBytes**: demonstrate pushing raw RGB565 buffers where endianness matters:
  - fill a small sprite buffer manually with `LGFXBase::color565(...)` and push with swap toggled.

### 3) Geometry / drawing primitives

The drawing primitive API is extensive and is implemented primarily in:

- `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
- `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp`

#### Core primitives (selected signatures)

From `LGFXBase.hpp` (subset; representative):

```cpp
void drawPixel(int32_t x, int32_t y);
template<typename T> void drawPixel(int32_t x, int32_t y, const T& color);

void drawFastHLine(int32_t x, int32_t y, int32_t w);
void drawFastVLine(int32_t x, int32_t y, int32_t h);
void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

void fillRect(int32_t x, int32_t y, int32_t w, int32_t h);
void drawRect(int32_t x, int32_t y, int32_t w, int32_t h);

void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r);
void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r);

void drawCircle(int32_t x, int32_t y, int32_t r);
void fillCircle(int32_t x, int32_t y, int32_t r);

void drawEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry);
void fillEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry);

void drawTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

void drawBezier(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void drawBezier(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3);

void drawArc(int32_t x, int32_t y, int32_t r0, int32_t r1, float angle0, float angle1);
void fillArc(int32_t x, int32_t y, int32_t r0, int32_t r1, float angle0, float angle1);

void floodFill(int32_t x, int32_t y);
```

Also present:

- gradient lines: `drawGradientLine(...)`, plus helpers for H/V gradients
- “smooth” (anti-aliased/wide) lines: `drawSmoothLine(...)` / `drawWideLine(...)`
- affine fill: `fillAffine(matrix, w, h)` (fills a transformed rectangle region)
- smooth fills: `fillSmoothRoundRect(...)`, `fillSmoothCircle(...)`
- gradient rect fills: `fillGradientRect(...)`

How to demo on Cardputer:

- **Primitives gallery**: page through a grid of examples with a label and parameter values:
  - `drawLine` (various slopes), `drawTriangle` vs `fillTriangle`, `drawCircle` vs `fillCircle`, etc.
- **Arc demo**: show a radial gauge (progress ring) using `fillArc` and animate it 0→360.
- **Bezier demo**: show control points + curve; animate control points to show dynamic update.
- **Flood fill**: draw a bounded region and flood fill at different seed points.

Pseudocode sketch for a primitives gallery page:

```cpp
void draw_primitives_page(M5Canvas& c, int page) {
  c.fillScreen(TFT_BLACK);
  c.setTextColor(TFT_WHITE, TFT_BLACK);
  c.setTextSize(1);
  c.setCursor(0, 0);
  c.printf("Primitives page %d", page);

  c.startWrite();
  // Example: rounded rects
  c.drawRoundRect(10, 20, 80, 40, 8, TFT_GREEN);
  c.fillRoundRect(110, 20, 80, 40, 8, TFT_BLUE);
  // Example: arcs
  c.fillArc(60, 100, 30, 40, 0.0f, 240.0f, TFT_ORANGE);
  // Example: bezier
  c.drawBezier(10, 120, 60, 70, 120, 150, TFT_PINK);
  c.endWrite();
}
```

### 4) Color conversion, palettes, and color depth

#### Color helpers (LGFXBase static helpers)

From `LGFXBase.hpp`:

```cpp
static constexpr uint8_t  color332(uint8_t r, uint8_t g, uint8_t b);
static constexpr uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
static constexpr uint32_t color888(uint8_t r, uint8_t g, uint8_t b);

static constexpr uint16_t swap565(uint8_t r, uint8_t g, uint8_t b);
static constexpr uint32_t swap888(uint8_t r, uint8_t g, uint8_t b);

static uint8_t  color16to8(uint32_t rgb565);
static uint16_t color8to16(uint32_t rgb332);
static uint32_t color16to24(uint32_t rgb565);
static uint16_t color24to16(uint32_t rgb888);
```

Sprites also support paletted modes (useful for memory-constrained buffers). From `LGFX_Sprite.hpp`:

```cpp
void* setColorDepth(uint8_t bpp);
void* setColorDepth(color_depth_t depth);

bool createPalette(void);
bool createPalette(const uint16_t* colors, uint32_t count);
bool createPalette(const uint32_t* colors, uint32_t count);
void setPaletteGrayscale(void);
void setPaletteColor(size_t index, ...);
```

How to demo on Cardputer:

- **Color-depth matrix**: create multiple small sprites at different color depths (e.g., 16-bit vs 8-bit paletted) and draw the same gradient pattern; show memory usage via `bufferLength()`.
- **Palette editing**: animate palette entries (e.g., cycle the palette colors) while sprite pixel indices stay constant.

### 4.5) Clipping, scroll regions, and copy/scroll operations

These APIs are easy to miss but extremely useful for UI work: they enable partial redraw without manually maintaining dirty rectangles.

From `.../lgfx/v1/LGFXBase.hpp`:

```cpp
void setClipRect(int32_t x, int32_t y, int32_t w, int32_t h);
void getClipRect(int32_t *x, int32_t *y, int32_t *w, int32_t *h);
void clearClipRect(void);

void setScrollRect(int32_t x, int32_t y, int32_t w, int32_t h);
void getScrollRect(int32_t *x, int32_t *y, int32_t *w, int32_t *h);
void clearScrollRect(void);

void scroll(int_fast16_t dx, int_fast16_t dy = 0);
void copyRect(int32_t dst_x, int32_t dst_y, int32_t w, int32_t h, int32_t src_x, int32_t src_y);
```

How to demo on Cardputer:

- **Clip demo**: draw a large pattern, then enable a clip rect and show that subsequent draws are clipped; render the clip boundary overlay so the effect is obvious.
- **Scroll demo**: implement a terminal-like scrolling text region using:
  - `setScrollRect(...)` for the text area
  - `scroll(0, -line_height)` to make room for new lines
  - draw the new line at the bottom
- **copyRect demo**: implement a “rubber stamp” / sprite-less scrolling viewport (copy an area down/right and draw only the newly exposed strip).

### 5) Text rendering and fonts

Text is implemented in the core `LGFXBase` layer and supports:

- cursor-based printing (`setCursor` + `print/printf/write`)
- direct positioned drawing (`drawString`)
- datum-based anchoring (`setTextDatum`)
- scalable fonts (`setTextSize(sx, sy)`)
- multiple font backends (built-in, GFX free fonts, VLW fonts via `loadFont`)

#### Text style API (selected signatures)

From `LGFXBase.hpp`:

```cpp
void setCursor(int32_t x, int32_t y);
void setTextSize(float size);
void setTextSize(float sx, float sy);

void setTextDatum(uint8_t datum);
void setTextDatum(textdatum_t datum);

void setTextPadding(uint32_t padding_x);
void setTextWrap(bool wrapX, bool wrapY = false);

template <typename T> void setTextColor(T color);
template <typename T1, typename T2> void setTextColor(T1 fgcolor, T2 bgcolor);

size_t drawString(const char *string, int32_t x, int32_t y);
size_t drawNumber(long long_num, int32_t x, int32_t y, const IFont* font);
size_t drawFloat(float floatNumber, uint8_t dp, int32_t x, int32_t y, const IFont* font);

void setFont(const IFont* font);
bool loadFont(const uint8_t* array);
bool loadFont(const char *path);
void unloadFont(void);
```

How to demo on Cardputer:

- **Font gallery**: show:
  - built-in bitmap fonts
  - a GFX free font (from `lgfx/Fonts/GFXFF/...`)
  - a VLW-loaded font (if we decide to include a VLW asset or load from SD)
- **Datum demo**: draw the same string anchored at center/top-left/bottom-right etc; draw the anchor crosshair so datum semantics are visible.
- **Text wrap + padding**: show a paragraph clipped in a box with padding; toggle wrap modes.

### 6) Sprites / canvases (off-screen rendering)

Sprites are central to flicker-free UI because they let you render into RAM and then push once per frame. This repo already uses this approach heavily.

#### Key sprite API

From `LGFX_Sprite.hpp`:

```cpp
void setPsram(bool enabled);

void* createSprite(int32_t w, int32_t h);
void deleteSprite(void);

void* getBuffer(void) const;
uint32_t bufferLength(void) const;

void pushSprite(int32_t x, int32_t y);
template<typename T> void pushSprite(int32_t x, int32_t y, const T& transparent);

void pushRotated(float angle);
void pushRotatedWithAA(float angle);
void pushRotateZoom(...);
void pushRotateZoomWithAA(...);
void pushAffine(const float matrix[6]);
void pushAffineWithAA(const float matrix[6]);
```

M5GFX exposes `M5Canvas` as:

```cpp
class M5Canvas : public lgfx::LGFX_Sprite {
public:
  M5Canvas(LovyanGFX* parent) : LGFX_Sprite(parent) { _psram = true; }
  void* frameBuffer(uint8_t) { return getBuffer(); }
};
```

How to demo on Cardputer:

- **Canvas basics**: show a full-screen canvas animation (plasma, checkerboard, etc.) with `waitDMA()`.
- **Partial redraw**: use a sprite smaller than screen and move it around; demonstrate `pushSprite` with transparency.
- **Rotate/zoom/affine**: render a sprite containing a logo or checkerboard and:
  - rotate around a pivot
  - zoom in/out
  - apply an affine matrix (skew + rotation)

### 7) Images and decoders (BMP/JPG/PNG/QOI)

The core supports multiple image formats and data sources via `DataWrapper` and helper generators in `LGFXBase.hpp`. The key public methods (available regardless of Arduino) are:

```cpp
bool drawBmp(const uint8_t *data, uint32_t len, ...);
bool drawJpg(const uint8_t *data, uint32_t len, ...);
bool drawPng(const uint8_t *data, uint32_t len, ...);
bool drawQoi(const uint8_t *data, uint32_t len, ...);
```

For Arduino builds, `lgfx_filesystem_support.hpp` adds convenience overloads for FS/Stream/HTTP (guarded by `#if defined(ARDUINO)` + header presence).

Implementation notes (important for demo reliability):

- PNG decoding uses a global `pngle_t*` that is *reused* across calls to avoid allocation failures; explicit cleanup is via:
  - `void releasePngMemory(void);`
  - (implemented in `LGFXBase.cpp` by destroying the global pngle instance)
- QOI decoding allocates a DMA line buffer per decode call (see `LGFXBase::draw_qoi(...)`).

How to demo on Cardputer:

- **Format zoo**: ship a small embedded test image in each format (BMP/JPG/PNG/QOI) and render them side-by-side.
- **Scaling/cropping**: use `maxWidth/maxHeight/offX/offY/scale_x/scale_y` to show:
  - cropping into a viewport
  - downscaling to fit
- **Alpha**: use a PNG with transparency over a background pattern (checkerboard) to verify alpha blending.

### 8) QR codes

`LGFXBase` exposes a QR code renderer (using an embedded QR implementation in `lgfx/utility/lgfx_qrcode.*`).

From `LGFXBase.hpp`:

```cpp
void qrcode(const char *string, int32_t x = -1, int32_t y = -1, int32_t width = -1, uint8_t version = 1);
```

How to demo on Cardputer:

- Render a QR code encoding:
  - the build/version string
  - Wi-Fi credentials (for testing only)
  - a URL to the repo
- Provide a size slider (keyboard-controlled) to change `width` and show how module thickness is derived.

### 9) Screenshot export (PNG encoding)

`LGFXBase` supports encoding a region of the current display to a PNG buffer in memory:

From `LGFXBase.hpp`:

```cpp
void* createPng(size_t* datalen, int32_t x = 0, int32_t y = 0, int32_t width = 0, int32_t height = 0);
void releasePngMemory(void);
```

From `LGFXBase.cpp`, `createPng` reads rows via:

```cpp
readRectRGB(x, y + row, w, 1, pImage);
```

How to demo on Cardputer:

- Provide a “Screenshot” action:
  - call `createPng(&len, 0, 0, display.width(), display.height())`
  - stream the PNG bytes over USB-Serial-JTAG
- This demo is an excellent “integration test” for readback and for downstream tooling (PC can open the PNG).

### 10) Widgets: `LGFX_Button`

M5GFX includes a simple immediate-mode “button” widget. It is not a UI framework; it’s a helper that draws a rounded rectangle with label and tracks press state (you provide the pointer/touch state externally).

Key API from `LGFX_Button.hpp`:

```cpp
template<typename T>
void initButton(LovyanGFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                const T& outline, const T& fill, const T& textcolor,
                const char *label, float textsize_x = 1.0f, float textsize_y = 0.0f);

void drawButton(bool inverted = false, const char* long_name = nullptr);

bool contains(int16_t x, int16_t y) const;
void press(bool p);
bool isPressed(void) const;
bool justPressed(void) const;
bool justReleased(void) const;
```

How to demo on Cardputer:

- Cardputer has no touch panel by default, so we cannot demo `contains(x,y)` based on touch. Instead:
  - simulate “press” via keyboard keys (e.g., space toggles pressed)
  - draw with `inverted=true` when pressed
- This still demonstrates:
  - text datum usage inside the button (internally uses `setTextDatum(...)`)
  - drawing helpers (`fillRoundRect`, `drawRoundRect`)

### 11) Touch support (exists, but likely not used on Cardputer)

M5GFX includes touch drivers (`Touch_FT5x06`, `Touch_GT911`, `Touch_CST816S`) and `LGFX_Device` exposes:

```cpp
ITouch* touch(void) const;
uint_fast8_t getTouch(touch_point_t *tp, uint_fast8_t count = 1);
```

How to demo on Cardputer:

- Not applicable unless the hardware is extended with a touch unit.
- The demo suite can include a “touch diagnostics” page that:
  - checks if `display.touch()` is non-null
  - if present, plots touch points
  - otherwise shows “no touch controller detected”

### 12) Bitmaps (1-bit) and indexed pixel streams

These are important for “UI iconography” and for memory-efficient assets.

From `.../lgfx/v1/LGFXBase.hpp`:

```cpp
template<typename T>
void drawBitmap (int32_t x, int32_t y, const uint8_t* bitmap, int32_t w, int32_t h, const T& color);

template<typename T>
void drawXBitmap(int32_t x, int32_t y, const uint8_t* bitmap, int32_t w, int32_t h, const T& color);

template<typename T>
void writeIndexedPixels(const uint8_t* data, T* palette, int32_t len, uint8_t depth = 8);
```

How to demo on Cardputer:

- **Bitmap icon gallery**: embed a handful of 1-bit icons (keyboard symbol, battery, wifi) and render them with different foreground/background colors.
- **Indexed pixel stream**: generate a small paletted strip (e.g., a rainbow gradient in 256 entries) and write it efficiently using `writeIndexedPixels`.

### 13) Readback (readPixel / readRect) and “introspection”

Readback enables a class of demos/tools:

- screenshot creation (covered above)
- pixel inspector / magnifier
- verification tests (e.g., draw something, read it back and assert)

From `.../lgfx/v1/LGFXBase.hpp`:

```cpp
uint16_t readPixel(int32_t x, int32_t y);
RGBColor readPixelRGB(int32_t x, int32_t y);

void readRectRGB(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t* data);
template<typename T> void readRect(int32_t x, int32_t y, int32_t w, int32_t h, T* data);
```

How to demo on Cardputer:

- **Pixel inspector**: move a cursor with keyboard keys and show:
  - coordinates
  - `readPixel(...)` RGB565 value
  - `readPixelRGB(...)` RGB888 value
- **Render-test page**: draw a known pattern (e.g., red/green/blue squares), read back a few points, and print PASS/FAIL.

## Demo-suite firmware proposal (Cardputer graphics catalog)

This section describes how to structure a single firmware that can exercise the above features without turning into an unmaintainable mess.

### UX goals

- It should be possible to **browse demos** on-device with only the Cardputer keyboard.
- Each demo should explain itself (what it’s showing; which APIs it uses).
- Demos should be written in a way that is easy to copy into future projects.

### Suggested architecture (modules and data flow)

```text
app_main
  ├── init display (M5GFX autodetect)
  ├── init input (keyboard scan)
  ├── init SceneManager
  │     ├── registry of demos
  │     └── current demo state
  └── main loop
        ├── poll input -> events
        ├── current_demo.update(dt, events)
        ├── current_demo.render(canvas/display)
        └── present (pushSprite + waitDMA if needed)
```

Suggested “demo interface” pseudocode:

```cpp
struct InputEvent {
  enum Kind { KeyDown, KeyUp } kind;
  int key; // normalized key enum
};

class Demo {
public:
  virtual const char* name() const = 0;
  virtual const char* description() const = 0;
  virtual void init(m5gfx::M5GFX& display) = 0;
  virtual void update(uint32_t dt_ms, std::span<const InputEvent> events) = 0;
  virtual void render(m5gfx::M5GFX& display, m5gfx::M5Canvas& canvas) = 0;
  virtual bool needs_wait_dma() const { return true; }
  virtual ~Demo() = default;
};
```

### Demo list (coverage-driven)

The following set aims to cover essentially every “user-facing” concept in LGFXBase/M5GFX without requiring external peripherals.

1. **Bring-up & diagnostics**
   - Shows: board name, width/height, color depth, heap, DMA free, brightness.
   - APIs: `init`, `getBoard`, `width/height`, `setBrightness`, `printf`.
2. **Primitives gallery**
   - Shows: lines/rects/circles/triangles/bezier/arcs/gradient/smooth lines.
   - APIs: drawing primitives + `startWrite/endWrite`.
3. **Text & datums**
   - Shows: `setTextDatum`, padding, wrap, scaling, `drawString` vs `printf`.
4. **Canvas animation (full-screen)**
   - Shows: M5Canvas usage, `getBuffer`, `pushSprite`, `waitDMA`.
   - Implementation baseline: `0011-cardputer-m5gfx-plasma-animation`.
5. **Sprite transforms**
   - Shows: rotate, rotate+AA, zoom, affine; pivot control.
   - APIs: `setPivot`, `pushRotated(WithAA)`, `pushRotateZoom(WithAA)`, `pushAffine(WithAA)`.
6. **Palettes & low-bpp sprites**
   - Shows: memory savings and palette animation.
   - APIs: `setColorDepth`, `createPalette`, `setPaletteColor`, `bufferLength`.
7. **Image formats zoo**
   - Shows: BMP/JPG/PNG/QOI decode (embedded assets).
   - APIs: `drawBmp`, `drawJpg`, `drawPng`, `drawQoi`, plus scale/crop params.
8. **PNG alpha compositing**
   - Shows: PNG with alpha over checkerboard (verifies transparency).
9. **QR code**
   - Shows: QR code at different versions/sizes.
   - APIs: `qrcode(...)`.
10. **LGFX_Button (keyboard simulated)**
    - Shows: button rendering states; “pressed” toggles.
    - APIs: `LGFX_Button::initButton`, `drawButton`, `press`.
11. **Screenshot export**
    - Shows: encode current screen to PNG and stream over serial.
    - APIs: `createPng`, `readRectRGB`.
12. **Scroll & clip toolbox**
    - Shows: clip rect and scroll rect; terminal-style scrolling.
    - APIs: `setClipRect`, `clearClipRect`, `setScrollRect`, `scroll`, `copyRect`.
13. **Bitmap + indexed pixels**
    - Shows: 1-bit icons, indexed pixel streaming.
    - APIs: `drawBitmap`, `drawXBitmap`, `writeIndexedPixels`.

### Coverage matrix (demo ↔ capability)

This matrix is intended as a checklist: when a demo is implemented, we should be able to point at this table and confidently claim “we exercised the feature”.

Legend: `X` = primary focus, `~` = incidental/secondary, `-` = not used.

| Capability | 1 Bring-up | 2 Primitives | 3 Text | 4 Canvas | 5 Xforms | 6 Palette | 7 Images | 8 Alpha | 9 QR | 10 Button | 11 Screenshot | 12 Scroll/Clip | 13 Bitmap/Indexed |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `M5GFX::autodetect` / board init | X | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ |
| `startWrite/endWrite` batching | ~ | X | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ |
| DMA present + `waitDMA()` | ~ | ~ | ~ | X | X | ~ | ~ | ~ | ~ | ~ | ~ | ~ | ~ |
| Primitives (rect/line/circle/arc/bezier) | - | X | ~ | ~ | ~ | ~ | - | - | - | ~ | ~ | ~ | ~ |
| Text style + datums + wrap | ~ | ~ | X | ~ | ~ | ~ | ~ | ~ | ~ | X | ~ | ~ | ~ |
| Sprites (`LGFX_Sprite` / `M5Canvas`) | ~ | ~ | ~ | X | X | X | ~ | ~ | ~ | ~ | ~ | ~ | ~ |
| Transforms (rotate/zoom/affine + AA) | - | - | - | ~ | X | - | - | - | - | - | - | - | - |
| Image decode (BMP/JPG/PNG/QOI) | - | - | - | - | - | - | X | X | - | - | - | - | - |
| QR code | - | - | ~ | - | - | - | - | - | X | - | - | - | - |
| `createPng` screenshot | - | - | - | - | - | - | - | - | - | - | X | - | - |
| Clip / scroll / copyRect | - | ~ | ~ | - | - | - | - | - | - | - | - | X | - |
| 1-bit bitmaps / indexed pixels | - | ~ | ~ | - | - | X | - | - | - | - | - | - | X |

### Source map (capability ↔ file)

This is the “where is it implemented?” index for reviewers and future work.

- Board autodetect + Cardputer ST7789 offsets + backlight PWM:
  - `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`
- Wrapper conveniences (`progressBar`, state stack, `M5Canvas` alias):
  - `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.h`
  - `M5Cardputer-UserDemo/components/M5GFX/src/M5GFX.cpp`
- Core drawing/text/DMA APIs:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFXBase.cpp`
- Sprite backing store + sprite transforms:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Sprite.cpp`
- Button widget:
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Button.hpp`
  - `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/v1/LGFX_Button.cpp`
- Image decoders and utility libs:
  - PNG (pngle): `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/utility/lgfx_pngle.*` + `LGFXBase::draw_png`
  - JPEG (tjpgd): `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/utility/lgfx_tjpgd.*` + `LGFXBase::draw_jpg`
  - QOI: `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/utility/lgfx_qoi.*` + `LGFXBase::draw_qoi`
  - QR: `M5Cardputer-UserDemo/components/M5GFX/src/lgfx/utility/lgfx_qrcode.*` + `LGFXBase::qrcode`

### Demo implementation patterns (what to standardize)

#### Pattern A: full-screen canvas with DMA-safe present

Use when the render path is “full screen redraw”:

```cpp
canvas.fillScreen(TFT_BLACK);
// draw...
canvas.pushSprite(0, 0);
display.waitDMA();
```

#### Pattern B: mixed direct draw + sprite overlays

Use when you want to demonstrate primitives directly on the panel (no sprite), but still want a stable overlay HUD:

```cpp
display.startWrite();
// heavy direct primitives
display.endWrite();

hud_canvas.pushSprite(0, 0, TFT_BLACK /*transparent key*/);
display.waitDMA();
```

#### Pattern C: multi-sprite layout (Cardputer style)

`M5Cardputer-UserDemo/main/hal/hal_cardputer.cpp` uses multiple sprites:

- a main content canvas
- a keyboard bar canvas
- a system status bar canvas

This layout pattern is attractive for the demo suite because it keeps an always-on navigation/status region without re-rendering the entire screen.

## Open questions / risks (call these out early)

- **GIF**: M5GFX itself does not appear to include GIF playback; this repo has a separate `animatedgif` component and `esp32-s3-m5/components/echo_gif`. If we want GIF demos, treat them as “extra” beyond M5GFX.
- **Filesystem integration**: Arduino-only convenience helpers exist for FS/HTTP; for ESP-IDF we should either:
  - embed assets as arrays, or
  - add an ESP-IDF file wrapper that can feed `DataWrapper` (this is a separate effort).
- **Memory**: full-screen 240×135 RGB565 sprite is ~64 KiB; that’s fine. Larger buffers (e.g., `createPng`) can allocate additional DMA memory; we should keep screenshot regions small or stream line-by-line.

## Next actions (implementation-facing)

- Create a new `esp32-s3-m5/00XX-cardputer-m5gfx-demo-suite/` ESP-IDF project that:
  - depends on `../../M5Cardputer-UserDemo/components/M5GFX` (as existing tutorials do)
  - includes a keyboard input module (reuse from `0015-cardputer-serial-terminal` or `0012-cardputer-typewriter`)
- Implement the demo registry + menu navigation first, then add demos incrementally in the order listed above.
