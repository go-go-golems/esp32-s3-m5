/*
 * Tutorial 0016: LCD status UI (AtomS3R).
 */

#include "lcd_ui.h"

#include <inttypes.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "display_hal.h"
#include "gpio_rx.h"
#include "signal_state.h"

static const char *TAG = "atoms3r_gpio_sig_tester";

static M5Canvas *s_canvas = nullptr;

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

static void ui_task(void *arg) {
    (void)arg;
    if (!s_canvas) {
        vTaskDelete(nullptr);
        return;
    }

    uint32_t last_edges = 0;
    int64_t last_us = esp_timer_get_time();

    while (true) {
        tester_state_t st = tester_state_snapshot();
        rx_stats_t rx = gpio_rx_snapshot();

        int64_t now_us = esp_timer_get_time();
        uint32_t d_edges = rx.edges - last_edges;
        int64_t dt_us = now_us - last_us;
        uint32_t per_s = 0;
        if (dt_us > 0) {
            per_s = (uint32_t)((uint64_t)d_edges * 1000000ULL / (uint64_t)dt_us);
        }
        last_edges = rx.edges;
        last_us = now_us;

        s_canvas->fillScreen(TFT_BLACK);
        s_canvas->setTextColor(TFT_WHITE, TFT_BLACK);
        s_canvas->setTextWrap(false);
        s_canvas->setCursor(0, 0);

        s_canvas->printf("AtomS3R GPIO Signal Tester (0016)\n");
        s_canvas->printf("MODE: %s\n", mode_str(st.mode));
        s_canvas->printf("PIN:  %s (GPIO%d)\n", (st.active_pin == 1) ? "G1" : "G2", st.active_pin);

        s_canvas->printf("TX:   %s", tx_mode_str(st.tx.mode));
        if (st.tx.mode == TxMode::Square) {
            s_canvas->printf(" %d Hz", st.tx.hz);
        } else if (st.tx.mode == TxMode::Pulse) {
            s_canvas->printf(" w=%dus p=%dms", st.tx.width_us, st.tx.period_ms);
        }
        s_canvas->printf("\n");

        s_canvas->printf("RX:   edges=%" PRIu32 " (+%" PRIu32 "/s)\n", rx.edges, per_s);
        s_canvas->printf("      rises=%" PRIu32 " falls=%" PRIu32 " last=%" PRIu32 " lvl=%d\n",
                         rx.rises, rx.falls, rx.last_tick, rx.last_level);

        if (st.mode == TesterMode::UartTx || st.mode == TesterMode::UartRx) {
            uart_tester_stats_t us = uart_tester_stats_snapshot();
            s_canvas->printf("UART: %d %s\n",
                             st.uart_baud,
                             (st.uart_map == UartMap::Swapped) ? "swapped" : "normal");
            s_canvas->printf("      tx=%" PRIu32 " rx=%" PRIu32 "\n", us.tx_bytes_total, us.rx_bytes_total);
            s_canvas->printf("      buf=%" PRIu32 " drop=%" PRIu32 "\n", us.rx_buf_used, us.rx_buf_dropped);
        }

        display_present_canvas(*s_canvas);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_TUTORIAL_0016_UI_REFRESH_MS));
    }
}

void lcd_ui_start(M5Canvas *canvas) {
    s_canvas = canvas;
    BaseType_t ok = xTaskCreatePinnedToCore(ui_task, "lcd_ui", 4096, nullptr, 1, nullptr, tskNO_AFFINITY);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "failed to create lcd ui task");
    } else {
        ESP_LOGI(TAG, "lcd ui started: refresh_ms=%d", CONFIG_TUTORIAL_0016_UI_REFRESH_MS);
    }
}



