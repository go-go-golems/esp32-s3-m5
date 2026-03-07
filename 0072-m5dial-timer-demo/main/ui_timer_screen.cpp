#include "ui_timer_screen.h"

#include <stdio.h>

namespace tutorial_0072 {

namespace {

lv_color_t color_from_hex(uint32_t rgb) {
  return lv_color_make((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

const char *state_text(TimerState state) {
  switch (state) {
    case TimerState::kIdle:
      return "READY";
    case TimerState::kRunning:
      return "RUNNING";
    case TimerState::kPaused:
      return "PAUSED";
    case TimerState::kComplete:
      return "DONE";
  }
  return "READY";
}

lv_color_t accent_color(TimerState state) {
  switch (state) {
    case TimerState::kIdle:
      return color_from_hex(0xE6B04D);
    case TimerState::kRunning:
      return color_from_hex(0x5FD1C2);
    case TimerState::kPaused:
      return color_from_hex(0xE6B04D);
    case TimerState::kComplete:
      return color_from_hex(0xF16B5A);
  }
  return color_from_hex(0xE6B04D);
}

void format_mmss(uint32_t total_ms, char *out, size_t out_size) {
  const uint32_t total_sec = total_ms / 1000U;
  const uint32_t minutes = total_sec / 60U;
  const uint32_t seconds = total_sec % 60U;
  snprintf(out, out_size, "%02" PRIu32 ":%02" PRIu32, minutes, seconds);
}

}  // namespace

bool TimerScreen::init() {
  root_ = lv_obj_create(nullptr);
  if (!root_) {
    return false;
  }

  lv_obj_remove_style_all(root_);
  lv_obj_set_size(root_, 240, 240);
  lv_obj_set_style_bg_color(root_, color_from_hex(0x0F1115), 0);
  lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(root_, 0, 0);

  orb_ = lv_obj_create(root_);
  lv_obj_remove_style_all(orb_);
  lv_obj_set_size(orb_, 178, 178);
  lv_obj_align(orb_, LV_ALIGN_CENTER, 0, 4);
  lv_obj_set_style_radius(orb_, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(orb_, color_from_hex(0x151A21), 0);
  lv_obj_set_style_bg_opa(orb_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(orb_, 2, 0);
  lv_obj_set_style_border_color(orb_, color_from_hex(0x242C36), 0);
  lv_obj_set_style_shadow_width(orb_, 24, 0);
  lv_obj_set_style_shadow_color(orb_, color_from_hex(0x090B0D), 0);
  lv_obj_set_style_shadow_opa(orb_, LV_OPA_40, 0);

  arc_ = lv_arc_create(root_);
  lv_obj_set_size(arc_, 208, 208);
  lv_obj_align(arc_, LV_ALIGN_CENTER, 0, 4);
  lv_arc_set_rotation(arc_, 270);
  lv_arc_set_bg_angles(arc_, 0, 360);
  lv_arc_set_mode(arc_, LV_ARC_MODE_NORMAL);
  lv_obj_remove_style(arc_, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(arc_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(arc_, 12, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc_, 12, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc_, color_from_hex(0x232A32), LV_PART_MAIN);

  title_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(title_label_, color_from_hex(0xF4EEE6), 0);
  lv_label_set_text(title_label_, "M5DIAL TIMER");
  lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 18);

  status_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(status_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, -28);

  time_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(time_label_, color_from_hex(0xF4EEE6), 0);
  lv_obj_align(time_label_, LV_ALIGN_CENTER, 0, 10);

  duration_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(duration_label_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(duration_label_, color_from_hex(0x8E989F), 0);
  lv_obj_align(duration_label_, LV_ALIGN_CENTER, 0, 56);

  step_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(step_label_, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(step_label_, color_from_hex(0xD7C19A), 0);
  lv_obj_align(step_label_, LV_ALIGN_BOTTOM_MID, 0, -34);

  hint_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(hint_label_, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(hint_label_, color_from_hex(0x74808A), 0);
  lv_obj_set_width(hint_label_, 200);
  lv_label_set_long_mode(hint_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(hint_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_MID, 0, -10);

  lv_scr_load(root_);
  return true;
}

void TimerScreen::apply(const TimerSnapshot &snapshot) {
  if (!root_) {
    return;
  }

  const lv_color_t accent = accent_color(snapshot.state);
  lv_obj_set_style_border_color(orb_, accent, 0);
  lv_obj_set_style_arc_color(arc_, accent, LV_PART_INDICATOR);
  lv_obj_set_style_text_color(status_label_, accent, 0);

  lv_arc_set_range(arc_, 0, snapshot.duration_ms == 0 ? 1 : snapshot.duration_ms);
  lv_arc_set_value(arc_, static_cast<int32_t>(snapshot.remaining_ms));

  lv_label_set_text(status_label_, state_text(snapshot.state));

  char buffer[24];
  format_mmss(snapshot.remaining_ms, buffer, sizeof(buffer));
  lv_label_set_text(time_label_, buffer);

  format_mmss(snapshot.duration_ms, buffer, sizeof(buffer));
  char duration_line[32];
  snprintf(duration_line, sizeof(duration_line), "Set %s", buffer);
  lv_label_set_text(duration_label_, duration_line);

  char step_line[40];
  snprintf(step_line, sizeof(step_line), "Turn to adjust %lus", static_cast<unsigned long>(snapshot.adjustment_step_sec));
  lv_label_set_text(step_label_, step_line);

  const char *hint = "Press start or pause. Hold to reset.";
  if (snapshot.state == TimerState::kRunning) {
    hint = "Press to pause. Hold to reset.";
  } else if (snapshot.state == TimerState::kPaused) {
    hint = "Turn to retime. Press resume. Hold reset.";
  } else if (snapshot.state == TimerState::kComplete) {
    hint = "Press to run again. Hold to reset.";
  }
  lv_label_set_text(hint_label_, hint);
}

}  // namespace tutorial_0072
