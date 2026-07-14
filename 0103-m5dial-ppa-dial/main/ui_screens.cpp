#include "ui_screens.h"

#include <cmath>
#include <cstdio>

#include "lvgl.h"

namespace {
// PPA Group Control dark theme (matches the Mac app / prototype).
constexpr lv_color_t col(uint32_t rgb) { return lv_color_hex(rgb); }
constexpr uint32_t kBg = 0x111111;
constexpr uint32_t kFg = 0xFFFFFF;
constexpr uint32_t kDim = 0x888888;
constexpr uint32_t kGreen = 0x14A038;
constexpr uint32_t kRed = 0xC03020;

constexpr int kMaxDots = 16;
constexpr int kDotRadius = 104; // rim circle for position dots
constexpr int kSlidePx = 70;
constexpr uint32_t kSlideMs = 150;

// Status screen
lv_obj_t *s_status_scr = nullptr;
lv_obj_t *s_status_title = nullptr;
lv_obj_t *s_status_line1 = nullptr;
lv_obj_t *s_status_line2 = nullptr;
lv_obj_t *s_status_spinner = nullptr;

// Carousel screen
lv_obj_t *s_car_scr = nullptr;
lv_obj_t *s_car_name = nullptr;
lv_obj_t *s_car_sub = nullptr;
lv_obj_t *s_car_online_label = nullptr;
lv_obj_t *s_car_online_arc = nullptr;
lv_obj_t *s_car_dots[kMaxDots] = {};

// Activation overlay
lv_obj_t *s_ovl = nullptr;
lv_obj_t *s_ovl_name = nullptr;
lv_obj_t *s_ovl_result = nullptr;
lv_obj_t *s_ovl_count = nullptr;
lv_obj_t *s_ovl_arc = nullptr;

void style_screen(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(scr, col(kBg), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color,
                     lv_align_t align, int x, int y) {
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, col(color), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(l, align, x, y);
    lv_label_set_text(l, "");
    return l;
}

void build_status_screen() {
    s_status_scr = lv_obj_create(nullptr);
    style_screen(s_status_scr);
    s_status_title = make_label(s_status_scr, &lv_font_montserrat_28, kFg, LV_ALIGN_CENTER, 0, -30);
    s_status_line1 = make_label(s_status_scr, &lv_font_montserrat_14, kDim, LV_ALIGN_CENTER, 0, 6);
    s_status_line2 = make_label(s_status_scr, &lv_font_montserrat_14, kDim, LV_ALIGN_CENTER, 0, 30);
    s_status_spinner = lv_spinner_create(s_status_scr, 1000, 60);
    lv_obj_set_size(s_status_spinner, 40, 40);
    lv_obj_align(s_status_spinner, LV_ALIGN_CENTER, 0, 72);
    lv_obj_set_style_arc_color(s_status_spinner, col(kGreen), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_status_spinner, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(s_status_spinner, 4, LV_PART_MAIN);
}

void build_carousel_screen() {
    s_car_scr = lv_obj_create(nullptr);
    style_screen(s_car_scr);

    // Bottom rim arc: module reachability gauge.
    s_car_online_arc = lv_arc_create(s_car_scr);
    lv_obj_set_size(s_car_online_arc, 232, 232);
    lv_obj_center(s_car_online_arc);
    lv_arc_set_bg_angles(s_car_online_arc, 55, 125); // bottom sector
    lv_arc_set_range(s_car_online_arc, 0, 100);
    lv_arc_set_value(s_car_online_arc, 0);
    lv_obj_remove_style(s_car_online_arc, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(s_car_online_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_car_online_arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_car_online_arc, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_car_online_arc, col(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_car_online_arc, col(kGreen), LV_PART_INDICATOR);

    s_car_name = make_label(s_car_scr, &lv_font_montserrat_28, kFg, LV_ALIGN_CENTER, 0, -14);
    lv_label_set_long_mode(s_car_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_car_name, 190);
    s_car_sub = make_label(s_car_scr, &lv_font_montserrat_14, kDim, LV_ALIGN_CENTER, 0, 22);
    s_car_online_label = make_label(s_car_scr, &lv_font_montserrat_14, kDim, LV_ALIGN_CENTER, 0, 68);

    for (int i = 0; i < kMaxDots; i++) {
        lv_obj_t *dot = lv_obj_create(s_car_scr);
        lv_obj_set_size(dot, 6, 6);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_bg_color(dot, col(kDim), 0);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        s_car_dots[i] = dot;
    }

    // Activation overlay lives on the carousel screen, hidden by default.
    s_ovl = lv_obj_create(s_car_scr);
    lv_obj_set_size(s_ovl, 240, 240);
    lv_obj_center(s_ovl);
    lv_obj_set_style_radius(s_ovl, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_ovl, col(kBg), 0);
    lv_obj_set_style_bg_opa(s_ovl, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_ovl, 0, 0);
    lv_obj_clear_flag(s_ovl, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_ovl, LV_OBJ_FLAG_HIDDEN);

    s_ovl_arc = lv_arc_create(s_ovl);
    lv_obj_set_size(s_ovl_arc, 180, 180);
    lv_obj_center(s_ovl_arc);
    lv_arc_set_bg_angles(s_ovl_arc, 0, 360);
    lv_arc_set_rotation(s_ovl_arc, 270);
    lv_arc_set_range(s_ovl_arc, 0, 100);
    lv_arc_set_value(s_ovl_arc, 0);
    lv_obj_remove_style(s_ovl_arc, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(s_ovl_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(s_ovl_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ovl_arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_ovl_arc, col(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_ovl_arc, col(kGreen), LV_PART_INDICATOR);

    s_ovl_name = make_label(s_ovl, &lv_font_montserrat_18, kDim, LV_ALIGN_CENTER, 0, -34);
    s_ovl_result = make_label(s_ovl, &lv_font_montserrat_28, kFg, LV_ALIGN_CENTER, 0, 0);
    s_ovl_count = make_label(s_ovl, &lv_font_montserrat_14, kDim, LV_ALIGN_CENTER, 0, 34);
}

void slide_in(lv_obj_t *obj, int from_dx) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_time(&a, kSlideMs);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {
        lv_obj_set_style_translate_x(static_cast<lv_obj_t *>(var), v, 0);
    });
    lv_anim_set_values(&a, from_dx, 0);
    lv_anim_start(&a);
}
} // namespace

bool ui_init() {
    build_status_screen();
    build_carousel_screen();
    lv_scr_load(s_status_scr);
    return true;
}

void ui_show_status(const char *title, const char *line1, const char *line2,
                    bool error, bool spinner) {
    lv_label_set_text(s_status_title, title);
    lv_obj_set_style_text_color(s_status_title, col(error ? kRed : kFg), 0);
    lv_label_set_text(s_status_line1, line1 ? line1 : "");
    lv_label_set_text(s_status_line2, line2 ? line2 : "");
    if (spinner) lv_obj_clear_flag(s_status_spinner, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_status_spinner, LV_OBJ_FLAG_HIDDEN);
    if (lv_scr_act() != s_status_scr) lv_scr_load(s_status_scr);
}

void ui_show_carousel() {
    if (lv_scr_act() != s_car_scr) lv_scr_load(s_car_scr);
}

void ui_carousel_update(int index, int total, const char *name, bool is_active,
                        int active_index, int slide_dir) {
    lv_label_set_text(s_car_name, name);
    lv_obj_set_style_text_color(s_car_name, col(is_active ? kGreen : kFg), 0);
    if (is_active) {
        lv_label_set_text(s_car_sub, "AKTIV");
        lv_obj_set_style_text_color(s_car_sub, col(kGreen), 0);
    } else {
        lv_label_set_text(s_car_sub, "druecken = schalten");
        lv_obj_set_style_text_color(s_car_sub, col(kDim), 0);
    }
    if (slide_dir != 0) {
        slide_in(s_car_name, slide_dir * kSlidePx);
        slide_in(s_car_sub, slide_dir * kSlidePx / 2);
    }

    // Position dots along the top rim, centered around 12 o'clock.
    const int n = total > kMaxDots ? kMaxDots : total;
    const float step_deg = 14.0f;
    const float start_deg = -90.0f - step_deg * (n - 1) / 2.0f;
    for (int i = 0; i < kMaxDots; i++) {
        lv_obj_t *dot = s_car_dots[i];
        if (i >= n) {
            lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_HIDDEN);
        const float a = (start_deg + step_deg * i) * 3.14159265f / 180.0f;
        const int dx = static_cast<int>(kDotRadius * cosf(a));
        const int dy = static_cast<int>(kDotRadius * sinf(a));
        const bool sel = i == index;
        lv_obj_set_size(dot, sel ? 10 : 6, sel ? 10 : 6);
        lv_obj_align(dot, LV_ALIGN_CENTER, dx, dy);
        const uint32_t c = i == active_index ? kGreen : (sel ? kFg : kDim);
        lv_obj_set_style_bg_color(dot, col(c), 0);
    }
}

void ui_carousel_set_online(int online, int total_actions) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d/%d online", online, total_actions);
    lv_label_set_text(s_car_online_label, buf);
    const bool all = total_actions > 0 && online == total_actions;
    lv_obj_set_style_text_color(s_car_online_label, col(all ? kGreen : kRed), 0);
    lv_obj_set_style_arc_color(s_car_online_arc, col(all ? kGreen : kRed), LV_PART_INDICATOR);
    lv_arc_set_value(s_car_online_arc,
                     total_actions > 0 ? (100 * online) / total_actions : 0);
}

void ui_activation_show(const char *scene_name) {
    lv_label_set_text(s_ovl_name, scene_name);
    lv_label_set_text(s_ovl_result, "Schalte ...");
    lv_obj_set_style_text_color(s_ovl_result, col(kFg), 0);
    lv_label_set_text(s_ovl_count, "");
    lv_arc_set_value(s_ovl_arc, 0);
    lv_obj_set_style_arc_color(s_ovl_arc, col(kGreen), LV_PART_INDICATOR);
    lv_obj_clear_flag(s_ovl, LV_OBJ_FLAG_HIDDEN);
}

void ui_activation_progress(int done, int total) {
    lv_arc_set_value(s_ovl_arc, total > 0 ? (100 * done) / total : 0);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d/%d Module", done, total);
    lv_label_set_text(s_ovl_count, buf);
}

void ui_activation_result(bool ok, int done, int total) {
    lv_label_set_text(s_ovl_result, ok ? "OK" : "Fehler");
    lv_obj_set_style_text_color(s_ovl_result, col(ok ? kGreen : kRed), 0);
    lv_obj_set_style_arc_color(s_ovl_arc, col(ok ? kGreen : kRed), LV_PART_INDICATOR);
    lv_arc_set_value(s_ovl_arc, 100);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d/%d Module", done, total);
    lv_label_set_text(s_ovl_count, buf);
}

void ui_activation_hide() { lv_obj_add_flag(s_ovl, LV_OBJ_FLAG_HIDDEN); }
