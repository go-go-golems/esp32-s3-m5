#pragma once

#include <stdint.h>

extern "C" {
#include "lvgl.h"
}

#include "input_keyboard.h"

// Registers an LVGL KEYPAD indev and returns it (or nullptr on failure).
lv_indev_t *lvgl_port_cardputer_kb_init(void);

// Feed keyboard edge events into the LVGL indev queue.
void lvgl_port_cardputer_kb_feed(const std::vector<KeyEvent> &events);

