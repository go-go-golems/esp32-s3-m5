#include "timer_controller.h"

namespace tutorial_0072 {

void TimerController::handle_event(const InputEvent &event, TimerModel &model, TimerScreen &screen) {
  switch (event.type) {
    case InputEventType::kEncoderDelta:
      if (event.value != 0) {
        model.adjust_by_detents(event.value);
      }
      break;
    case InputEventType::kButtonShortPress:
      model.toggle_run_pause(event.timestamp_us);
      break;
    case InputEventType::kButtonLongPress:
      model.reset();
      break;
    case InputEventType::kSwipe:
      switch (event.swipe) {
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
      break;
  }
}

}  // namespace tutorial_0072
