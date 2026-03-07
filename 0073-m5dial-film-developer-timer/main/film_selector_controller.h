#pragma once

#include "film_selector_screen.h"
#include "input_events.h"
#include "recipe_selector_model.h"

namespace tutorial_0073 {

enum class FilmSelectorCommand {
  kNone = 0,
  kStartProcess,
};

class FilmSelectorController {
 public:
  FilmSelectorCommand handle_event(const tutorial_0072::InputEvent &event,
                                   RecipeSelectorModel &model,
                                   FilmSelectorScreen &screen);
};

}  // namespace tutorial_0073
