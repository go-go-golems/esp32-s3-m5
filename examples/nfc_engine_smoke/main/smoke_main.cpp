// SPDX-License-Identifier: MIT
//
// gogolem_nfc Engine write smoke. Exercises the full read-write-read cycle on
// the sacrificial NTAG215: activate, raw read, NDEF read, reversible write
// (save/write/verify/restore/verify), NDEF read (still valid after restoration).

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cstdio>
#include <cstring>

#include "gogolem/nfc/engine.hpp"
#include "gogolem/nfc/mutation.hpp"
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
            // Activate and identify.
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

            // Raw read page 0.
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

            // Reversible write test on page 5 (user area 4-129 on NTAG215).
            MutationPermit permit{};
            permit.allowed = MutationKind::ReversibleWrite;
            // UID will be bound at runtime — set to the known tag UID.
            permit.expected_uid = {0x04, 0xDA, 0xF7, 0x4D, 0x9E, 0x61, 0x80, 0, 0, 0};
            permit.expected_uid_length = 7;
            permit.require_readback = true;
            permit.require_restoration = true;
            auto wr = engine.reversible_write(5, permit);
            printf("smoke write ok=%u write=%u verify=%u restore=%u\n",
                   wr.ok() ? 1u : 0u,
                   wr.ok() ? (wr.value().write_succeeded ? 1u : 0u) : 0u,
                   wr.ok() ? (wr.value().verification_succeeded ? 1u : 0u) : 0u,
                   wr.ok() ? (wr.value().restoration_succeeded ? 1u : 0u) : 0u);
            if (!wr.ok()) {
                printf("smoke write err=%s detail=%s\n",
                       error_layer_name(wr.error().layer), wr.error().detail.data());
            }

            // Read NDEF again — should still be valid after restoration.
            auto ndef2 = engine.read_ndef();
            if (ndef2.ok()) {
                printf("smoke ndef_after_write ok=1 records=%u\n", static_cast<unsigned>(ndef2.value().records.size()));
            } else {
                printf("smoke ndef_after_write ok=0 layer=%s\n", error_layer_name(ndef2.error().layer));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
