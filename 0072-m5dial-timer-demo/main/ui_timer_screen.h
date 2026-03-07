#pragma once

#include "lvgl.h"
#include "timer_model.h"

namespace tutorial_0072 {

class TimerScreen {
 public:
  bool init();
  void apply(const TimerSnapshot &snapshot);
  void cycle_theme(int delta);

 private:
  int theme_index_ = 0;
  lv_obj_t *root_ = nullptr;
  lv_obj_t *orb_ = nullptr;
  lv_obj_t *arc_ = nullptr;
  lv_obj_t *title_label_ = nullptr;
  lv_obj_t *status_label_ = nullptr;
  lv_obj_t *time_label_ = nullptr;
  lv_obj_t *duration_label_ = nullptr;
  lv_obj_t *step_label_ = nullptr;
  lv_obj_t *hint_label_ = nullptr;
};

}  // namespace tutorial_0072
