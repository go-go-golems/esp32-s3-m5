#pragma once

#include "lvgl.h"
#include "recipe_selector_model.h"

namespace tutorial_0073 {

class FilmSelectorScreen {
 public:
  bool init();
  void apply(const SelectorSnapshot &snapshot);
  void cycle_theme(int delta);

 private:
  int theme_index_ = 0;
  lv_obj_t *root_ = nullptr;
  lv_obj_t *orb_ = nullptr;
  lv_obj_t *accent_ring_ = nullptr;
  lv_obj_t *title_label_ = nullptr;
  lv_obj_t *field_label_ = nullptr;
  lv_obj_t *value_label_ = nullptr;
  lv_obj_t *detail_label_ = nullptr;
  lv_obj_t *summary_label_ = nullptr;
  lv_obj_t *hint_label_ = nullptr;
};

}  // namespace tutorial_0073
