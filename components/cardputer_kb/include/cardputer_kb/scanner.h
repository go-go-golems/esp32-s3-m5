#pragma once

#include <stdint.h>

#include <vector>

namespace cardputer_kb {

struct KeyPos {
    int x; // 0..13
    int y; // 0..3

    bool operator==(const KeyPos &o) const { return x == o.x && y == o.y; }
};

struct ScanSnapshot {
    bool use_alt_in01 = false;
    std::vector<KeyPos> pressed;
    std::vector<uint8_t> pressed_keynums;
};

// Matrix scanner for Cardputer keyboard.
// Returns physical key positions and vendor-style keyNum values (1..56).
class MatrixScanner {
  public:
    void init();
    ScanSnapshot scan();

  private:
    bool use_alt_in01_ = false;
};

} // namespace cardputer_kb

