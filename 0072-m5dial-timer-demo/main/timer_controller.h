#pragma once

#include <stdint.h>

#include "input_events.h"
#include "timer_model.h"
#include "ui_timer_screen.h"

namespace tutorial_0072 {

class TimerController {
 public:
  void handle_event(const InputEvent &event, TimerModel &model, TimerScreen &screen);
};

}  // namespace tutorial_0072
