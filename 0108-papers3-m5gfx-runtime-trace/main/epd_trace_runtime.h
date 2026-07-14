#pragma once

#include <cstdint>

#include "sdkconfig.h"

namespace papers3::trace {

enum class Event : uint32_t {
    kAppOperationBegin = 1,
    kAppOperationEnd = 2,
    kDisplayEnqueue = 10,
    kUpdateDequeue = 11,
    kUpdatePrepared = 12,
    kPowerOnBegin = 20,
    kPowerOnEnd = 21,
    kFrameBegin = 30,
    kFrameEnd = 31,
    kPowerOffBegin = 40,
    kPowerOffEnd = 41,
    kDisplayIdle = 50,
};

uint32_t BeginOperation(uint32_t mode, bool full);
void EndOperation(uint32_t operation, bool result, uint64_t elapsed_us);
void Reset();
void PrintStatus();
void DumpJsonLines();
const char* EventName(uint32_t event);

}  // namespace papers3::trace
