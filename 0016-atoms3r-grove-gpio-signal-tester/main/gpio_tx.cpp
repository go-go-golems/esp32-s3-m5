/*
 * Tutorial 0016: GPIO TX generator (GPIO-owned mode).
 *
 * Uses esp_timer to avoid a busy-wait toggling task.
 */

#include "gpio_tx.h"

#include <inttypes.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

static gpio_num_t s_tx_pin = GPIO_NUM_NC;
static tx_config_t s_cfg = {};
static volatile int s_level = 0;

static esp_timer_handle_t s_square_timer = nullptr;
static esp_timer_handle_t s_pulse_periodic = nullptr;
static esp_timer_handle_t s_pulse_low_oneshot = nullptr;

static void stop_timers(void) {
    if (s_square_timer) {
        (void)esp_timer_stop(s_square_timer);
    }
    if (s_pulse_periodic) {
        (void)esp_timer_stop(s_pulse_periodic);
    }
    if (s_pulse_low_oneshot) {
        (void)esp_timer_stop(s_pulse_low_oneshot);
    }
}

static void square_cb(void *arg) {
    (void)arg;
    if (s_tx_pin == GPIO_NUM_NC) return;
    int lvl = __atomic_load_n(&s_level, __ATOMIC_RELAXED);
    lvl = lvl ? 0 : 1;
    __atomic_store_n(&s_level, lvl, __ATOMIC_RELAXED);
    gpio_set_level(s_tx_pin, lvl);
}

static void pulse_low_cb(void *arg) {
    (void)arg;
    if (s_tx_pin == GPIO_NUM_NC) return;
    gpio_set_level(s_tx_pin, 0);
    __atomic_store_n(&s_level, 0, __ATOMIC_RELAXED);
}

static void pulse_periodic_cb(void *arg) {
    (void)arg;
    if (s_tx_pin == GPIO_NUM_NC) return;
    gpio_set_level(s_tx_pin, 1);
    __atomic_store_n(&s_level, 1, __ATOMIC_RELAXED);
    // schedule low edge after width_us
    if (s_pulse_low_oneshot && s_cfg.width_us > 0) {
        (void)esp_timer_stop(s_pulse_low_oneshot);
        (void)esp_timer_start_once(s_pulse_low_oneshot, (uint64_t)s_cfg.width_us);
    }
}

static void ensure_timer(esp_timer_handle_t &h, const char *name, esp_timer_cb_t cb) {
    if (h) return;
    esp_timer_create_args_t args = {};
    args.callback = cb;
    args.arg = nullptr;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = name;
    ESP_ERROR_CHECK(esp_timer_create(&args, &h));
}

void gpio_tx_stop(void) {
    stop_timers();
    if (s_tx_pin != GPIO_NUM_NC) {
        gpio_set_level(s_tx_pin, 0);
        gpio_set_direction(s_tx_pin, GPIO_MODE_INPUT);
        (void)gpio_pullup_dis(s_tx_pin);
        (void)gpio_pulldown_dis(s_tx_pin);
        ESP_LOGI(TAG, "tx stopped: gpio=%d", (int)s_tx_pin);
    }
    s_tx_pin = GPIO_NUM_NC;
    s_cfg = {};
    __atomic_store_n(&s_level, 0, __ATOMIC_RELAXED);
}

void gpio_tx_apply(gpio_num_t pin, const tx_config_t &cfg) {
    if (pin != s_tx_pin && s_tx_pin != GPIO_NUM_NC) {
        gpio_tx_stop();
    }

    s_tx_pin = pin;
    s_cfg = cfg;

    // Ensure pin is output for TX modes (except Stopped).
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    (void)gpio_pullup_dis(pin);
    (void)gpio_pulldown_dis(pin);

    stop_timers();

    if (cfg.mode == TxMode::High) {
        __atomic_store_n(&s_level, 1, __ATOMIC_RELAXED);
        gpio_set_level(pin, 1);
        ESP_LOGI(TAG, "tx high: gpio=%d", (int)pin);
        return;
    }
    if (cfg.mode == TxMode::Low) {
        __atomic_store_n(&s_level, 0, __ATOMIC_RELAXED);
        gpio_set_level(pin, 0);
        ESP_LOGI(TAG, "tx low: gpio=%d", (int)pin);
        return;
    }
    if (cfg.mode == TxMode::Square) {
        if (cfg.hz <= 0) {
            ESP_LOGW(TAG, "tx square invalid hz=%d (stopping)", cfg.hz);
            gpio_tx_stop();
            return;
        }
        ensure_timer(s_square_timer, "tx_sq", &square_cb);
        const uint64_t half_period_us = (uint64_t)(500000ULL / (uint64_t)cfg.hz);
        __atomic_store_n(&s_level, 0, __ATOMIC_RELAXED);
        gpio_set_level(pin, 0);
        ESP_ERROR_CHECK(esp_timer_start_periodic(s_square_timer, half_period_us > 0 ? half_period_us : 1));
        ESP_LOGI(TAG, "tx square: gpio=%d hz=%d half_period_us=%" PRIu64, (int)pin, cfg.hz, half_period_us);
        return;
    }
    if (cfg.mode == TxMode::Pulse) {
        if (cfg.width_us <= 0 || cfg.period_ms <= 0) {
            ESP_LOGW(TAG, "tx pulse invalid width_us=%d period_ms=%d (stopping)", cfg.width_us, cfg.period_ms);
            gpio_tx_stop();
            return;
        }
        ensure_timer(s_pulse_periodic, "tx_pulse", &pulse_periodic_cb);
        ensure_timer(s_pulse_low_oneshot, "tx_pulse_lo", &pulse_low_cb);
        __atomic_store_n(&s_level, 0, __ATOMIC_RELAXED);
        gpio_set_level(pin, 0);
        ESP_ERROR_CHECK(esp_timer_start_periodic(s_pulse_periodic, (uint64_t)cfg.period_ms * 1000ULL));
        ESP_LOGI(TAG, "tx pulse: gpio=%d width_us=%d period_ms=%d", (int)pin, cfg.width_us, cfg.period_ms);
        return;
    }

    // Stopped or unknown.
    gpio_tx_stop();
}


