#include "film_selector_controller.h"

namespace tutorial_0073 {

FilmSelectorCommand FilmSelectorController::handle_event(const tutorial_0072::InputEvent &event,
                                                         RecipeSelectorModel &model,
                                                         FilmSelectorScreen &screen) {
  const SelectorSnapshot snapshot = model.snapshot();

  switch (event.type) {
    case tutorial_0072::InputEventType::kEncoderDelta:
      if (event.value != 0 && snapshot.field != SelectorField::kReview) {
        model.adjust(event.value);
      }
      break;
    case tutorial_0072::InputEventType::kButtonShortPress:
      if (snapshot.field == SelectorField::kReview) {
        return FilmSelectorCommand::kStartProcess;
      }
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

  return FilmSelectorCommand::kNone;
}

}  // namespace tutorial_0073
