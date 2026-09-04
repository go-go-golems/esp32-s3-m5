// SPDX-License-Identifier: MIT
//
// gogolem_nfc feature explorer — app_main. Creates the I²C bus, initializes
// the Engine in the NVS-selected mode, and starts the USB Serial/JTAG console.
// The component owns NFC logic; the example owns presentation and policy.

#include "console_adapter.hpp"
#include "demo_profiles.hpp"
#include "nvs_mode_store.hpp"

#include "driver/i2c_master.h"
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "gogolem/nfc/engine.hpp"

using namespace gogolem::nfc;
using namespace gogolem::nfc::example;

static constexpr const char* TAG = "main";
static constexpr i2c_port_num_t NFC_I2C_PORT = I2C_NUM_1;
static constexpr gpio_num_t NFC_I2C_SDA = GPIO_NUM_12;
static constexpr gpio_num_t NFC_I2C_SCL = GPIO_NUM_11;

static i2c_master_bus_handle_t make_bus() {
    i2c_master_bus_handle_t bus{};
    i2c_master_bus_config_t cfg{};
    cfg.i2c_port = NFC_I2C_PORT;
    cfg.sda_io_num = NFC_I2C_SDA;
    cfg.scl_io_num = NFC_I2C_SCL;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.intr_priority = 0;
    cfg.trans_queue_depth = 0;
    cfg.flags.enable_internal_pullup = 1;
    i2c_new_master_bus(&cfg, &bus);
    return bus;
}

static bool init_nvs() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    return err == ESP_OK;
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "gogolem_nfc feature explorer starting");
    if (!init_nvs()) { ESP_LOGE(TAG, "NVS init failed"); return; }

    Mode mode = load_boot_mode();
    ESP_LOGI(TAG, "boot mode=%s", example::mode_name(mode));

    i2c_master_bus_handle_t bus = make_bus();
    if (!bus) { ESP_LOGE(TAG, "I2C bus creation failed"); return; }

    static Engine engine;
    EngineConfig cfg{};
    cfg.bus = bus;
    cfg.mode = mode;
    if (mode == Mode::EmulationUltralight) cfg.emulation_profile = make_ultralight_profile();
    else if (mode == Mode::EmulationNtag213) cfg.emulation_profile = make_ntag213_profile();

    auto begin = engine.begin(cfg);
    if (!begin.ok()) {
        ESP_LOGE(TAG, "Engine init failed: %s", error_layer_name(begin.error().layer));
        return;
    }
    ESP_LOGI(TAG, "Engine ready mode=%s state=%s", example::mode_name(mode),
             lifecycle_state_name(engine.state()));

    // Start console.
    esp_console_repl_t* repl{};
    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.prompt = "nfc-explorer> ";
    repl_cfg.task_stack_size = 12288;
    const esp_console_dev_usb_serial_jtag_config_t hw_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_console_new_repl_usb_serial_jtag(&hw_cfg, &repl_cfg, &repl);
    esp_console_register_help_command();
    register_console_commands(engine, bus);
    esp_console_start_repl(repl);

    ESP_LOGI(TAG, "console ready; run 'nfc-capabilities' and 'help'");

    // Emulation update loop.
    for (;;) {
        if (mode != Mode::Reader) {
            engine.update_emulation();
            vTaskDelay(pdMS_TO_TICKS(1));
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}
