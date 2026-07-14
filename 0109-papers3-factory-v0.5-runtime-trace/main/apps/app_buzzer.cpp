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

void AppBuzzer::onCreate()
{
    setAppInfo().name = "AppBuzzer";
    mclog::tagInfo(getAppInfo().name, "onCreate");

    open();  // Open by default
}

static std::mutex _mutex;
static bool _is_beeper_on           = false;
static bool _is_beeper_task_running = false;
static void _beeper_task(void* arg)
{
    _mutex.lock();
    _is_beeper_task_running = true;
    _mutex.unlock();

    bool running = true;
    while (running) {
        for (int i = 0; i < 4; i++) {
            GetHAL().tone(4000, 100);
            vTaskDelay(pdMS_TO_TICKS(100));
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_is_beeper_on) {
                running = false;
                break;
            }
        }

        // Delay 1s and check if beeper is still on
        for (int i = 0; i < 10; i++) {
            vTaskDelay(pdMS_TO_TICKS(100));
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_is_beeper_on) {
                running = false;
                break;
            }
        }
    }

    _mutex.lock();
    _is_beeper_task_running = false;
    _mutex.unlock();

    vTaskDelete(NULL);
}

void AppBuzzer::onRunning()
{
    if (GetHAL().wasTouchClickedArea(238, 320, 207, 207)) {
        bool current_is_beeper_on = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            current_is_beeper_on = _is_beeper_on;
        }

        // Stop beeper
        if (current_is_beeper_on) {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _is_beeper_on = false;
            }

            // Wait until beeper task is stopped
            while (1) {
                vTaskDelay(pdMS_TO_TICKS(100));
                std::lock_guard<std::mutex> lock(_mutex);
                if (!_is_beeper_task_running) {
                    break;
                }
            }

            GetHAL().display.drawPng(img_icon_mute_on_start, img_icon_mute_on_end - img_icon_mute_on_start, 298, 380);
        }
        // Start beeper
        else {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _is_beeper_on = true;
            }
            xTaskCreate(_beeper_task, "beeper", 1024 * 4, NULL, 5, NULL);

            GetHAL().display.drawPng(img_icon_mute_off_start, img_icon_mute_off_end - img_icon_mute_off_start, 298,
                                     380);
        }
    }

    // Handle refresh request
    else if (GetHAL().isRefreshRequested()) {
        bool current_is_beeper_on = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            current_is_beeper_on = _is_beeper_on;
        }

        if (current_is_beeper_on) {
            GetHAL().display.drawPng(img_icon_mute_off_start, img_icon_mute_off_end - img_icon_mute_off_start, 298,
                                     380);
        } else {
            GetHAL().display.drawPng(img_icon_mute_on_start, img_icon_mute_on_end - img_icon_mute_on_start, 298, 380);
        }
    }
}
