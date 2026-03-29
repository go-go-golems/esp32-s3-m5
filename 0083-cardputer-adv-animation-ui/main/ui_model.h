#pragma once

#include <stdint.h>

#include "ui_kb.h"

enum class UiMode : uint8_t {
    Browse = 0,
    Help = 1,
};

struct ScrollModel {
    float pos_px;
    float target_px;
    float min_px;
    float max_px;
    float easing;
    float snap_epsilon_px;
    bool animating;
};

struct UiState {
    UiMode mode;
    bool autoplay;
    int autoplay_dir;
    uint32_t autoplay_period_ms;
    int64_t last_autoplay_us;

    int screen_w;
    int screen_h;
    int line_count;
    float line_step_px;
    float virtual_width_px;
    int selected_index;
    int active_index;

    char last_text[12];
    ui_key_kind_t last_kind;
    uint8_t last_mods;
    uint8_t last_keynum;

    ScrollModel scroll;
    bool dirty;
};

void ui_model_init(UiState *ui, int screen_w, int screen_h, int line_count, uint32_t autoplay_period_ms);
void ui_model_handle_event(UiState *ui, const ui_key_event_t *ev);
bool ui_model_tick(UiState *ui, int64_t now_us);
