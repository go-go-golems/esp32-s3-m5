#pragma once

#include "film_timer_model.h"
#include "lvgl.h"

namespace tutorial_0073 {

class FilmProcessScreen {
 public:
  bool init();
  void apply(const FilmTimerSnapshot &snapshot);
  void cycle_theme(int delta);

 private:
  int theme_index_ = 0;
  lv_obj_t *root_ = nullptr;
  lv_obj_t *orb_ = nullptr;
  lv_obj_t *arc_ = nullptr;
  lv_obj_t *title_label_ = nullptr;
  lv_obj_t *status_label_ = nullptr;
  lv_obj_t *time_label_ = nullptr;
  lv_obj_t *film_label_ = nullptr;
  lv_obj_t *process_label_ = nullptr;
  lv_obj_t *meta_label_ = nullptr;
  lv_obj_t *hint_label_ = nullptr;
};

}  // namespace tutorial_0073
