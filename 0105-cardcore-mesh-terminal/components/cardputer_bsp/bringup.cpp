#include "cardputer_bsp/bringup.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace cardcore::bsp {
namespace {
constexpr char kTag[] = "cardputer_bsp";

constexpr gpio_num_t kI2cSda = GPIO_NUM_8;
constexpr gpio_num_t kI2cScl = GPIO_NUM_9;
constexpr uint8_t kKeyboardAddress = 0x34;
constexpr uint8_t kCapExpanderAddress = 0x43;

constexpr gpio_num_t kLoraCs = GPIO_NUM_5;
constexpr gpio_num_t kLoraReset = GPIO_NUM_3;
constexpr gpio_num_t kLoraDio1 = GPIO_NUM_4;
constexpr gpio_num_t kLoraBusy = GPIO_NUM_6;
constexpr gpio_num_t kLoraSck = GPIO_NUM_40;
constexpr gpio_num_t kLoraMosi = GPIO_NUM_14;
constexpr gpio_num_t kLoraMiso = GPIO_NUM_39;

// PI4IOE5V6408 registers. P0 is the only Cap output Cardcore changes.
constexpr uint8_t kIoexRegDeviceId = 0x01;
constexpr uint8_t kIoexRegDirection = 0x03;  // 1 means output.
constexpr uint8_t kIoexRegOutput = 0x05;
constexpr uint8_t kIoexRegHighZ = 0x07;      // 0 means push-pull.
constexpr uint8_t kIoexRfEnableBit = 0;

constexpr uint8_t kTca8418RegConfig = 0x01;
constexpr uint8_t kSx1262CmdGetStatus = 0xC0;

struct Handles {
    i2c_master_bus_handle_t i2c_bus = nullptr;
    i2c_master_dev_handle_t keyboard = nullptr;
    i2c_master_dev_handle_t cap_ioex = nullptr;
    spi_device_handle_t sx1262 = nullptr;
    bool initialized = false;
};

Handles s_handles;

esp_err_t add_i2c_device(uint8_t address, i2c_master_dev_handle_t* out) {
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address;
    config.scl_speed_hz = 400000;
    return i2c_master_bus_add_device(s_handles.i2c_bus, &config, out);
}

esp_err_t read_register(i2c_master_dev_handle_t device, uint8_t reg, uint8_t* value) {
    return i2c_master_transmit_receive(device, &reg, 1, value, 1, 100);
}

esp_err_t write_register(i2c_master_dev_handle_t device, uint8_t reg, uint8_t value) {
    const uint8_t command[] = {reg, value};
    return i2c_master_transmit(device, command, sizeof(command), 100);
}

esp_err_t update_register_bit(i2c_master_dev_handle_t device, uint8_t reg, uint8_t bit, bool set) {
    uint8_t value = 0;
    ESP_RETURN_ON_ERROR(read_register(device, reg, &value), kTag, "read IO expander register");
    const uint8_t updated = set ? static_cast<uint8_t>(value | (1U << bit))
                                : static_cast<uint8_t>(value & ~(1U << bit));
    if (updated == value) {
        return ESP_OK;
    }
    return write_register(device, reg, updated);
}

esp_err_t initialize_i2c() {
    i2c_master_bus_config_t config = {};
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = kI2cSda;
    config.scl_io_num = kI2cScl;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&config, &s_handles.i2c_bus), kTag, "create I2C0 bus");
    ESP_RETURN_ON_ERROR(add_i2c_device(kKeyboardAddress, &s_handles.keyboard), kTag, "add TCA8418");
    ESP_RETURN_ON_ERROR(add_i2c_device(kCapExpanderAddress, &s_handles.cap_ioex), kTag, "add Cap IO expander");

    uint8_t keyboard_config = 0;
    ESP_RETURN_ON_ERROR(read_register(s_handles.keyboard, kTca8418RegConfig, &keyboard_config), kTag,
                        "probe TCA8418");
    ESP_LOGI(kTag, "TCA8418 found at 0x%02x, CFG=0x%02x", kKeyboardAddress, keyboard_config);

    uint8_t ioex_id = 0;
    ESP_RETURN_ON_ERROR(read_register(s_handles.cap_ioex, kIoexRegDeviceId, &ioex_id), kTag,
                        "probe Cap PI4IOE5V6408");
    ESP_LOGI(kTag, "Cap IO expander found at 0x%02x, ID=0x%02x", kCapExpanderAddress, ioex_id);

    // RMW each register so undocumented Cap pins retain their reset state.
    ESP_RETURN_ON_ERROR(update_register_bit(s_handles.cap_ioex, kIoexRegDirection, kIoexRfEnableBit, true), kTag,
                        "set Cap P0 output");
    ESP_RETURN_ON_ERROR(update_register_bit(s_handles.cap_ioex, kIoexRegHighZ, kIoexRfEnableBit, false), kTag,
                        "set Cap P0 push-pull");
    ESP_RETURN_ON_ERROR(update_register_bit(s_handles.cap_ioex, kIoexRegOutput, kIoexRfEnableBit, true), kTag,
                        "enable Cap RF front-end");
    ESP_LOGI(kTag, "Cap RF front-end enabled through IO expander P0");
    return ESP_OK;
}

esp_err_t initialize_sx1262_status_probe() {
    gpio_config_t outputs = {};
    outputs.pin_bit_mask = (1ULL << kLoraReset);
    outputs.mode = GPIO_MODE_OUTPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&outputs), kTag, "configure SX1262 reset");

    gpio_config_t inputs = {};
    inputs.pin_bit_mask = (1ULL << kLoraBusy) | (1ULL << kLoraDio1);
    inputs.mode = GPIO_MODE_INPUT;
    ESP_RETURN_ON_ERROR(gpio_config(&inputs), kTag, "configure SX1262 inputs");

    spi_bus_config_t bus = {};
    bus.sclk_io_num = kLoraSck;
    bus.mosi_io_num = kLoraMosi;
    bus.miso_io_num = kLoraMiso;
    bus.quadwp_io_num = GPIO_NUM_NC;
    bus.quadhd_io_num = GPIO_NUM_NC;
    bus.max_transfer_sz = 16;
    ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO), kTag, "initialize SX1262 SPI bus");

    spi_device_interface_config_t device = {};
    device.clock_speed_hz = 1000000;
    device.mode = 0;
    device.spics_io_num = kLoraCs;
    device.queue_size = 1;
    ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI3_HOST, &device, &s_handles.sx1262), kTag, "add SX1262 SPI device");

    gpio_set_level(kLoraReset, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(kLoraReset, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    if (gpio_get_level(kLoraBusy) != 0) {
        ESP_LOGE(kTag, "SX1262 BUSY remains high after reset");
        return ESP_ERR_TIMEOUT;
    }

    const uint8_t tx[] = {kSx1262CmdGetStatus, 0x00};
    uint8_t rx[sizeof(tx)] = {};
    spi_transaction_t transaction = {};
    transaction.length = sizeof(tx) * 8;
    transaction.tx_buffer = tx;
    transaction.rx_buffer = rx;
    ESP_RETURN_ON_ERROR(spi_device_transmit(s_handles.sx1262, &transaction), kTag, "read SX1262 status");
    ESP_LOGI(kTag, "SX1262 status probe succeeded, status=0x%02x", rx[1]);
    return ESP_OK;
}
} // namespace

esp_err_t initialize_bringup_diagnostics() {
    if (s_handles.initialized) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(initialize_i2c(), kTag, "initialize shared I2C diagnostics");
    ESP_RETURN_ON_ERROR(initialize_sx1262_status_probe(), kTag, "initialize SX1262 status diagnostic");
    s_handles.initialized = true;
    return ESP_OK;
}

} // namespace cardcore::bsp
