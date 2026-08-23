// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 — M14-Pro scanner protocol engine (ESP-IDF port of the
// official Arduino qrcode_m14.cpp). See sources/protocol-pdf/ for the spec
// and sources/arduino-lib/src/qrcode_m14.cpp for the reference.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "driver/uart.h"

class QRCodeM14 {
   public:
    enum CmdResult { OK = 0, INVALID, TIMEOUT, ACK_MISMATCH };

    // Mirror of QRCodeM14::TriggerMode_t (protocol PID 0x61 FID 0x41).
    enum TriggerMode {
        KEY = 0,
        CONTINUOUS = 1,
        AUTO = 2,
        PULSE = 4,
        MOTION = 5,
    };
    enum FillLightMode { FILL_OFF = 0, FILL_ON_DECODE = 2, FILL_ON = 3 };
    enum PosLightMode { POS_OFF = 0, POS_FLASH_DECODE = 1, POS_ON_DECODE = 2 };

    // Open the UART at 115200 8N1. This firmware routes the stacked scanner
    // on G13/G14 to avoid the Gateway H2's fixed G18 connection.
    void begin(uart_port_t port, int tx, int rx, int baud = 115200);

    static const char *resultName(CmdResult result);

    // Send a framed command; optionally match an ACK byte sequence.
    CmdResult sendCmd(const uint8_t *cmd, size_t n, const uint8_t *ack = nullptr,
                      size_t ack_len = 0, uint32_t timeout_ms = 1000);

    // Status Read (0x43 0x02 <id>) -> reply 44 <pid> <fid> <len_hi> <len_lo>
    // <data...>. Decodes the big-endian length at offset [3:4] and copies data
    // to out. Returns false on timeout/parse error.
    bool getInfos(uint8_t id, char *out, size_t out_cap);

    // Control + config helpers (exact bytes from qrcode_m14.cpp). Return the
    // transport/ACK result so callers never confuse "queued" with "applied".
    CmdResult startDecode();            // 32 75 01 (no reply)
    CmdResult stopDecode();             // 32 75 02 -> 33 75 02 00 00
    CmdResult setTriggerMode(TriggerMode m);      // 21 61 41 <m>
    CmdResult setFillLightMode(FillLightMode m);  // 21 62 41 <m>
    CmdResult setPosLightMode(PosLightMode m);    // 21 62 42 <m>
    CmdResult setModeUart();             // 21 42 40 00 (force RS232/UART output)
    CmdResult enableSuffixCrLf();       // suffix enable + suffix bytes
    CmdResult setFillLightBrightness(int pct);  // 21 62 48 <0..100>
    CmdResult setDecodeSuccessBeep(int count);  // 21 63 42 <count>
    CmdResult factoryReset();           // 32 76 01 (no reply defined)

    // Bytes currently available to read from the UART (for the scan pump).
    int available();

    // Pull up to cap bytes; returns count read (0 on timeout).
    int readBytes(uint8_t *buf, size_t cap, int timeout_ms);

    uart_port_t port() const { return _port; }

   private:
    uart_port_t _port = UART_NUM_MAX;
};
