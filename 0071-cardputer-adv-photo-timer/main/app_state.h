#pragma once

#include <stdint.h>

#include "esp_err.h"

#include "photo_timer_types.h"

esp_err_t app_state_init(const TimerConfig* cfg);
void app_state_tick(void);

TimerSnapshot app_state_snapshot(void);
TimerConfig app_state_config_copy(void);
uint32_t app_state_config_revision(void);

esp_err_t app_state_replace_config(const TimerConfig* cfg);
esp_err_t app_state_timer_action(const char* action, const char* preset_id);
