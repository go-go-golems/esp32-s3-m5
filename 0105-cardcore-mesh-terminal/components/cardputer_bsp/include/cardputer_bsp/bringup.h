#pragma once

#include "esp_err.h"

namespace cardcore::bsp {

// Native, read-mostly Cardputer-ADV + Cap LoRa-1262 diagnostic sequence.
// It owns I2C0/SPI3 and must run before any other board client is created.
esp_err_t initialize_bringup_diagnostics();

} // namespace cardcore::bsp
