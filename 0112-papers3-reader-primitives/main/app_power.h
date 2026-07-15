// Power lifecycle (Phase 10). Owner-task-only.
//
// Sleep is a coordinated sequence, never a bare powerOff() (design §Phase
// 10). Documented quiesce order (task 542x):
//   1. touch tick producer off (input quiesced)
//   2. final persistence flush (positions/bookmarks/lastbook already atomic)
//   3. retained sleep image presented CleanFull (the present's bounded
//      busy-wait guarantees the EPD is idle before power drops)
//   4. SD unmounted
//   5. console log flushed
//   6. wake source configured and the power transition taken
// Wake is a reboot on this hardware; ReaderBootRestore() is the resume
// contract and reopens the last book at its persisted locator.
#pragma once

#include "app_events.h"

namespace reader {

enum class SleepMode : uint8_t {
    DeepTimer = 1,  // esp deep sleep, timer wake (USB stays powered)
    RtcOff = 2,     // true power-off, BM8563 RTC alarm or side button wakes
    Off = 3,        // true power-off, side button wakes
};

// Runs the quiesce sequence and enters `mode`. Does not return on success.
// `seconds` is the wake delay for DeepTimer/RtcOff (ignored for Off).
StatusCode PowerSleep(SleepMode mode, uint32_t seconds);

void FillPowerSnapshot(PowerSnapshot *out);

// Inactivity auto-sleep policy: after `seconds` without touch input the
// owner powers off (Off mode). 0 disables (default). Any input restarts
// the window, which is also the user-cancel path.
void PowerSetAutoSleep(uint32_t seconds);
void PowerAutoTick(int64_t now_us);

// Logs this boot's reset reason and wakeup cause (resume evidence).
void PowerLogBootCause();

}  // namespace reader
