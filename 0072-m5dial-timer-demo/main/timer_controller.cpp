#include "timer_controller.h"

namespace tutorial_0072 {

void TimerController::update(M5DialBoard &board, TimerModel &model, TimerScreen &screen, uint64_t now_us) {
  const int encoder_delta = board.take_encoder_steps();
  if (encoder_delta != 0) {
    model.adjust_by_detents(encoder_delta);
  }

  switch (board.take_swipe()) {
    case SwipeDirection::kLeft:
    case SwipeDirection::kUp:
      screen.cycle_theme(1);
      break;
    case SwipeDirection::kRight:
    case SwipeDirection::kDown:
      screen.cycle_theme(-1);
      break;
    case SwipeDirection::kNone:
      break;
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
