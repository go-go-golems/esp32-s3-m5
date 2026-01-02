#include "demo_d3_text_utf8.h"

#include <stdio.h>

#include "lgfx/v1/lgfx_fonts.hpp"

namespace {

static void draw_line(M5Canvas &g, int x, int y, const char *label, const char *text) {
    g.setFont(&fonts::Font0);
    g.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g.drawString(label, x, y);
    y += 12;
    g.setTextColor(TFT_WHITE, TFT_BLACK);
    g.drawString(text, x, y);
}

} // namespace

void demo_d3_text_utf8_render(M5Canvas &body) {
    const int w = body.width();
    const int h = body.height();

    body.fillRect(0, 0, w, h, TFT_BLACK);
    body.drawRect(0, 0, w, h, TFT_DARKGREY);

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);

    body.setFont(&fonts::Font0);
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("D3 Text: UTF-8 / symbols sanity", 6, 6);
    body.setTextColor(TFT_DARKGREY, TFT_BLACK);
    body.drawString("(Render result depends on active font/glyph coverage)", 6, 20);

    int y = 36;
    draw_line(body, 6, y, "Latin + accents:", "Hello äöü ß ñ ç");
    y += 26;
    draw_line(body, 6, y, "Arrows:", "← ↑ → ↓  ↩ ↪");
    y += 26;
    draw_line(body, 6, y, "Math / misc:", "π ≈ 3.14159   ±  ×  ÷   ✓");

    // Also show how the same text looks under a different builtin font.
    if (y + 26 < h) {
        body.setFont(&fonts::Font2);
        body.setTextColor(TFT_WHITE, TFT_BLACK);
        body.drawString("Font2 sample: ←↑→↓ ✓", 6, y + 20);
    }
}
