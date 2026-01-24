#pragma once

#include <stdint.h>

#include "M5GFX.h"

#include "sim_engine.h"
#include "ui_kb.h"

typedef enum {
    UI_MODE_LIVE = 0,
    UI_MODE_MENU,
    UI_MODE_PATTERN,
    UI_MODE_PARAMS,
    UI_MODE_BRIGHTNESS,
    UI_MODE_FRAME,
    UI_MODE_HELP,
    UI_MODE_PRESETS,
    UI_MODE_JS,
} ui_mode_t;

typedef enum {
    UI_MENU_PATTERN = 0,
    UI_MENU_PARAMS,
    UI_MENU_BRIGHTNESS,
    UI_MENU_FRAME,
    UI_MENU_PRESETS,
    UI_MENU_JS,
    UI_MENU_HELP,
    UI_MENU_COUNT,
} ui_menu_item_t;

typedef struct {
    ui_mode_t mode;
    int menu_index;

    int pattern_index;

    int params_index;

    int preset_index;

    int js_example_index;
    char js_output[512];

    bool color_active;
    int color_channel; // 0=r 1=g 2=b
    char color_hex[7];
    int color_hex_len;
    led_rgb8_t color_orig;
    led_rgb8_t color_cur;
    int color_target; // internal id
} ui_overlay_t;

void ui_overlay_init(ui_overlay_t *ui);
void ui_overlay_handle(ui_overlay_t *ui, sim_engine_t *engine, const ui_key_event_t *ev);

// Draws overlay UI (status/hints/panels) on top of an already-rendered preview.
void ui_overlay_draw(ui_overlay_t *ui,
                     M5Canvas *canvas,
                     const led_pattern_cfg_t *cfg,
                     uint16_t led_count,
                     uint32_t frame_ms);
