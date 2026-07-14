/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "apps.h"
#include <mooncake_log.h>
#include <assets.h>
#include <hal.h>

using namespace mooncake;

void AppPower::onCreate()
{
    setAppInfo().name = "AppPower";
    mclog::tagInfo(getAppInfo().name, "onCreate");

    open();  // Open by default
}

void AppPower::onRunning()
{
    update_bat_voltage();
    update_icon_chg();
    update_shut_down_button();
    check_low_battery_power_off();
}

void AppPower::update_bat_voltage()
{
    if (GetHAL().millis() - _time_count.batVoltage > 2000 || GetHAL().isRefreshRequested()) {
        std::string voltage = fmt::format(" {:0.2f}V ", GetHAL().getBatteryVoltage());

        GetHAL().display.setEpdMode(epd_mode_t::epd_fastest);
        GetHAL().display.loadFont(font_montserrat_medium_24);
        GetHAL().display.setTextDatum(middle_center);
        GetHAL().display.setTextColor(TFT_BLACK, TFT_WHITE);
        GetHAL().display.drawString(voltage.c_str(), 856, 41);

        _time_count.batVoltage = GetHAL().millis();
    }
}

void AppPower::update_icon_chg()
{
    if (GetHAL().millis() - _time_count.iconChg > 100 || GetHAL().isRefreshRequested()) {
        bool is_usb_connected = GetHAL().isUsbConnected();
        if (is_usb_connected != _current_icon_chg || GetHAL().isRefreshRequested()) {
            _current_icon_chg = is_usb_connected;

            GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
            if (_current_icon_chg) {
                GetHAL().display.drawPng(img_icon_chg_start, img_icon_chg_end - img_icon_chg_start, 733, 16);
            } else {
                GetHAL().display.fillRect(733, 16, 50, 50, TFT_WHITE);
            }
        }

        _time_count.iconChg = GetHAL().millis();
    }
}

void AppPower::update_shut_down_button()
{
    if (GetHAL().wasTouchClickedArea(839, 97, 110, 160)) {
        GetHAL().tone(4000, 100);

        GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
        GetHAL().display.drawPng(img_logo_start, img_logo_end - img_logo_start, 0, 0);
        GetHAL().delay(2000);

        GetHAL().powerOff();
    }
}

void AppPower::check_low_battery_power_off()
{
    // Only auto power off when wifi start scanning
    if (!AppWifi::is_wifi_start_scanning()) {
        return;
    }

    if (GetHAL().millis() - _time_count.lowBat > 2000) {
        bool is_usb_connected = GetHAL().isUsbConnected();
        float battery_voltage = GetHAL().getBatteryVoltage();

        if (!is_usb_connected && (battery_voltage < 3.8)) {
            GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
            GetHAL().display.drawPng(img_logo_start, img_logo_end - img_logo_start, 0, 0);
            GetHAL().delay(2000);

            GetHAL().powerOff();
        }

        _time_count.lowBat = GetHAL().millis();
    }
}
