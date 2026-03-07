#pragma once

#include <stdint.h>

#include "m5dial_board.h"
#include "timer_model.h"

namespace tutorial_0072 {

class TimerController {
 public:
  void update(M5DialBoard &board, TimerModel &model, uint64_t now_us);
};

}  // namespace tutorial_0072
