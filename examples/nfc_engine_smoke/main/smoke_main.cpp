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
#include "gogolem/nfc/service.hpp"
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

    Service service;
    ServiceConfig scfg{};
    scfg.engine.bus = bus;
    scfg.engine.mode = Mode::Reader;

    auto start = service.start(scfg);
    printf("smoke service start ok=%u running=%u\n", start.ok() ? 1u : 0u, service.running() ? 1u : 0u);

    for (;;) {
        if (service.running()) {
            // Submit commands from the main task; the worker executes them.
            Command cmd{};
            cmd.kind = ServiceCommand::ActivateOne;
            service.submit(cmd);
            vTaskDelay(pdMS_TO_TICKS(500));

            cmd.kind = ServiceCommand::RawRead;
            cmd.address = 0;
            service.submit(cmd);
            vTaskDelay(pdMS_TO_TICKS(500));

            cmd.kind = ServiceCommand::ReadNdef;
            service.submit(cmd);
            vTaskDelay(pdMS_TO_TICKS(500));

            // Read the latest snapshot.
            ServiceSnapshot snap{};
            if (service.latest(snap)) {
                printf("smoke snap ops=%lu fail=%lu tag=%u ndef_ok=%u recs=%lu raw_ok=%u\n",
                       static_cast<unsigned long>(snap.operations),
                       static_cast<unsigned long>(snap.failures),
                       snap.tag_present ? 1u : 0u,
                       snap.ndef_ok ? 1u : 0u,
                       static_cast<unsigned long>(snap.ndef_records),
                       snap.raw_read_ok ? 1u : 0u);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
