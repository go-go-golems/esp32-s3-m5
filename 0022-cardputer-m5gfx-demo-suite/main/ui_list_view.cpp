#include "ui_list_view.h"

#include <algorithm>

void ListView::set_items(std::vector<std::string> items) {
    items_ = std::move(items);
    clamp_selection();
    ensure_selection_visible();
}

void ListView::set_bounds(int x, int y, int w, int h) {
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;
    ensure_selection_visible();
}

void ListView::set_style(ListViewStyle style) {
    style_ = style;
}

int ListView::selected_index() const {
    return selected_;
}

void ListView::set_selected_index(int idx) {
    selected_ = idx;
    clamp_selection();
    ensure_selection_visible();
}

bool ListView::on_key(NavKey key) {
    if (items_.empty()) {
        return false;
    }

    int prev_selected = selected_;
    int prev_scroll_top = scroll_top_;

    switch (key) {
        case NavKey::Up:
            selected_ = std::max(0, selected_ - 1);
            break;
        case NavKey::Down:
            selected_ = std::min((int)items_.size() - 1, selected_ + 1);
            break;
        case NavKey::PageUp:
            selected_ = std::max(0, selected_ - visible_rows_);
            break;
        case NavKey::PageDown:
            selected_ = std::min((int)items_.size() - 1, selected_ + visible_rows_);
            break;
        case NavKey::Enter:
            break;
    }

    ensure_selection_visible();
    return selected_ != prev_selected || scroll_top_ != prev_scroll_top;
}

void ListView::render(lgfx::v1::LovyanGFX &g) {
    g.setTextDatum(lgfx::textdatum_t::top_left);
    g.setTextSize(1, 1);

    line_h_ = g.fontHeight() + style_.row_gap;
    visible_rows_ = std::max(1, (h_ - 2 * style_.padding_y) / std::max(1, line_h_));

    ensure_selection_visible();

    g.fillRect(x_, y_, w_, h_, style_.bg);
    g.drawRect(x_, y_, w_, h_, style_.border);

    int row_y = y_ + style_.padding_y;
    for (int i = 0; i < visible_rows_; i++) {
        int idx = scroll_top_ + i;
        if (idx >= (int)items_.size()) {
            break;
        }

        bool sel = (idx == selected_);
        if (sel) {
            g.fillRoundRect(x_ + 2, row_y, w_ - 4, line_h_ - 1, 4, style_.sel_bg);
            g.setTextColor(style_.sel_fg, style_.sel_bg);
        } else {
            g.setTextColor(style_.fg, style_.bg);
        }

        int max_w = std::max(0, w_ - 2 * style_.padding_x);
        auto label = fit_label(g, items_[idx], max_w);
        g.drawString(label.c_str(), x_ + style_.padding_x, row_y + 1);

        row_y += line_h_;
    }
}

void ListView::clamp_selection() {
    if (items_.empty()) {
        selected_ = 0;
        scroll_top_ = 0;
        return;
    }
    selected_ = std::max(0, std::min((int)items_.size() - 1, selected_));
}

void ListView::ensure_selection_visible() {
    clamp_selection();

    if (items_.empty()) {
        scroll_top_ = 0;
        return;
    }

    if (selected_ < scroll_top_) {
        scroll_top_ = selected_;
    }
    if (selected_ >= scroll_top_ + visible_rows_) {
        scroll_top_ = selected_ - visible_rows_ + 1;
    }

    int max_scroll_top = std::max(0, (int)items_.size() - visible_rows_);
    scroll_top_ = std::max(0, std::min(max_scroll_top, scroll_top_));
}

std::string ListView::fit_label(lgfx::v1::LovyanGFX &g, const std::string &s, int max_w) const {
    if (max_w <= 0) {
        return std::string();
    }
    if (g.textWidth(s.c_str()) <= max_w) {
        return s;
    }

    const char *ell = "...";
    int ell_w = g.textWidth(ell);
    std::string out = s;
    while (!out.empty() && g.textWidth(out.c_str()) + ell_w > max_w) {
        out.pop_back();
    }
    out += ell;
    return out;
}
