#pragma once

#include <string>
#include <vector>

#include "esp_err.h"

struct KeyEvent {
    std::string key;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool fn = false;
};

class CardputerKeyboard {
public:
    struct KeyPos {
        int x = 0;
        int y = 0;
    };

    esp_err_t init();
    std::vector<KeyEvent> poll();

private:
    bool inited_ = false;
    bool allow_autodetect_in01_ = true;
    bool use_alt_in01_ = false;

    std::vector<KeyPos> prev_down_{};
};
