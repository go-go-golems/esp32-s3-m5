#include "film_process_screen.h"

#include <cstdlib>
#include <cstdio>

namespace tutorial_0073 {

namespace {

struct ThemePalette {
  const char *name;
  uint32_t root_bg;
  uint32_t orb_bg;
  uint32_t orb_border;
  uint32_t arc_bg;
  uint32_t accent_idle;
  uint32_t accent_run;
  uint32_t accent_done;
  uint32_t text_primary;
  uint32_t text_muted;
  uint32_t text_hint;
};

constexpr ThemePalette kThemes[] = {
    {"TRAY", 0x101014, 0x191C23, 0x313845, 0x242B34, 0xD1A851, 0x77D8D0, 0xF07A64, 0xF5F1EA, 0x9BA3AD, 0x7B8894},
    {"MOSS", 0x0D120D, 0x151D15, 0x2E3C31, 0x203023, 0xB5C96A, 0x8FE29A, 0xF39C75, 0xEEF5EC, 0x95A78F, 0x74866F},
    {"TUNG", 0x17100E, 0x241915, 0x443128, 0x2F221D, 0xE1A870, 0xE2C06E, 0xF16D58, 0xF8EDE8, 0xBCA39B, 0x987F78},
    {"BLUE", 0x071219, 0x0F1D28, 0x214256, 0x152A36, 0x74C2E5, 0x5AD2D2, 0xF08A6A, 0xE9F8F9, 0x8CAFB6, 0x71949B},
};

lv_color_t color_from_hex(uint32_t rgb) {
  return lv_color_make((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

const ThemePalette &theme_palette(int index) {
  const int theme_count = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));
  int normalized = index % theme_count;
  if (normalized < 0) {
    normalized += theme_count;
  }
  return kThemes[normalized];
}

const char *state_text(FilmTimerState state) {
  switch (state) {
    case FilmTimerState::kEmpty:
      return "EMPTY";
    case FilmTimerState::kReady:
      return "READY";
    case FilmTimerState::kRunning:
      return "RUNNING";
    case FilmTimerState::kPaused:
      return "PAUSED";
    case FilmTimerState::kComplete:
      return "DONE";
  }
  return "READY";
}

lv_color_t accent_color(const ThemePalette &theme, FilmTimerState state) {
  switch (state) {
    case FilmTimerState::kRunning:
      return color_from_hex(theme.accent_run);
    case FilmTimerState::kComplete:
      return color_from_hex(theme.accent_done);
    case FilmTimerState::kEmpty:
    case FilmTimerState::kReady:
    case FilmTimerState::kPaused:
      return color_from_hex(theme.accent_idle);
  }
  return color_from_hex(theme.accent_idle);
}

void format_mmss(uint32_t total_ms, char *out, size_t out_size) {
  const uint32_t total_sec = total_ms / 1000U;
  const uint32_t minutes = total_sec / 60U;
  const uint32_t seconds = total_sec % 60U;
  std::snprintf(out, out_size, "%02u:%02u", static_cast<unsigned>(minutes), static_cast<unsigned>(seconds));
}

void set_multiline_label(lv_obj_t *label, const char *text) {
  lv_obj_set_width(label, 172);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(label, text);
}

}  // namespace

bool FilmProcessScreen::init() {
  root_ = lv_obj_create(nullptr);
  if (!root_) {
    return false;
  }

  lv_obj_remove_style_all(root_);
  lv_obj_set_size(root_, 240, 240);
  lv_obj_set_style_pad_all(root_, 0, 0);
  lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);

  orb_ = lv_obj_create(root_);
  lv_obj_remove_style_all(orb_);
  lv_obj_set_size(orb_, 184, 184);
  lv_obj_align(orb_, LV_ALIGN_CENTER, 0, 2);
  lv_obj_set_style_radius(orb_, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(orb_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(orb_, 2, 0);
  lv_obj_set_style_shadow_width(orb_, 22, 0);
  lv_obj_set_style_shadow_opa(orb_, LV_OPA_30, 0);
  lv_obj_set_style_shadow_color(orb_, color_from_hex(0x050708), 0);

  arc_ = lv_arc_create(root_);
  lv_obj_set_size(arc_, 212, 212);
  lv_obj_align(arc_, LV_ALIGN_CENTER, 0, 2);
  lv_arc_set_rotation(arc_, 270);
  lv_arc_set_bg_angles(arc_, 0, 360);
  lv_arc_set_mode(arc_, LV_ARC_MODE_NORMAL);
  lv_obj_remove_style(arc_, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(arc_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(arc_, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc_, 10, LV_PART_INDICATOR);

  title_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 14);

  status_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(status_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, -34);

  time_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_48, 0);
  lv_obj_align(time_label_, LV_ALIGN_CENTER, 0, 4);

  film_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(film_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(film_label_, LV_ALIGN_CENTER, 0, 48);

  process_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(process_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(process_label_, LV_ALIGN_BOTTOM_MID, 0, -42);

  meta_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(meta_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(meta_label_, LV_ALIGN_BOTTOM_MID, 0, -22);

  hint_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(hint_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_MID, 0, -4);

  return true;
}

void FilmProcessScreen::cycle_theme(int delta) {
  theme_index_ += delta;
}

void FilmProcessScreen::apply(const FilmTimerSnapshot &snapshot) {
  if (!root_) {
    return;
  }
  if (lv_scr_act() != root_) {
    lv_scr_load(root_);
  }

  const ThemePalette &theme = theme_palette(theme_index_);
  const lv_color_t accent = accent_color(theme, snapshot.state);

  lv_obj_set_style_bg_color(root_, color_from_hex(theme.root_bg), 0);
  lv_obj_set_style_bg_color(orb_, color_from_hex(theme.orb_bg), 0);
  lv_obj_set_style_border_color(orb_, color_from_hex(theme.orb_border), 0);
  lv_obj_set_style_arc_color(arc_, color_from_hex(theme.arc_bg), LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc_, accent, LV_PART_INDICATOR);
  lv_obj_set_style_text_color(title_label_, color_from_hex(theme.text_muted), 0);
  lv_obj_set_style_text_color(status_label_, accent, 0);
  lv_obj_set_style_text_color(time_label_, color_from_hex(theme.text_primary), 0);
  lv_obj_set_style_text_color(film_label_, color_from_hex(theme.text_primary), 0);
  lv_obj_set_style_text_color(process_label_, color_from_hex(theme.text_muted), 0);
  lv_obj_set_style_text_color(meta_label_, color_from_hex(theme.text_muted), 0);
  lv_obj_set_style_text_color(hint_label_, color_from_hex(theme.text_hint), 0);

  lv_label_set_text_fmt(title_label_, "FILM PROCESS  |  %s", theme.name);
  lv_label_set_text(status_label_, state_text(snapshot.state));
  lv_arc_set_range(arc_, 0, snapshot.duration_ms == 0 ? 1 : snapshot.duration_ms);
  lv_arc_set_value(arc_, static_cast<int32_t>(snapshot.remaining_ms));

  char time_text[24];
  char film_text[160];
  char process_text[160];
  char meta_text[160];
  char hint_text[128];
  time_text[0] = '\0';
  film_text[0] = '\0';
  process_text[0] = '\0';
  meta_text[0] = '\0';
  hint_text[0] = '\0';

  format_mmss(snapshot.remaining_ms, time_text, sizeof(time_text));
  std::snprintf(film_text,
                sizeof(film_text),
                "%.*s",
                static_cast<int>(snapshot.selection.film.size()),
                snapshot.selection.film.data());
  std::snprintf(process_text,
                sizeof(process_text),
                "%.*s  |  %.*s",
                static_cast<int>(snapshot.selection.developer.size()),
                snapshot.selection.developer.data(),
                static_cast<int>(snapshot.selection.dilution.size()),
                snapshot.selection.dilution.data());
  std::snprintf(meta_text,
                sizeof(meta_text),
                "%d.%d C  |  %.*s",
                snapshot.selection.temperature_tenths_c / 10,
                std::abs(snapshot.selection.temperature_tenths_c % 10),
                static_cast<int>(snapshot.selection.push_pull_type.size()),
                snapshot.selection.push_pull_type.data());

  switch (snapshot.state) {
    case FilmTimerState::kReady:
      std::snprintf(hint_text, sizeof(hint_text), "Press start. Hold selector. Swipe theme.");
      break;
    case FilmTimerState::kRunning:
      std::snprintf(hint_text, sizeof(hint_text), "Press pause. Hold reset. Swipe theme.");
      break;
    case FilmTimerState::kPaused:
      std::snprintf(hint_text, sizeof(hint_text), "Press resume. Hold selector. Swipe theme.");
      break;
    case FilmTimerState::kComplete:
      std::snprintf(hint_text, sizeof(hint_text), "Press run again. Hold selector. Swipe theme.");
      break;
    case FilmTimerState::kEmpty:
      std::snprintf(hint_text, sizeof(hint_text), "No recipe loaded.");
      break;
  }

  set_multiline_label(time_label_, time_text);
  set_multiline_label(film_label_, film_text);
  set_multiline_label(process_label_, process_text);
  set_multiline_label(meta_label_, meta_text);
  set_multiline_label(hint_label_, hint_text);
}

}  // namespace tutorial_0073
