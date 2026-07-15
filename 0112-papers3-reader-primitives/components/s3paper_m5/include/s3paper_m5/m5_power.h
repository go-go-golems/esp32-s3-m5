// M5Unified power shims (Phase 10): the ONLY module that may call M5.Power
// and M5.Rtc. Owner-task-only, after M5Backend::Init().
//
// Verified wake sources for the actual M5PaperS3 (m5unified 0.2.18,
// Power_Class.cpp): the touch INT (GPIO48) is NOT an RTC IO on ESP32-S3,
// so deep sleep wakes by TIMER ONLY; a full power-off wakes via the BM8563
// RTC alarm (timerSleep) or the physical side button. Battery is a plain
// ADC divider (GPIO3, ratio 2.0) with charge status on GPIO4.
#pragma once

#include <stdint.h>

namespace s3paper {

struct PowerStatus {
    int32_t battery_level;   // 0..100, -1 unknown
    int32_t battery_mv;      // millivolts, -1 unknown
    bool charging;
    uint8_t wakeup_cause;    // esp_sleep_wakeup_cause_t of this boot
    uint8_t reset_reason;    // esp_reset_reason_t of this boot
};

// Reads battery/charge state and this boot's wake/reset causes.
PowerStatus PowerRead();

// Display sleep + deep sleep with a timer wake (RAM lost; wake = reboot).
// Never returns.
[[noreturn]] void PowerDeepSleep(uint64_t wake_after_us);

// Display sleep + BM8563 alarm + power-latch off (true power-off; wakes
// after `seconds` via RTC or any time via the side button). Never returns.
[[noreturn]] void PowerRtcOff(int32_t seconds);

// Display sleep + power-latch off. Wake: side button only. Never returns.
[[noreturn]] void PowerOff();

}  // namespace s3paper
