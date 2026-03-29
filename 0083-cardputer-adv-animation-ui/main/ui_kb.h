#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_KEY_UNKNOWN = 0,
    UI_KEY_UP,
    UI_KEY_DOWN,
    UI_KEY_LEFT,
    UI_KEY_RIGHT,
    UI_KEY_ENTER,
    UI_KEY_BACK,
    UI_KEY_TAB,
    UI_KEY_SPACE,
    UI_KEY_DEL,
    UI_KEY_TEXT,
} ui_key_kind_t;

typedef enum {
    UI_MOD_SHIFT = 1 << 0,
    UI_MOD_CTRL = 1 << 1,
    UI_MOD_ALT = 1 << 2,
    UI_MOD_FN = 1 << 3,
} ui_key_mods_t;

typedef struct {
    ui_key_kind_t kind;
    uint8_t mods;
    char text[12];
    uint8_t keynum;
} ui_key_event_t;

typedef struct {
    bool ready;
    uint8_t backend;
    bool caps;
    bool shift;
    bool ctrl;
    bool alt;
    bool opt;
    bool fn;
    uint8_t mods;
    uint8_t pressed_keynums[56];
    uint8_t pressed_count;
    bool last_event_valid;
    ui_key_event_t last_event;
    uint32_t seq;
} ui_kb_debug_state_t;

void ui_kb_start(QueueHandle_t q);
bool ui_kb_is_ready(void);
bool ui_kb_debug_get_state(ui_kb_debug_state_t *out);

#ifdef __cplusplus
}
#endif
