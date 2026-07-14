/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <M5Unified.hpp>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>

class Hal {
public:
    void init();

    M5GFX& display     = M5.Display;
    m5::IMU_Class& imu = M5.Imu;
    m5::RTC_Class& rtc = M5.Rtc;

    /* -------------------------------------------------------------------------- */
    /*                                   System                                   */
    /* -------------------------------------------------------------------------- */
    void delay(std::uint32_t ms)
    {
        m5gfx::delay(ms);
    }
    std::uint32_t millis()
    {
        return m5gfx::millis();
    }
    void feedTheDog();
    void requestRefresh()
    {
        _refresh_request = true;
    }
    bool isRefreshRequested()
    {
        return _refresh_request;
    }
    void clearRefreshRequest()
    {
        if (_refresh_request) {
            _refresh_request = false;
        }
    }

    /* -------------------------------------------------------------------------- */
    /*                                    Power                                   */
    /* -------------------------------------------------------------------------- */
    int getChgState();
    bool isUsbConnected();
    float getBatteryVoltage();
    void powerOff();
    void sleepAndWakeupTest();

    /* -------------------------------------------------------------------------- */
    /*                                   SD Card                                  */
    /* -------------------------------------------------------------------------- */
    struct SdCardTestResult_t {
        bool is_mounted = false;
        std::string size;
        std::string type;
        std::string name;

        bool operator==(const SdCardTestResult_t& other) const
        {
            return is_mounted == other.is_mounted && size == other.size && type == other.type && name == other.name;
        }
    };

    void sdCardTest();
    SdCardTestResult_t& getSdCardTestResult()
    {
        return _sd_card_test_result;
    }

    /* -------------------------------------------------------------------------- */
    /*                                    WiFi                                    */
    /* -------------------------------------------------------------------------- */
    struct WifiScanResult_t {
        std::vector<std::pair<int, std::string>> ap_list;  // From strongest to weakest
        std::string bestSsid;
        int bestRssi = -100;
    };

    void wifiScan();
    WifiScanResult_t& getWifiScanResult()
    {
        return _wifi_scan_result;
    }

    /* -------------------------------------------------------------------------- */
    /*                                   Buzzer                                   */
    /* -------------------------------------------------------------------------- */
    void tone(int frequency, int duration);
    void noTone();

    /* -------------------------------------------------------------------------- */
    /*                                    Touch                                   */
    /* -------------------------------------------------------------------------- */
    bool isTouchPressed()
    {
        return M5.Touch.getCount() > 0;
    }
    const m5::Touch_Class::touch_detail_t& getTouchDetail()
    {
        return M5.Touch.getDetail();
    }
    bool wasTouchClickedArea(int x, int y, int w, int h);

private:
    bool _refresh_request    = false;
    bool _is_sd_card_mounted = false;
    WifiScanResult_t _wifi_scan_result;
    SdCardTestResult_t _sd_card_test_result;

    void rtc_init();
    void power_init();
    void sd_card_init();
    void wifi_init();
    void buzzer_init();
    void ext_port_init();
};

Hal& GetHAL();
