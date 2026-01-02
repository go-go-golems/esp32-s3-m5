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

static void apply_config_snapshot(const tester_state_t &st) {
    // Always keep both pins in a safe base state, then configure the active pin only.
    gpio_tx_stop();
    gpio_rx_disable();
    uart_tester_stop();

    safe_reset_pin(GPIO_NUM_1);
    safe_reset_pin(GPIO_NUM_2);

    const gpio_num_t pin = pin_num_from_active(st.active_pin);

    if (st.mode == TesterMode::Idle) {
        return;
    }
    if (st.mode == TesterMode::Tx) {
        gpio_tx_apply(pin, st.tx);
        return;
    }
    if (st.mode == TesterMode::Rx) {
        gpio_rx_configure(pin, st.rx_edges, st.rx_pull);
        return;
    }
    if (st.mode == TesterMode::UartTx) {
        uart_tester_config_t cfg = {};
        cfg.baud = st.uart_baud;
        cfg.map = st.uart_map;
        (void)uart_tester_start_tx(cfg, st.uart_tx_token, st.uart_tx_delay_ms);
        if (!st.uart_tx_enabled) {
            uart_tester_tx_stop();
        }
        return;
    }
    if (st.mode == TesterMode::UartRx) {
        uart_tester_config_t cfg = {};
        cfg.baud = st.uart_baud;
        cfg.map = st.uart_map;
        (void)uart_tester_start_rx(cfg);
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
        case TesterMode::UartTx:
            return "UART_TX";
        case TesterMode::UartRx:
            return "UART_RX";
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
    tester_state_t snapshot = {};
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
    s_state.uart_baud = 115200;
    s_state.uart_map = UartMap::Normal;
    s_state.uart_tx_enabled = false;
    s_state.uart_tx_delay_ms = 1000;
    s_state.uart_tx_token[0] = '\0';
    snapshot = s_state;
    portEXIT_CRITICAL(&s_state_mux);

    // IMPORTANT: never call GPIO driver APIs / printf / logging while holding s_state_mux.
    apply_config_snapshot(snapshot);
    ESP_LOGI(TAG, "tester init: mode=%s pin=%d", mode_str(snapshot.mode), snapshot.active_pin);
}

tester_state_t tester_state_snapshot(void) {
    tester_state_t out = {};
    portENTER_CRITICAL(&s_state_mux);
    out = s_state;
    portEXIT_CRITICAL(&s_state_mux);
    return out;
}

static void print_status_snapshot(const tester_state_t &st) {
    rx_stats_t rx = gpio_rx_snapshot();
    uart_tester_stats_t us = uart_tester_stats_snapshot();
    printf("mode=%s pin=%d (gpio=%d) tx=%s",
           mode_str(st.mode),
           st.active_pin,
           (int)pin_num_from_active(st.active_pin),
           tx_mode_str(st.tx.mode));
    if (st.tx.mode == TxMode::Square) {
        printf(" hz=%d", st.tx.hz);
    } else if (st.tx.mode == TxMode::Pulse) {
        printf(" width_us=%d period_ms=%d", st.tx.width_us, st.tx.period_ms);
    }
    printf(" rx_edges=%s rx_pull=%s rx_edges_total=%" PRIu32 " rises=%" PRIu32 " falls=%" PRIu32 " last_tick=%" PRIu32 " last_level=%d\n",
           rx_edges_str(st.rx_edges),
           rx_pull_str(st.rx_pull),
           rx.edges, rx.rises, rx.falls,
           rx.last_tick,
           rx.last_level);

    if (st.mode == TesterMode::UartTx || st.mode == TesterMode::UartRx) {
        printf("uart: baud=%d map=%s tx_en=%d tx_delay_ms=%d tx_token=%s tx_bytes=%" PRIu32 " rx_bytes=%" PRIu32 " rx_buf_used=%" PRIu32 " rx_buf_dropped=%" PRIu32 "\n",
               st.uart_baud,
               (st.uart_map == UartMap::Swapped) ? "swapped" : "normal",
               st.uart_tx_enabled ? 1 : 0,
               st.uart_tx_delay_ms,
               st.uart_tx_token,
               us.tx_bytes_total,
               us.rx_bytes_total,
               us.rx_buf_used,
               us.rx_buf_dropped);
    }
}

void tester_state_apply_event(const CtrlEvent &ev) {
    tester_state_t snapshot = {};
    bool do_apply = false;
    bool do_status = false;
    bool do_uart_rx_get = false;
    bool do_uart_tx_set = false;
    bool do_uart_tx_stop = false;

    portENTER_CRITICAL(&s_state_mux);
    switch (ev.type) {
        case CtrlType::SetMode:
            if (ev.arg0 == 0) s_state.mode = TesterMode::Idle;
            else if (ev.arg0 == 1) s_state.mode = TesterMode::Tx;
            else if (ev.arg0 == 2) s_state.mode = TesterMode::Rx;
            else if (ev.arg0 == 3) s_state.mode = TesterMode::UartTx;
            else if (ev.arg0 == 4) s_state.mode = TesterMode::UartRx;
            do_apply = true;
            break;
        case CtrlType::SetPin:
            if (ev.arg0 == 1 || ev.arg0 == 2) {
                s_state.active_pin = (int)ev.arg0;
                do_apply = true;
            }
            break;
        case CtrlType::Status:
            do_status = true;
            break;
        case CtrlType::TxHigh:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::High;
            do_apply = true;
            break;
        case CtrlType::TxLow:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::Low;
            do_apply = true;
            break;
        case CtrlType::TxStop:
            s_state.tx.mode = TxMode::Stopped;
            if (s_state.mode == TesterMode::Tx) {
                s_state.mode = TesterMode::Idle;
            }
            do_apply = true;
            break;
        case CtrlType::TxSquare:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::Square;
            s_state.tx.hz = (int)ev.arg0;
            do_apply = true;
            break;
        case CtrlType::TxPulse:
            s_state.mode = TesterMode::Tx;
            s_state.tx.mode = TxMode::Pulse;
            s_state.tx.width_us = (int)ev.arg0;
            s_state.tx.period_ms = (int)ev.arg1;
            do_apply = true;
            break;
        case CtrlType::RxEdges:
            if (ev.arg0 == 0) s_state.rx_edges = RxEdgeMode::Rising;
            else if (ev.arg0 == 1) s_state.rx_edges = RxEdgeMode::Falling;
            else if (ev.arg0 == 2) s_state.rx_edges = RxEdgeMode::Both;
            if (s_state.mode == TesterMode::Rx) {
                do_apply = true;
            }
            break;
        case CtrlType::RxPull:
            if (ev.arg0 == 0) s_state.rx_pull = RxPullMode::None;
            else if (ev.arg0 == 1) s_state.rx_pull = RxPullMode::Up;
            else if (ev.arg0 == 2) s_state.rx_pull = RxPullMode::Down;
            if (s_state.mode == TesterMode::Rx) {
                do_apply = true;
            }
            break;
        case CtrlType::RxReset:
            // Safe to call outside the critical section (atomics only), but keep ordering deterministic:
            // we just mark the intent here and do it after we release the mux.
            break;

        case CtrlType::UartBaud:
            if (ev.arg0 > 0) {
                s_state.uart_baud = (int)ev.arg0;
                if (s_state.mode == TesterMode::UartTx || s_state.mode == TesterMode::UartRx) {
                    do_apply = true;
                }
            }
            break;
        case CtrlType::UartMap:
            s_state.uart_map = (ev.arg0 != 0) ? UartMap::Swapped : UartMap::Normal;
            if (s_state.mode == TesterMode::UartTx || s_state.mode == TesterMode::UartRx) {
                do_apply = true;
            }
            break;
        case CtrlType::UartTxStart:
            {
                const bool was_uart_tx = (s_state.mode == TesterMode::UartTx);
                s_state.mode = TesterMode::UartTx;
                s_state.uart_tx_delay_ms = (int)ev.arg0;
                s_state.uart_tx_enabled = true;
                s_state.uart_tx_token[0] = '\0';
                if (ev.str0[0]) {
                    strlcpy(s_state.uart_tx_token, ev.str0, sizeof(s_state.uart_tx_token));
                }
                do_apply = !was_uart_tx;
                do_uart_tx_set = was_uart_tx;
            }
            break;
        case CtrlType::UartTxStop:
            // Stay in UART_TX mode; just disable TX loop.
            {
                const bool was_uart_tx = (s_state.mode == TesterMode::UartTx);
                if (!was_uart_tx) {
                    s_state.mode = TesterMode::UartTx;
                    do_apply = true;  // need to start UART driver/tasks, then stop TX
                }
                s_state.uart_tx_enabled = false;
                do_uart_tx_stop = true;
            }
            break;
        case CtrlType::UartRxGet:
            // Drain/print is done outside the mux (non-deterministic latency and printf).
            if (s_state.mode != TesterMode::UartRx) {
                s_state.mode = TesterMode::UartRx;
                do_apply = true;
            }
            do_uart_rx_get = true;
            break;
        case CtrlType::UartRxClear:
            if (s_state.mode != TesterMode::UartRx) {
                s_state.mode = TesterMode::UartRx;
                do_apply = true;
            }
            break;
    }
    snapshot = s_state;
    portEXIT_CRITICAL(&s_state_mux);

    // IMPORTANT: never call GPIO driver APIs / printf / logging while holding s_state_mux.
    if (ev.type == CtrlType::RxReset) {
        gpio_rx_reset();
    }
    if (ev.type == CtrlType::UartRxClear) {
        uart_tester_rx_clear();
    }
    if (do_uart_tx_set) {
        uart_tester_tx_set(snapshot.uart_tx_token, snapshot.uart_tx_delay_ms);
    }
    if (do_apply) {
        apply_config_snapshot(snapshot);
    }
    if (do_uart_tx_stop) {
        uart_tester_tx_stop();
    }
    if (do_uart_rx_get) {
        const int max_default = 64;
        int want = (int)ev.arg0;
        if (want <= 0) want = max_default;
        if (want > 1024) want = 1024;
        uint8_t buf[1024] = {};
        size_t n = uart_tester_rx_drain(buf, (size_t)want);
        printf("uart_rx_get: %u bytes\n", (unsigned)n);
        if (n > 0) {
            // Print as hex + ASCII-ish (printable bytes), but keep it simple.
            for (size_t i = 0; i < n; i++) {
                printf("%02X ", (unsigned)buf[i]);
            }
            printf("\n");
        }
    }
    if (do_status) {
        print_status_snapshot(snapshot);
    }
}


