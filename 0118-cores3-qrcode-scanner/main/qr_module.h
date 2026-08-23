// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 — Module13.2 QRCode module: PI4IOE5V6408 power+TRIG control,
// UART init, and a scan-result pump that emits complete decoded codes on a
// FreeRTOS queue. Mirrors sources/arduino-lib/src/M5ModuleQRCode.cpp.
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <cstdint>
#include <memory>

#include <M5Unified.h>
#include <utility/PI4IOE5V6408_Class.hpp>

#include "qr_engine.h"

// Decoded scan result handed to the UI.
struct ScanResult {
    char text[256];
    int64_t ts_us;
};

class QRModule {
   public:
    // CoreS3 Port-C UART (module DIP set to UART). TX=G17 -> QR_RX,
    // RX=G18 <- QR_TX. I2C expander PI4IOE5V6408 @0x43 on M5.In_I2C.
    static constexpr int kUartTx = 17;
    static constexpr int kUartRx = 18;
    static constexpr uart_port_t kUart = UART_NUM_1;
    // Expander channels (match M5ModuleQRCode.cpp CHANNEL_QRCODE_*).
    static constexpr uint8_t kChPowerEn = 0;  // QR_5V_EN
    static constexpr uint8_t kChTrig = 4;     // TRIG

    bool begin();
    void setEnable(bool en);
    void setTriggerLevel(bool high);  // TRIG is active-low
    void startScan();
    void stopScan();

    QRCodeM14 &engine() { return _engine; }
    QueueHandle_t resultQueue() const { return _q; }

   private:
    QRCodeM14 _engine;
    std::unique_ptr<m5::PI4IOE5V6408_Class> _io_exp;
    QueueHandle_t _q = nullptr;
    char _line[256];
    size_t _len = 0;
    int64_t _last_us = 0;

    void pump(const uint8_t *data, int n);
    static void rxTask(void *arg);
    void emit(const char *text);
};
