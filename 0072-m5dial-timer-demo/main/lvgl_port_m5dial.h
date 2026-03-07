#pragma once

#include "lvgl.h"
#include "m5dial_board.h"

namespace tutorial_0072 {

struct LvglPortM5DialConfig {
  int buffer_lines = 40;
  int tick_ms = 2;
  bool double_buffer = false;
  bool swap_bytes = false;
};

bool lvgl_port_m5dial_init(LGFX_M5Dial &display, const LvglPortM5DialConfig &cfg);

}  // namespace tutorial_0072
