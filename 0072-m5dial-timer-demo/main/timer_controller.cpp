#include "timer_controller.h"

namespace tutorial_0072 {

void TimerController::update(M5DialBoard &board, TimerModel &model, uint64_t now_us) {
  const int encoder_delta = board.take_encoder_steps();
  if (encoder_delta != 0) {
    model.adjust_by_detents(encoder_delta);
  }

  if (board.take_button_long_press()) {
    model.reset();
    return;
  }

  if (board.take_button_press()) {
    model.toggle_run_pause(now_us);
  }
}

}  // namespace tutorial_0072
