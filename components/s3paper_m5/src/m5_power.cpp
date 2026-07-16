#include "s3paper_m5/m5_power.h"

#include <M5Unified.hpp>

#include "esp_sleep.h"
#include "esp_system.h"

namespace s3paper {

PowerStatus PowerRead() {
    PowerStatus out{};
    out.battery_level = M5.Power.getBatteryLevel();
    out.battery_mv = M5.Power.getBatteryVoltage();
    out.charging =
        M5.Power.isCharging() == m5::Power_Class::is_charging_t::is_charging;
    out.wakeup_cause = static_cast<uint8_t>(esp_sleep_get_wakeup_cause());
    out.reset_reason = static_cast<uint8_t>(esp_reset_reason());
    return out;
}

[[noreturn]] void PowerDeepSleep(uint64_t wake_after_us) {
    // touch_wakeup=false: GPIO48 is not RTC-capable on the S3 (see header).
    M5.Power.deepSleep(wake_after_us, false);
    esp_deep_sleep_start();  // deepSleep never returns; belt and braces
}

[[noreturn]] void PowerRtcOff(int32_t seconds) {
    M5.Power.timerSleep(seconds);
    for (;;) {
        vTaskDelay(portMAX_DELAY);  // power latch drops momentarily
    }
}

[[noreturn]] void PowerOff() {
    M5.Power.powerOff();
    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}

}  // namespace s3paper
