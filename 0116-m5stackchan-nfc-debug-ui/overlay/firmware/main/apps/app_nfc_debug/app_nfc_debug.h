// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <memory>
#include <mooncake.h>

#include "gogolem/nfc/service.hpp"

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
    gogolem::nfc::Service _service;
    std::unique_ptr<nfc_debug::view::NfcDebugView> _view;
    uint32_t _last_operations = 0;
    std::atomic<bool> _auto_poll{false};
};
