#include "demo_b2_perf.h"

#include <algorithm>
#include <inttypes.h>
#include <stdio.h>

namespace {

static int clampi(int v, int lo, int hi) {
    return std::max(lo, std::min(v, hi));
}

static void draw_bar(M5Canvas &g, int x, int y, int w, int h, uint32_t fg, uint32_t bg, uint32_t value_us,
                     uint32_t max_us, const char *label) {
    g.fillRect(x, y, w, h, bg);
    g.drawRect(x, y, w, h, TFT_DARKGREY);
    int fill_w = 0;
    if (max_us > 0) {
        fill_w = (int)((uint64_t)value_us * (uint64_t)w / (uint64_t)max_us);
    }
    fill_w = clampi(fill_w, 0, w);
    g.fillRect(x, y, fill_w, h, fg);
    g.setTextColor(TFT_WHITE, bg);
    g.drawString(label, x + 3, y + 2);
}

} // namespace

void demo_b2_perf_render(M5Canvas &body, const PerfSnapshot &perf, bool perf_enabled) {
    const int w = body.width();
    const int h = body.height();

    body.fillRect(0, 0, w, h, TFT_BLACK);
    body.drawRect(0, 0, w, h, TFT_DARKGREY);

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);

    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("B2 Perf overlay (body)", 6, 6);

    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    body.drawString("F: toggle perf   Tab: next   Shift+Tab: prev", 6, 22);

    if (!perf_enabled) {
        body.setTextColor(TFT_DARKGREY, TFT_BLACK);
        body.drawString("(perf overlay disabled)", 6, 40);
        return;
    }

    uint32_t max_us = std::max<uint32_t>(1, std::max({perf.avg_total_us, perf.avg_render_us, perf.avg_present_us}));
    max_us = std::max<uint32_t>(max_us, perf.last.total_us);

    const int bar_x = 6;
    const int bar_w = w - 12;
    const int bar_h = 14;
    int y = 44;

    draw_bar(body, bar_x, y, bar_w, bar_h, TFT_DARKGREEN, TFT_BLACK, perf.avg_render_us, max_us, "avg render");
    y += bar_h + 6;
    draw_bar(body, bar_x, y, bar_w, bar_h, TFT_DARKCYAN, TFT_BLACK, perf.avg_present_us, max_us, "avg present");
    y += bar_h + 6;
    draw_bar(body, bar_x, y, bar_w, bar_h, TFT_MAGENTA, TFT_BLACK, perf.avg_total_us, max_us, "avg total");
    y += bar_h + 10;

    char buf[96];
    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    snprintf(buf, sizeof(buf), "last: upd=%" PRIu32 "us ren=%" PRIu32 "us pres=%" PRIu32 "us tot=%" PRIu32 "us",
             perf.last.update_us, perf.last.render_us, perf.last.present_us, perf.last.total_us);
    body.drawString(buf, 6, y);
    y += 14;
    snprintf(buf, sizeof(buf), "avg total=%" PRIu32 "us  heap=%" PRIu32 "  dma=%" PRIu32, perf.avg_total_us,
             perf.heap_free, perf.dma_free);
    body.drawString(buf, 6, y);
}
