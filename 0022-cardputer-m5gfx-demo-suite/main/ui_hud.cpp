#include "ui_hud.h"

#include <stdio.h>

void PerfTracker::on_frame_present(FrameTimingsUs t) {
    snapshot_.last = t;

    constexpr uint8_t kShift = 3; // 1/8 EMA
    if (!has_avg_) {
        snapshot_.avg_total_us = t.total_us;
        snapshot_.avg_render_us = t.render_us;
        snapshot_.avg_present_us = t.present_us;
        has_avg_ = true;
        return;
    }

    snapshot_.avg_total_us = ema_u32(snapshot_.avg_total_us, t.total_us, kShift);
    snapshot_.avg_render_us = ema_u32(snapshot_.avg_render_us, t.render_us, kShift);
    snapshot_.avg_present_us = ema_u32(snapshot_.avg_present_us, t.present_us, kShift);
}

void PerfTracker::sample_memory(uint32_t heap_free, uint32_t dma_free) {
    snapshot_.heap_free = heap_free;
    snapshot_.dma_free = dma_free;
}

const PerfSnapshot &PerfTracker::snapshot() const {
    return snapshot_;
}

uint32_t PerfTracker::ema_u32(uint32_t prev, uint32_t sample, uint8_t shift) {
    int32_t diff = (int32_t)sample - (int32_t)prev;
    return (uint32_t)((int32_t)prev + (diff >> shift));
}

void draw_header_bar(lgfx::v1::LovyanGFX &g, const HudState &s, uint32_t bg, uint32_t fg) {
    g.fillRect(0, 0, g.width(), g.height(), bg);
    g.drawFastHLine(0, g.height() - 1, g.width(), 0x0000);

    g.setTextSize(1, 1);
    g.setTextDatum(lgfx::textdatum_t::middle_left);
    g.setTextColor(fg, bg);
    g.drawString(s.screen_name.c_str(), 4, g.height() / 2);

    char buf[96];
    snprintf(buf, sizeof(buf), "fps:%d H:%uk D:%uk", s.fps, (unsigned)(s.heap_free / 1024),
             (unsigned)(s.dma_free / 1024));

    g.setTextDatum(lgfx::textdatum_t::middle_right);
    g.drawString(buf, g.width() - 4, g.height() / 2);
}

void draw_perf_bar(lgfx::v1::LovyanGFX &g, const PerfSnapshot &p, uint32_t bg, uint32_t fg) {
    g.fillRect(0, 0, g.width(), g.height(), bg);
    g.drawFastHLine(0, 0, g.width(), 0x7BEF);

    g.setTextSize(1, 1);
    g.setTextDatum(lgfx::textdatum_t::middle_left);
    g.setTextColor(fg, bg);

    int fps = (p.avg_total_us > 0) ? (int)(1000000 / p.avg_total_us) : 0;

    char buf[128];
    snprintf(buf, sizeof(buf), "fps:%d total:%ums render:%ums pres:%ums", fps, (unsigned)(p.avg_total_us / 1000),
             (unsigned)(p.avg_render_us / 1000), (unsigned)(p.avg_present_us / 1000));
    g.drawString(buf, 4, g.height() / 2);
}

