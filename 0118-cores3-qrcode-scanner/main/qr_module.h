// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 (revised) — Module13.2 QRCode module.
//
// ARCHITECTURE: a single FreeRTOS task owns UART1 (the only code that calls
// uart_read_bytes/uart_write_bytes on UART1). All other code (console, UI)
// talks to it via a request queue + response semaphore, so UART access is
// fully serialized — no race between the scan pump and command replies.
//
// Mirrors sources/arduino-lib/src/M5ModuleQRCode.cpp for the hardware wiring
// (PI4IOE5V6408 @0x43 power-en ch0 + TRIG ch4) and the M14-Pro protocol.
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

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

// Request sent to the UART-owner task. Commands that need a reply fill the
// response slot + give the semaphore; fire-and-forget commands leave
// resp_sem NULL.
enum class QRReqType {
    GetInfo,        // status read 0x43 0x02 <id> -> reply in resp_str
    StartDecode,    // 32 75 01 (no reply)
    StopDecode,     // 32 75 02
    SetTriggerMode,
    SetFillLight,
    SetPosLight,
    SetBrightness,
    SetBeep,
    FactoryReset,
    SetModeUart,    // 21 42 40 00 (force RS232 output)
    EnableSuffixCrLf,  // 21 51 4C 01 + 21 51 C2 00 02 0D 0A
};

struct QRRequest {
    QRReqType type;
    uint8_t arg_u8 = 0;          // mode/brightness/beep-count/info-id
    SemaphoreHandle_t resp_sem = nullptr;  // caller gives; task gives on done
    // Response is written through these pointers (shared with the caller), so
    // the owner task's local copy of the request doesn't lose the result
    // (FreeRTOS queues copy the struct by value).
    char *resp_out = nullptr;
    size_t resp_cap = 0;
    bool *resp_ok_flag = nullptr;
};

class QRModule {
   public:
    // Scanner UART on G13/G14 (matches the QRCode DIP route to M5-Bus
    // pin 23 = QR_RX <- G13 TX, pin 26 = QR_TX -> G14 RX).
    static constexpr int kUartTx = 13;
    static constexpr int kUartRx = 14;
    static constexpr uart_port_t kUart = UART_NUM_1;
    static constexpr uint8_t kChPowerEn = 0;  // QR_5V_EN
    static constexpr uint8_t kChTrig = 4;     // TRIG

    bool begin();
    void setEnable(bool en);
    void setTriggerLevel(bool high);  // TRIG active-low
    void startScan();
    void stopScan();

    // Synchronous command API (blocks until the UART-owner task replies).
    bool getInfo(uint8_t id, char *out, size_t cap);
    void setTriggerMode(QRCodeM14::TriggerMode m);
    void setFillLightMode(QRCodeM14::FillLightMode m);
    void setPosLightMode(QRCodeM14::PosLightMode m);
    void setBrightness(int pct);
    void setBeep(int count);
    void factoryReset();
    void setModeUart();           // route through owner task (no direct engine access)
    void enableSuffixCrLf();

    QRCodeM14 &engine() { return _engine; }
    QueueHandle_t resultQueue() const { return _q; }

    // Debug-only: claim UART for a raw command (pauses the scan pump).
    void pausePump() { _pump_paused = true; }
    void resumePump() { _pump_paused = false; }

   private:
    QRCodeM14 _engine;
    std::unique_ptr<m5::PI4IOE5V6408_Class> _io_exp;
    QueueHandle_t _req_q = nullptr;   // QRRequest in
    QueueHandle_t _q = nullptr;       // ScanResult out (UI consumes)
    volatile bool _pump_paused = false;

    // scan accumulator (owned by the UART task)
    char _line[256];
    size_t _len = 0;
    int64_t _last_us = 0;

    void pump(const uint8_t *data, int n);
    void emit(const char *text);
    void handle(QRRequest &r);
    static void ownerTask(void *arg);
};
