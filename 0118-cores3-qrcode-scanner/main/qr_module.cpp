// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 (revised) — single UART-owner task + value queues.
#include "qr_module.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "driver/gpio.h"
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
    // Preload valid levels while both channels are still high-impedance. TRIG
    // is active-low, so it must already be high before output drive or scanner
    // power is enabled. This is the ordering proven by the minimal 0119 probe.
    _io_exp->digitalWrite(kChPowerEn, false);
    _io_exp->digitalWrite(kChTrig, true);
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
    // Keep active-low TRIG idle-high across both power states. On power-up,
    // establish TRIG first; on power-down, remove power first.
    if (en) {
        setTriggerLevel(true);
        _io_exp->digitalWrite(kChPowerEn, true);
    } else {
        _io_exp->digitalWrite(kChPowerEn, false);
        setTriggerLevel(true);
    }
}

void QRModule::setTriggerLevel(bool high) {
    if (_io_exp) _io_exp->digitalWrite(kChTrig, high);
}

bool QRModule::enqueue(QRReqType type, uint8_t arg) {
    if (!_ready || !_req_q) return false;
    QRRequest request{};
    request.type = type;
    request.arg_u8 = arg;
    if (xQueueSend(_req_q, &request, 0) != pdTRUE) {
        ESP_LOGW(kTag, "request queue full: type=%d", (int)type);
        return false;
    }
    return true;
}

QRCodeM14::CmdResult QRModule::command(QRReqType type, uint8_t arg) {
    QRRequest request{};
    request.type = type;
    request.arg_u8 = arg;
    QRResponse response{};
    if (!transact(request, &response)) return QRCodeM14::INVALID;
    return response.cmd_result;
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

QRCodeM14::CmdResult QRModule::startScan() {
    return command(QRReqType::StartDecode);
}
QRCodeM14::CmdResult QRModule::stopScan() {
    return command(QRReqType::StopDecode);
}
bool QRModule::requestStartScan() {
    return enqueue(QRReqType::StartDecode);
}
bool QRModule::requestStopScan() {
    return enqueue(QRReqType::StopDecode);
}
bool QRModule::requestTriggerMode(QRCodeM14::TriggerMode m) {
    return enqueue(QRReqType::SetTriggerMode, (uint8_t)m);
}

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

QRCodeM14::CmdResult QRModule::setTriggerMode(QRCodeM14::TriggerMode m) {
    return command(QRReqType::SetTriggerMode, (uint8_t)m);
}
QRCodeM14::CmdResult QRModule::setFillLightMode(QRCodeM14::FillLightMode m) {
    return command(QRReqType::SetFillLight, (uint8_t)m);
}
QRCodeM14::CmdResult QRModule::setPosLightMode(QRCodeM14::PosLightMode m) {
    return command(QRReqType::SetPosLight, (uint8_t)m);
}
QRCodeM14::CmdResult QRModule::setBrightness(int pct) {
    return command(QRReqType::SetBrightness,
                   (uint8_t)std::clamp(pct, 0, 100));
}
QRCodeM14::CmdResult QRModule::setBeep(int count) {
    return command(QRReqType::SetBeep, (uint8_t)std::max(count, 0));
}
QRCodeM14::CmdResult QRModule::factoryReset() {
    return command(QRReqType::FactoryReset);
}
QRCodeM14::CmdResult QRModule::setModeUart() {
    return command(QRReqType::SetModeUart);
}
QRCodeM14::CmdResult QRModule::enableSuffixCrLf() {
    return command(QRReqType::EnableSuffixCrLf);
}
QRCodeM14::CmdResult QRModule::powerCycle() {
    return command(QRReqType::PowerCycle);
}
QRCodeM14::CmdResult QRModule::setHardwareTrigger(bool high) {
    return command(QRReqType::SetTriggerLevel, high ? 1 : 0);
}
QRCodeM14::CmdResult QRModule::pulseHardwareTrigger() {
    return command(QRReqType::PulseTrigger);
}

bool QRModule::getElectricalState(char *out, size_t cap) {
    if (!out || cap == 0) return false;
    QRRequest request{};
    request.type = QRReqType::GetElectricalState;
    QRResponse response{};
    if (!transact(request, &response) || !response.ok) return false;
    std::strncpy(out, response.info, cap - 1);
    out[cap - 1] = 0;
    return true;
}

bool QRModule::probeBauds(char *out, size_t cap) {
    if (!out || cap == 0) return false;
    QRRequest request{};
    request.type = QRReqType::ProbeBauds;
    QRResponse response{};
    if (!transact(request, &response) || !response.ok) return false;
    std::strncpy(out, response.info, cap - 1);
    out[cap - 1] = 0;
    return true;
}

bool QRModule::probeRoutes(char *out, size_t cap) {
    if (!out || cap == 0) return false;
    QRRequest request{};
    request.type = QRReqType::ProbeRoutes;
    QRResponse response{};
    if (!transact(request, &response) || !response.ok) return false;
    std::strncpy(out, response.info, cap - 1);
    out[cap - 1] = 0;
    return true;
}

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
            response.cmd_result = response.ok ? QRCodeM14::OK : QRCodeM14::TIMEOUT;
            break;
        case QRReqType::RawCommand: {
            response.cmd_result = _engine.sendCmd(
                request.raw_cmd, request.raw_len, nullptr, 0, 0);
            if (response.cmd_result == QRCodeM14::OK) {
                int got = _engine.readBytes(response.raw, sizeof(response.raw), 800);
                response.raw_len = got > 0 ? (size_t)got : 0;
                response.ok = got >= 0;
            }
            break;
        }
        case QRReqType::StartDecode:
            response.cmd_result = _engine.startDecode();
            break;
        case QRReqType::StopDecode:
            response.cmd_result = _engine.stopDecode();
            break;
        case QRReqType::SetTriggerMode:
            response.cmd_result = _engine.setTriggerMode(
                (QRCodeM14::TriggerMode)request.arg_u8);
            break;
        case QRReqType::SetFillLight:
            response.cmd_result = _engine.setFillLightMode(
                (QRCodeM14::FillLightMode)request.arg_u8);
            break;
        case QRReqType::SetPosLight:
            response.cmd_result = _engine.setPosLightMode(
                (QRCodeM14::PosLightMode)request.arg_u8);
            break;
        case QRReqType::SetBrightness:
            response.cmd_result = _engine.setFillLightBrightness(request.arg_u8);
            break;
        case QRReqType::SetBeep:
            response.cmd_result = _engine.setDecodeSuccessBeep(request.arg_u8);
            break;
        case QRReqType::FactoryReset:
            response.cmd_result = _engine.factoryReset();
            break;
        case QRReqType::SetModeUart:
            response.cmd_result = _engine.setModeUart();
            break;
        case QRReqType::EnableSuffixCrLf:
            response.cmd_result = _engine.enableSuffixCrLf();
            break;
        case QRReqType::PowerCycle:
            ESP_LOGI(kTag, "scanner power cycle: off");
            setEnable(false);
            uart_flush_input(kUart);
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP_LOGI(kTag, "scanner power cycle: on, TRIG idle high");
            setEnable(true);
            vTaskDelay(pdMS_TO_TICKS(1200));
            uart_flush_input(kUart);
            _len = 0;
            _last_rx_us = 0;
            response.cmd_result = QRCodeM14::OK;
            break;
        case QRReqType::SetTriggerLevel:
            setTriggerLevel(request.arg_u8 != 0);
            ESP_LOGI(kTag, "TRIG=%s", request.arg_u8 ? "HIGH" : "LOW");
            response.cmd_result = QRCodeM14::OK;
            break;
        case QRReqType::PulseTrigger:
            ESP_LOGI(kTag, "TRIG pulse: LOW 100ms -> HIGH");
            setTriggerLevel(false);
            vTaskDelay(pdMS_TO_TICKS(100));
            setTriggerLevel(true);
            response.cmd_result = QRCodeM14::OK;
            break;
        case QRReqType::GetElectricalState: {
            int power_wr = _io_exp ? _io_exp->getWriteValue(kChPowerEn) : -1;
            int power_pin = _io_exp ? _io_exp->digitalRead(kChPowerEn) : -1;
            int trig_wr = _io_exp ? _io_exp->getWriteValue(kChTrig) : -1;
            int trig_pin = _io_exp ? _io_exp->digitalRead(kChTrig) : -1;
            int rx = gpio_get_level((gpio_num_t)kUartRx);
            std::snprintf(response.info, sizeof(response.info),
                          "pwr_wr=%d pin=%d trig_wr=%d pin=%d rx_g14=%d",
                          power_wr, power_pin, trig_wr, trig_pin, rx);
            response.cmd_result = QRCodeM14::OK;
            break;
        }
        case QRReqType::ProbeBauds: {
            static const int bauds[] = {
                115200, 9600, 19200, 38400, 57600, 4800, 2400, 1200, 128000
            };
            char fw[32] = {0};
            for (int baud : bauds) {
                if (_engine.configureHostBaud(baud) != QRCodeM14::OK) continue;
                vTaskDelay(pdMS_TO_TICKS(50));
                if (_engine.getInfos(0xC1, fw, sizeof(fw))) {
                    std::snprintf(response.info, sizeof(response.info),
                                  "baud=%d firmware=%s", baud, fw);
                    response.cmd_result = QRCodeM14::OK;
                    break;
                }
            }
            if (response.cmd_result != QRCodeM14::OK) {
                _engine.configureHostBaud(115200);
                std::snprintf(response.info, sizeof(response.info),
                              "no response; restored baud=115200");
                response.cmd_result = QRCodeM14::TIMEOUT;
            }
            break;
        }
        case QRReqType::ProbeRoutes: {
            struct Route { int tx; int rx; const char *name; };
            // Known QRCode DIP pairs. G6/G7 is omitted because those are H2
            // control lines unless the H2 disconnect state is re-verified.
            static const Route routes[] = {
                {13, 14, "G13/G14-pins23/26"},
                {17, 18, "G17/G18-pins16/15"},
                {43, 44, "G43/G44-pins14/13"},
            };
            _engine.configureHostBaud(115200);
            char fw[32] = {0};
            for (const auto &route : routes) {
                if (_engine.configureHostPins(route.tx, route.rx) != QRCodeM14::OK) continue;
                vTaskDelay(pdMS_TO_TICKS(50));
                if (_engine.getInfos(0xC1, fw, sizeof(fw))) {
                    std::snprintf(response.info, sizeof(response.info),
                                  "%s firmware=%s", route.name, fw);
                    response.cmd_result = QRCodeM14::OK;
                    break;
                }
            }
            if (response.cmd_result != QRCodeM14::OK) {
                _engine.configureHostPins(kUartTx, kUartRx);
                std::snprintf(response.info, sizeof(response.info),
                              "no route; restored G13/G14");
                response.cmd_result = QRCodeM14::TIMEOUT;
            }
            break;
        }
    }
    if (response.cmd_result == QRCodeM14::OK) response.ok = true;

    if (request.reply_queue) {
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
