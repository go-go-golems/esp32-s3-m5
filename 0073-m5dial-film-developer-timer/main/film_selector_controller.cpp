#include "film_selector_controller.h"

namespace tutorial_0073 {

void FilmSelectorController::handle_event(const tutorial_0072::InputEvent &event,
                                          RecipeSelectorModel &model,
                                          FilmSelectorScreen &screen) {
  switch (event.type) {
    case tutorial_0072::InputEventType::kEncoderDelta:
      if (event.value != 0) {
        model.adjust(event.value);
      }
      break;
    case tutorial_0072::InputEventType::kButtonShortPress:
      model.confirm();
      break;
    case tutorial_0072::InputEventType::kButtonLongPress:
      model.back();
      break;
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
}

}  // namespace tutorial_0073
