#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern "C" {
#include "lvgl.h"
}

// Initialize the command palette overlay system.
// Must be called on the LVGL UI thread after LVGL init.
void command_palette_init(lv_indev_t *kb_indev, lv_group_t *app_group, QueueHandle_t ctrl_q);

bool command_palette_is_open(void);
void command_palette_toggle(void);
void command_palette_close(void);

