#include "atoms3r_canvas.h"

#include "display_hal.h"

#include <cstdint>

namespace papers3_wasm {

namespace {

struct ClampedRect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    bool valid;
};

bool g_canvas_initialized = false;
int32_t g_canvas_width = 128;
int32_t g_canvas_height = 128;

uint32_t NormalizeColor(uint32_t color)
{
    return color & 0x00FFFFFFu;
}

void EnsureCanvasInitialized()
{
    if (g_canvas_initialized) {
        return;
    }

    auto &display = display_get();
    g_canvas_width = display.width();
    g_canvas_height = display.height();
    g_canvas_initialized = true;
}

ClampedRect ClampRect(int32_t x, int32_t y, int32_t w, int32_t h)
{
    EnsureCanvasInitialized();
    if (w <= 0 || h <= 0) {
        return { 0, 0, 0, 0, false };
    }

    const int64_t raw_x2 = static_cast<int64_t>(x) + static_cast<int64_t>(w);
    const int64_t raw_y2 = static_cast<int64_t>(y) + static_cast<int64_t>(h);

    const int32_t x1 = x < 0 ? 0 : (x > g_canvas_width ? g_canvas_width : x);
    const int32_t y1 = y < 0 ? 0 : (y > g_canvas_height ? g_canvas_height : y);
    const int32_t x2 = raw_x2 < 0 ? 0 : (raw_x2 > g_canvas_width ? g_canvas_width : static_cast<int32_t>(raw_x2));
    const int32_t y2 =
        raw_y2 < 0 ? 0 : (raw_y2 > g_canvas_height ? g_canvas_height : static_cast<int32_t>(raw_y2));

    if (x2 <= x1 || y2 <= y1) {
        return { 0, 0, 0, 0, false };
    }

    return { x1, y1, x2 - x1, y2 - y1, true };
}

}  // namespace

void InitializePaperCanvas()
{
    EnsureCanvasInitialized();
}

int32_t PaperCanvasWidth()
{
    EnsureCanvasInitialized();
    return g_canvas_width;
}

int32_t PaperCanvasHeight()
{
    EnsureCanvasInitialized();
    return g_canvas_height;
}

void PaperCanvasResetFrame()
{
    display_get().waitDMA();
}

void PaperCanvasScreenClear(uint32_t color)
{
    display_get().fillScreen(NormalizeColor(color));
}

void PaperCanvasDrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    const ClampedRect rect = ClampRect(x, y, w, h);
    if (!rect.valid) {
        return;
    }

    display_get().drawRect(rect.x, rect.y, rect.w, rect.h, NormalizeColor(color));
}

void PaperCanvasFillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    const ClampedRect rect = ClampRect(x, y, w, h);
    if (!rect.valid) {
        return;
    }

    display_get().fillRect(rect.x, rect.y, rect.w, rect.h, NormalizeColor(color));
}

void PaperCanvasPresent(int32_t)
{
    display_get().waitDMA();
}

}  // namespace papers3_wasm
