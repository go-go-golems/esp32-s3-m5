#include "demo_manager.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lvgl_font_util.h"

namespace {

struct PomodoroState {
    DemoManager *mgr = nullptr;
    lv_obj_t *root = nullptr;
    lv_obj_t *arc = nullptr;
    lv_obj_t *time_label = nullptr;
    lv_obj_t *status_label = nullptr;
    lv_timer_t *tick_timer = nullptr;

    int32_t duration_ms = 0;
    int32_t remaining_ms = 0;

    uint32_t last_tick_ms = 0;
    uint32_t last_label_sec = 0;

    bool running = false;
};

static PomodoroState s_pomodoro;

static void fmt_mmss(char *out, size_t n, int32_t ms) {
    if (ms < 0) ms = 0;
    int32_t total_s = ms / 1000;
    int32_t m = total_s / 60;
    int32_t s = total_s % 60;
    snprintf(out, n, "%02ld:%02ld", (long)m, (long)s);
}

static void ui_refresh(PomodoroState *p) {
    if (!p || !p->arc) return;

    lv_arc_set_range(p->arc, 0, p->duration_ms);
    lv_arc_set_value(p->arc, p->remaining_ms);

    char buf[16];
    fmt_mmss(buf, sizeof(buf), p->remaining_ms);
    lv_label_set_text(p->time_label, buf);

    const char *status = "PAUSED";
    if (p->remaining_ms <= 0) status = "DONE";
    else if (p->running) status = "RUNNING";
    lv_label_set_text(p->status_label, status);
}

static void set_duration_minutes(PomodoroState *p, int minutes) {
    if (!p) return;
    if (minutes < 1) minutes = 1;
    if (minutes > 99) minutes = 99;

    p->duration_ms = minutes * 60 * 1000;
    p->remaining_ms = p->duration_ms;
    p->last_label_sec = 0;
    p->running = false;

    ui_refresh(p);
}

static void stop_and_reset(PomodoroState *p) {
    if (!p) return;
    p->running = false;
    p->remaining_ms = p->duration_ms;
    p->last_label_sec = 0;
    ui_refresh(p);
}

static void toggle_run(PomodoroState *p) {
    if (!p) return;
    p->running = !p->running;
    p->last_tick_ms = lv_tick_get();
    ui_refresh(p);
}

static void on_done(PomodoroState *p) {
    if (!p) return;
    p->running = false;
    p->remaining_ms = 0;
    ui_refresh(p);
}

static void tick_cb(lv_timer_t *t) {
    auto *p = static_cast<PomodoroState *>(t->user_data);
    const uint32_t now = lv_tick_get();

    if (!p->running) {
        p->last_tick_ms = now;
        return;
    }

    uint32_t dt = now - p->last_tick_ms;
    p->last_tick_ms = now;
    if (dt > 500) dt = 500;

    p->remaining_ms -= (int32_t)dt;
    if (p->remaining_ms <= 0) {
        on_done(p);
        return;
    }

    lv_arc_set_value(p->arc, p->remaining_ms);

    const uint32_t sec = (uint32_t)(p->remaining_ms / 1000);
    if (sec != p->last_label_sec) {
        p->last_label_sec = sec;
        char buf[16];
        fmt_mmss(buf, sizeof(buf), p->remaining_ms);
        lv_label_set_text(p->time_label, buf);
    }
}

static void key_cb(lv_event_t *e) {
    auto *p = static_cast<PomodoroState *>(lv_event_get_user_data(e));
    const uint32_t key = lv_event_get_key(e);

    if (key == (uint32_t)' ' || key == LV_KEY_ENTER) {
        toggle_run(p);
        return;
    }

    if (key == (uint32_t)'r' || key == (uint32_t)'R' || key == LV_KEY_BACKSPACE) {
        stop_and_reset(p);
        return;
    }

    if (!p->running) {
        if (key == (uint32_t)'[') {
            int mins = (p->duration_ms / 60000) - 1;
            set_duration_minutes(p, mins);
        } else if (key == (uint32_t)']') {
            int mins = (p->duration_ms / 60000) + 1;
            set_duration_minutes(p, mins);
        }
    }
}

} // namespace

lv_obj_t *demo_pomodoro_create(DemoManager *mgr) {
    s_pomodoro = PomodoroState{};
    s_pomodoro.mgr = mgr;

    s_pomodoro.root = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(s_pomodoro.root, lv_color_black(), 0);
    lv_obj_set_style_text_color(s_pomodoro.root, lv_palette_main(LV_PALETTE_GREEN), 0);

    lv_obj_t *title = lv_label_create(s_pomodoro.root);
    lv_label_set_text(title, "POMODORO");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    s_pomodoro.arc = lv_arc_create(s_pomodoro.root);
    lv_obj_set_size(s_pomodoro.arc, 110, 110);
    lv_obj_align(s_pomodoro.arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(s_pomodoro.arc, 270);
    lv_arc_set_bg_angles(s_pomodoro.arc, 0, 360);
    lv_arc_set_mode(s_pomodoro.arc, LV_ARC_MODE_NORMAL);

    lv_obj_remove_style(s_pomodoro.arc, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_pomodoro.arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_pomodoro.arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_pomodoro.arc, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);

    s_pomodoro.time_label = lv_label_create(s_pomodoro.root);
    lv_obj_set_style_text_font(s_pomodoro.time_label, lvgl_font_time_big(), 0);
    lv_obj_align(s_pomodoro.time_label, LV_ALIGN_CENTER, 0, -2);

    s_pomodoro.status_label = lv_label_create(s_pomodoro.root);
    lv_obj_set_style_text_font(s_pomodoro.status_label, lvgl_font_body(), 0);
    lv_obj_align(s_pomodoro.status_label, LV_ALIGN_CENTER, 0, 36);

    lv_obj_t *hint = lv_label_create(s_pomodoro.root);
    lv_obj_set_style_text_font(hint, lvgl_font_small(), 0);
    lv_label_set_text(hint, "Space start/pause  R reset  [ ] minutes  Fn+` menu");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -2);

    lv_obj_add_event_cb(s_pomodoro.root, key_cb, LV_EVENT_KEY, &s_pomodoro);
    lv_obj_add_flag(s_pomodoro.root, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    set_duration_minutes(&s_pomodoro, 25);
    s_pomodoro.last_tick_ms = lv_tick_get();
    s_pomodoro.tick_timer = lv_timer_create(tick_cb, 50, &s_pomodoro);

    return s_pomodoro.root;
}

void demo_pomodoro_bind_group(DemoManager *mgr) {
    if (!mgr || !mgr->group) return;
    if (!s_pomodoro.root) return;
    lv_group_add_obj(mgr->group, s_pomodoro.root);
    lv_group_focus_obj(s_pomodoro.root);
}
