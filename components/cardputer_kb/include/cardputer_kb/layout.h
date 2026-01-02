#pragma once

#include <stdint.h>

namespace cardputer_kb {

static constexpr int kRows = 4;
static constexpr int kCols = 14;

static inline uint8_t keynum_from_xy(int x, int y) {
    if (x < 0 || x >= kCols || y < 0 || y >= kRows) {
        return 0;
    }
    return (uint8_t)((y * kCols) + (x + 1));
}

static inline void xy_from_keynum(uint8_t keynum, int *out_x, int *out_y) {
    if (out_x) *out_x = -1;
    if (out_y) *out_y = -1;
    if (keynum < 1 || keynum > (kRows * kCols)) {
        return;
    }
    uint8_t idx = (uint8_t)(keynum - 1);
    if (out_x) *out_x = idx % kCols;
    if (out_y) *out_y = idx / kCols;
}

// Vendor legend table (Cardputer “picture” coordinate system; 4 rows × 14 cols).
// This is for UI/debug labeling only, not for decoding semantics.
static constexpr const char *kKeyLegend[kRows][kCols] = {
    {"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "del"},
    {"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\"},
    {"fn", "shift", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "enter"},
    {"ctrl", "opt", "alt", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "space"},
};

static inline const char *legend_for_xy(int x, int y) {
    if (x < 0 || x >= kCols || y < 0 || y >= kRows) {
        return "";
    }
    return kKeyLegend[y][x];
}

} // namespace cardputer_kb

