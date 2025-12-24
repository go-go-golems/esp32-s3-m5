/*
 * Tutorial 0013 (AtomS3R GIF console):
 * Debug helper: UART RX heartbeat by counting GPIO edges on the configured UART RX GPIO.
 *
 * IMPORTANT: This should not "steal" the pin away from UART. Do NOT call gpio_config()
 * on the UART RX pin; only enable interrupt type + ISR handler.
 */

#include <inttypes.h>
#include <stdint.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#if CONFIG_TUTORIAL_0013_CONSOLE_RX_HEARTBEAT_ENABLE
static const char *TAG = "atoms3r_gif_console";
static volatile uint32_t s_uart_rx_edges = 0;
static volatile uint32_t s_uart_rx_rises = 0;
static volatile uint32_t s_uart_rx_falls = 0;

static void IRAM_ATTR uart_rx_edge_isr(void *arg) {
    const int gpio = (int)(intptr_t)arg;
    int level = gpio_get_level((gpio_num_t)gpio);
    __atomic_fetch_add(&s_uart_rx_edges, 1, __ATOMIC_RELAXED);
    if (level) {
        __atomic_fetch_add(&s_uart_rx_rises, 1, __ATOMIC_RELAXED);
    } else {
        __atomic_fetch_add(&s_uart_rx_falls, 1, __ATOMIC_RELAXED);
    }
}

static void uart_rx_heartbeat_task(void *arg) {
    const int rx_gpio = (int)(intptr_t)arg;
    uint32_t last_edges = 0;
    uint32_t last_rises = 0;
    uint32_t last_falls = 0;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0013_CONSOLE_RX_HEARTBEAT_INTERVAL_MS));

        const uint32_t edges = s_uart_rx_edges;
        const uint32_t rises = s_uart_rx_rises;
        const uint32_t falls = s_uart_rx_falls;

        const uint32_t d_edges = edges - last_edges;
        const uint32_t d_rises = rises - last_rises;
        const uint32_t d_falls = falls - last_falls;

        last_edges = edges;
        last_rises = rises;
        last_falls = falls;

        ESP_LOGI(TAG, "uart rx heartbeat: rx_gpio=%d level=%d edges=%" PRIu32 " (d=%" PRIu32 ") rises=%" PRIu32 " falls=%" PRIu32,
                 rx_gpio,
                 gpio_get_level((gpio_num_t)rx_gpio),
                 edges, d_edges,
                 d_rises, d_falls);
    }
}
#endif

void uart_rx_heartbeat_start(void) {
#if CONFIG_TUTORIAL_0013_CONSOLE_RX_HEARTBEAT_ENABLE
    const int rx_gpio = CONFIG_TUTORIAL_0013_CONSOLE_UART_RX_GPIO;

    esp_err_t err = gpio_set_intr_type((gpio_num_t)rx_gpio, GPIO_INTR_ANYEDGE);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "uart rx heartbeat: gpio_set_intr_type failed: rx_gpio=%d err=%s",
                 rx_gpio, esp_err_to_name(err));
        return;
    }
    (void)gpio_pullup_en((gpio_num_t)rx_gpio);
    (void)gpio_pulldown_dis((gpio_num_t)rx_gpio);

    // ISR service may already be installed by other modules; INVALID_STATE is OK.
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "uart rx heartbeat: gpio_install_isr_service failed: %s", esp_err_to_name(err));
        return;
    }
    err = gpio_isr_handler_add((gpio_num_t)rx_gpio, uart_rx_edge_isr, (void *)(intptr_t)rx_gpio);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "uart rx heartbeat: gpio_isr_handler_add failed: rx_gpio=%d err=%s",
                 rx_gpio, esp_err_to_name(err));
        return;
    }

    static TaskHandle_t s_uart_rx_task = nullptr;
    if (!s_uart_rx_task) {
        BaseType_t ok = xTaskCreatePinnedToCore(
            uart_rx_heartbeat_task,
            "uart_rx_hb",
            4096,
            (void *)(intptr_t)rx_gpio,
            1,
            &s_uart_rx_task,
            tskNO_AFFINITY);
        if (ok != pdPASS) {
            ESP_LOGW(TAG, "uart rx heartbeat: failed to create task");
            s_uart_rx_task = nullptr;
            return;
        }
    }

    ESP_LOGI(TAG, "uart rx heartbeat enabled: rx_gpio=%d interval_ms=%d",
             rx_gpio,
             CONFIG_TUTORIAL_0013_CONSOLE_RX_HEARTBEAT_INTERVAL_MS);
#endif
}


