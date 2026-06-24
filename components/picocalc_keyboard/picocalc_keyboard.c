#include "picocalc_keyboard.h"

#include <stddef.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "picocalc_kbd";

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;
static SemaphoreHandle_t s_lock = NULL;
static bool s_initialized = false;
static uint8_t s_last_status = 0;
static uint32_t s_error_count = 0;
static uint32_t s_recover_count = 0;
static esp_err_t s_last_error = ESP_OK;

static void note_error(esp_err_t err)
{
    if (err != ESP_OK) {
        s_error_count++;
        s_last_error = err;
    }
}

static esp_err_t ensure_lock(void)
{
    if (!s_lock) {
        s_lock = xSemaphoreCreateMutex();
        if (!s_lock) {
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

static esp_err_t take_lock(void)
{
    esp_err_t err = ensure_lock();
    if (err != ESP_OK) {
        return err;
    }
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static void give_lock(void)
{
    if (s_lock) {
        xSemaphoreGive(s_lock);
    }
}

static void teardown_locked(void)
{
    if (s_dev) {
        (void)i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
    if (s_bus) {
        (void)i2c_del_master_bus(s_bus);
        s_bus = NULL;
    }
    s_initialized = false;
    s_last_status = 0;
}

esp_err_t picocalc_keyboard_init(void)
{
    esp_err_t err = take_lock();
    if (err != ESP_OK) {
        note_error(err);
        return err;
    }

    if (s_initialized) {
        give_lock();
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = PICOCALC_KBD_I2C_SCL_GPIO,
        .sda_io_num = PICOCALC_KBD_I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    err = i2c_new_master_bus(&bus_cfg, &s_bus);
    if (err != ESP_OK) {
        note_error(err);
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        give_lock();
        return err;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PICOCALC_KBD_I2C_ADDR,
        .scl_speed_hz = PICOCALC_KBD_I2C_SPEED_HZ,
    };

    err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) {
        note_error(err);
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        teardown_locked();
        give_lock();
        return err;
    }

    // The PicoCalc southbridge has no host-controlled reset pin. Keep a short
    // post-init delay before the first register access, matching existing Pico
    // firmware bring-up guidance.
    vTaskDelay(pdMS_TO_TICKS(100));

    s_initialized = true;
    ESP_LOGI(TAG,
             "initialized PicoCalc keyboard I2C: sda=%d scl=%d speed=%d addr=0x%02x recoveries=%u",
             PICOCALC_KBD_I2C_SDA_GPIO,
             PICOCALC_KBD_I2C_SCL_GPIO,
             PICOCALC_KBD_I2C_SPEED_HZ,
             PICOCALC_KBD_I2C_ADDR,
             (unsigned)s_recover_count);
    give_lock();
    return ESP_OK;
}

esp_err_t picocalc_keyboard_recover(void)
{
    esp_err_t err = take_lock();
    if (err != ESP_OK) {
        note_error(err);
        return err;
    }

    ++s_recover_count;
    ESP_LOGW(TAG, "recovering PicoCalc keyboard I2C bus/device (attempt=%u)", (unsigned)s_recover_count);
    teardown_locked();
    give_lock();

    vTaskDelay(pdMS_TO_TICKS(100));
    return picocalc_keyboard_init();
}

esp_err_t picocalc_keyboard_probe_address(uint8_t addr, int timeout_ms)
{
    esp_err_t err = picocalc_keyboard_init();
    if (err != ESP_OK) {
        return err;
    }

    err = take_lock();
    if (err != ESP_OK) {
        note_error(err);
        return err;
    }
    if (!s_bus) {
        give_lock();
        note_error(ESP_ERR_INVALID_STATE);
        return ESP_ERR_INVALID_STATE;
    }
    err = i2c_master_probe(s_bus, addr, timeout_ms);
    note_error(err == ESP_ERR_NOT_FOUND ? ESP_OK : err);
    give_lock();
    return err;
}

esp_err_t picocalc_keyboard_read_register(uint8_t reg, uint8_t *dst, size_t len)
{
    if (!dst || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = picocalc_keyboard_init();
    if (err != ESP_OK) {
        return err;
    }

    err = take_lock();
    if (err != ESP_OK) {
        note_error(err);
        return err;
    }
    if (!s_dev) {
        give_lock();
        note_error(ESP_ERR_INVALID_STATE);
        return ESP_ERR_INVALID_STATE;
    }

    err = i2c_master_transmit(s_dev, &reg, 1, 50);
    if (err != ESP_OK) {
        note_error(err);
        give_lock();
        return err;
    }

    // Existing Pico firmware waits after selecting the FIFO register. Keeping
    // the delay for all register reads is harmless at diagnostic polling rates
    // and avoids one special timing path during bring-up.
    vTaskDelay(pdMS_TO_TICKS(2));

    err = i2c_master_receive(s_dev, dst, len, 50);
    note_error(err);
    give_lock();
    return err;
}

esp_err_t picocalc_keyboard_read_status(uint8_t *status)
{
    uint8_t value = 0;
    esp_err_t err = picocalc_keyboard_read_register(PICOCALC_KBD_REG_STATUS, &value, 1);
    if (err != ESP_OK) {
        return err;
    }
    s_last_status = value;
    if (status) {
        *status = value;
    }
    return ESP_OK;
}

uint8_t picocalc_keyboard_fifo_count(uint8_t status)
{
    return status & PICOCALC_KBD_COUNT_MASK;
}

esp_err_t picocalc_keyboard_poll_event(picocalc_key_event_t *event)
{
    if (!event) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(event, 0, sizeof(*event));

    uint8_t status = 0;
    esp_err_t err = picocalc_keyboard_read_status(&status);
    if (err != ESP_OK) {
        return err;
    }

    if (picocalc_keyboard_fifo_count(status) == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t bytes[2] = {0};
    err = picocalc_keyboard_read_register(PICOCALC_KBD_REG_FIFO, bytes, sizeof(bytes));
    if (err != ESP_OK) {
        return err;
    }

    event->state = bytes[0];
    event->key = bytes[1];
    event->valid = bytes[0] != 0 || bytes[1] != 0;
    return event->valid ? ESP_OK : ESP_ERR_NOT_FOUND;
}

void picocalc_keyboard_get_diag(picocalc_keyboard_diag_t *diag)
{
    if (!diag) {
        return;
    }

    diag->initialized = s_initialized;
    diag->last_status = s_last_status;
    diag->error_count = s_error_count;
    diag->recover_count = s_recover_count;
    diag->last_error = s_last_error;
}

const char *picocalc_keyboard_state_name(uint8_t state)
{
    switch (state) {
    case PICOCALC_KBD_STATE_PRESSED:
        return "pressed";
    case PICOCALC_KBD_STATE_REPEATED:
        return "repeat";
    case PICOCALC_KBD_STATE_RELEASED:
        return "released";
    default:
        return "unknown";
    }
}

const char *picocalc_keyboard_key_name(uint8_t key)
{
    switch (key) {
    case 0x08:
        return "backspace";
    case 0x09:
        return "tab";
    case 0x0a:
        return "enter";
    case 0x81:
        return "f1";
    case 0x82:
        return "f2";
    case 0x83:
        return "f3";
    case 0x84:
        return "f4";
    case 0x85:
        return "f5";
    case 0x86:
        return "f6";
    case 0x87:
        return "f7";
    case 0x88:
        return "f8";
    case 0x89:
        return "f9";
    case 0x90:
        return "f10";
    case 0x91:
        return "power";
    case 0xa1:
        return "lalt";
    case 0xa2:
        return "lshift";
    case 0xa3:
        return "rshift";
    case 0xa4:
        return "sym";
    case 0xa5:
        return "lctrl";
    case 0xb1:
        return "esc";
    case 0xb4:
        return "left";
    case 0xb5:
        return "up";
    case 0xb6:
        return "down";
    case 0xb7:
        return "right";
    case 0xc1:
        return "capslock";
    case 0xd0:
        return "break";
    case 0xd1:
        return "insert";
    case 0xd2:
        return "home";
    case 0xd4:
        return "delete";
    case 0xd5:
        return "end";
    case 0xd6:
        return "pageup";
    case 0xd7:
        return "pagedown";
    default:
        return "";
    }
}
