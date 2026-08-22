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
            // Activate and identify the tag.
            auto act = engine.activate_one();
            if (act.ok()) {
                const auto& tag = act.value().tag;
                printf("smoke activate ok=1 source=%s uid=", act.value().source == ActivationSource::WUPA ? "WUPA" : "REQA");
                for (uint8_t i = 0; i < tag.uid_length; ++i) printf("%02X", tag.uid[i]);
                printf(" family=%s\n", tag_family_name(tag.family));
                engine.deactivate();
            } else {
                printf("smoke activate ok=0 layer=%s\n", error_layer_name(act.error().layer));
            }

            // Raw read page 0 (should return 16 bytes with UID + capability container).
            auto raw = engine.raw_read(0);
            if (raw.ok()) {
                printf("smoke raw_read ok=1 len=%u hex=", static_cast<unsigned>(raw.value().size()));
                for (auto b : raw.value()) printf("%02X", b);
                printf("\n");
            } else {
                printf("smoke raw_read ok=0 layer=%s\n", error_layer_name(raw.error().layer));
            }

            // Read NDEF (should be valid empty on the NTAG215).
            auto ndef = engine.read_ndef();
            if (ndef.ok()) {
                printf("smoke ndef_read ok=1 records=%u\n", static_cast<unsigned>(ndef.value().records.size()));
            } else {
                printf("smoke ndef_read ok=0 layer=%s\n", error_layer_name(ndef.error().layer));
            }

            // Dump the entire card.
            auto dmp = engine.dump();
            printf("smoke dump ok=%u\n", dmp.ok() ? 1u : 0u);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
