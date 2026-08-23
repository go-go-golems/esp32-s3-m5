// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 — Module13.2 QRCode module control + scan pump.
#include "qr_module.h"

#include <string.h>

#include <M5Unified.h>
#include <utility/PI4IOE5V6408_Class.hpp>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *kTag = "qr_module";

bool QRModule::begin() {
    // --- I2C GPIO expander: power enable + TRIG ---
    _io_exp.reset(new m5::PI4IOE5V6408_Class(0x43, 100000, &M5.In_I2C));
    if (!_io_exp || !_io_exp->begin()) {
        ESP_LOGE(kTag, "PI4IOE5V6408 not found @0x43 (is the module stacked? 12V?)");
        return false;
    }
    for (uint8_t ch : {kChPowerEn, kChTrig}) {
        _io_exp->setDirection(ch, true);   // output
        _io_exp->setPullMode(ch, true);   // pull up
        _io_exp->enablePull(ch, true);
        _io_exp->setHighImpedance(ch, false);
    }

    // --- Power on the engine, TRIG idle high (active-low) ---
    setEnable(true);
    vTaskDelay(pdMS_TO_TICKS(300));

    // --- UART to the M14-Pro engine ---
    _engine.begin(kUart, kUartTx, kUartRx, 115200);

    // --- Result queue + RX pump task ---
    _q = xQueueCreate(8, sizeof(ScanResult));
    xTaskCreate(rxTask, "qr_rx", 4096, this, 5, nullptr);
    return true;
}

void QRModule::setEnable(bool en) {
    _io_exp->digitalWrite(kChPowerEn, en);
    setTriggerLevel(en);  // hold TRIG high when powered
}

void QRModule::setTriggerLevel(bool high) {
    _io_exp->digitalWrite(kChTrig, high);
}

void QRModule::startScan() { _engine.startDecode(); }

void QRModule::stopScan() { _engine.stopDecode(); }

void QRModule::emit(const char *text) {
    ScanResult r{};
    strncpy(r.text, text, sizeof(r.text) - 1);
    r.ts_us = esp_timer_get_time();
    // Keep recent history (depth 8); drop newest if full rather than block.
    xQueueSend(_q, &r, 0);
}

void QRModule::pump(const uint8_t *data, int n) {
    int64_t now = esp_timer_get_time();
    // Quiet-time boundary: a >50ms gap starts a new line (handles a missing
    // suffix or merged reads; see design doc §4.5).
    if (_len && (now - _last_us) > 50000) _len = 0;
    for (int i = 0; i < n; i++) {
        if (_len < sizeof(_line) - 1) _line[_len++] = (char)data[i];
        // default suffix \r\n marks a complete code
        if (_len >= 2 && _line[_len - 2] == '\r' && _line[_len - 1] == '\n') {
            _line[_len - 2] = 0;
            emit(_line);
            _len = 0;
        }
    }
    _last_us = now;
    if (_len >= sizeof(_line) - 1) {  // safety cap
        _line[_len] = 0;
        emit(_line);
        _len = 0;
    }
}

void QRModule::rxTask(void *arg) {
    QRModule *self = static_cast<QRModule *>(arg);
    uint8_t buf[128];
    while (true) {
        int n = self->_engine.readBytes(buf, sizeof(buf), 50);
        if (n > 0) self->pump(buf, n);
    }
}
