// SPDX-License-Identifier: MIT
// ESP-62 Phase 3 — on-screen UI. A single task drains the scan-result
// queue and redraws the CoreS3 LCD (320x240 landscape): top bar (state +
// firmware), center (last decoded code, large), bottom (recent history),
// footer (button hints). The display is touched only from this task to
// avoid M5GFX races. Buttons: BtnA toggles scanning, BtnB cycles mode.
#pragma once

class QRModule;

class QRUI {
   public:
    void start(QRModule &module);
};
