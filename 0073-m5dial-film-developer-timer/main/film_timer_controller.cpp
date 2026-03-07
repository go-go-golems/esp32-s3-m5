#include "film_timer_controller.h"

#include "film_process_screen.h"

namespace tutorial_0073 {

FilmTimerCommand FilmTimerController::handle_event(const tutorial_0072::InputEvent &event,
                                                   FilmTimerModel &model,
                                                   FilmProcessScreen &screen) {
  switch (event.type) {
    case tutorial_0072::InputEventType::kEncoderDelta:
      break;
    case tutorial_0072::InputEventType::kButtonShortPress:
      model.toggle_run_pause(event.timestamp_us);
      break;
    case tutorial_0072::InputEventType::kButtonLongPress: {
      const FilmTimerSnapshot snapshot = model.snapshot();
      if (snapshot.state == FilmTimerState::kRunning || snapshot.state == FilmTimerState::kPaused) {
        model.reset();
      } else {
        model.reset();
        return FilmTimerCommand::kReturnToSelection;
      }
      break;
    }
    case tutorial_0072::InputEventType::kSwipe:
      switch (event.swipe) {
        case tutorial_0072::SwipeDirection::kLeft:
        case tutorial_0072::SwipeDirection::kUp:
          screen.cycle_theme(1);
          break;
        case tutorial_0072::SwipeDirection::kRight:
        case tutorial_0072::SwipeDirection::kDown:
          screen.cycle_theme(-1);
          break;
        case tutorial_0072::SwipeDirection::kNone:
          break;
      }
      break;
  }

  return FilmTimerCommand::kNone;
}

}  // namespace tutorial_0073
