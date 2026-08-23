/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "qrcode_m14.h"
#include <Arduino.h>
#include <M5Unified.h>
#include <utility/PI4IOE5V6408_Class.hpp>
#include <functional>
#include <string>
#include <memory>

class M5ModuleQRCode : public QRCodeM14 {
public:
    /**
     * @brief Module configuration.
     */
    struct Config_t {
        int pin_tx                = 1;
        int pin_rx                = 3;
        HardwareSerial* serial    = &Serial1;
        unsigned long baudrate    = 115200;
        m5::I2C_Class* i2c        = &M5.In_I2C;
        uint8_t pi4ioe5v6408_addr = 0x43;
    };

    ~M5ModuleQRCode();

    Config_t getConfig() const
    {
        return _config;
    }
    void setConfig(const Config_t& config)
    {
        _config = config;
    }

    /**
     * @brief Initialize module.
     *
     * @return true
     * @return false
     */
    bool begin();

    /**
     * @brief Check if qr module is connected.
     *
     * @return true
     * @return false
     */
    bool checkConnection();

    /**
     * @brief Set qrcode module power enable.
     *
     * @param enable
     */
    void setEnable(bool enable);

    /**
     * @brief Set qrcode module trigger level.
     *
     * @param level
     */
    void setTriggerLevel(bool level);

    /**
     * @brief Update scan result.
     */
    void update();

    /**
     * @brief Check if scan result is available.
     *
     * @return true
     * @return false
     */
    inline bool available() const
    {
        return !_scan_result.empty();
    }

    /**
     * @brief Get scan result.
     *
     * @return std::string
     */
    inline std::string getScanResult() const
    {
        return _scan_result;
    }

    /**
     * @brief Set on scan result callback.
     *
     * @param callback
     */
    inline void onScanResult(std::function<void(const std::string&)> callback)
    {
        _on_scan_result = callback;
    }

private:
    Config_t _config;
    m5::PI4IOE5V6408_Class* _pi4ioe5v6408 = nullptr;
    std::string _scan_result;
    std::function<void(const std::string&)> _on_scan_result;

    bool _init_pi4ioe5v6408();
    bool _init_qrcode();
};
