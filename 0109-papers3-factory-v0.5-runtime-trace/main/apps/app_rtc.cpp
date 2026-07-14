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

void AppRtc::onCreate()
{
    setAppInfo().name = "AppRtc";
    mclog::tagInfo(getAppInfo().name, "onCreate");

    open();  // Open by default
}

void AppRtc::onRunning()
{
    update_date_time();
    update_sleep_and_wake_up_button();
}

void AppRtc::update_date_time()
{
    if (GetHAL().millis() - _time_update_time_count > 1000 || GetHAL().isRefreshRequested()) {
        auto date_time = GetHAL().rtc.getDateTime();

        GetHAL().display.setEpdMode(epd_mode_t::epd_fastest);

        GetHAL().display.fillRect(15, 178, 205, 44, TFT_WHITE);
        GetHAL().display.setTextDatum(middle_right);
        GetHAL().display.setTextColor(TFT_BLACK);
        GetHAL().display.loadFont(font_montserrat_medium_36);

        auto hours   = date_time.time.hours;
        auto minutes = date_time.time.minutes;
        auto seconds = date_time.time.seconds;

        GetHAL().display.drawString(fmt::format("{:02d}:{:02d}:{:02d}", hours, minutes, seconds).c_str(), 218, 200);

        _time_update_time_count = GetHAL().millis();
    }

    if (GetHAL().millis() - _date_update_time_count > 1000 || GetHAL().isRefreshRequested()) {
        auto date_time = GetHAL().rtc.getDateTime();

        GetHAL().display.setEpdMode(epd_mode_t::epd_fastest);

        GetHAL().display.fillRect(15, 236, 205, 46, TFT_WHITE);
        GetHAL().display.setTextDatum(middle_right);
        GetHAL().display.setTextColor(TFT_BLACK);
        GetHAL().display.loadFont(font_montserrat_medium_36);

        auto year  = date_time.date.year;
        auto month = date_time.date.month;
        auto day   = date_time.date.date;

        GetHAL().display.drawString(fmt::format("{}/{:02d}/{:02d}", year, month, day).c_str(), 218, 260);

        _date_update_time_count = GetHAL().millis();
    }
}

void AppRtc::update_sleep_and_wake_up_button()
{
    if (GetHAL().wasTouchClickedArea(839, 267, 110, 160)) {
        GetHAL().tone(4000, 100);

        GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
        GetHAL().display.drawPng(img_logo_start, img_logo_end - img_logo_start, 0, 0);
        GetHAL().delay(2000);

        GetHAL().sleepAndWakeupTest();
    }
}
