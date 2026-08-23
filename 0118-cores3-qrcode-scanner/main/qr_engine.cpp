// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 — M14-Pro protocol engine (ESP-IDF UART port).
#include "qr_engine.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *kTag = "qr_engine";

void QRCodeM14::begin(uart_port_t port, int tx, int rx, int baud) {
    _port = port;
    uart_config_t u = {};
    u.baud_rate = baud;
    u.data_bits = UART_DATA_8_BITS;
    u.parity = UART_PARITY_DISABLE;
    u.stop_bits = UART_STOP_BITS_1;
    u.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    u.source_clk = UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_param_config(port, &u));
    // RX buffer only; we don't need a TX buffer / event queue in the engine.
    ESP_ERROR_CHECK(uart_driver_install(port, 1024, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_set_pin(port, tx, rx, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));
    ESP_LOGI(kTag, "uart%d @ %d 8N1 tx=%d rx=%d", port, baud, tx, rx);
}

QRCodeM14::CmdResult QRCodeM14::sendCmd(const uint8_t *cmd, size_t n,
                                         const uint8_t *ack, size_t ack_len,
                                         uint32_t timeout_ms) {
    if (!cmd || n == 0) return INVALID;
    uart_flush_input(_port);  // "clear rx buffer"
    uart_write_bytes(_port, cmd, n);
    if (!ack || ack_len == 0) return OK;  // no reply expected

    uint8_t rx[16];
    size_t got = 0;
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (esp_timer_get_time() < deadline) {
        int len = uart_read_bytes(_port, rx + got, ack_len - got, pdMS_TO_TICKS(5));
        if (len > 0) {
            got += len;
            if (got >= ack_len) break;
        }
    }
    if (got < ack_len) return TIMEOUT;
    return (memcmp(rx, ack, ack_len) == 0) ? OK : ACK_MISMATCH;
}

bool QRCodeM14::getInfos(uint8_t id, char *out, size_t out_cap) {
    uint8_t cmd[3] = {0x43, 0x02, id};
    if (sendCmd(cmd, 3, nullptr, 0, 0) != OK) {
        // sendCmd returns OK immediately when no ack expected; still drain.
    }
    uint8_t buf[128];
    int n = uart_read_bytes(_port, buf, sizeof(buf), pdMS_TO_TICKS(500));
    if (n < 5 || buf[0] != 0x44) return false;
    uint16_t len = (uint16_t)((buf[3] << 8) | buf[4]);
    if (len > out_cap - 1) len = (uint16_t)(out_cap - 1);
    memcpy(out, buf + 5, len);
    out[len] = 0;
    return true;
}

void QRCodeM14::startDecode() {
    static const uint8_t cmd[] = {0x32, 0x75, 0x01};
    sendCmd(cmd, sizeof(cmd));
}

void QRCodeM14::stopDecode() {
    static const uint8_t cmd[] = {0x32, 0x75, 0x02};
    static const uint8_t ack[] = {0x33, 0x75, 0x02, 0x00, 0x00};
    sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 150);
}

void QRCodeM14::setTriggerMode(TriggerMode m) {
    uint8_t cmd[] = {0x21, 0x61, 0x41, (uint8_t)m};
    uint8_t ack[] = {0x22, 0x61, 0x41, (uint8_t)m, 0x00};
    sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 200);
}

void QRCodeM14::setFillLightMode(FillLightMode m) {
    uint8_t cmd[] = {0x21, 0x62, 0x41, (uint8_t)m};
    uint8_t ack[] = {0x22, 0x62, 0x41, (uint8_t)m, 0x00};
    sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 200);
}

void QRCodeM14::setPosLightMode(PosLightMode m) {
    uint8_t cmd[] = {0x21, 0x62, 0x42, (uint8_t)m};
    uint8_t ack[] = {0x22, 0x62, 0x42, (uint8_t)m, 0x00};
    sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 100);
}

void QRCodeM14::setModeUart() {
    static const uint8_t cmd[] = {0x21, 0x42, 0x40, 0x00};
    sendCmd(cmd, sizeof(cmd));
}

int QRCodeM14::available() {
    size_t n = 0;
    uart_get_buffered_data_len(_port, &n);
    return (int)n;
}

int QRCodeM14::readBytes(uint8_t *buf, size_t cap, int timeout_ms) {
    return uart_read_bytes(_port, buf, cap, pdMS_TO_TICKS(timeout_ms));
}
