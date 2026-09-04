/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "M5ModuleQRCode.h"
#include "debug.h"

#define CHANNEL_QRCODE_POWER_EN 0
#define CHANNEL_QRCODE_TRIG     4

M5ModuleQRCode::~M5ModuleQRCode()
{
    if (_pi4ioe5v6408 != nullptr) {
        delete _pi4ioe5v6408;
        _pi4ioe5v6408 = nullptr;
    }
}

bool M5ModuleQRCode::begin()
{
    if (!_init_pi4ioe5v6408()) {
        return false;
    }

    setEnable(true);
    delay(300);

    if (!_init_qrcode()) {
        return false;
    }

    return true;
}

bool M5ModuleQRCode::_init_pi4ioe5v6408()
{
    _LOG_DEBUG("init pi4ioe5v6408\n");

    if (_config.i2c == nullptr) {
        _config.i2c = &M5.In_I2C;
    }

    // Probe device
    if (_pi4ioe5v6408 != nullptr) {
        delete _pi4ioe5v6408;
        _pi4ioe5v6408 = nullptr;
    }

    _pi4ioe5v6408 = new m5::PI4IOE5V6408_Class(_config.pi4ioe5v6408_addr, 100000, _config.i2c);

    if (_pi4ioe5v6408 == nullptr) {
        _LOG_ERROR("pi4ioe5v6408 malloc failed\n");
        return false;
    }

    if (!_pi4ioe5v6408->begin()) {
        _LOG_ERROR("pi4ioe5v6408 not found at 0x%02x\n", _config.pi4ioe5v6408_addr);
        delete _pi4ioe5v6408;
        _pi4ioe5v6408 = nullptr;
        return false;
    }

    _LOG_DEBUG("pi4ioe5v6408 found at 0x%02x\n", _config.pi4ioe5v6408_addr);

    _pi4ioe5v6408->setDirection(CHANNEL_QRCODE_POWER_EN, true);
    _pi4ioe5v6408->setPullMode(CHANNEL_QRCODE_POWER_EN, true);
    _pi4ioe5v6408->enablePull(CHANNEL_QRCODE_POWER_EN, true);
    _pi4ioe5v6408->setHighImpedance(CHANNEL_QRCODE_POWER_EN, false);

    _pi4ioe5v6408->setDirection(CHANNEL_QRCODE_TRIG, true);
    _pi4ioe5v6408->setPullMode(CHANNEL_QRCODE_TRIG, true);
    _pi4ioe5v6408->enablePull(CHANNEL_QRCODE_TRIG, true);
    _pi4ioe5v6408->setHighImpedance(CHANNEL_QRCODE_TRIG, false);

    return true;
}

bool M5ModuleQRCode::_init_qrcode()
{
    _LOG_DEBUG("init qrcode\n");

    _LOG_DEBUG("init qrcode serial tx: %d, rx: %d\n", _config.pin_tx, _config.pin_rx);
    _config.serial->begin(115200, SERIAL_8N1, _config.pin_rx, _config.pin_tx);

    _setup(_config.serial);

    return true;
}

bool M5ModuleQRCode::checkConnection()
{
    auto version = getFirmwareVersion();
    return !version.empty();
}

void M5ModuleQRCode::setEnable(bool enable)
{
    if (enable) {
        _pi4ioe5v6408->digitalWrite(CHANNEL_QRCODE_POWER_EN, true);
        setTriggerLevel(true);
    } else {
        _pi4ioe5v6408->digitalWrite(CHANNEL_QRCODE_POWER_EN, false);
        setTriggerLevel(false);
    }
}

void M5ModuleQRCode::setTriggerLevel(bool level)
{
    if (level) {
        _pi4ioe5v6408->digitalWrite(CHANNEL_QRCODE_TRIG, true);
    } else {
        _pi4ioe5v6408->digitalWrite(CHANNEL_QRCODE_TRIG, false);
    }
}

void M5ModuleQRCode::update()
{
    _scan_result.clear();

    if (QRCodeM14::available()) {
        waitScanResult(_scan_result, 0);

        if (_on_scan_result && available()) {
            _on_scan_result(_scan_result);
        }
    }
}
