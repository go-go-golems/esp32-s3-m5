/*
 * SPDX-FileCopyrightText: 2026 ESP-60-M5STACKCHAN-NFC
 * SPDX-License-Identifier: MIT
 */
#include "app_nfc_debug.h"
#include "view/nfc_debug_view.h"

#include <assets/assets.h>
#include <esp_err.h>
#include <hal/board/hal_bridge.h>
#include <hal/hal.h>
#include <mooncake_log.h>

AppNfcDebug::AppNfcDebug()
{
    setAppInfo().name = "NFC.LAB";
    static auto icon = assets::get_image("icon_setup.bin");
    setAppInfo().icon = static_cast<void*>(&icon);
    static uint32_t theme_color = 0x6D4AFF;
    setAppInfo().userData = static_cast<void*>(&theme_color);
}

AppNfcDebug::~AppNfcDebug() = default;

void AppNfcDebug::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create");
}

void AppNfcDebug::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");
    _last_generation = 0;
    const esp_err_t result = _service.start(hal_bridge::board_get_i2c_bus());
    if (result != ESP_OK) {
        mclog::tagError(getAppInfo().name, "service start failed: {}", esp_err_to_name(result));
    }

    LvglLockGuard lock;
    _view = std::make_unique<nfc_debug::view::NfcDebugView>(_service, [this]() { close(); });
    if (result != ESP_OK) _view->show_start_error(result);
}

void AppNfcDebug::onRunning()
{
    nfc_debug::Snapshot snapshot{};
    if (!_service.latest(snapshot) || snapshot.generation == _last_generation) return;
    _last_generation = snapshot.generation;
    mclog::tagInfo(getAppInfo().name, "state={} generation={} errors={}",
                   nfc_debug::reader_state_name(snapshot.reader_state),
                   snapshot.generation, snapshot.counters.failed);

    LvglLockGuard lock;
    if (_view) _view->update(snapshot);
}

void AppNfcDebug::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    _service.stop();

    LvglLockGuard lock;
    _view.reset();
}
