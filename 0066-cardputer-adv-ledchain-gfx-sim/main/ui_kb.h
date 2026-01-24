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
    UI_KEY_TEXT, // e.g. "p", "E", "?", "bksp"
} ui_key_kind_t;

typedef enum {
    UI_MOD_SHIFT = 1 << 0,
    UI_MOD_CTRL = 1 << 1,
    UI_MOD_ALT = 1 << 2,
    UI_MOD_FN = 1 << 3,
} ui_key_mods_t;

typedef struct {
    ui_key_kind_t kind;
    uint8_t mods;          // ui_key_mods_t bits
    char text[12];         // only for UI_KEY_TEXT; otherwise empty
    uint8_t keynum;        // physical keyNum (1..56) when available; 0 otherwise
} ui_key_event_t;

// Starts the Cardputer keyboard scan task and pushes ui_key_event_t messages into q.
// q items must be sizeof(ui_key_event_t).
void ui_kb_start(QueueHandle_t q);

#ifdef __cplusplus
}  // extern "C"
#endif

