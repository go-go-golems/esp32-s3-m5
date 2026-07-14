/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "apps.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mooncake_log.h>
#include <assets.h>
#include <hal.h>
#include <mutex>

using namespace mooncake;

void AppImu::onCreate()
{
    setAppInfo().name = "AppImu";
    mclog::tagInfo(getAppInfo().name, "onCreate");

    open();  // Open by default
}

void AppImu::onRunning()
{
    if (GetHAL().millis() - _time_count > 500 || GetHAL().isRefreshRequested()) {
        GetHAL().imu.update();
        auto imu_data = GetHAL().imu.getImuData();
        // mclog::info("{} {} {}", imu_data.accel.x, imu_data.accel.y, imu_data.accel.z);

        GetHAL().display.setEpdMode(epd_mode_t::epd_fastest);
        GetHAL().display.setTextDatum(middle_left);
        GetHAL().display.setTextColor(TFT_BLACK);
        GetHAL().display.loadFont(font_montserrat_medium_18);

        std::string text;

        GetHAL().display.fillRect(15, 409, 102, 98, TFT_WHITE);
        text = fmt::format("AX: {:0.1f}", imu_data.accel.x);
        GetHAL().display.drawString(text.c_str(), 24, 420);
        text = fmt::format("AY: {:0.1f}", imu_data.accel.y);
        GetHAL().display.drawString(text.c_str(), 24, 458);
        text = fmt::format("AZ: {:0.1f}", imu_data.accel.z);
        GetHAL().display.drawString(text.c_str(), 24, 496);

        GetHAL().display.fillRect(117, 409, 105, 98, TFT_WHITE);
        text = fmt::format("GX: {:0.1f}", imu_data.gyro.x);
        GetHAL().display.drawString(text.c_str(), 122, 420);
        text = fmt::format("GY: {:0.1f}", imu_data.gyro.y);
        GetHAL().display.drawString(text.c_str(), 122, 458);
        text = fmt::format("GZ: {:0.1f}", imu_data.gyro.z);
        GetHAL().display.drawString(text.c_str(), 122, 496);

        _time_count = GetHAL().millis();
    }
}
