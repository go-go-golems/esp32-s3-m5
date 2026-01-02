/*
 * Tutorial 0016: UART peripheral tester for GROVE pins (GPIO1/GPIO2).
 *
 * - TX mode: repeat-send a configured payload with inter-send delay.
 * - RX mode: buffer incoming bytes into a stream buffer; drained explicitly via REPL verb.
 *
 * Control plane is USB Serial/JTAG (manual_repl); UART is the DUT interface.
 */

#include "uart_tester.h"

#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

static constexpr uart_port_t kUart = UART_NUM_1;

// RX stream buffer sizing.
static constexpr size_t kRxBufBytes = 4096;
static constexpr size_t kRxTriggerBytes = 1;

// Shared state (protected by mux).
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static bool s_running = false;
static bool s_tx_mode = false;
static bool s_rx_mode = false;

static int s_delay_ms = 1000;
static char s_payload[64] = {};
static size_t s_payload_len = 0;
static bool s_tx_enabled = false;

static StreamBufferHandle_t s_rx_sb = nullptr;

static TaskHandle_t s_tx_task = nullptr;
static TaskHandle_t s_rx_task = nullptr;

static volatile uint32_t s_tx_bytes = 0;
static volatile uint32_t s_rx_bytes = 0;
static volatile uint32_t s_rx_dropped = 0;

static void pins_for_map(UartMap map, gpio_num_t *rx, gpio_num_t *tx) {
    if (!rx || !tx) return;
    if (map == UartMap::Swapped) {
        *rx = GPIO_NUM_2;
        *tx = GPIO_NUM_1;
        return;
    }
    *rx = GPIO_NUM_1;
    *tx = GPIO_NUM_2;
}

static esp_err_t uart_install_and_config(const uart_tester_config_t &cfg) {
    uart_config_t uc = {};
    uc.baud_rate = cfg.baud;
    uc.data_bits = UART_DATA_8_BITS;
    uc.parity = UART_PARITY_DISABLE;
    uc.stop_bits = UART_STOP_BITS_1;
    uc.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uc.source_clk = UART_SCLK_DEFAULT;

    ESP_RETURN_ON_ERROR(uart_param_config(kUart, &uc), TAG, "uart_param_config");

    gpio_num_t rx = GPIO_NUM_1;
    gpio_num_t tx = GPIO_NUM_2;
    pins_for_map(cfg.map, &rx, &tx);
    ESP_RETURN_ON_ERROR(uart_set_pin(kUart, (int)tx, (int)rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "uart_set_pin");

    // Install driver. If already installed, delete first.
    // Keep RX/TX driver buffers modest; we additionally buffer RX in our own stream buffer.
    (void)uart_driver_delete(kUart);
    ESP_RETURN_ON_ERROR(uart_driver_install(kUart, 1024, 1024, 0, nullptr, 0), TAG, "uart_driver_install");

    ESP_LOGI(TAG, "uart configured: port=%d baud=%d map=%s (rx=%d tx=%d)",
             (int)kUart,
             cfg.baud,
             (cfg.map == UartMap::Swapped) ? "swapped" : "normal",
             (int)rx,
             (int)tx);

    return ESP_OK;
}

static void tx_task(void *arg) {
    (void)arg;

    while (true) {
        bool running = false;
        bool enabled = false;
        int delay_ms = 0;
        char payload[sizeof(s_payload)] = {};
        size_t payload_len = 0;

        portENTER_CRITICAL(&s_mux);
        running = s_running && s_tx_mode;
        enabled = s_tx_enabled;
        delay_ms = s_delay_ms;
        payload_len = s_payload_len;
        if (payload_len > 0) {
            memcpy(payload, s_payload, payload_len);
        }
        portEXIT_CRITICAL(&s_mux);

        if (!running) {
            break;
        }
        if (!enabled || payload_len == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        int wrote = uart_write_bytes(kUart, payload, (size_t)payload_len);
        if (wrote > 0) {
            __atomic_fetch_add(&s_tx_bytes, (uint32_t)wrote, __ATOMIC_RELAXED);
        }

        if (delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        } else {
            // Best-effort back-to-back, but avoid starving other tasks.
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    vTaskDelete(nullptr);
}

static void rx_task(void *arg) {
    (void)arg;

    uint8_t buf[64] = {};

    while (true) {
        bool running = false;
        portENTER_CRITICAL(&s_mux);
        running = s_running && s_rx_mode;
        portEXIT_CRITICAL(&s_mux);
        if (!running) {
            break;
        }

        int got = uart_read_bytes(kUart, buf, sizeof(buf), pdMS_TO_TICKS(100));
        if (got <= 0) {
            continue;
        }

        __atomic_fetch_add(&s_rx_bytes, (uint32_t)got, __ATOMIC_RELAXED);

        if (s_rx_sb) {
            size_t sent = xStreamBufferSend(s_rx_sb, (const void *)buf, (size_t)got, 0);
            if (sent < (size_t)got) {
                __atomic_fetch_add(&s_rx_dropped, (uint32_t)((size_t)got - sent), __ATOMIC_RELAXED);
            }
        } else {
            __atomic_fetch_add(&s_rx_dropped, (uint32_t)got, __ATOMIC_RELAXED);
        }
    }

    vTaskDelete(nullptr);
}

static void stop_tasks_and_driver(void) {
    // Mark stopped first so tasks can exit.
    portENTER_CRITICAL(&s_mux);
    s_running = false;
    s_tx_mode = false;
    s_rx_mode = false;
    s_tx_enabled = false;
    s_payload_len = 0;
    s_payload[0] = '\0';
    portEXIT_CRITICAL(&s_mux);

    // Best-effort delete tasks.
    if (s_tx_task) {
        vTaskDelete(s_tx_task);
        s_tx_task = nullptr;
    }
    if (s_rx_task) {
        vTaskDelete(s_rx_task);
        s_rx_task = nullptr;
    }

    // Best-effort delete driver.
    (void)uart_driver_delete(kUart);

    if (s_rx_sb) {
        vStreamBufferDelete(s_rx_sb);
        s_rx_sb = nullptr;
    }
}

void uart_tester_stop(void) {
    stop_tasks_and_driver();
    ESP_LOGI(TAG, "uart tester stopped");
}

esp_err_t uart_tester_start_tx(const uart_tester_config_t &cfg, const char *payload, int delay_ms) {
    uart_tester_stop();

    ESP_RETURN_ON_ERROR(uart_install_and_config(cfg), TAG, "install/config");

    s_rx_sb = xStreamBufferCreate(kRxBufBytes, kRxTriggerBytes);
    if (!s_rx_sb) {
        ESP_LOGE(TAG, "failed to create rx stream buffer (%u bytes)", (unsigned)kRxBufBytes);
        uart_tester_stop();
        return ESP_ERR_NO_MEM;
    }

    // Reset counters for this run.
    __atomic_store_n(&s_tx_bytes, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_rx_bytes, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_rx_dropped, 0, __ATOMIC_RELAXED);

    portENTER_CRITICAL(&s_mux);
    s_running = true;
    s_tx_mode = true;
    s_rx_mode = false;
    s_delay_ms = (delay_ms >= 0) ? delay_ms : 0;
    s_payload[0] = '\0';
    s_payload_len = 0;
    if (payload && *payload) {
        strlcpy(s_payload, payload, sizeof(s_payload));
        s_payload_len = strnlen(s_payload, sizeof(s_payload) - 1);
        s_tx_enabled = true;
    } else {
        s_tx_enabled = false;
    }
    portEXIT_CRITICAL(&s_mux);

    BaseType_t ok = xTaskCreatePinnedToCore(tx_task, "uart_tx", 4096, nullptr, 1, &s_tx_task, tskNO_AFFINITY);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create uart tx task");
        uart_tester_stop();
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "uart tx started: delay_ms=%d payload_len=%u", s_delay_ms, (unsigned)s_payload_len);
    return ESP_OK;
}

esp_err_t uart_tester_start_rx(const uart_tester_config_t &cfg) {
    uart_tester_stop();

    ESP_RETURN_ON_ERROR(uart_install_and_config(cfg), TAG, "install/config");

    s_rx_sb = xStreamBufferCreate(kRxBufBytes, kRxTriggerBytes);
    if (!s_rx_sb) {
        ESP_LOGE(TAG, "failed to create rx stream buffer (%u bytes)", (unsigned)kRxBufBytes);
        uart_tester_stop();
        return ESP_ERR_NO_MEM;
    }

    // Reset counters for this run.
    __atomic_store_n(&s_tx_bytes, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_rx_bytes, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_rx_dropped, 0, __ATOMIC_RELAXED);

    portENTER_CRITICAL(&s_mux);
    s_running = true;
    s_tx_mode = false;
    s_rx_mode = true;
    portEXIT_CRITICAL(&s_mux);

    BaseType_t ok = xTaskCreatePinnedToCore(rx_task, "uart_rx", 4096, nullptr, 1, &s_rx_task, tskNO_AFFINITY);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "failed to create uart rx task");
        uart_tester_stop();
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "uart rx started");
    return ESP_OK;
}

void uart_tester_tx_set(const char *payload, int delay_ms) {
    portENTER_CRITICAL(&s_mux);
    s_delay_ms = (delay_ms >= 0) ? delay_ms : 0;
    s_payload[0] = '\0';
    s_payload_len = 0;
    if (payload && *payload) {
        strlcpy(s_payload, payload, sizeof(s_payload));
        s_payload_len = strnlen(s_payload, sizeof(s_payload) - 1);
        s_tx_enabled = true;
    } else {
        s_tx_enabled = false;
    }
    portEXIT_CRITICAL(&s_mux);
}

void uart_tester_tx_stop(void) {
    portENTER_CRITICAL(&s_mux);
    s_tx_enabled = false;
    portEXIT_CRITICAL(&s_mux);
}

void uart_tester_rx_clear(void) {
    if (s_rx_sb) {
        xStreamBufferReset(s_rx_sb);
    }
    __atomic_store_n(&s_rx_dropped, 0, __ATOMIC_RELAXED);
}

size_t uart_tester_rx_drain(uint8_t *out, size_t max_bytes) {
    if (!out || max_bytes == 0 || !s_rx_sb) return 0;
    return xStreamBufferReceive(s_rx_sb, (void *)out, max_bytes, 0);
}

uart_tester_stats_t uart_tester_stats_snapshot(void) {
    uart_tester_stats_t s = {};
    s.tx_bytes_total = __atomic_load_n(&s_tx_bytes, __ATOMIC_RELAXED);
    s.rx_bytes_total = __atomic_load_n(&s_rx_bytes, __ATOMIC_RELAXED);
    s.rx_buf_dropped = __atomic_load_n(&s_rx_dropped, __ATOMIC_RELAXED);
    s.rx_buf_used = s_rx_sb ? (uint32_t)xStreamBufferBytesAvailable(s_rx_sb) : 0;
    return s;
}


