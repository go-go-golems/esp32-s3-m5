#pragma once

#include "film_selector_screen.h"
#include "film_timer_model.h"
#include "input_events.h"

namespace tutorial_0073 {

enum class FilmTimerCommand {
  kNone = 0,
  kReturnToSelection,
};

class FilmProcessScreen;

class FilmTimerController {
 public:
  FilmTimerCommand handle_event(const tutorial_0072::InputEvent &event, FilmTimerModel &model, FilmProcessScreen &screen);
};

}  // namespace tutorial_0073
