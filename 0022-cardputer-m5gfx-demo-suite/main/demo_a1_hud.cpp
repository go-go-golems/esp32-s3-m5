#include "demo_a1_hud.h"

#include <inttypes.h>
#include <stdio.h>

void demo_a1_hud_render(M5Canvas &body, const HudState &hud, bool hud_enabled, bool perf_enabled) {
    body.fillRect(0, 0, body.width(), body.height(), TFT_BLACK);
    body.drawRect(0, 0, body.width(), body.height(), TFT_DARKGREY);

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);

    int y = 6;
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("A1 HUD overlay (body)", 6, y);
    y += 16;

    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    body.drawString("H: toggle HUD   F: toggle perf", 6, y);
    y += 14;
    body.drawString("Tab: next scene   Shift+Tab: prev", 6, y);
    y += 18;

    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("Current state:", 6, y);
    y += 14;

    char buf[96];
    snprintf(buf, sizeof(buf), "HUD enabled: %s", hud_enabled ? "yes" : "no");
    body.drawString(buf, 12, y);
    y += 12;
    snprintf(buf, sizeof(buf), "Perf enabled: %s", perf_enabled ? "yes" : "no");
    body.drawString(buf, 12, y);
    y += 16;

    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    snprintf(buf, sizeof(buf), "screen: %s", hud.screen_name.c_str());
    body.drawString(buf, 12, y);
    y += 12;
    snprintf(buf, sizeof(buf), "fps: %d   frame: %dms", hud.fps, hud.frame_ms);
    body.drawString(buf, 12, y);
    y += 12;
    snprintf(buf, sizeof(buf), "heap: %" PRIu32 "   dma: %" PRIu32, hud.heap_free, hud.dma_free);
    body.drawString(buf, 12, y);
}
