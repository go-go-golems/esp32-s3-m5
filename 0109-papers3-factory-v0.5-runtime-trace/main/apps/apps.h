/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <mooncake.h>
#include <M5GFX.h>
#include <cstdint>
#include <memory>

/**
 * @brief
 *
 */
class AppPower : public mooncake::AppAbility {
public:
    void onCreate() override;
    void onRunning() override;

private:
    struct UpdateTimeCount_t {
        uint32_t batVoltage = 0;
        uint32_t iconChg    = 0;
        uint32_t lowBat     = 0;
    };
    UpdateTimeCount_t _time_count;
    bool _current_icon_chg = false;

    void update_bat_voltage();
    void update_icon_chg();
    void update_shut_down_button();
    void check_low_battery_power_off();
};

/**
 * @brief
 *
 */
class AppSdCard : public mooncake::AppAbility {
public:
    void onCreate() override;
    void onRunning() override;

private:
    uint32_t _time_count = 0;
};

/**
 * @brief
 *
 */
class AppRtc : public mooncake::AppAbility {
public:
    void onCreate() override;
    void onRunning() override;

private:
    uint32_t _time_update_time_count = 0;
    uint32_t _date_update_time_count = 0;

    void update_date_time();
    void update_sleep_and_wake_up_button();
};

/**
 * @brief
 *
 */
class AppBuzzer : public mooncake::AppAbility {
public:
    void onCreate() override;
    void onRunning() override;
};

/**
 * @brief
 *
 */
class AppImu : public mooncake::AppAbility {
public:
    void onCreate() override;
    void onRunning() override;

private:
    uint32_t _time_count = 0;
};

/**
 * @brief
 *
 */
class AppWifi : public mooncake::AppAbility {
public:
    void onCreate() override;
    void onRunning() override;

    static bool is_wifi_start_scanning();

private:
    enum State_t {
        STATE_IDLE = 0,
        STATE_FIRST_SCAN,
        STATE_SCANNING_RESULT,
    };

    State_t _state       = STATE_IDLE;
    uint32_t _time_count = 0;

    void handle_state_idle();
    void handle_state_first_scan();
    void handle_state_scanning_result();
};
