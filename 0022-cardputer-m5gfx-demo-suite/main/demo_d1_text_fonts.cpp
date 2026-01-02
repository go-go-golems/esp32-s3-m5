#include "demo_d1_text_fonts.h"

#include <stdio.h>

#include "lgfx/v1/lgfx_fonts.hpp"

namespace {

static void draw_font_sample(M5Canvas &g, int x, int y, const char *name, const lgfx::IFont *font,
                             const char *sample, int max_w) {
    g.setFont(&fonts::Font0);
    g.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    g.drawString(name, x, y);
    y += 12;

    g.setFont(font);
    g.setTextColor(TFT_WHITE, TFT_BLACK);
    g.drawString(sample, x, y);

    g.setFont(&fonts::Font0);
    const int tw = g.textWidth(sample);
    char buf[64];
    snprintf(buf, sizeof(buf), "textWidth=%d  fontH=%d", tw, (int)g.fontHeight());
    g.setTextColor(TFT_DARKGREY, TFT_BLACK);
    g.drawString(buf, x + max_w / 2, y);
}

} // namespace

void demo_d1_text_fonts_render(M5Canvas &body) {
    const int w = body.width();
    const int h = body.height();

    body.fillRect(0, 0, w, h, TFT_BLACK);
    body.drawRect(0, 0, w, h, TFT_DARKGREY);

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);

    body.setFont(&fonts::Font0);
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("D1 Text: builtin fonts + metrics", 6, 6);
    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    body.drawString("Tab: next   Shift+Tab: prev   Esc/Del: menu", 6, 20);

    const int x = 6;
    const int max_w = w - 12;
    int y = 36;

    // Keep the selection tight to fit within the Cardputer body height.
    draw_font_sample(body, x, y, "Font0 (GLCD)", &fonts::Font0, "Hello, Cardputer!", max_w);
    y += 24;
    draw_font_sample(body, x, y, "Font2", &fonts::Font2, "Hello, Cardputer!", max_w);
    y += 24;
    draw_font_sample(body, x, y, "Font4", &fonts::Font4, "Hello, Cardputer!", max_w);
}
