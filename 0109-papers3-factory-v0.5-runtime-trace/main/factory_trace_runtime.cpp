#include "factory_trace_runtime.h"

#include <atomic>
#include <cinttypes>
#include <cstdio>

#include <M5Unified.hpp>
#include "esp_timer.h"
#include "sdkconfig.h"

#if defined(CONFIG_PAPERS3_FACTORY_RUNTIME_TRACE) && CONFIG_PAPERS3_FACTORY_RUNTIME_TRACE
namespace {
constexpr uint32_t kCapacity = CONFIG_PAPERS3_FACTORY_TRACE_CAPACITY;

struct TraceRecord {
    uint32_t commit;
    uint32_t sequence;
    int64_t timestamp_us;
    uint32_t event;
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t reserved;
};
static_assert(sizeof(TraceRecord) == 48, "trace record layout changed");

TraceRecord g_records[kCapacity] = {};
std::atomic<uint32_t> g_next_sequence{0};

const char* EventName(uint32_t event)
{
    switch (event) {
    case 10: return "DISPLAY_ENQUEUE";
    case 11: return "UPDATE_DEQUEUE";
    case 12: return "UPDATE_PREPARED";
    case 20: return "POWER_ON_BEGIN";
    case 21: return "POWER_ON_END";
    case 30: return "FRAME_BEGIN";
    case 31: return "FRAME_END";
    case 40: return "POWER_OFF_BEGIN";
    case 41: return "POWER_OFF_END";
    case 50: return "DISPLAY_IDLE";
    default: return "UNKNOWN";
    }
}
}  // namespace

extern "C" void lgfx_epd_trace_emit(uint32_t event, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                                      uint32_t e)
{
    const uint32_t sequence = g_next_sequence.fetch_add(1, std::memory_order_relaxed);
    TraceRecord& record = g_records[sequence % kCapacity];
    __atomic_store_n(&record.commit, 0U, __ATOMIC_RELAXED);
    record.sequence = sequence;
    record.timestamp_us = esp_timer_get_time();
    record.event = event;
    record.a = a;
    record.b = b;
    record.c = c;
    record.d = d;
    record.e = e;
    record.reserved = 0;
    __atomic_store_n(&record.commit, sequence + 1U, __ATOMIC_RELEASE);
}

void FactoryTraceDumpAfterDisplayIdle()
{
    M5.Display.waitDisplay();
    const uint32_t total = g_next_sequence.load(std::memory_order_acquire);
    const uint32_t begin = total > kCapacity ? total - kCapacity : 0;
    std::printf("FACTORY_TRACE_DUMP_BEGIN schema=esp50.factory-v05-runtime-trace.v1 begin=%" PRIu32
                " end=%" PRIu32 " overwritten=%" PRIu32 "\n", begin, total, begin);
    for (uint32_t sequence = begin; sequence < total; ++sequence) {
        const TraceRecord& record = g_records[sequence % kCapacity];
        const uint32_t commit = __atomic_load_n(&record.commit, __ATOMIC_ACQUIRE);
        if (commit != sequence + 1U || record.sequence != sequence) {
            std::printf("{\"schema\":1,\"sequence\":%" PRIu32 ",\"valid\":false}\n", sequence);
            continue;
        }
        std::printf("{\"schema\":1,\"sequence\":%" PRIu32 ",\"timestamp_us\":%" PRId64
                    ",\"event\":%" PRIu32 ",\"name\":\"%s\",\"a\":%" PRIu32
                    ",\"b\":%" PRIu32 ",\"c\":%" PRIu32 ",\"d\":%" PRIu32
                    ",\"e\":%" PRIu32 "}\n",
                    record.sequence, record.timestamp_us, record.event, EventName(record.event),
                    record.a, record.b, record.c, record.d, record.e);
    }
    std::printf("FACTORY_TRACE_DUMP_END total=%" PRIu32 "\n", total);
    std::fflush(stdout);
}
#else
void FactoryTraceDumpAfterDisplayIdle() {}
#endif
