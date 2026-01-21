#include "mled_time.h"

#include "esp_timer.h"

uint32_t mled_time_local_ms(void)
{
    const uint64_t ms = (uint64_t)(esp_timer_get_time() / 1000);
    return (uint32_t)(ms & 0xffffffffu);
}

int32_t mled_time_u32_duration(uint32_t start_ms, uint32_t end_ms)
{
    const uint32_t diff = (end_ms - start_ms);
    if (diff > 0x7fffffffu) {
        return (int32_t)(diff - 0x100000000u);
    }
    return (int32_t)diff;
}

int32_t mled_time_u32_diff(uint32_t a, uint32_t b)
{
    const uint32_t diff = (a - b);
    if (diff > 0x7fffffffu) {
        return (int32_t)(diff - 0x100000000u);
    }
    return (int32_t)diff;
}

int mled_time_is_due(uint32_t now_ms, uint32_t execute_at_ms)
{
    const uint32_t diff = (now_ms - execute_at_ms);
    return diff <= 0x7fffffffu;
}

