#include "demo_b3_screenshot.h"

#include <inttypes.h>
#include <stdio.h>

void demo_b3_screenshot_render(M5Canvas &body, const ScreenshotDemoState &s) {
    body.fillRect(0, 0, body.width(), body.height(), TFT_BLACK);
    body.drawRect(0, 0, body.width(), body.height(), TFT_DARKGREY);

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);

    int y = 6;
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("B3 Screenshot to serial", 6, y);
    y += 16;

    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    body.drawString("Press P (any scene) or Enter (this scene)", 6, y);
    y += 12;
    body.drawString("Host: tools/capture_screenshot_png.py <port> out.png", 6, y);
    y += 18;

    body.setTextColor(TFT_DARKGREY, TFT_BLACK);
    body.drawString("Protocol: PNG_BEGIN <len> + raw bytes + PNG_END", 6, y);
    y += 18;

    body.setTextColor(TFT_WHITE, TFT_BLACK);
    if (!s.has_result) {
        body.drawString("Last send: (none yet)", 6, y);
        return;
    }

    char buf[96];
    snprintf(buf, sizeof(buf), "Last send: %s  len=%u  count=%" PRIu32, s.last_ok ? "OK" : "FAIL",
             (unsigned)s.last_len, s.count);
    body.drawString(buf, 6, y);
    y += 12;
    snprintf(buf, sizeof(buf), "last_sent_us=%" PRIi64, s.last_sent_us);
    body.setTextColor(TFT_DARKGREY, TFT_BLACK);
    body.drawString(buf, 6, y);
}

