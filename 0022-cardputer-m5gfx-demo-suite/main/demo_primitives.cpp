#include "demo_primitives.h"

#include <algorithm>
#include <stdint.h>

void primitives_render(M5Canvas &body) {
    const int w = body.width();
    const int h = body.height();

    body.fillRect(0, 0, w, h, TFT_BLACK);

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("C2 Primitives", 6, 6);

    // Frame + crosshair.
    body.drawRect(0, 0, w, h, TFT_DARKGREY);
    body.drawLine(0, h / 2, w - 1, h / 2, TFT_DARKGREY);
    body.drawLine(w / 2, 0, w / 2, h - 1, TFT_DARKGREY);

    // Corners: filled rects to show color + fill.
    body.fillRect(6, 24, 28, 18, TFT_RED);
    body.fillRect(w - 6 - 28, 24, 28, 18, TFT_GREEN);
    body.fillRect(6, h - 6 - 18, 28, 18, TFT_BLUE);
    body.fillRect(w - 6 - 28, h - 6 - 18, 28, 18, TFT_YELLOW);

    // Circles in center.
    const int cx = w / 2;
    const int cy = h / 2;
    for (int r = 6; r <= std::min(w, h) / 2 - 6; r += 10) {
        uint32_t col = (r % 20 == 0) ? TFT_CYAN : TFT_MAGENTA;
        body.drawCircle(cx, cy, r, col);
    }

    // Triangles.
    body.fillTriangle(42, 30, 70, 60, 14, 60, TFT_DARKGREEN);
    body.drawTriangle(w - 42, 30, w - 70, 60, w - 14, 60, TFT_ORANGE);

    // Text baseline / datum demo.
    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    body.drawString("Lines / rects / circles / triangles", 6, h - 18);
}
