// SPDX-License-Identifier: MIT
#pragma once

#include "driver/i2c_master.h"
#include "gogolem/nfc/engine.hpp"

namespace gogolem::nfc::example {

void register_console_commands(Engine& engine, i2c_master_bus_handle_t bus);

}  // namespace gogolem::nfc::example
