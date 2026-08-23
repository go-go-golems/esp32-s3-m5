// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 (revised) — single UART-owner task + value queues.
#include "qr_module.h"

#include <algorithm>
#include <cstring>

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
        _io_exp->setDirection(ch, true);
        _io_exp->setPullMode(ch, true);
        _io_exp->enablePull(ch, true);
        _io_exp->setHighImpedance(ch, false);
    }
    setEnable(true);
    vTaskDelay(pdMS_TO_TICKS(300));

    _engine.begin(kUart, kUartTx, kUartRx, 115200);

    _req_q = xQueueCreate(8, sizeof(QRRequest));
    _result_q = xQueueCreate(8, sizeof(ScanResult));
    if (!_req_q || !_result_q) {
        ESP_LOGE(kTag, "queue allocation failed");
        if (_req_q) vQueueDelete(_req_q);
        if (_result_q) vQueueDelete(_result_q);
        _req_q = nullptr;
        _result_q = nullptr;
        return false;
    }
    if (xTaskCreate(ownerTask, "qr_uart", 4096, this, 5, nullptr) != pdPASS) {
        ESP_LOGE(kTag, "UART-owner task creation failed");
        vQueueDelete(_req_q);
        vQueueDelete(_result_q);
        _req_q = nullptr;
        _result_q = nullptr;
        return false;
    }
    _ready = true;
    ESP_LOGI(kTag, "begin: done (UART-owner task started)");
    return true;
}

void QRModule::setEnable(bool en) {
    if (!_io_exp) return;
    _io_exp->digitalWrite(kChPowerEn, en);
    setTriggerLevel(en);
}

void QRModule::setTriggerLevel(bool high) {
    if (_io_exp) _io_exp->digitalWrite(kChTrig, high);
}

bool QRModule::enqueue(QRReqType type, uint8_t arg) {
    if (!_ready || !_req_q) {
        ESP_LOGW(kTag, "reject request type=%d: module not ready", (int)type);
        return false;
    }
    QRRequest request{};
    request.type = type;
    request.arg_u8 = arg;
    if (xQueueSend(_req_q, &request, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(kTag, "request queue full: type=%d", (int)type);
        return false;
    }
    return true;
}

bool QRModule::transact(QRRequest &request, QRResponse *response) {
    if (!_ready || !_req_q || !response) return false;

    QueueHandle_t reply = xQueueCreate(1, sizeof(QRResponse));
    if (!reply) {
        ESP_LOGE(kTag, "reply queue allocation failed");
        return false;
    }
    request.reply_queue = reply;

    bool sent = xQueueSend(_req_q, &request, pdMS_TO_TICKS(100)) == pdTRUE;
    bool received = false;
    if (sent) {
        // Owner-side UART operations are internally bounded. Waiting for the
        // response preserves reply-queue lifetime and cannot race deletion.
        received = xQueueReceive(reply, response, portMAX_DELAY) == pdTRUE;
    } else {
        ESP_LOGW(kTag, "request queue full: type=%d", (int)request.type);
    }
    vQueueDelete(reply);
    return sent && received;
}

void QRModule::startScan() { enqueue(QRReqType::StartDecode); }
void QRModule::stopScan() { enqueue(QRReqType::StopDecode); }

bool QRModule::getInfo(uint8_t id, char *out, size_t cap) {
    if (!out || cap == 0) return false;
    out[0] = 0;

    QRRequest request{};
    request.type = QRReqType::GetInfo;
    request.arg_u8 = id;
    QRResponse response{};
    if (!transact(request, &response) || !response.ok) return false;

    std::strncpy(out, response.info, cap - 1);
    out[cap - 1] = 0;
    return true;
}

bool QRModule::rawCommand(const uint8_t *cmd, size_t cmd_len, uint8_t *out,
                          size_t out_cap, size_t *out_len) {
    if (!cmd || cmd_len == 0 || cmd_len > sizeof(QRRequest::raw_cmd) ||
        !out || out_cap == 0 || !out_len) {
        return false;
    }

    QRRequest request{};
    request.type = QRReqType::RawCommand;
    request.raw_len = cmd_len;
    std::memcpy(request.raw_cmd, cmd, cmd_len);
    QRResponse response{};
    if (!transact(request, &response) || !response.ok) return false;

    size_t n = std::min(out_cap, response.raw_len);
    std::memcpy(out, response.raw, n);
    *out_len = n;
    return true;
}

void QRModule::setTriggerMode(QRCodeM14::TriggerMode m) {
    enqueue(QRReqType::SetTriggerMode, (uint8_t)m);
}
void QRModule::setFillLightMode(QRCodeM14::FillLightMode m) {
    enqueue(QRReqType::SetFillLight, (uint8_t)m);
}
void QRModule::setPosLightMode(QRCodeM14::PosLightMode m) {
    enqueue(QRReqType::SetPosLight, (uint8_t)m);
}
void QRModule::setBrightness(int pct) {
    enqueue(QRReqType::SetBrightness, (uint8_t)std::clamp(pct, 0, 100));
}
void QRModule::setBeep(int count) {
    enqueue(QRReqType::SetBeep, (uint8_t)std::max(count, 0));
}
void QRModule::factoryReset() { enqueue(QRReqType::FactoryReset); }
void QRModule::setModeUart() { enqueue(QRReqType::SetModeUart); }
void QRModule::enableSuffixCrLf() { enqueue(QRReqType::EnableSuffixCrLf); }

void QRModule::emit(const char *text) {
    if (!_result_q || !text || !text[0]) return;
    ScanResult result{};
    std::strncpy(result.text, text, sizeof(result.text) - 1);
    result.ts_us = esp_timer_get_time();
    if (xQueueSend(_result_q, &result, 0) == pdTRUE) {
        ESP_LOGI(kTag, "emit code: %s", result.text);
    } else {
        ESP_LOGW(kTag, "result queue full; dropping code");
    }
}

void QRModule::pump(const uint8_t *data, int n) {
    int64_t now = esp_timer_get_time();

    // Evaluate quiet time even when uart_read_bytes timed out. This emits a
    // one-shot scan without requiring a later packet to arrive.
    if (_len && _last_rx_us && (now - _last_rx_us) >= 30000) {
        _line[_len] = 0;
        emit(_line);
        _len = 0;
    }

    if (!data || n <= 0) return;
    for (int i = 0; i < n; ++i) {
        if (_len < sizeof(_line) - 1) _line[_len++] = (char)data[i];
        if (_len >= 2 && _line[_len - 2] == '\r' && _line[_len - 1] == '\n') {
            _line[_len - 2] = 0;
            emit(_line);
            _len = 0;
        } else if (_len >= 64) {
            _line[_len] = 0;
            emit(_line);
            _len = 0;
        }
    }
    _last_rx_us = now;
}

void QRModule::handle(const QRRequest &request) {
    QRResponse response{};
    switch (request.type) {
        case QRReqType::GetInfo:
            response.ok = _engine.getInfos(request.arg_u8, response.info,
                                           sizeof(response.info));
            break;
        case QRReqType::RawCommand: {
            _engine.sendCmd(request.raw_cmd, request.raw_len, nullptr, 0, 0);
            int got = _engine.readBytes(response.raw, sizeof(response.raw), 800);
            response.raw_len = got > 0 ? (size_t)got : 0;
            response.ok = got >= 0;
            break;
        }
        case QRReqType::StartDecode: _engine.startDecode(); break;
        case QRReqType::StopDecode: _engine.stopDecode(); break;
        case QRReqType::SetTriggerMode:
            _engine.setTriggerMode((QRCodeM14::TriggerMode)request.arg_u8);
            break;
        case QRReqType::SetFillLight:
            _engine.setFillLightMode((QRCodeM14::FillLightMode)request.arg_u8);
            break;
        case QRReqType::SetPosLight:
            _engine.setPosLightMode((QRCodeM14::PosLightMode)request.arg_u8);
            break;
        case QRReqType::SetBrightness:
            _engine.setFillLightBrightness(request.arg_u8);
            break;
        case QRReqType::SetBeep:
            _engine.setDecodeSuccessBeep(request.arg_u8);
            break;
        case QRReqType::FactoryReset: _engine.factoryReset(); break;
        case QRReqType::SetModeUart: _engine.setModeUart(); break;
        case QRReqType::EnableSuffixCrLf: _engine.enableSuffixCrLf(); break;
    }

    if (request.reply_queue) {
        // The synchronous caller owns this queue and waits until this response
        // arrives before deleting it.
        xQueueSend(request.reply_queue, &response, portMAX_DELAY);
    }
}

void QRModule::ownerTask(void *arg) {
    QRModule *self = static_cast<QRModule *>(arg);
    uint8_t buf[128];
    while (true) {
        QRRequest request{};
        while (xQueueReceive(self->_req_q, &request, 0) == pdTRUE) {
            self->handle(request);
        }
        int n = self->_engine.readBytes(buf, sizeof(buf), 30);
        self->pump(n > 0 ? buf : nullptr, n);
    }
}
