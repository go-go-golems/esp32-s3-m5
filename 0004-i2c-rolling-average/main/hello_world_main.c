/*
 * ESP32-S3 tutorial 0004:
 * - esp_timer callback triggers periodic polling (callback -> queue)
 * - an I2C polling task performs the I2C transaction and sends samples (queue)
 * - an averaging task computes a rolling average from samples
 *
 * Sensor: SHT30/SHT31 compatible (I2C address 0x44 by default)
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "i2c_avg_demo";

// Adjust these for your board / Grove port.
static const gpio_num_t I2C_SDA_GPIO = GPIO_NUM_8;
static const gpio_num_t I2C_SCL_GPIO = GPIO_NUM_9;
static const i2c_port_t I2C_PORT = I2C_NUM_0;

static const uint16_t SHT3X_ADDR = 0x44;
static const int I2C_TIMEOUT_MS = 100;

#define POLL_PERIOD_MS (1000U)
#define ROLLING_WINDOW (10U)

typedef struct {
    uint32_t seq;
    uint32_t ts_ms;
} poll_evt_t;

typedef struct {
    uint32_t seq;
    uint32_t ts_ms;
    float temp_c;
    float rel_humidity;
} sample_t;

static QueueHandle_t poll_queue = NULL;
static QueueHandle_t sample_queue = NULL;

static i2c_master_bus_handle_t i2c_bus = NULL;
static i2c_master_dev_handle_t sht_dev = NULL;

static uint8_t sht_crc8(const uint8_t *data, size_t len) {
    // SHT3x CRC: polynomial 0x31, init 0xFF
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static esp_err_t sht3x_read(sample_t *out, uint32_t seq) {
    // Single-shot, high repeatability, clock stretching disabled
    const uint8_t cmd[2] = {0x2C, 0x06};
    uint8_t buf[6] = {0};

    esp_err_t err = i2c_master_transmit(sht_dev, cmd, sizeof(cmd), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // typical measurement time < 15ms

    err = i2c_master_receive(sht_dev, buf, sizeof(buf), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    if (sht_crc8(&buf[0], 2) != buf[2] || sht_crc8(&buf[3], 2) != buf[5]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t raw_t = (uint16_t)((buf[0] << 8) | buf[1]);
    uint16_t raw_rh = (uint16_t)((buf[3] << 8) | buf[4]);

    float t = -45.0f + 175.0f * ((float)raw_t / 65535.0f);
    float rh = 100.0f * ((float)raw_rh / 65535.0f);

    out->seq = seq;
    out->ts_ms = (uint32_t)esp_log_timestamp();
    out->temp_c = t;
    out->rel_humidity = rh;
    return ESP_OK;
}

static void poll_timer_cb(void *arg) {
    (void)arg;

    static uint32_t seq = 0;
    poll_evt_t evt = {
        .seq = seq++,
        .ts_ms = (uint32_t)esp_log_timestamp(),
    };

    // Timer callback should be short: just enqueue.
    (void)xQueueSend(poll_queue, &evt, 0);
}

static void i2c_poll_task(void *arg) {
    (void)arg;
    poll_evt_t evt;

    while (true) {
        if (xQueueReceive(poll_queue, &evt, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        sample_t s = {0};
        esp_err_t err = sht3x_read(&s, evt.seq);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "poll seq=%" PRIu32 " read failed: %s", evt.seq, esp_err_to_name(err));
            continue;
        }

        (void)xQueueSend(sample_queue, &s, pdMS_TO_TICKS(10));
    }
}

static void avg_task(void *arg) {
    (void)arg;

    float window[ROLLING_WINDOW] = {0};
    size_t idx = 0;
    size_t count = 0;
    float sum = 0.0f;

    sample_t s;
    while (true) {
        if (xQueueReceive(sample_queue, &s, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (count < ROLLING_WINDOW) {
            window[idx] = s.temp_c;
            sum += s.temp_c;
            count++;
        } else {
            sum -= window[idx];
            window[idx] = s.temp_c;
            sum += s.temp_c;
        }
        idx = (idx + 1) % ROLLING_WINDOW;

        float avg = sum / (float)count;
        ESP_LOGI(TAG,
                 "seq=%" PRIu32 " T=%.2fC RH=%.1f%% avg(%" PRIu32 ")=%.2fC",
                 s.seq,
                 (double)s.temp_c,
                 (double)s.rel_humidity,
                 (uint32_t)count,
                 (double)avg);
    }
}

static void i2c_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 8,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &i2c_bus));

    // Quick probe (optional) + attach device handle.
    esp_err_t probe = i2c_master_probe(i2c_bus, SHT3X_ADDR, I2C_TIMEOUT_MS);
    if (probe != ESP_OK) {
        ESP_LOGW(TAG, "probe addr=0x%02x failed: %s (wiring/sensor?)", SHT3X_ADDR, esp_err_to_name(probe));
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHT3X_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_cfg, &sht_dev));
}

void app_main(void) {
    ESP_LOGI(TAG, "starting (I2C sda=%d scl=%d addr=0x%02x)", (int)I2C_SDA_GPIO, (int)I2C_SCL_GPIO, SHT3X_ADDR);

    poll_queue = xQueueCreate(16, sizeof(poll_evt_t));
    sample_queue = xQueueCreate(16, sizeof(sample_t));
    if (poll_queue == NULL || sample_queue == NULL) {
        ESP_LOGE(TAG, "failed to create queues");
        return;
    }

    i2c_init();

    ESP_ERROR_CHECK(esp_timer_init());

    esp_timer_handle_t t = NULL;
    const esp_timer_create_args_t t_args = {
        .callback = &poll_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "poll_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&t_args, &t));
    ESP_ERROR_CHECK(esp_timer_start_periodic(t, POLL_PERIOD_MS * 1000ULL));

    (void)xTaskCreate(i2c_poll_task, "i2c_poll", 4096, NULL, 10, NULL);
    (void)xTaskCreate(avg_task, "avg", 4096, NULL, 10, NULL);

    ESP_LOGI(TAG,
             "started: polling every %" PRIu32 "ms, rolling window=%" PRIu32,
             (uint32_t)POLL_PERIOD_MS,
             (uint32_t)ROLLING_WINDOW);
}
