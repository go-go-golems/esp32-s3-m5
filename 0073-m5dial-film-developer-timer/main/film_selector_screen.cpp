#include "film_selector_screen.h"

#include <cstdlib>
#include <cstdio>

namespace tutorial_0073 {

namespace {

struct ThemePalette {
  const char *name;
  uint32_t root_bg;
  uint32_t orb_bg;
  uint32_t orb_border;
  uint32_t accent;
  uint32_t text_primary;
  uint32_t text_muted;
  uint32_t text_detail;
  uint32_t text_hint;
};

constexpr ThemePalette kThemes[] = {
    {"LAB", 0x0E1116, 0x141B23, 0x25303B, 0xD9A441, 0xF6F1E8, 0x9BA6B1, 0xC8D0D7, 0x72808D},
    {"CHEM", 0x10100D, 0x1A1C16, 0x34382E, 0xB9CE72, 0xF3F6E8, 0x9BA78A, 0xD1DABC, 0x7F8F76},
    {"DARK", 0x120B10, 0x20141A, 0x3B2730, 0xE28C6B, 0xF8EEE9, 0xBBA19E, 0xE4C1B4, 0xA38584},
    {"AQUA", 0x071419, 0x0E2128, 0x22404C, 0x77D8D0, 0xEAF9F8, 0x89B2B7, 0xB7E2DE, 0x6E9DA2},
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

const char *field_title(SelectorField field) {
  switch (field) {
    case SelectorField::kFilm:
      return "FILM";
    case SelectorField::kDeveloper:
      return "DEVELOPER";
    case SelectorField::kDilution:
      return "DILUTION";
    case SelectorField::kTemperature:
      return "TEMPERATURE";
    case SelectorField::kPushPull:
      return "PUSH / PULL";
    case SelectorField::kReview:
      return "REVIEW";
  }
  return "FILM";
}

void format_temperature(int16_t temperature_tenths_c, char *out, size_t out_size) {
  std::snprintf(out, out_size, "%d.%d C", temperature_tenths_c / 10, std::abs(temperature_tenths_c % 10));
}

void set_multiline_label(lv_obj_t *label, const char *text, lv_text_align_t align) {
  lv_obj_set_width(label, 164);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(label, align, 0);
  lv_label_set_text(label, text);
}

}  // namespace

bool FilmSelectorScreen::init() {
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
  lv_obj_set_size(orb_, 188, 188);
  lv_obj_align(orb_, LV_ALIGN_CENTER, 0, 4);
  lv_obj_set_style_radius(orb_, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(orb_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(orb_, 2, 0);
  lv_obj_set_style_shadow_width(orb_, 20, 0);
  lv_obj_set_style_shadow_opa(orb_, LV_OPA_30, 0);
  lv_obj_set_style_shadow_color(orb_, color_from_hex(0x050708), 0);

  accent_ring_ = lv_arc_create(root_);
  lv_obj_set_size(accent_ring_, 214, 214);
  lv_obj_align(accent_ring_, LV_ALIGN_CENTER, 0, 4);
  lv_arc_set_rotation(accent_ring_, 225);
  lv_arc_set_bg_angles(accent_ring_, 0, 360);
  lv_obj_remove_style(accent_ring_, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(accent_ring_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(accent_ring_, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_width(accent_ring_, 10, LV_PART_INDICATOR);

  title_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 16);

  field_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(field_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(field_label_, LV_ALIGN_TOP_MID, 0, 42);

  value_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(value_label_, &lv_font_montserrat_20, 0);
  lv_obj_align(value_label_, LV_ALIGN_CENTER, 0, -8);

  detail_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(detail_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(detail_label_, LV_ALIGN_CENTER, 0, 38);

  summary_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(summary_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(summary_label_, LV_ALIGN_BOTTOM_MID, 0, -38);

  hint_label_ = lv_label_create(root_);
  lv_obj_set_style_text_font(hint_label_, &lv_font_montserrat_18, 0);
  lv_obj_align(hint_label_, LV_ALIGN_BOTTOM_MID, 0, -12);

  return true;
}

void FilmSelectorScreen::cycle_theme(int delta) {
  theme_index_ += delta;
}

void FilmSelectorScreen::apply(const SelectorSnapshot &snapshot) {
  if (!root_) {
    return;
  }
  if (lv_scr_act() != root_) {
    lv_scr_load(root_);
  }

  const ThemePalette &theme = theme_palette(theme_index_);
  lv_obj_set_style_bg_color(root_, color_from_hex(theme.root_bg), 0);
  lv_obj_set_style_bg_color(orb_, color_from_hex(theme.orb_bg), 0);
  lv_obj_set_style_border_color(orb_, color_from_hex(theme.orb_border), 0);
  lv_obj_set_style_arc_color(accent_ring_, color_from_hex(theme.orb_border), LV_PART_MAIN);
  lv_obj_set_style_arc_color(accent_ring_, color_from_hex(theme.accent), LV_PART_INDICATOR);
  lv_obj_set_style_text_color(title_label_, color_from_hex(theme.text_muted), 0);
  lv_obj_set_style_text_color(field_label_, color_from_hex(theme.accent), 0);
  lv_obj_set_style_text_color(value_label_, color_from_hex(theme.text_primary), 0);
  lv_obj_set_style_text_color(detail_label_, color_from_hex(theme.text_detail), 0);
  lv_obj_set_style_text_color(summary_label_, color_from_hex(theme.text_muted), 0);
  lv_obj_set_style_text_color(hint_label_, color_from_hex(theme.text_hint), 0);

  lv_label_set_text_fmt(title_label_, "M5DIAL FILM DEV  |  %s", theme.name);
  lv_label_set_text(field_label_, field_title(snapshot.field));

  lv_arc_set_range(accent_ring_, 0, snapshot.option_count == 0 ? 1 : static_cast<int32_t>(snapshot.option_count));
  lv_arc_set_value(accent_ring_, snapshot.option_count == 0 ? 0 : static_cast<int32_t>(snapshot.option_index + 1));

  char value_text[160];
  char detail_text[192];
  char summary_text[256];
  char hint_text[96];
  value_text[0] = '\0';
  detail_text[0] = '\0';
  summary_text[0] = '\0';
  hint_text[0] = '\0';

  switch (snapshot.field) {
    case SelectorField::kFilm:
      std::snprintf(value_text, sizeof(value_text), "%.*s", static_cast<int>(snapshot.selection.film.size()),
                    snapshot.selection.film.data());
      std::snprintf(detail_text, sizeof(detail_text), "Browse starter catalog");
      std::snprintf(summary_text, sizeof(summary_text), "%u of %u", static_cast<unsigned>(snapshot.option_index + 1),
                    static_cast<unsigned>(snapshot.option_count));
      std::snprintf(hint_text, sizeof(hint_text), "Turn browse. Press next. Swipe theme.");
      break;
    case SelectorField::kDeveloper:
      std::snprintf(value_text, sizeof(value_text), "%.*s", static_cast<int>(snapshot.selection.developer.size()),
                    snapshot.selection.developer.data());
      std::snprintf(detail_text, sizeof(detail_text), "%.*s", static_cast<int>(snapshot.selection.film.size()),
                    snapshot.selection.film.data());
      std::snprintf(summary_text, sizeof(summary_text), "%u of %u developers",
                    static_cast<unsigned>(snapshot.option_index + 1), static_cast<unsigned>(snapshot.option_count));
      std::snprintf(hint_text, sizeof(hint_text), "Turn change developer. Press next.");
      break;
    case SelectorField::kDilution:
      std::snprintf(value_text, sizeof(value_text), "%.*s", static_cast<int>(snapshot.selection.dilution.size()),
                    snapshot.selection.dilution.data());
      std::snprintf(detail_text, sizeof(detail_text), "%.*s in %.*s", static_cast<int>(snapshot.selection.developer.size()),
                    snapshot.selection.developer.data(), static_cast<int>(snapshot.selection.film.size()),
                    snapshot.selection.film.data());
      std::snprintf(summary_text, sizeof(summary_text), "%u of %u dilutions",
                    static_cast<unsigned>(snapshot.option_index + 1), static_cast<unsigned>(snapshot.option_count));
      std::snprintf(hint_text, sizeof(hint_text), "Turn change dilution. Press next.");
      break;
    case SelectorField::kTemperature: {
      format_temperature(snapshot.selection.temperature_tenths_c, value_text, sizeof(value_text));
      std::snprintf(detail_text, sizeof(detail_text), "%.*s  |  %.*s",
                    static_cast<int>(snapshot.selection.developer.size()), snapshot.selection.developer.data(),
                    static_cast<int>(snapshot.selection.dilution.size()), snapshot.selection.dilution.data());
      std::snprintf(summary_text, sizeof(summary_text), "%u of %u temperatures",
                    static_cast<unsigned>(snapshot.option_index + 1), static_cast<unsigned>(snapshot.option_count));
      std::snprintf(hint_text, sizeof(hint_text), "Turn change temp. Press next.");
      break;
    }
    case SelectorField::kPushPull:
      std::snprintf(value_text, sizeof(value_text), "%.*s", static_cast<int>(snapshot.selection.push_pull_type.size()),
                    snapshot.selection.push_pull_type.data());
      std::snprintf(detail_text, sizeof(detail_text), "%.*s  |  %d.%d C",
                    static_cast<int>(snapshot.selection.developer.size()), snapshot.selection.developer.data(),
                    snapshot.selection.temperature_tenths_c / 10, std::abs(snapshot.selection.temperature_tenths_c % 10));
      std::snprintf(summary_text, sizeof(summary_text), "%u of %u process variants",
                    static_cast<unsigned>(snapshot.option_index + 1), static_cast<unsigned>(snapshot.option_count));
      std::snprintf(hint_text, sizeof(hint_text), "Turn change push/pull. Press review.");
      break;
    case SelectorField::kReview:
      std::snprintf(value_text, sizeof(value_text), "%us", snapshot.resolved_recipe ? snapshot.resolved_recipe->time_seconds : 0U);
      std::snprintf(detail_text,
                    sizeof(detail_text),
                    "%.*s\n%.*s  |  %.*s",
                    static_cast<int>(snapshot.selection.film.size()),
                    snapshot.selection.film.data(),
                    static_cast<int>(snapshot.selection.developer.size()),
                    snapshot.selection.developer.data(),
                    static_cast<int>(snapshot.selection.dilution.size()),
                    snapshot.selection.dilution.data());
      std::snprintf(summary_text,
                    sizeof(summary_text),
                    "%d.%d C  |  %.*s",
                    snapshot.selection.temperature_tenths_c / 10,
                    std::abs(snapshot.selection.temperature_tenths_c % 10),
                    static_cast<int>(snapshot.selection.push_pull_type.size()),
                    snapshot.selection.push_pull_type.data());
      std::snprintf(hint_text, sizeof(hint_text), "Press start later. Hold goes back.");
      break;
  }

  set_multiline_label(value_label_, value_text, LV_TEXT_ALIGN_CENTER);
  set_multiline_label(detail_label_, detail_text, LV_TEXT_ALIGN_CENTER);
  set_multiline_label(summary_label_, summary_text, LV_TEXT_ALIGN_CENTER);
  set_multiline_label(hint_label_, hint_text, LV_TEXT_ALIGN_CENTER);
}

}  // namespace tutorial_0073
