#include "ui_timer_screen.h"

#include <stdio.h>

namespace tutorial_0072 {

namespace {

constexpr uint32_t kLabelUpdateQuantumMs = 1000;
constexpr uint32_t kArcUpdateQuantumMs = 100;

struct ThemePalette {
  const char *name;
  uint32_t root_bg;
  uint32_t orb_bg;
  uint32_t orb_border_base;
  uint32_t arc_bg;
  uint32_t text_primary;
  uint32_t text_muted;
  uint32_t text_warm;
  uint32_t text_hint;
};

constexpr ThemePalette kThemes[] = {
    {"EMBER", 0x0F1115, 0x151A21, 0x242C36, 0x232A32, 0xF4EEE6, 0x8E989F, 0xD7C19A, 0x74808A},
    {"LAGOON", 0x08151A, 0x0F2128, 0x1E3A44, 0x153640, 0xE9F7F7, 0x8CB0B7, 0xA2D9D2, 0x71A2A9},
    {"SUNSET", 0x180C10, 0x28131B, 0x4A2330, 0x3A1B25, 0xF9F0EA, 0xC3A09B, 0xF0BE8C, 0xB68788},
    {"GROVE", 0x0A120D, 0x111E15, 0x25402D, 0x1A3322, 0xEEF8EE, 0x93B39A, 0xB9D8A3, 0x799A7B},
};

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

const ThemePalette &theme_palette(int index) {
  const int theme_count = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));
  int normalized = index % theme_count;
  if (normalized < 0) {
    normalized += theme_count;
  }
  return kThemes[normalized];
}

void format_mmss(uint32_t total_ms, char *out, size_t out_size) {
  const uint32_t total_sec = total_ms / 1000U;
  const uint32_t minutes = total_sec / 60U;
  const uint32_t seconds = total_sec % 60U;
  snprintf(out, out_size, "%02" PRIu32 ":%02" PRIu32, minutes, seconds);
}

uint32_t quantize_remaining_ms(uint32_t value_ms, uint32_t quantum_ms) {
  if (value_ms == 0 || quantum_ms == 0) {
    return value_ms;
  }
  return ((value_ms + quantum_ms - 1U) / quantum_ms) * quantum_ms;
}

}  // namespace

bool TimerScreen::init() {
  root_ = lv_obj_create(nullptr);
  if (!root_) {
    return false;
  }

  lv_obj_remove_style_all(root_);
  lv_obj_set_size(root_, 240, 240);
  lv_obj_set_style_bg_color(root_, color_from_hex(theme_palette(theme_index_).root_bg), 0);
  lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(root_, 0, 0);

  orb_ = lv_obj_create(root_);
  lv_obj_remove_style_all(orb_);
  lv_obj_set_size(orb_, 178, 178);
  lv_obj_align(orb_, LV_ALIGN_CENTER, 0, 4);
  lv_obj_set_style_radius(orb_, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(orb_, color_from_hex(theme_palette(theme_index_).orb_bg), 0);
  lv_obj_set_style_bg_opa(orb_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(orb_, 2, 0);
  lv_obj_set_style_border_color(orb_, color_from_hex(theme_palette(theme_index_).orb_border_base), 0);
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
  lv_obj_set_style_arc_color(arc_, color_from_hex(theme_palette(theme_index_).arc_bg), LV_PART_MAIN);

  title_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(title_label_, color_from_hex(theme_palette(theme_index_).text_primary), 0);
  lv_label_set_text(title_label_, "M5DIAL TIMER");
  lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 18);

  status_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(status_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, -28);

  time_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(time_label_, color_from_hex(theme_palette(theme_index_).text_primary), 0);
  lv_obj_align(time_label_, LV_ALIGN_CENTER, 0, 10);

  duration_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(duration_label_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(duration_label_, color_from_hex(theme_palette(theme_index_).text_muted), 0);
  lv_obj_align(duration_label_, LV_ALIGN_CENTER, 0, 56);

  step_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(step_label_, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(step_label_, color_from_hex(theme_palette(theme_index_).text_warm), 0);
  lv_obj_align(step_label_, LV_ALIGN_BOTTOM_MID, 0, -34);

  hint_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(hint_label_, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(hint_label_, color_from_hex(theme_palette(theme_index_).text_hint), 0);
  lv_obj_set_width(hint_label_, 200);
  lv_label_set_long_mode(hint_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(hint_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_MID, 0, -10);

  lv_scr_load(root_);
  return true;
}

void TimerScreen::cycle_theme(int delta) {
  theme_index_ += delta;
}

void TimerScreen::apply(const TimerSnapshot &snapshot) {
  if (!root_) {
    return;
  }

  const ThemePalette &theme = theme_palette(theme_index_);
  const lv_color_t accent = accent_color(snapshot.state);
  const uint32_t remaining_label_ms = quantize_remaining_ms(snapshot.remaining_ms, kLabelUpdateQuantumMs);
  const uint32_t remaining_arc_ms = quantize_remaining_ms(snapshot.remaining_ms, kArcUpdateQuantumMs);
  const bool theme_changed = !cached_.valid || cached_.theme_index != theme_index_;
  const bool state_changed = !cached_.valid || cached_.state != snapshot.state;
  const bool duration_changed = !cached_.valid || cached_.duration_ms != snapshot.duration_ms;
  const bool remaining_label_changed = !cached_.valid || cached_.remaining_label_ms != remaining_label_ms;
  const bool remaining_arc_changed = !cached_.valid || cached_.remaining_arc_ms != remaining_arc_ms;
  const bool step_changed = !cached_.valid || cached_.adjustment_step_sec != snapshot.adjustment_step_sec;

  if (theme_changed) {
    lv_obj_set_style_bg_color(root_, color_from_hex(theme.root_bg), 0);
    lv_obj_set_style_bg_color(orb_, color_from_hex(theme.orb_bg), 0);
    lv_obj_set_style_arc_color(arc_, color_from_hex(theme.arc_bg), LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label_, color_from_hex(theme.text_primary), 0);
    lv_obj_set_style_text_color(time_label_, color_from_hex(theme.text_primary), 0);
    lv_obj_set_style_text_color(duration_label_, color_from_hex(theme.text_muted), 0);
    lv_obj_set_style_text_color(step_label_, color_from_hex(theme.text_warm), 0);
    lv_obj_set_style_text_color(hint_label_, color_from_hex(theme.text_hint), 0);
  }

  if (theme_changed || state_changed) {
    lv_obj_set_style_border_color(orb_, accent, 0);
    lv_obj_set_style_arc_color(arc_, accent, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(status_label_, accent, 0);
    lv_label_set_text(status_label_, state_text(snapshot.state));
  }

  if (duration_changed) {
    lv_arc_set_range(arc_, 0, snapshot.duration_ms == 0 ? 1 : snapshot.duration_ms);
  }

  if (duration_changed || remaining_arc_changed) {
    lv_arc_set_value(arc_, static_cast<int32_t>(remaining_arc_ms));
  }

  char buffer[24];
  if (remaining_label_changed) {
    format_mmss(remaining_label_ms, buffer, sizeof(buffer));
    lv_label_set_text(time_label_, buffer);
  }

  if (duration_changed) {
    format_mmss(snapshot.duration_ms, buffer, sizeof(buffer));
    char duration_line[32];
    snprintf(duration_line, sizeof(duration_line), "Set %s", buffer);
    lv_label_set_text(duration_label_, duration_line);
  }

  if (theme_changed || step_changed) {
    char step_line[40];
    snprintf(step_line,
             sizeof(step_line),
             "%s style  |  turn %lus",
             theme.name,
             static_cast<unsigned long>(snapshot.adjustment_step_sec));
    lv_label_set_text(step_label_, step_line);
  }

  if (theme_changed || state_changed) {
    const char *hint = "Swipe color. Press start. Hold reset.";
    if (snapshot.state == TimerState::kRunning) {
      hint = "Swipe style. Press pause. Hold reset.";
    } else if (snapshot.state == TimerState::kPaused) {
      hint = "Swipe style. Turn retime. Press resume.";
    } else if (snapshot.state == TimerState::kComplete) {
      hint = "Swipe style. Press run again. Hold reset.";
    }
    lv_label_set_text(hint_label_, hint);
  }

  cached_.valid = true;
  cached_.theme_index = theme_index_;
  cached_.state = snapshot.state;
  cached_.duration_ms = snapshot.duration_ms;
  cached_.remaining_label_ms = remaining_label_ms;
  cached_.remaining_arc_ms = remaining_arc_ms;
  cached_.adjustment_step_sec = snapshot.adjustment_step_sec;
}

}  // namespace tutorial_0072
