#pragma once

#include <string>
#include <vector>

#include "esp_err.h"

#include "cardputer_kb/bindings.h"
#include "cardputer_kb/scanner.h"

struct KeyEvent {
    std::string key;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
    bool fn = false;
};

class CardputerKeyboard {
public:
    esp_err_t init();
    std::vector<KeyEvent> poll();

private:
    bool inited_ = false;

    cardputer_kb::MatrixScanner scanner_{};
    std::vector<uint8_t> prev_pressed_keynums_{};
    bool prev_action_valid_ = false;
    cardputer_kb::Action prev_action_{};
};

