// SPDX-License-Identifier: MIT
//
// gogolem_nfc Engine smoke. Creates the CoreS3 internal I2C bus (GPIO12/11,
// port 1), attaches the Engine, and proves begin()/scan()/end() run on the
// real ST25R3916 at 0x50. With no tag in the field, scan() must return
// success with an empty tag list — a valid no-tag outcome, not an error.

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>

#include "gogolem/nfc/engine.hpp"
#include "gogolem/nfc/types.hpp"

using namespace gogolem::nfc;

static i2c_master_bus_handle_t make_bus() {
    i2c_master_bus_handle_t bus{};
    i2c_master_bus_config_t cfg{};
    cfg.i2c_port = I2C_NUM_1;
    cfg.sda_io_num = GPIO_NUM_12;
    cfg.scl_io_num = GPIO_NUM_11;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.intr_priority = 0;
    cfg.trans_queue_depth = 0;
    cfg.flags.enable_internal_pullup = 1;
    i2c_new_master_bus(&cfg, &bus);
    return bus;
}

extern "C" void app_main(void) {
    i2c_master_bus_handle_t bus = make_bus();

    Engine engine;
    EngineConfig cfg{};
    cfg.bus = bus;
    cfg.mode = Mode::Reader;

    auto begin = engine.begin(cfg);
    printf("smoke begin ok=%u state=%s\n", begin.ok() ? 1u : 0u,
           lifecycle_state_name(engine.state()));

    for (;;) {
        if (begin.ok()) {
            auto scan = engine.scan(1000);
            printf("smoke scan ok=%u tags=%u state=%s\n",
                   scan.ok() ? 1u : 0u,
                   scan.ok() ? static_cast<unsigned>(scan.value().tags.size()) : 0u,
                   lifecycle_state_name(engine.state()));
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
