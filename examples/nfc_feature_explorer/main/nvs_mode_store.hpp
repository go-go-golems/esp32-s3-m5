// SPDX-License-Identifier: MIT
#pragma once

#include "esp_err.h"
#include "gogolem/nfc/types.hpp"

namespace gogolem::nfc::example {

Mode load_boot_mode();
esp_err_t store_boot_mode(Mode mode);
const char* mode_name(Mode mode);

}  // namespace gogolem::nfc::example
