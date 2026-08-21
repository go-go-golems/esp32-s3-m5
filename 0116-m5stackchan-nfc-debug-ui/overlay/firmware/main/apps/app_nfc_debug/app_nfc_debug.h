/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "nfc_debug_service.h"
#include <mooncake.h>

class AppNfcDebug : public mooncake::AppAbility {
public:
    AppNfcDebug();

    void onCreate() override;
    void onOpen() override;
    void onRunning() override;
    void onClose() override;

private:
    nfc_debug::Service _service;
    uint32_t _last_generation = 0;
};
