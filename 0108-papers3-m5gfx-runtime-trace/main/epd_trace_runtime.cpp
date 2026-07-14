#include "epd_trace_runtime.h"

#include <atomic>
#include <cinttypes>
#include <cstdio>

#include "esp_timer.h"

namespace papers3::trace {

#if CONFIG_PAPERS3_M5GFX_RUNTIME_TRACE

namespace {

constexpr uint32_t kCapacity = CONFIG_PAPERS3_M5GFX_TRACE_CAPACITY;

struct TraceRecord {
    uint32_t commit;
    uint32_t sequence;
    int64_t timestamp_us;
    uint32_t operation;
    uint32_t event;
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
};

static_assert(sizeof(TraceRecord) == 48, "trace record layout changed");

TraceRecord g_records[kCapacity] = {};
std::atomic<uint32_t> g_next_sequence{0};
std::atomic<uint32_t> g_next_operation{1};
std::atomic<uint32_t> g_current_operation{0};

void Emit(uint32_t event, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
{
    const uint32_t sequence = g_next_sequence.fetch_add(1, std::memory_order_relaxed);
    TraceRecord& record = g_records[sequence % kCapacity];
    __atomic_store_n(&record.commit, 0U, __ATOMIC_RELAXED);
    record.sequence = sequence;
    record.timestamp_us = esp_timer_get_time();
    record.operation = g_current_operation.load(std::memory_order_relaxed);
    record.event = event;
    record.a = a;
    record.b = b;
    record.c = c;
    record.d = d;
    record.e = e;
    __atomic_store_n(&record.commit, sequence + 1U, __ATOMIC_RELEASE);
}

}  // namespace

extern "C" void lgfx_epd_trace_emit(uint32_t event, uint32_t a, uint32_t b, uint32_t c, uint32_t d,
                                      uint32_t e)
{
    Emit(event, a, b, c, d, e);
}

uint32_t BeginOperation(uint32_t mode, bool full)
{
    const uint32_t operation = g_next_operation.fetch_add(1, std::memory_order_relaxed);
    g_current_operation.store(operation, std::memory_order_relaxed);
    Emit(static_cast<uint32_t>(Event::kAppOperationBegin), mode, full ? 1U : 0U, 0, 0, 0);
    return operation;
}

void EndOperation(uint32_t operation, bool result, uint64_t elapsed_us)
{
    Emit(static_cast<uint32_t>(Event::kAppOperationEnd), result ? 1U : 0U,
         static_cast<uint32_t>(elapsed_us), static_cast<uint32_t>(elapsed_us >> 32U), 0, 0);
    uint32_t expected = operation;
    g_current_operation.compare_exchange_strong(expected, 0, std::memory_order_relaxed);
}

void Reset()
{
    g_next_sequence.store(0, std::memory_order_relaxed);
    g_current_operation.store(0, std::memory_order_relaxed);
    for (TraceRecord& record : g_records) {
        __atomic_store_n(&record.commit, 0U, __ATOMIC_RELAXED);
    }
}

void PrintStatus()
{
    const uint32_t total = g_next_sequence.load(std::memory_order_acquire);
    const uint32_t retained = total < kCapacity ? total : kCapacity;
    const uint32_t overwritten = total > kCapacity ? total - kCapacity : 0;
    std::printf("EPD_TRACE_STATUS enabled=yes capacity=%" PRIu32 " total=%" PRIu32
                " retained=%" PRIu32 " overwritten=%" PRIu32 " current_operation=%" PRIu32 "\n",
                kCapacity, total, retained, overwritten,
                g_current_operation.load(std::memory_order_relaxed));
}

void DumpJsonLines()
{
    const uint32_t total = g_next_sequence.load(std::memory_order_acquire);
    const uint32_t begin = total > kCapacity ? total - kCapacity : 0;
    std::printf("EPD_TRACE_DUMP_BEGIN schema=esp50.m5gfx-runtime-trace.v1 begin=%" PRIu32
                " end=%" PRIu32 " overwritten=%" PRIu32 "\n",
                begin, total, begin);
    for (uint32_t sequence = begin; sequence < total; ++sequence) {
        const TraceRecord& record = g_records[sequence % kCapacity];
        const uint32_t commit = __atomic_load_n(&record.commit, __ATOMIC_ACQUIRE);
        if (commit != sequence + 1U || record.sequence != sequence) {
            std::printf("{\"schema\":1,\"sequence\":%" PRIu32 ",\"valid\":false}\n", sequence);
            continue;
        }
        std::printf("{\"schema\":1,\"sequence\":%" PRIu32 ",\"timestamp_us\":%" PRId64
                    ",\"operation\":%" PRIu32 ",\"event\":%" PRIu32
                    ",\"name\":\"%s\",\"a\":%" PRIu32 ",\"b\":%" PRIu32
                    ",\"c\":%" PRIu32 ",\"d\":%" PRIu32 ",\"e\":%" PRIu32 "}\n",
                    record.sequence, record.timestamp_us, record.operation, record.event,
                    EventName(record.event), record.a, record.b, record.c, record.d, record.e);
    }
    std::printf("EPD_TRACE_DUMP_END total=%" PRIu32 "\n", total);
}

#else

uint32_t BeginOperation(uint32_t, bool) { return 0; }
void EndOperation(uint32_t, bool, uint64_t) {}
void Reset() {}
void PrintStatus() { std::printf("EPD_TRACE_STATUS enabled=no capacity=0 total=0 retained=0 overwritten=0 current_operation=0\n"); }
void DumpJsonLines() { std::printf("EPD_TRACE_DUMP_BEGIN schema=esp50.m5gfx-runtime-trace.v1 begin=0 end=0 overwritten=0\nEPD_TRACE_DUMP_END total=0\n"); }

#endif

const char* EventName(uint32_t event)
{
    switch (static_cast<Event>(event)) {
    case Event::kAppOperationBegin: return "APP_OPERATION_BEGIN";
    case Event::kAppOperationEnd: return "APP_OPERATION_END";
    case Event::kDisplayEnqueue: return "DISPLAY_ENQUEUE";
    case Event::kUpdateDequeue: return "UPDATE_DEQUEUE";
    case Event::kUpdatePrepared: return "UPDATE_PREPARED";
    case Event::kPowerOnBegin: return "POWER_ON_BEGIN";
    case Event::kPowerOnEnd: return "POWER_ON_END";
    case Event::kFrameBegin: return "FRAME_BEGIN";
    case Event::kFrameEnd: return "FRAME_END";
    case Event::kPowerOffBegin: return "POWER_OFF_BEGIN";
    case Event::kPowerOffEnd: return "POWER_OFF_END";
    case Event::kDisplayIdle: return "DISPLAY_IDLE";
    }
    return "UNKNOWN";
}

}  // namespace papers3::trace
