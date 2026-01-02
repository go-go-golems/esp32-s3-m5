#include "demo_d2_text_wrap.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

static std::string fit_ellipsis(lgfx::v1::LovyanGFX &g, const std::string &s, int max_w) {
    if (max_w <= 0) return std::string();
    if (g.textWidth(s.c_str()) <= max_w) return s;

    const char *ell = "...";
    const int ell_w = g.textWidth(ell);
    std::string out = s;
    while (!out.empty() && (g.textWidth(out.c_str()) + ell_w) > max_w) {
        out.pop_back();
    }
    out += ell;
    return out;
}

static std::vector<std::string> wrap_hard(lgfx::v1::LovyanGFX &g, std::string s, int max_w) {
    std::vector<std::string> out;
    if (max_w <= 0) {
        out.push_back(std::string());
        return out;
    }

    while (!s.empty()) {
        size_t cut = s.size();
        while (cut > 0 && g.textWidth(s.substr(0, cut).c_str()) > max_w) {
            cut--;
        }
        if (cut == 0) cut = 1;
        out.push_back(s.substr(0, cut));
        s.erase(0, cut);
    }
    if (out.empty()) out.push_back(std::string());
    return out;
}

} // namespace

void demo_d2_text_wrap_render(M5Canvas &body) {
    const int w = body.width();
    const int h = body.height();

    body.fillRect(0, 0, w, h, TFT_BLACK);
    body.drawRect(0, 0, w, h, TFT_DARKGREY);

    body.setTextSize(1, 1);
    body.setTextDatum(lgfx::textdatum_t::top_left);
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    body.drawString("D2 Text: wrap + ellipsis", 6, 6);

    const std::string long_line =
        "This is a long line intended to demonstrate wrapping and ellipsis on a small 240x135 display.";

    const int box_x = 6;
    const int box_w = w - 12;
    const int box_h = (h - 34) / 2;

    // Top box: hard wrap.
    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    body.drawString("Wrap (hard, byte-based):", 6, 20);
    body.drawRect(box_x, 32, box_w, box_h, TFT_DARKGREY);

    int y = 34;
    body.setTextColor(TFT_WHITE, TFT_BLACK);
    for (const auto &seg : wrap_hard(body, long_line, box_w - 6)) {
        if (y + body.fontHeight() >= 32 + box_h) break;
        body.drawString(seg.c_str(), box_x + 3, y);
        y += body.fontHeight() + 1;
    }

    // Bottom box: ellipsis.
    const int box2_y = 32 + box_h + 6;
    body.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    body.drawString("Ellipsis (single line):", 6, box2_y - 12);
    body.drawRect(box_x, box2_y, box_w, h - box2_y - 2, TFT_DARKGREY);

    body.setTextColor(TFT_WHITE, TFT_BLACK);
    std::string fitted = fit_ellipsis(body, long_line, box_w - 6);
    body.drawString(fitted.c_str(), box_x + 3, box2_y + 2);
}

