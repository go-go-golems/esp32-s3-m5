// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 (revised) — single UART-owner task + request queue.
#include "qr_module.h"

#include <cstdio>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *kTag = "qr_module";

bool QRModule::begin() {
    ESP_LOGI(kTag, "begin: probing PI4IOE5V6408 @0x43");
    _io_exp.reset(new m5::PI4IOE5V6408_Class(0x43, 100000, &M5.In_I2C));
    if (!_io_exp || !_io_exp->begin()) {
        ESP_LOGE(kTag, "PI4IOE5V6408 not found @0x43 (is the module stacked? 12V?)");
        return false;
    }
    ESP_LOGI(kTag, "begin: expander OK");
    for (uint8_t ch : {kChPowerEn, kChTrig}) {
        _io_exp->setDirection(ch, true);    // output
        _io_exp->setPullMode(ch, true);    // pull up
        _io_exp->enablePull(ch, true);
        _io_exp->setHighImpedance(ch, false);
    }
    setEnable(true);  // power on engine, TRIG idle high
    vTaskDelay(pdMS_TO_TICKS(300));

    _engine.begin(kUart, kUartTx, kUartRx, 115200);

    _req_q = xQueueCreate(8, sizeof(QRRequest));
    _q = xQueueCreate(8, sizeof(ScanResult));
    xTaskCreate(ownerTask, "qr_uart", 4096, this, 5, nullptr);
    ESP_LOGI(kTag, "begin: done (UART-owner task started)");
    return true;
}

void QRModule::setEnable(bool en) {
    _io_exp->digitalWrite(kChPowerEn, en);
    setTriggerLevel(en);
}
void QRModule::setTriggerLevel(bool high) { _io_exp->digitalWrite(kChTrig, high); }
void QRModule::startScan() {
    QRRequest r{QRReqType::StartDecode};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::stopScan() {
    QRRequest r{QRReqType::StopDecode};
    xQueueSend(_req_q, &r, 0);
}

bool QRModule::getInfo(uint8_t id, char *out, size_t cap) {
    QRRequest r;
    r.type = QRReqType::GetInfo;
    r.arg_u8 = id;
    r.resp_sem = xSemaphoreCreateBinary();
    r.resp_out = out;
    r.resp_cap = cap;
    bool ok_flag = false;
    r.resp_ok_flag = &ok_flag;
    ESP_LOGI(kTag, "getInfo: enqueue id=0x%02x", id);
    xQueueSend(_req_q, &r, portMAX_DELAY);
    // Bounded wait so a non-responding engine can't deadlock the caller
    bool ok = xSemaphoreTake(r.resp_sem, pdMS_TO_TICKS(1500));
    ESP_LOGI(kTag, "getInfo: id=0x%02x ok=%d resp_ok=%d", id, ok, ok_flag);
    vSemaphoreDelete(r.resp_sem);
    if (!ok) return false;
    return ok_flag;
}

// The simple setters enqueue a request and don't wait (fire-and-forget);
// they still go through the UART-owner task so UART access stays serialized.
void QRModule::setTriggerMode(QRCodeM14::TriggerMode m) {
    QRRequest r{QRReqType::SetTriggerMode, (uint8_t)m};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::setFillLightMode(QRCodeM14::FillLightMode m) {
    QRRequest r{QRReqType::SetFillLight, (uint8_t)m};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::setPosLightMode(QRCodeM14::PosLightMode m) {
    QRRequest r{QRReqType::SetPosLight, (uint8_t)m};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::setBrightness(int pct) {
    QRRequest r{QRReqType::SetBrightness, (uint8_t)pct};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::setBeep(int count) {
    QRRequest r{QRReqType::SetBeep, (uint8_t)count};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::factoryReset() {
    QRRequest r{QRReqType::FactoryReset};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::setModeUart() {
    QRRequest r{QRReqType::SetModeUart};
    xQueueSend(_req_q, &r, 0);
}
void QRModule::enableSuffixCrLf() {
    QRRequest r{QRReqType::EnableSuffixCrLf};
    xQueueSend(_req_q, &r, 0);
}

// --- scan accumulator (UART-owner task only) ---
void QRModule::emit(const char *text) {
    ESP_LOGI(kTag, "emit code: %s", text);
    ScanResult r{};
    strncpy(r.text, text, sizeof(r.text) - 1);
    r.ts_us = esp_timer_get_time();
    xQueueSend(_q, &r, 0);
}

void QRModule::pump(const uint8_t *data, int n) {
    int64_t now = esp_timer_get_time();
    if (n > 0) { ESP_LOGV(kTag, "pump %d bytes", n); }
    // The engine streams decoded text WITHOUT a suffix by default, repeating
    // every ~100ms in continuous mode. Emit a code on a quiet-time gap (>=30ms
    // with no new bytes) or on a length cap, whichever comes first.
    if (_len && (now - _last_us) > 30000) {
        _line[_len] = 0;
        emit(_line);
        _len = 0;
    }
    for (int i = 0; i < n; i++) {
        if (_len < sizeof(_line) - 1) _line[_len++] = (char)data[i];
        // if a suffix is configured, emit on \r\n too
        if (_len >= 2 && _line[_len - 2] == '\r' && _line[_len - 1] == '\n') {
            _line[_len - 2] = 0;
            emit(_line);
            _len = 0;
        }
        // length cap: emit if we've buffered a reasonable code chunk
        if (_len >= 64) {
            _line[_len] = 0;
            emit(_line);
            _len = 0;
        }
    }
    _last_us = now;
}

void QRModule::handle(QRRequest &r) {
    switch (r.type) {
        case QRReqType::GetInfo: {
            char tmp[64] = {0};
            bool ok = _engine.getInfos(r.arg_u8, tmp, sizeof(tmp));
            if (r.resp_ok_flag) *r.resp_ok_flag = ok;
            if (ok && r.resp_out && r.resp_cap) {
                strncpy(r.resp_out, tmp, r.resp_cap - 1);
                r.resp_out[r.resp_cap - 1] = 0;
            }
            break;
        }
        case QRReqType::StartDecode: _engine.startDecode(); break;
        case QRReqType::StopDecode:  _engine.stopDecode(); break;
        case QRReqType::SetTriggerMode: _engine.setTriggerMode((QRCodeM14::TriggerMode)r.arg_u8); break;
        case QRReqType::SetFillLight:  _engine.setFillLightMode((QRCodeM14::FillLightMode)r.arg_u8); break;
        case QRReqType::SetPosLight:   _engine.setPosLightMode((QRCodeM14::PosLightMode)r.arg_u8); break;
        case QRReqType::SetBrightness: _engine.setFillLightBrightness(r.arg_u8); break;
        case QRReqType::SetBeep:       _engine.setDecodeSuccessBeep(r.arg_u8); break;
        case QRReqType::FactoryReset:  _engine.factoryReset(); break;
        case QRReqType::SetModeUart:    _engine.setModeUart(); break;
        case QRReqType::EnableSuffixCrLf: _engine.enableSuffixCrLf(); break;
    }
    if (r.resp_sem) xSemaphoreGive(r.resp_sem);
}

void QRModule::ownerTask(void *arg) {
    QRModule *self = static_cast<QRModule *>(arg);
    uint8_t buf[128];
    int tick = 0;
    while (true) {
        if (self->_pump_paused) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        // Prioritize command requests: drain them first, then pump scans.
        QRRequest req;
        while (xQueueReceive(self->_req_q, &req, 0) == pdTRUE) {
            ESP_LOGI(kTag, "owner: handle req type=%d", (int)req.type);
            self->handle(req);
        }
        int n = self->_engine.readBytes(buf, sizeof(buf), 30);
        if (n > 0) self->pump(buf, n);
        if (++tick % 50 == 0) ESP_LOGI(kTag, "owner alive tick=%d", tick);
    }
}
