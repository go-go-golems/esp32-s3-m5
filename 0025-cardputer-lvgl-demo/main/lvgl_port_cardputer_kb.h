#pragma once

#include <stdint.h>

extern "C" {
#include "lvgl.h"
}

#include "input_keyboard.h"

// Registers an LVGL KEYPAD indev and returns it (or nullptr on failure).
lv_indev_t *lvgl_port_cardputer_kb_init(void);

// Translates a Cardputer KeyEvent into an LVGL key code (LV_KEY_* or ASCII).
// Returns 0 if the event has no LVGL representation.
uint32_t lvgl_port_cardputer_kb_translate(const KeyEvent &ev);

// Feed keyboard edge events into the LVGL indev queue.
void lvgl_port_cardputer_kb_feed(const std::vector<KeyEvent> &events);

// Queue a single LVGL keycode as a "tap" (press + release).
// Must be called on the LVGL/UI thread.
void lvgl_port_cardputer_kb_queue_key(uint32_t key);
