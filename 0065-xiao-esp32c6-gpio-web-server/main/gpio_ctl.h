#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t gpio_ctl_init(void);

void gpio_ctl_get(bool *out_d2, bool *out_d3);

esp_err_t gpio_ctl_set_d2(bool logical_level);
esp_err_t gpio_ctl_set_d3(bool logical_level);
esp_err_t gpio_ctl_set(bool has_d2, bool d2_level, bool has_d3, bool d3_level);

esp_err_t gpio_ctl_toggle_d2(void);
esp_err_t gpio_ctl_toggle_d3(void);

int gpio_ctl_d2_gpio_num(void);
int gpio_ctl_d3_gpio_num(void);
bool gpio_ctl_d2_active_low(void);
bool gpio_ctl_d3_active_low(void);

#ifdef __cplusplus
}  // extern "C"
#endif

