/*
 * Tutorial 0016: GPIO RX edge counter (GPIO-owned mode).
 *
 * This is intentionally simple and deterministic:
 * - configure pin as input + pull
 * - attach GPIO ISR
 * - ISR only increments counters and stores last tick/level
 */

#include "gpio_rx.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

static gpio_num_t s_rx_pin = GPIO_NUM_NC;

static volatile uint32_t s_edges = 0;
static volatile uint32_t s_rises = 0;
static volatile uint32_t s_falls = 0;
static volatile uint32_t s_last_tick = 0;
static volatile int s_last_level = 0;

static void IRAM_ATTR rx_isr(void *arg) {
    const int gpio = (int)(intptr_t)arg;
    int level = gpio_get_level((gpio_num_t)gpio);
    __atomic_fetch_add(&s_edges, 1, __ATOMIC_RELAXED);
    if (level) {
        __atomic_fetch_add(&s_rises, 1, __ATOMIC_RELAXED);
    } else {
        __atomic_fetch_add(&s_falls, 1, __ATOMIC_RELAXED);
    }
    __atomic_store_n(&s_last_tick, (uint32_t)xTaskGetTickCountFromISR(), __ATOMIC_RELAXED);
    __atomic_store_n(&s_last_level, level, __ATOMIC_RELAXED);
}

static gpio_int_type_t edge_mode_to_intr(RxEdgeMode m) {
    switch (m) {
        case RxEdgeMode::Rising:
            return GPIO_INTR_POSEDGE;
        case RxEdgeMode::Falling:
            return GPIO_INTR_NEGEDGE;
        case RxEdgeMode::Both:
            return GPIO_INTR_ANYEDGE;
    }
    return GPIO_INTR_ANYEDGE;
}

static void apply_pull(gpio_num_t pin, RxPullMode pull) {
    // Use best-effort pull controls; explicit pulls help avoid floating inputs.
    if (pull == RxPullMode::Up) {
        (void)gpio_pullup_en(pin);
        (void)gpio_pulldown_dis(pin);
    } else if (pull == RxPullMode::Down) {
        (void)gpio_pulldown_en(pin);
        (void)gpio_pullup_dis(pin);
    } else {
        (void)gpio_pullup_dis(pin);
        (void)gpio_pulldown_dis(pin);
    }
}

void gpio_rx_disable(void) {
    if (s_rx_pin == GPIO_NUM_NC) {
        return;
    }

    // Best-effort: detach ISR; keep pin in a safe input state.
    (void)gpio_isr_handler_remove(s_rx_pin);
    (void)gpio_set_intr_type(s_rx_pin, GPIO_INTR_DISABLE);
    gpio_set_direction(s_rx_pin, GPIO_MODE_INPUT);
    apply_pull(s_rx_pin, RxPullMode::None);

    ESP_LOGI(TAG, "rx disabled: gpio=%d", (int)s_rx_pin);
    s_rx_pin = GPIO_NUM_NC;
}

void gpio_rx_reset(void) {
    __atomic_store_n(&s_edges, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_rises, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_falls, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_last_tick, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&s_last_level, 0, __ATOMIC_RELAXED);
}

void gpio_rx_configure(gpio_num_t pin, RxEdgeMode edges, RxPullMode pull) {
    if (pin != s_rx_pin && s_rx_pin != GPIO_NUM_NC) {
        gpio_rx_disable();
    }

    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << (int)pin;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = (pull == RxPullMode::Up) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io.pull_down_en = (pull == RxPullMode::Down) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io.intr_type = edge_mode_to_intr(edges);
    ESP_ERROR_CHECK(gpio_config(&io));
    apply_pull(pin, pull);

    // ISR service may already be installed by other modules; INVALID_STATE is OK.
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    err = gpio_isr_handler_add(pin, rx_isr, (void *)(intptr_t)pin);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "gpio_isr_handler_add failed: gpio=%d err=%s", (int)pin, esp_err_to_name(err));
        return;
    }

    s_rx_pin = pin;
    ESP_LOGI(TAG, "rx configured: gpio=%d edges=%d pull=%d", (int)pin, (int)edges, (int)pull);
}

rx_stats_t gpio_rx_snapshot(void) {
    rx_stats_t s = {};
    s.edges = __atomic_load_n(&s_edges, __ATOMIC_RELAXED);
    s.rises = __atomic_load_n(&s_rises, __ATOMIC_RELAXED);
    s.falls = __atomic_load_n(&s_falls, __ATOMIC_RELAXED);
    s.last_tick = __atomic_load_n(&s_last_tick, __ATOMIC_RELAXED);
    s.last_level = __atomic_load_n(&s_last_level, __ATOMIC_RELAXED);
    return s;
}



