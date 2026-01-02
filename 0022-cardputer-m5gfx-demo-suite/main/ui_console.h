#pragma once

#include <stddef.h>

#include <deque>
#include <string>
#include <vector>

#include "lgfx/v1/LGFXBase.hpp"

struct ConsoleState {
    std::deque<std::string> lines{};
    std::string input{};
    size_t dropped_lines = 0;
    int scrollback = 0; // 0=follow tail; >0 = user scrolled up by N lines
    bool dirty = true;
};

void console_init(ConsoleState &s);
void console_append_line_wrapped(ConsoleState &s, lgfx::v1::LovyanGFX &g, const std::string &line, int max_w,
                                 size_t max_lines);
void console_trim(ConsoleState &s, size_t max_lines);
void console_render(lgfx::v1::LovyanGFX &g, ConsoleState &s);

