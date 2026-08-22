// SPDX-License-Identifier: MIT
//
// NFC.LAB app — uses the reusable gogolem_nfc_engine Service on the board-owned
// I²C bus. UI callbacks only submit commands; the worker owns NFC operations.

#include "app_nfc_debug.h"
#include "view/nfc_debug_view.h"

#include <esp_err.h>
#include <hal/board/hal_bridge.h>
#include <hal/hal.h>
#include <mooncake_log.h>

using namespace gogolem::nfc;

AppNfcDebug::AppNfcDebug()
{
    setAppInfo().name = "NFC.LAB";
    static uint32_t theme_color = 0x6D4AFF;
    setAppInfo().userData = static_cast<void*>(&theme_color);
}

AppNfcDebug::~AppNfcDebug() = default;

void AppNfcDebug::onCreate()
{
    mclog::tagInfo(getAppInfo().name, "on create (NFC-only auto-open)");
    open();
}

void AppNfcDebug::onOpen()
{
    mclog::tagInfo(getAppInfo().name, "on open");
    _last_operations = 0;

    ServiceConfig scfg{};
    scfg.engine.bus = hal_bridge::board_get_i2c_bus();
    scfg.engine.mode = Mode::Reader;

    auto start = _service.start(scfg);
    if (!start.ok()) {
        mclog::tagError(getAppInfo().name, "service start failed: {}", error_layer_name(start.error().layer));
    }

    LvglLockGuard lock;
    _view = std::make_unique<nfc_debug::view::NfcDebugView>(_service, []() {},
        [this](bool on) { _auto_poll.store(on); });
    if (!start.ok()) _view->show_start_error(start.error());
}

void AppNfcDebug::onRunning()
{
    // Auto-poll: submit commands periodically when enabled.
    if (_auto_poll.load()) {
        gogolem::nfc::Command cmd{};
        cmd.kind = gogolem::nfc::ServiceCommand::ActivateOne;
        _service.submit(cmd);
        cmd.kind = gogolem::nfc::ServiceCommand::RawRead;
        cmd.address = 0;
        _service.submit(cmd);
        cmd.kind = gogolem::nfc::ServiceCommand::ReadNdef;
        _service.submit(cmd);
    }

    gogolem::nfc::ServiceSnapshot snap{};
    if (!_service.latest(snap) || snap.operations == _last_operations) return;
    _last_operations = snap.operations;

    mclog::tagInfo(getAppInfo().name, "ops={} fail={} tag={} ndef_ok={}",
                   snap.operations, snap.failures,
                   snap.tag_present ? 1 : 0,
                   snap.ndef_ok ? 1 : 0);

    LvglLockGuard lock;
    if (_view) _view->update(snap);
}

void AppNfcDebug::onClose()
{
    mclog::tagInfo(getAppInfo().name, "on close");
    _service.stop();

    LvglLockGuard lock;
    _view.reset();
}
