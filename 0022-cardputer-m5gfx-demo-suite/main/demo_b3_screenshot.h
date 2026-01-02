#pragma once

#include <stdint.h>

#include "M5GFX.h"

struct ScreenshotDemoState {
    bool has_result = false;
    bool last_ok = false;
    size_t last_len = 0;
    uint32_t count = 0;
    int64_t last_sent_us = 0;
};

void demo_b3_screenshot_render(M5Canvas &body, const ScreenshotDemoState &s);
