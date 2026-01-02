#pragma once

#include <string>
#include <vector>

#include "lgfx/v1/LGFXBase.hpp"

enum class NavKey {
    Up,
    Down,
    PageUp,
    PageDown,
    Enter,
};

struct ListViewStyle {
    int padding_x = 6;
    int padding_y = 6;
    int row_gap = 2;
    uint32_t bg = 0x0000;      // TFT_BLACK
    uint32_t fg = 0xFFFF;      // TFT_WHITE
    uint32_t sel_bg = 0x001F;  // TFT_BLUE (RGB565)
    uint32_t sel_fg = 0xFFFF;  // TFT_WHITE
    uint32_t border = 0x7BEF;  // TFT_DARKGREY (RGB565)
};

class ListView {
public:
    void set_items(std::vector<std::string> items);
    void set_bounds(int x, int y, int w, int h);
    void set_style(ListViewStyle style);

    int selected_index() const;
    void set_selected_index(int idx);

    bool on_key(NavKey key);
    void render(lgfx::v1::LovyanGFX &g);

private:
    void clamp_selection();
    void ensure_selection_visible();
    std::string fit_label(lgfx::v1::LovyanGFX &g, const std::string &s, int max_w) const;

    std::vector<std::string> items_{};
    ListViewStyle style_{};

    int x_ = 0;
    int y_ = 0;
    int w_ = 0;
    int h_ = 0;

    int selected_ = 0;
    int scroll_top_ = 0;

    int line_h_ = 16;
    int visible_rows_ = 1;
};
