/*
 * Tutorial 0016: tester state machine (single writer).
 *
 * - app_main consumes CtrlEvent and calls tester_state_apply_event()
 * - UI reads via tester_state_snapshot()
 */

#include "signal_state.h"

#include <inttypes.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"

#include "esp_log.h"

#include "control_plane.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

static tester_state_t s_state = {};
static portMUX_TYPE s_state_mux = portMUX_INITIALIZER_UNLOCKED;

static gpio_num_t pin_num_from_active(int active_pin) {
    return (active_pin == 1) ? GPIO_NUM_1 : GPIO_NUM_2;
}

static void safe_reset_pin(gpio_num_t pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    (void)gpio_pullup_dis(pin);
    (void)gpio_pulldown_dis(pin);
    (void)gpio_set_intr_type(pin, GPIO_INTR_DISABLE);
}

static void apply_config_locked(void) {
    // Always keep both pins in a safe base state, then configure the active pin only.
    gpio_tx_stop();
    gpio_rx_disable();

    safe_reset_pin(GPIO_NUM_1);
    safe_reset_pin(GPIO_NUM_2);

    const gpio_num_t pin = pin_num_from_active(s_state.active_pin);

    if (s_state.mode == TesterMode::Idle) {
        return;
    }
    if (s_state.mode == TesterMode::Tx) {
        gpio_tx_apply(pin, s_state.tx);
        return;
    }
    if (s_state.mode == TesterMode::Rx) {
        gpio_rx_configure(pin, s_state.rx_edges, s_state.rx_pull);
        return;
    }
}

static const char *mode_str(TesterMode m) {
    switch (m) {
        case TesterMode::Idle:
            return "IDLE";
        case TesterMode::Tx:
            return "TX";
        case TesterMode::Rx:
            return "RX";
    }
    return "?";
}

static const char *tx_mode_str(TxMode m) {
    switch (m) {
        case TxMode::Stopped:
            return "STOP";
        case TxMode::High:
            return "HIGH";
        case TxMode::Low:
            return "LOW";
        case TxMode::Square:
            return "SQUARE";
        case TxMode::Pulse:
            return "PULSE";
    }
    return "?";
}

static const char *rx_edges_str(RxEdgeMode m) {
    switch (m) {
        case RxEdgeMode::Rising:
            return "RISING";
        case RxEdgeMode::Falling:
            return "FALLING";
        case RxEdgeMode::Both:
            return "BOTH";
    }
    return "?";
}

static const char *rx_pull_str(RxPullMode p) {
    switch (p) {
        case RxPullMode::None:
            return "NONE";
        case RxPullMode::Up:
            return "UP";
        case RxPullMode::Down:
            return "DOWN";
    }
    return "?";
}

void tester_state_init(void) {
    portENTER_CRITICAL(&s_state_mux);
    memset(&s_state, 0, sizeof(s_state));
    s_state.mode = TesterMode::Idle;
    s_state.active_pin = CONFIG_TUTORIAL_0016_DEFAULT_ACTIVE_PIN;
    s_state.tx.mode = TxMode::Stopped;
    s_state.tx.hz = 1000;
    s_state.tx.width_us = 100;
    s_state.tx.period_ms = 10;
    s_state.rx_edges = RxEdgeMode::Both;
    s_state.rx_pull = RxPullMode::Up;
    apply_config_locked();
    portEXIT_CRITICAL(&s_state_mux);

    ESP_LOGI(TAG, "tester init: mode=%s pin=%d", mode_str(s_state.mode), s_state.active_pin);
}

tester_state_t tester_state_snapshot(void) {
    tester_state_t out = {};
    portENTER_CRITICAL(&s_state_mux);
    out = s_state;
    portEXIT_CRITICAL(&s_state_mux);
    return out;
}

static void print_status_locked(void) {
    rx_stats_t rx = gpio_rx_snapshot();
    printf("mode=%s pin=%d (gpio=%d) tx=%s",
           mode_str(s_state.mode),
           s_state.active_pin,
           (int)pin_num_from_active(s_state.active_pin),
           tx_mode_str(s_state.tx.mode));
    if (s_state.tx.mode == TxMode::Square) {
        printf(" hz=%d", s_state.tx.hz);
    } else if (s_state.tx.mode == TxMode::Pulse) {
        printf(" width_us=%d period_ms=%d", s_state.tx.width_us, s_state.tx.period_ms);
    }
    printf(" rx_edges=%s rx_pull=%s rx_edges_total=%" PRIu32 " rises=%" PRIu32 " falls=%" PRIu32 " last_tick=%" PRIu32 " last_level=%d\n",
           rx_edges_str(s_state.rx_edges),
           rx_pull_str(s_state.rx_pull),
           rx.edges, rx.rises, rx.falls,
           rx.last_tick,
           rx.last_level);
}

void tester_state_apply_event(const CtrlEvent &ev) {
    portENTER_CRITICAL(&s_state_mux);
    switch (ev.type) {
        case CtrlType::SetMode:
            if (ev.arg0 == 0) s_state.mode = TesterMode::Idle;
            else if (ev.arg0 == 1) s_state.mode = TesterMode::Tx;
            else if (ev.arg0 == 2) s_state.mode = TesterMode::Rx;
            apply_config_locked();
            break;
        case CtrlType::SetPin:
            if (ev.arg0 == 1 || ev.arg0 == 2) {
                s_state.active_pin = (int)ev.arg0;
                apply_config_locked();
            }
            break;
        case CtrlType::Status:
            print_status_locked();
            break;
        case CtrlType::TxHigh:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::High;
            apply_config_locked();
            break;
        case CtrlType::TxLow:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::Low;
            apply_config_locked();
            break;
        case CtrlType::TxStop:
            s_state.tx.mode = TxMode::Stopped;
            if (s_state.mode == TesterMode::Tx) {
                s_state.mode = TesterMode::Idle;
            }
            apply_config_locked();
            break;
        case CtrlType::TxSquare:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::Square;
            s_state.tx.hz = (int)ev.arg0;
            apply_config_locked();
            break;
        case CtrlType::TxPulse:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::Pulse;
            s_state.tx.width_us = (int)ev.arg0;
            s_state.tx.period_ms = (int)ev.arg1;
            apply_config_locked();
            break;
        case CtrlType::RxEdges:
            if (ev.arg0 == 0) s_state.rx_edges = RxEdgeMode::Rising;
            else if (ev.arg0 == 1) s_state.rx_edges = RxEdgeMode::Falling;
            else if (ev.arg0 == 2) s_state.rx_edges = RxEdgeMode::Both;
            if (s_state.mode == TesterMode::Rx) {
                apply_config_locked();
            }
            break;
        case CtrlType::RxPull:
            if (ev.arg0 == 0) s_state.rx_pull = RxPullMode::None;
            else if (ev.arg0 == 1) s_state.rx_pull = RxPullMode::Up;
            else if (ev.arg0 == 2) s_state.rx_pull = RxPullMode::Down;
            if (s_state.mode == TesterMode::Rx) {
                apply_config_locked();
            }
            break;
        case CtrlType::RxReset:
            gpio_rx_reset();
            break;
    }
    portEXIT_CRITICAL(&s_state_mux);
}


