#include "ui_console.h"

#include <algorithm>

void console_init(ConsoleState &s) {
    if (!s.lines.empty()) {
        return;
    }
    s.lines.push_back("Cardputer demo-suite terminal (E1)");
    s.lines.push_back("Type, Enter to send, Fn+W/S scroll, Del backspace, Fn+Del back.");
    s.lines.push_back("");
    s.dirty = true;
}

static std::vector<std::string> wrap_line(lgfx::v1::LovyanGFX &g, std::string s, int max_w) {
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
        if (cut == 0) {
            cut = 1;
        }
        out.push_back(s.substr(0, cut));
        s.erase(0, cut);
    }
    if (out.empty()) {
        out.push_back(std::string());
    }
    return out;
}

void console_append_line_wrapped(ConsoleState &s, lgfx::v1::LovyanGFX &g, const std::string &line, int max_w,
                                 size_t max_lines) {
    for (const auto &seg : wrap_line(g, line, max_w)) {
        s.lines.push_back(seg);
    }
    console_trim(s, max_lines);
    s.dirty = true;
}

void console_trim(ConsoleState &s, size_t max_lines) {
    while (s.lines.size() > max_lines) {
        s.lines.pop_front();
        s.dropped_lines++;
    }
    if (s.lines.empty()) {
        s.lines.push_back(std::string());
    }
}

void console_render(lgfx::v1::LovyanGFX &g, ConsoleState &s) {
    if (!s.dirty) {
        return;
    }

    g.fillRect(0, 0, g.width(), g.height(), TFT_BLACK);
    g.setTextSize(1, 1);
    g.setTextDatum(lgfx::textdatum_t::top_left);

    int line_h = g.fontHeight() + 1;
    int input_h = line_h + 2;
    int output_h = (int)g.height() - input_h;
    int output_rows = (line_h > 0) ? (output_h / line_h) : 0;

    int total = (int)s.lines.size();
    int tail_start = std::max(0, total - output_rows);
    int start = std::max(0, tail_start - s.scrollback);

    g.setTextColor(TFT_GREEN, TFT_BLACK);
    for (int i = 0; i < output_rows; i++) {
        int idx = start + i;
        if (idx >= total) {
            break;
        }
        g.drawString(s.lines[(size_t)idx].c_str(), 2, i * line_h);
    }

    g.drawFastHLine(0, output_h, g.width(), TFT_DARKGREY);

    g.setTextColor(TFT_WHITE, TFT_BLACK);
    g.drawString("> ", 2, output_h + 1);
    g.drawString(s.input.c_str(), 18, output_h + 1);

    int cursor_x = 18 + g.textWidth(s.input.c_str());
    g.drawFastHLine(cursor_x, output_h + line_h, 8, TFT_WHITE);

    s.dirty = false;
}

