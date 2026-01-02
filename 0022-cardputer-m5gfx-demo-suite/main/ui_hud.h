#pragma once

#include <stdint.h>

#include <string>

#include "lgfx/v1/LGFXBase.hpp"

struct FrameTimingsUs {
    uint32_t update_us = 0;
    uint32_t render_us = 0;
    uint32_t present_us = 0;
    uint32_t total_us = 0;
};

struct PerfSnapshot {
    FrameTimingsUs last{};
    uint32_t heap_free = 0;
    uint32_t dma_free = 0;

    uint32_t avg_total_us = 0;
    uint32_t avg_render_us = 0;
    uint32_t avg_present_us = 0;
};

struct HudState {
    std::string screen_name{"Home"};
    uint32_t heap_free = 0;
    uint32_t dma_free = 0;
    int fps = 0;
    int frame_ms = 0;
};

class PerfTracker {
public:
    void on_frame_present(FrameTimingsUs t);
    void sample_memory(uint32_t heap_free, uint32_t dma_free);
    const PerfSnapshot &snapshot() const;

private:
    static uint32_t ema_u32(uint32_t prev, uint32_t sample, uint8_t shift);

    PerfSnapshot snapshot_{};
    bool has_avg_ = false;
};

void draw_header_bar(lgfx::v1::LovyanGFX &g, const HudState &s, uint32_t bg, uint32_t fg);
void draw_perf_bar(lgfx::v1::LovyanGFX &g, const PerfSnapshot &p, uint32_t bg, uint32_t fg);

