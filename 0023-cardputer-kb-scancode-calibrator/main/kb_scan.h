#pragma once

#include <stdint.h>

#include <vector>

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

class KbScanner {
  public:
    void init();
    ScanSnapshot scan();

  private:
    bool use_alt_in01_ = false;
};

