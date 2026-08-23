// SPDX-License-Identifier: MIT
// ESP-62 Phase 3 — on-screen UI. A single task drains the scan-result
// queue and redraws the CoreS3 LCD (320x240 landscape): top bar (state +
// firmware), center (last decoded code, large), bottom (recent history),
// footer (touch hint). The display is touched only from this task to avoid
// M5GFX races. Any touch requests a hardware-trigger fallback pulse.
#pragma once

class QRModule;

class QRUI {
   public:
    // Firmware was queried once during minimal startup; the UI must not issue
    // a second scanner transaction while boot diagnostics are in progress.
    void start(QRModule &module, const char *firmware, bool configured_ready);
};
