// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 (revised) — Module13.2 QRCode module.
//
// ARCHITECTURE: one FreeRTOS task exclusively owns UART1. UI and console
// tasks submit commands through a request queue. Synchronous operations use
// a per-call reply queue; requests and responses are copied by value, so no
// queued object refers to a caller's stack or to a prematurely deleted
// semaphore.
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <cstddef>
#include <cstdint>
#include <memory>

#include <M5Unified.h>
#include <utility/PI4IOE5V6408_Class.hpp>

#include "qr_engine.h"

struct ScanResult {
    char text[256];
    int64_t ts_us;
};

enum class QRReqType : uint8_t {
    GetInfo,
    RawCommand,
    StartDecode,
    StopDecode,
    SetTriggerMode,
    SetFillLight,
    SetPosLight,
    SetBrightness,
    SetBeep,
    FactoryReset,
    SetModeUart,
    EnableSuffixCrLf,
};

// Queue payloads contain only owned values and FreeRTOS object handles.
// reply_queue remains valid because synchronous callers wait for the owner
// response before deleting it.
struct QRRequest {
    QRReqType type = QRReqType::GetInfo;
    uint8_t arg_u8 = 0;
    QueueHandle_t reply_queue = nullptr;
    uint8_t raw_cmd[16] = {0};
    size_t raw_len = 0;
};

struct QRResponse {
    bool ok = false;
    char info[64] = {0};
    uint8_t raw[128] = {0};
    size_t raw_len = 0;
};

class QRModule {
   public:
    // QRCode DIP route: M5-Bus pin 23 QR_RX <- CoreS3 G13 TX;
    // M5-Bus pin 26 QR_TX -> CoreS3 G14 RX.
    static constexpr int kUartTx = 13;
    static constexpr int kUartRx = 14;
    static constexpr uart_port_t kUart = UART_NUM_1;
    static constexpr uint8_t kChPowerEn = 0;
    static constexpr uint8_t kChTrig = 4;

    bool begin();
    bool ready() const { return _ready; }
    void setEnable(bool en);
    void setTriggerLevel(bool high);
    void startScan();
    void stopScan();

    bool getInfo(uint8_t id, char *out, size_t cap);
    bool rawCommand(const uint8_t *cmd, size_t cmd_len, uint8_t *out,
                    size_t out_cap, size_t *out_len);
    void setTriggerMode(QRCodeM14::TriggerMode m);
    void setFillLightMode(QRCodeM14::FillLightMode m);
    void setPosLightMode(QRCodeM14::PosLightMode m);
    void setBrightness(int pct);
    void setBeep(int count);
    void factoryReset();
    void setModeUart();
    void enableSuffixCrLf();

    QueueHandle_t resultQueue() const { return _result_q; }

   private:
    QRCodeM14 _engine;
    std::unique_ptr<m5::PI4IOE5V6408_Class> _io_exp;
    QueueHandle_t _req_q = nullptr;
    QueueHandle_t _result_q = nullptr;
    bool _ready = false;

    char _line[256] = {0};
    size_t _len = 0;
    int64_t _last_rx_us = 0;

    bool enqueue(QRReqType type, uint8_t arg = 0);
    bool transact(QRRequest &request, QRResponse *response);
    void pump(const uint8_t *data, int n);
    void emit(const char *text);
    void handle(const QRRequest &request);
    static void ownerTask(void *arg);
};
