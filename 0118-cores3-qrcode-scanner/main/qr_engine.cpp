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
    // RX + TX buffers (TX must be >0 to avoid uart_write_bytes blocking on a
    // zero-size TX queue). No event queue: the scan pump polls.
    ESP_ERROR_CHECK(uart_driver_install(port, 1024, 1024, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_set_pin(port, tx, rx, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));
    ESP_LOGI(kTag, "uart%d @ %d 8N1 tx=%d rx=%d", port, baud, tx, rx);
}

const char *QRCodeM14::resultName(CmdResult result) {
    switch (result) {
        case OK: return "ok";
        case INVALID: return "invalid";
        case TIMEOUT: return "timeout";
        case ACK_MISMATCH: return "ack-mismatch";
        default: return "unknown";
    }
}

QRCodeM14::CmdResult QRCodeM14::sendCmd(const uint8_t *cmd, size_t n,
                                         const uint8_t *ack, size_t ack_len,
                                         uint32_t timeout_ms) {
    if (!cmd || n == 0 || _port == UART_NUM_MAX || ack_len > 16) return INVALID;
    ESP_LOGI(kTag, "tx %u bytes", (unsigned)n);
    ESP_LOG_BUFFER_HEXDUMP(kTag, cmd, n, ESP_LOG_INFO);
    uart_flush_input(_port);
    int written = uart_write_bytes(_port, cmd, n);
    if (written != (int)n) {
        ESP_LOGE(kTag, "uart write failed: wanted=%u wrote=%d", (unsigned)n, written);
        return INVALID;
    }
    if (!ack || ack_len == 0) return OK;

    uint8_t rx[16] = {0};
    size_t got = 0;
    int64_t deadline = esp_timer_get_time() + (int64_t)timeout_ms * 1000;
    while (esp_timer_get_time() < deadline && got < ack_len) {
        int len = uart_read_bytes(_port, rx + got, ack_len - got,
                                  pdMS_TO_TICKS(5));
        if (len > 0) got += len;
    }
    if (got < ack_len) {
        ESP_LOGW(kTag, "ack timeout: got=%u expected=%u", (unsigned)got,
                 (unsigned)ack_len);
        if (got) ESP_LOG_BUFFER_HEXDUMP(kTag, rx, got, ESP_LOG_WARN);
        return TIMEOUT;
    }
    if (memcmp(rx, ack, ack_len) != 0) {
        ESP_LOGW(kTag, "ack mismatch; received:");
        ESP_LOG_BUFFER_HEXDUMP(kTag, rx, got, ESP_LOG_WARN);
        ESP_LOGW(kTag, "expected:");
        ESP_LOG_BUFFER_HEXDUMP(kTag, ack, ack_len, ESP_LOG_WARN);
        return ACK_MISMATCH;
    }
    ESP_LOGI(kTag, "ack ok");
    return OK;
}

bool QRCodeM14::getInfos(uint8_t id, char *out, size_t out_cap) {
    uint8_t cmd[3] = {0x43, 0x02, id};
    sendCmd(cmd, 3, nullptr, 0, 0);
    uint8_t hdr[5];
    int got = 0;
    int64_t deadline = esp_timer_get_time() + 800 * 1000;
    while (got < 5 && esp_timer_get_time() < deadline) {
        int n = uart_read_bytes(_port, hdr + got, 5 - got, pdMS_TO_TICKS(20));
        if (n > 0) got += n;
    }
    ESP_LOGW(kTag, "getInfos id=0x%02x hdr_got=%d byte0=0x%02x", id, got, got ? hdr[0] : 0);
    if (got < 5 || hdr[0] != 0x44) return false;
    uint16_t len = (uint16_t)((hdr[3] << 8) | hdr[4]);
    if (len > out_cap - 1) len = (uint16_t)(out_cap - 1);
    // read exactly `len` data bytes (with a generous timeout)
    size_t need = len;
    got = 0;
    deadline = esp_timer_get_time() + 800 * 1000;
    while ((size_t)got < need && esp_timer_get_time() < deadline) {
        int n = uart_read_bytes(_port, (uint8_t *)out + got, need - got, pdMS_TO_TICKS(20));
        if (n > 0) got += n;
    }
    out[got] = 0;
    // drain any trailing bytes the engine streams after the payload
    uart_flush_input(_port);
    ESP_LOGW(kTag, "getInfos id=0x%02x data_len=%u data_got=%d -> '%s'", id, len, got, out);
    // A valid 0x44 header means the query was answered, even if len==0.
    return true;
}

QRCodeM14::CmdResult QRCodeM14::startDecode() {
    static const uint8_t cmd[] = {0x32, 0x75, 0x01};
    return sendCmd(cmd, sizeof(cmd));
}

QRCodeM14::CmdResult QRCodeM14::stopDecode() {
    static const uint8_t cmd[] = {0x32, 0x75, 0x02};
    static const uint8_t ack[] = {0x33, 0x75, 0x02, 0x00, 0x00};
    return sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 150);
}

QRCodeM14::CmdResult QRCodeM14::setTriggerMode(TriggerMode m) {
    uint8_t cmd[] = {0x21, 0x61, 0x41, (uint8_t)m};
    uint8_t ack[] = {0x22, 0x61, 0x41, (uint8_t)m, 0x00};
    return sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 200);
}

QRCodeM14::CmdResult QRCodeM14::setFillLightMode(FillLightMode m) {
    uint8_t cmd[] = {0x21, 0x62, 0x41, (uint8_t)m};
    uint8_t ack[] = {0x22, 0x62, 0x41, (uint8_t)m, 0x00};
    return sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 200);
}

QRCodeM14::CmdResult QRCodeM14::setPosLightMode(PosLightMode m) {
    uint8_t cmd[] = {0x21, 0x62, 0x42, (uint8_t)m};
    uint8_t ack[] = {0x22, 0x62, 0x42, (uint8_t)m, 0x00};
    return sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 100);
}

QRCodeM14::CmdResult QRCodeM14::setModeUart() {
    static const uint8_t cmd[] = {0x21, 0x42, 0x40, 0x00};
    return sendCmd(cmd, sizeof(cmd));
}

QRCodeM14::CmdResult QRCodeM14::enableSuffixCrLf() {
    static const uint8_t en[] = {0x21, 0x51, 0x4C, 0x01};
    static const uint8_t en_ack[] = {0x22, 0x51, 0x4C, 0x01, 0x00};
    CmdResult result = sendCmd(en, sizeof(en), en_ack, sizeof(en_ack), 200);
    if (result != OK) return result;

    static const uint8_t suf[] = {0x21, 0x51, 0xC2, 0x00, 0x02, 0x0D, 0x0A};
    static const uint8_t suf_ack[] = {0x22, 0x51, 0xC2, 0x00, 0x00};
    return sendCmd(suf, sizeof(suf), suf_ack, sizeof(suf_ack), 200);
}

QRCodeM14::CmdResult QRCodeM14::setFillLightBrightness(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    uint8_t cmd[] = {0x21, 0x62, 0x48, (uint8_t)pct};
    uint8_t ack[] = {0x22, 0x62, 0x48, (uint8_t)pct, 0x00};
    return sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 100);
}

QRCodeM14::CmdResult QRCodeM14::setDecodeSuccessBeep(int count) {
    uint8_t cmd[] = {0x21, 0x63, 0x42, (uint8_t)count};
    uint8_t ack[] = {0x22, 0x63, 0x42, (uint8_t)count};
    return sendCmd(cmd, sizeof(cmd), ack, sizeof(ack), 150);
}

QRCodeM14::CmdResult QRCodeM14::factoryReset() {
    static const uint8_t cmd[] = {0x32, 0x76, 0x01};
    return sendCmd(cmd, sizeof(cmd));
}

int QRCodeM14::available() {
    size_t n = 0;
    uart_get_buffered_data_len(_port, &n);
    return (int)n;
}

int QRCodeM14::readBytes(uint8_t *buf, size_t cap, int timeout_ms) {
    return uart_read_bytes(_port, buf, cap, pdMS_TO_TICKS(timeout_ms));
}
