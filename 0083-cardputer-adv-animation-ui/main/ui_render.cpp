#include "ui_render.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>

namespace {

static constexpr uint16_t kBg = 0x0841;
static constexpr uint16_t kPanel = 0x18C3;
static constexpr uint16_t kInk = 0xFFFF;
static constexpr uint16_t kMuted = 0xAD55;
static constexpr uint16_t kAccent = 0xFF64;
static constexpr uint16_t kAccent2 = 0x867F;
static constexpr uint16_t kTrack = 0x2104;
static constexpr uint16_t kViewport = 0x2945;

static bool is_active_marker(int idx, int count)
{
    if (idx == 0 || idx == count - 1) return true;
    const float step = (float)count / (float)(count / 8 + 1);
    const float rem = fmodf((float)idx, step);
    return fabsf(rem) < 0.5f || fabsf(rem - step) < 0.5f;
}

static const char *mode_name(const UiState *ui)
{
    return ui->mode == UiMode::Help ? "HELP" : (ui->autoplay ? "AUTO" : "BROWSE");
}

static const char *backend_name(const ui_kb_debug_state_t *dbg)
{
    if (!dbg || !dbg->ready) return "init";
    return dbg->backend == 1 ? "TCA8418" : "GPIO";
}

} // namespace

void ui_render_frame(M5Canvas *canvas, const UiState *ui, const ui_kb_debug_state_t *kb_dbg)
{
    if (!canvas || !ui) return;

    const int w = canvas->width();
    const int h = canvas->height();
    const int pad = 8;
    const int header_h = 18;
    const int minimap_h = 42;
    const int scrollbar_h = 14;
    const int footer_h = 16;
    const int content_y = header_h + minimap_h + scrollbar_h + 12;
    const int content_h = std::max(24, h - content_y - footer_h - 6);
    const int track_x = pad + 4;
    const int track_w = w - 2 * track_x;

    canvas->fillScreen(kBg);
    canvas->setTextDatum(lgfx::textdatum_t::top_left);
    canvas->setTextSize(1, 1);
    canvas->setTextColor(kInk, kBg);

    canvas->fillRoundRect(pad, 4, w - 2 * pad, header_h, 4, kPanel);
    canvas->drawString("0083 ADV Animation UI", pad + 6, 8);

    char hdr[96];
    snprintf(hdr,
             sizeof(hdr),
             "%s  pos=%03d%%  idx=%02d  kb=%s",
             mode_name(ui),
             (ui->scroll.max_px > 0.0f) ? (int)lroundf((ui->scroll.pos_px / ui->scroll.max_px) * 100.0f) : 0,
             ui->active_index + 1,
             backend_name(kb_dbg));
    canvas->setTextColor(kAccent, kPanel);
    canvas->drawRightString(hdr, w - pad - 6, 8);

    canvas->setTextColor(kInk, kBg);
    canvas->fillRoundRect(pad, header_h + 8, w - 2 * pad, minimap_h, 6, kPanel);
    canvas->drawString("POSITION INDICATOR", pad + 6, header_h + 12);

    const int mini_x = track_x;
    const int mini_y = header_h + 28;
    const int mini_w = track_w;
    const int mini_base_y = mini_y + 20;
    const float bar_pitch = (float)mini_w / (float)ui->line_count;
    const float scroll_pct = (ui->scroll.max_px > 0.0f) ? (ui->scroll.pos_px / ui->scroll.max_px) : 0.0f;
    const float scroll_head_x = (float)mini_x + scroll_pct * (float)(mini_w - 1);

    for (int i = 0; i < ui->line_count; i++) {
        const float cx = (float)mini_x + ((float)i + 0.5f) * bar_pitch;
        const float dist = fabsf(scroll_head_x - cx);
        const float dist_limit = 36.0f;
        float boost = 0.0f;
        if (dist < dist_limit) {
            const float norm = 1.0f - dist / dist_limit;
            boost = norm * norm * 14.0f;
        }
        const bool active = is_active_marker(i, ui->line_count);
        const int bar_h = std::min(28, (int)lroundf((active ? 12.0f : 7.0f) + boost));
        const int bar_w = std::max(2, (int)floorf(bar_pitch) - 1);
        const int x = (int)lroundf(cx - (float)bar_w * 0.5f);
        const int y = mini_base_y - bar_h;
        const uint16_t col = (i == ui->active_index) ? kAccent : (active ? kInk : kMuted);
        canvas->fillRect(x, y, bar_w, bar_h, col);
    }

    const int head_x = mini_x + (int)lroundf(scroll_pct * (float)(mini_w - 4));
    canvas->fillRect(head_x, mini_y - 2, 3, 24, kAccent2);

    const int scroll_y = header_h + minimap_h + 16;
    canvas->fillRoundRect(pad, scroll_y, w - 2 * pad, scrollbar_h + 8, 6, kPanel);
    canvas->drawString("SCROLL", pad + 6, scroll_y + 2);
    const int bar_y = scroll_y + 16;
    const int btn_w = 16;
    const int thumb_w = 42;
    canvas->fillRect(mini_x, bar_y, mini_w, scrollbar_h, kTrack);
    canvas->drawRect(mini_x, bar_y, mini_w, scrollbar_h, kMuted);
    canvas->fillRect(mini_x, bar_y, btn_w, scrollbar_h, kPanel);
    canvas->fillRect(mini_x + mini_w - btn_w, bar_y, btn_w, scrollbar_h, kPanel);
    canvas->drawString("<", mini_x + 5, bar_y + 3);
    canvas->drawString(">", mini_x + mini_w - 11, bar_y + 3);
    const int thumb_x = mini_x + btn_w + (int)lroundf(scroll_pct * (float)(std::max(1, mini_w - 2 * btn_w - thumb_w)));
    canvas->fillRoundRect(thumb_x, bar_y + 1, thumb_w, scrollbar_h - 2, 3, kAccent);

    const int body_x = pad;
    const int body_w = w - 2 * pad;
    canvas->fillRoundRect(body_x, content_y, body_w, content_h, 6, kPanel);
    canvas->drawString("VIEWPORT", body_x + 6, content_y + 4);

    const int viewport_x = body_x + 6;
    const int viewport_y = content_y + 18;
    const int viewport_w = body_w - 12;
    const int viewport_h = content_h - 24;
    canvas->fillRect(viewport_x, viewport_y, viewport_w, viewport_h, kViewport);
    canvas->drawRect(viewport_x, viewport_y, viewport_w, viewport_h, kMuted);

    const float world_left = ui->scroll.pos_px;
    const int center_x = viewport_x + viewport_w / 2;
    canvas->drawFastVLine(center_x, viewport_y, viewport_h, kAccent2);

    for (int i = 0; i < ui->line_count; i++) {
        const float world_x = (float)i * ui->line_step_px;
        const int sx = viewport_x + (int)lroundf(world_x - world_left);
        if (sx < viewport_x - 24 || sx > viewport_x + viewport_w + 24) continue;

        const bool active = is_active_marker(i, ui->line_count);
        const int item_h = active ? viewport_h - 18 : viewport_h - 30;
        const int item_y = viewport_y + viewport_h - item_h - 6;
        const uint16_t col = (i == ui->active_index) ? kAccent : (active ? kInk : kMuted);
        canvas->fillRoundRect(sx - 6, item_y, 12, item_h, 3, col);

        char label[16];
        snprintf(label, sizeof(label), "%02d", i + 1);
        canvas->setTextColor(kBg, col);
        canvas->drawCentreString(label, sx, item_y + 4);
    }

    canvas->setTextColor(kInk, kBg);
    char footer[128];
    snprintf(footer,
             sizeof(footer),
             "L/R move  U/D jump  Enter autoplay  Tab help  Del reset  easing=%.2f",
             (double)ui->scroll.easing);
    canvas->drawString(footer, pad, h - footer_h);

    if (ui->mode == UiMode::Help) {
        const int ov_w = w - 28;
        const int ov_h = 72;
        const int ov_x = 14;
        const int ov_y = (h - ov_h) / 2;
        canvas->fillRoundRect(ov_x, ov_y, ov_w, ov_h, 6, kBg);
        canvas->drawRoundRect(ov_x, ov_y, ov_w, ov_h, 6, kAccent);
        canvas->drawString("HELP", ov_x + 8, ov_y + 8);
        canvas->drawString("Fn+, / Fn+/ : left/right", ov_x + 8, ov_y + 24);
        canvas->drawString("Fn+; / Fn+. : page jump", ov_x + 8, ov_y + 36);
        canvas->drawString("Enter: autoplay  Del: reset", ov_x + 8, ov_y + 48);
        canvas->drawString("Tab or Back: close", ov_x + 8, ov_y + 60);
    }
}
