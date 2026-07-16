// Power lifecycle. Owner-task-only.
//
// Sleep is a coordinated sequence, never a bare powerOff(). Quiesce order
// (hardware-verified in ESP-50):
//   1. touch tick producer off (input quiesced)
//   2. final persistence flush (state files already atomic)
//   3. retained sleep image presented CleanFull (the present's bounded
//      busy-wait guarantees the EPD is idle before power drops)
//   4. SD unmounted
//   5. console log flushed
//   6. wake source configured and the power transition taken
// Wake is a reboot on this hardware; the boot flow is the resume contract.
#pragma once

#include "app_events.h"
#include "s3paper/widget.h"

namespace pulp {

enum class SleepMode : uint8_t {
    DeepTimer = 1,  // esp deep sleep, timer wake (USB stays powered)
    RtcOff = 2,     // true power-off, BM8563 RTC alarm or side button wakes
    Off = 3,        // true power-off, side button wakes
};

// Builds the sleep image into the widget arena and returns the content
// slot to present, or kNullWidget to use the built-in placeholder. Phase 8
// points this at the JS sleepImage(fn) lambda.
using SleepImageBuilder = s3paper::WidgetHandle (*)(SleepMode mode,
                                                    uint32_t seconds);
void PowerSetSleepImageBuilder(SleepImageBuilder builder);

// Runs the quiesce sequence and enters `mode`. Does not return on success.
StatusCode PowerSleep(SleepMode mode, uint32_t seconds);

void FillPowerSnapshot(PowerSnapshot *out);

// Inactivity auto-sleep policy: after `seconds` without touch input the
// owner powers off (Off mode). 0 disables (default).
void PowerSetAutoSleep(uint32_t seconds);
void PowerAutoTick(int64_t now_us);

// Logs this boot's reset reason and wakeup cause (resume evidence).
void PowerLogBootCause();

}  // namespace pulp
