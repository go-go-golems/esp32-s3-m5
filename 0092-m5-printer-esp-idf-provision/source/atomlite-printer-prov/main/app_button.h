#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define ATOM_LITE_BUTTON_GPIO 39

esp_err_t app_button_init(void);
bool app_button_is_pressed(void);
bool app_button_pressed_for(uint32_t duration_ms);
