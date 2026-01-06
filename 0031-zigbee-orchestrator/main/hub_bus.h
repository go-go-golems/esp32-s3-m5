#pragma once

#include "esp_err.h"
#include "esp_event.h"

esp_err_t hub_bus_start(void);
void hub_bus_stop(void);

esp_event_loop_handle_t hub_bus_get_loop(void);
