#include "papers3_canvas.h"

#include <M5Unified.hpp>

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
bool g_frame_active = false;
int32_t g_canvas_width = 960;
int32_t g_canvas_height = 540;
int32_t g_default_present_mode = 1;

uint32_t NormalizeColor(uint32_t color)
{
    return color & 0x00FFFFFFu;
}

epd_mode_t MapPresentMode(int32_t mode)
{
    switch (mode) {
        case 0:
            return epd_mode_t::epd_text;
        case 2:
            return epd_mode_t::epd_quality;
        case 1:
        default:
            return epd_mode_t::epd_fast;
    }
}

void EnsureCanvasInitialized()
{
    if (g_canvas_initialized) {
        return;
    }

    g_canvas_width = M5.Display.width();
    g_canvas_height = M5.Display.height();
    g_canvas_initialized = true;
}

void BeginFrameIfNeeded()
{
    EnsureCanvasInitialized();
    if (g_frame_active) {
        return;
    }

    M5.Display.waitDisplay();
    M5.Display.setEpdMode(MapPresentMode(g_default_present_mode));
    M5.Display.startWrite();
    g_frame_active = true;
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
    PaperCanvasResetFrame();
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
    if (g_frame_active) {
        M5.Display.endWrite();
        M5.Display.waitDisplay();
        g_frame_active = false;
    }
    g_default_present_mode = 1;
}

void PaperCanvasScreenClear(uint32_t color)
{
    BeginFrameIfNeeded();
    M5.Display.fillScreen(NormalizeColor(color));
}

void PaperCanvasDrawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    const ClampedRect rect = ClampRect(x, y, w, h);
    if (!rect.valid) {
        return;
    }

    BeginFrameIfNeeded();
    M5.Display.drawRect(rect.x, rect.y, rect.w, rect.h, NormalizeColor(color));
}

void PaperCanvasFillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    const ClampedRect rect = ClampRect(x, y, w, h);
    if (!rect.valid) {
        return;
    }

    BeginFrameIfNeeded();
    M5.Display.fillRect(rect.x, rect.y, rect.w, rect.h, NormalizeColor(color));
}

void PaperCanvasPresent(int32_t mode)
{
    g_default_present_mode = mode;
    if (!g_frame_active) {
        return;
    }

    M5.Display.setEpdMode(MapPresentMode(mode));
    M5.Display.endWrite();
    M5.Display.waitDisplay();
    g_frame_active = false;
}

}  // namespace papers3_wasm
