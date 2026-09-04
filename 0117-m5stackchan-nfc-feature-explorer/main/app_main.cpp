#include "driver/i2c_master.h"
#include "esp_console.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nfc_console.hpp"
#include "nfc_explorer.hpp"

namespace {
constexpr char TAG[] = "main";
constexpr i2c_port_num_t NFC_I2C_PORT = I2C_NUM_1;
constexpr gpio_num_t NFC_I2C_SDA = GPIO_NUM_12;
constexpr gpio_num_t NFC_I2C_SCL = GPIO_NUM_11;

i2c_master_bus_handle_t initialize_i2c()
{
    i2c_master_bus_handle_t bus{};
    const i2c_master_bus_config_t config{
        .i2c_port = NFC_I2C_PORT,
        .sda_io_num = NFC_I2C_SDA,
        .scl_io_num = NFC_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {.enable_internal_pullup = 1, .allow_pd = 0},
    };
    const esp_err_t err = i2c_new_master_bus(&config, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        return nullptr;
    }
    ESP_LOGI(TAG, "I2C ready port=%d SDA=%d SCL=%d", NFC_I2C_PORT, NFC_I2C_SDA, NFC_I2C_SCL);
    return bus;
}

bool initialize_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool start_console(NfcExplorer &explorer)
{
    esp_console_repl_t *repl{};
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "nfc-explorer> ";
    repl_config.task_stack_size = 12288;

    const esp_console_dev_usb_serial_jtag_config_t hardware_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    esp_err_t err = esp_console_new_repl_usb_serial_jtag(&hardware_config, &repl_config, &repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_new_repl_usb_serial_jtag failed: %s", esp_err_to_name(err));
        return false;
    }
    esp_console_register_help_command();
    nfc_console_register(explorer);
    err = esp_console_start_repl(repl);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}
}  // namespace

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP-60 native NFC feature explorer starting");
    if (!initialize_nvs()) {
        return;
    }

    const NfcBootMode mode = nfc_load_boot_mode();
    ESP_LOGI(TAG, "boot mode=%s", nfc_boot_mode_name(mode));

    i2c_master_bus_handle_t bus = initialize_i2c();
    if (!bus) {
        return;
    }

    static NfcExplorer explorer;
    if (!explorer.begin(bus, mode)) {
        ESP_LOGE(TAG, "NFC explorer initialization failed");
        return;
    }
    if (!start_console(explorer)) {
        return;
    }

    ESP_LOGI(TAG, "console ready; run 'nfc-capabilities' and 'help'");
    for (;;) {
        explorer.update();
        vTaskDelay(pdMS_TO_TICKS(mode == NfcBootMode::Reader ? 20 : 1));
    }
}
