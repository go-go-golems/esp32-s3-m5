// SPDX-License-Identifier: MIT
// ESP-62 Phase 2 — esp_console REPL with a `qr` command. Phase 2 implements
// `qr status` (the on-device probe: reads module firmware version + serial
// number over UART1). Phase 4 adds start/stop/mode/light/reset.
#pragma once

class QRModule;

class QRConsole {
   public:
    void start(QRModule &module);
};
