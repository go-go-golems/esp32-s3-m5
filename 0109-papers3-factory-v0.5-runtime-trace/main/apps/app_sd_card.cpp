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

void AppSdCard::onCreate()
{
    setAppInfo().name = "AppSdCard";
    mclog::tagInfo(getAppInfo().name, "onCreate");

    open();  // Open by default
}

void AppSdCard::onRunning()
{
    static auto last_result = GetHAL().getSdCardTestResult();

    if (GetHAL().millis() - _time_count > 2000 || GetHAL().isRefreshRequested()) {
        GetHAL().sdCardTest();
        auto& result = GetHAL().getSdCardTestResult();

        // Return if same result
        if (!GetHAL().isRefreshRequested()) {
            if (result == last_result) {
                _time_count = GetHAL().millis();
                return;
            }
        }
        last_result = result;

        GetHAL().display.setEpdMode(epd_mode_t::epd_text);

        GetHAL().display.fillRect(260, 178, 185, 104, TFT_WHITE);
        GetHAL().display.setTextDatum(middle_left);
        GetHAL().display.setTextColor(TFT_BLACK);
        GetHAL().display.loadFont(font_montserrat_medium_24);

        if (result.is_mounted) {
            GetHAL().display.drawString(result.size.c_str(), 267, 197);
            GetHAL().display.loadFont(font_montserrat_medium_18);
            GetHAL().display.drawString(result.type.c_str(), 267, 234);
            GetHAL().display.drawString(result.name.c_str(), 267, 267);
        } else {
            GetHAL().display.drawString("Not Found", 271, 231);
        }

        _time_count = GetHAL().millis();
    }
}
