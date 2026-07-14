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

static bool _is_wifi_start_scanning = false;
bool AppWifi::is_wifi_start_scanning()
{
    return _is_wifi_start_scanning;
}

void AppWifi::onCreate()
{
    setAppInfo().name = "AppWifi";
    mclog::tagInfo(getAppInfo().name, "onCreate");

    // Draw wifi scan icon
    GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
    GetHAL().display.drawPng(img_icon_wifi_scan_start, img_icon_wifi_scan_end - img_icon_wifi_scan_start, 543, 287);

    open();  // Open by default
}

static std::mutex _scanning_mutex;
static std::mutex _result_mutex;
static bool _is_scanning = false;
static void _wifi_scan_task(void* arg)
{
    mclog::info("start wifi scan task");

    while (1) {
        _scanning_mutex.lock();
        _is_scanning = true;
        _scanning_mutex.unlock();

        _result_mutex.lock();
        GetHAL().wifiScan();
        _result_mutex.unlock();

        _scanning_mutex.lock();
        _is_scanning = false;
        _scanning_mutex.unlock();

        GetHAL().delay(5000);
    }
}

void AppWifi::onRunning()
{
    switch (_state) {
        case STATE_IDLE:
            handle_state_idle();
            break;
        case STATE_FIRST_SCAN:
            handle_state_first_scan();
            break;
        case STATE_SCANNING_RESULT:
            handle_state_scanning_result();
            break;
        default:
            mclog::tagError(getAppInfo().name, "unknown state: {}", (int)_state);
            break;
    }
}

void AppWifi::handle_state_idle()
{
    if (GetHAL().isRefreshRequested()) {
        GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
        GetHAL().display.drawPng(img_icon_wifi_scan_start, img_icon_wifi_scan_end - img_icon_wifi_scan_start, 543, 287);
    }

    // Wait scan button clicked
    if (GetHAL().wasTouchClickedArea(506, 248, 276, 127)) {
        GetHAL().tone(4000, 100);

        xTaskCreate(_wifi_scan_task, "scan", 1024 * 4, NULL, 5, NULL);

        GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
        GetHAL().display.fillRect(543, 287, 201, 53, TFT_WHITE);

        _state = STATE_FIRST_SCAN;
    }
}

void AppWifi::handle_state_first_scan()
{
    if (GetHAL().millis() - _time_count > 1000 || GetHAL().isRefreshRequested()) {
        _scanning_mutex.lock();
        bool is_scanning = _is_scanning;
        _scanning_mutex.unlock();

        if (!is_scanning) {
            _state                  = STATE_SCANNING_RESULT;
            _is_wifi_start_scanning = true;
            _time_count             = 0;
            return;
        }

        GetHAL().display.setEpdMode(epd_mode_t::epd_text);
        GetHAL().display.setTextDatum(middle_center);
        GetHAL().display.setTextColor(TFT_BLACK);
        GetHAL().display.loadFont(font_montserrat_medium_24);
        GetHAL().display.drawString("SCANNING...", 644, 312);

        _time_count = GetHAL().millis();
    }
}

void AppWifi::handle_state_scanning_result()
{
    if (GetHAL().millis() - _time_count > 5000 || GetHAL().isRefreshRequested()) {
        std::lock_guard<std::mutex> lock(_result_mutex);

        GetHAL().display.setEpdMode(epd_mode_t::epd_quality);
        GetHAL().display.loadFont(font_montserrat_medium_18);
        GetHAL().display.setTextDatum(middle_left);

        // Clear
        GetHAL().display.fillRect(463, 97, 363, 431, TFT_WHITE);

        std::string text;

        // Title
        GetHAL().display.fillRect(465, 105, 164, 32, (uint32_t)0xE6E6E6);
        GetHAL().display.fillRect(465, 137, 5, 379, (uint32_t)0xE6E6E6);
        GetHAL().display.fillRect(470, 137, 351, 32, TFT_BLACK);
        GetHAL().display.drawString("SCAN RESULT:", 478, 121);

        // Best SSID
        GetHAL().display.setTextColor(TFT_WHITE);
        text = fmt::format("Best: {} ({} dBm)", GetHAL().getWifiScanResult().bestSsid,
                           GetHAL().getWifiScanResult().bestRssi);
        GetHAL().display.drawString(text.c_str(), 478, 153);

        // Wifi list
        int start_x = 478;
        int start_y = 197;
        int gap     = 33;
        int max_num = 10;

        GetHAL().display.setTextColor(TFT_BLACK);
        for (int i = 0; i < GetHAL().getWifiScanResult().ap_list.size(); i++) {
            if (i >= max_num) {
                break;
            }

            text = fmt::format("{} ({} dBm)", GetHAL().getWifiScanResult().ap_list[i].second,
                               GetHAL().getWifiScanResult().ap_list[i].first);
            GetHAL().display.drawString(text.c_str(), start_x, start_y + i * gap);
        }

        _time_count = GetHAL().millis();
    }
}
