/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "nfc_debug_service.h"
#include <memory>
#include <mooncake.h>

namespace nfc_debug::view {
class NfcDebugView;
}

class AppNfcDebug : public mooncake::AppAbility {
public:
    AppNfcDebug();
    ~AppNfcDebug() override;

    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    nfc_debug::Service _service;
    std::unique_ptr<nfc_debug::view::NfcDebugView> _view;
    uint32_t _last_generation = 0;
};
