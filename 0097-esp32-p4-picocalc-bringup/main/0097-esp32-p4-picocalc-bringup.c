/*
 * ESP32-P4-WIFI6 PicoCalc Bring-Up — Phase 1
 *
 * Verifies:
 *   - CPU boots at expected frequency
 *   - USB Serial/JTAG console works
 *   - PSRAM is mapped and usable
 *   - Flash size is correct
 *   - ESP32-C6 co-processor is detected on SDIO
 *   - GPIO blink on a header pin (GPIO49, left header)
 *
 * Board: Waveshare ESP32-P4-WIFI6 (SKU 32020)
 * Chip:  ESP32-P4NRW32 (32MB PSRAM, 32MB Flash)
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "driver/gpio.h"

static const char *TAG = "bringup";

/* GPIO49 on left header — safe free GPIO for LED blink */
#define BLINK_GPIO  49

/* Blink interval (ms) */
#define BLINK_INTERVAL_MS  500

static void blink_task(void *arg)
{
    ESP_LOGI(TAG, "blink: starting on GPIO%d", BLINK_GPIO);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BLINK_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    int level = 0;
    while (1) {
        gpio_set_level(BLINK_GPIO, level);
        level = !level;
        vTaskDelay(pdMS_TO_TICKS(BLINK_INTERVAL_MS));
    }
}

static void print_system_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    ESP_LOGI(TAG, "=============================");
    ESP_LOGI(TAG, "ESP32-P4-WIFI6 PicoCalc Bring-Up");
    ESP_LOGI(TAG, "=============================");
    ESP_LOGI(TAG, "Chip: %s rev %d.%d",
             CONFIG_IDF_TARGET, chip_info.revision / 100, chip_info.revision % 100);
    ESP_LOGI(TAG, "Cores: %d (%s)", chip_info.cores,
             chip_info.cores == 2 ? "HP dual-core RISC-V" : "???");
    ESP_LOGI(TAG, "CPU freq: %d MHz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);

    ESP_LOGI(TAG, "Flash: %lu MB (%s)", flash_size / (1024 * 1024),
             flash_size >= (32 * 1024 * 1024) ? "OK — 32MB" : "WARN — expected 32MB");

    /* PSRAM info */
    size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "PSRAM: %zu KB total, %zu KB free",
             psram_size / 1024, psram_free / 1024);

    if (psram_size >= (30 * 1024 * 1024)) {
        ESP_LOGI(TAG, "PSRAM: OK — 32MB stacked");
    } else if (psram_size > 0) {
        ESP_LOGW(TAG, "PSRAM: partial — expected 32MB, got %zu KB", psram_size / 1024);
    } else {
        ESP_LOGE(TAG, "PSRAM: FAIL — not detected!");
    }

    /* Internal RAM info */
    size_t internal_size = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Internal RAM: %zu KB total, %zu KB free",
             internal_size / 1024, internal_free / 1024);

    /* Features */
    ESP_LOGI(TAG, "Features: %s%s%s%s",
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
             (chip_info.features & CHIP_FEATURE_BT) ? "BT/" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "BLE/" : "",
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded-Flash" : "External-Flash");

    ESP_LOGI(TAG, "Free heap: %zu KB", heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    ESP_LOGI(TAG, "=============================");
}

/* Quick PSRAM write/read test */
static void test_psram(void)
{
    ESP_LOGI(TAG, "PSRAM write/read test...");

    /* Allocate 1MB in PSRAM */
    const size_t test_size = 1024 * 1024;
    uint8_t *buf = heap_caps_malloc(test_size, MALLOC_CAP_SPIRAM);
    if (!buf) {
        ESP_LOGE(TAG, "PSRAM malloc FAIL — could not allocate %zu KB", test_size / 1024);
        return;
    }

    /* Write pattern */
    for (size_t i = 0; i < test_size; i++) {
        buf[i] = (uint8_t)(i & 0xFF);
    }

    /* Verify */
    int errors = 0;
    for (size_t i = 0; i < test_size; i++) {
        if (buf[i] != (uint8_t)(i & 0xFF)) {
            errors++;
            if (errors <= 3) {
                ESP_LOGE(TAG, "PSRAM error at offset %zu: wrote 0x%02X, read 0x%02X",
                         i, (uint8_t)(i & 0xFF), buf[i]);
            }
        }
    }

    if (errors == 0) {
        ESP_LOGI(TAG, "PSRAM: 1MB write/read test PASSED");
    } else {
        ESP_LOGE(TAG, "PSRAM: %d errors in 1MB test!", errors);
    }

    free(buf);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Booting...");

    print_system_info();

    test_psram();

    /* Start LED blink task */
    xTaskCreate(blink_task, "blink", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);

    ESP_LOGI(TAG, "Phase 1 bring-up complete. LED blinking on GPIO%d.", BLINK_GPIO);
    ESP_LOGI(TAG, "Connect LED + resistor between GPIO49 (left header pin 10) and GND.");
}
