#include "app_console.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "esp_console.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

#include "app_events.h"
#include "app_owner.h"
#include "s3paper/input.h"

namespace reader {
namespace {

const char *kTag = "console";

constexpr uint32_t kReplyTimeoutMs = 500;

QueueHandle_t s_reply_queue = nullptr;
std::atomic<uint32_t> s_next_request_id{1};

// Stress fixture state (owned by the console module, not the app model).
struct StressProducerConfig {
    EventSource source;
    uint32_t count;
    std::atomic<uint32_t> sent;
    std::atomic<uint32_t> rejected_attempts;
    std::atomic<bool> done;
};

StressProducerConfig s_stress_console;
StressProducerConfig s_stress_input;

AppEvent MakeEvent(AppEventKind kind, EventSource source, bool wants_reply) {
    AppEvent event;
    std::memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.source = source;
    event.producer_seq = NextProducerSeq(source);
    event.request_id =
        wants_reply ? s_next_request_id.fetch_add(1) : 0;
    event.monotonic_us = esp_timer_get_time();
    event.reply_queue = wants_reply ? s_reply_queue : nullptr;
    return event;
}

// Posts a console op and waits for its reply. Prints explicit errors for
// queue-full and reply-timeout instead of hiding them.
StatusCode RunConsoleOpWithArgs(ConsoleOp op, uint32_t arg, uint32_t arg2,
                                AppReply *out, uint32_t timeout_ms) {
    AppEvent event = MakeEvent(AppEventKind::ConsoleCommand,
                               EventSource::Console, true);
    event.payload.console.op = op;
    event.payload.console.arg = arg;
    event.payload.console.arg2 = arg2;
    const StatusCode posted = PostEvent(event);
    if (posted != StatusCode::Ok) {
        printf("error: enqueue failed: %s\n", StatusCodeName(posted));
        return posted;
    }
    const StatusCode waited =
        AwaitReply(s_reply_queue, event.request_id, timeout_ms, out);
    if (waited != StatusCode::Ok) {
        printf("error: reply wait failed: %s (timeout %ums)\n",
               StatusCodeName(waited), static_cast<unsigned>(timeout_ms));
        return waited;
    }
    return out->status;
}

StatusCode RunConsoleOpWithArg(ConsoleOp op, uint32_t arg, AppReply *out,
                               uint32_t timeout_ms) {
    return RunConsoleOpWithArgs(op, arg, 0, out, timeout_ms);
}

StatusCode RunConsoleOp(ConsoleOp op, AppReply *out) {
    return RunConsoleOpWithArg(op, 0, out, kReplyTimeoutMs);
}

const char *PhaseName(uint8_t phase) {
    switch (phase) {
        case 0: return "booting";
        case 1: return "ready";
        case 2: return "shutting-down";
    }
    return "unknown";
}

int CmdStatus(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Status, &reply) != StatusCode::Ok) {
        return 1;
    }
    const StatusSnapshot &s = reply.payload.status_snapshot;
    printf("state=%s uptime_ms=%u owner_seq=%u\n", PhaseName(s.app_state),
           static_cast<unsigned>(s.uptime_ms),
           static_cast<unsigned>(s.owner_seq));
    for (uint8_t i = 0; i < static_cast<uint8_t>(AppEventKind::kCount); ++i) {
        printf("processed[%s]=%u\n",
               AppEventKindName(static_cast<AppEventKind>(i)),
               static_cast<unsigned>(s.processed_by_kind[i]));
    }
    return 0;
}

int CmdHeap(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Heap, &reply) != StatusCode::Ok) {
        return 1;
    }
    const HeapSnapshot &h = reply.payload.heap;
    printf("internal_free=%u internal_min_free=%u internal_largest=%u\n",
           static_cast<unsigned>(h.internal_free),
           static_cast<unsigned>(h.internal_min_free),
           static_cast<unsigned>(h.internal_largest_block));
    printf("psram_free=%u psram_largest=%u\n",
           static_cast<unsigned>(h.psram_free),
           static_cast<unsigned>(h.psram_largest_block));
    return 0;
}

int CmdDisplay(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Display, &reply) != StatusCode::Ok) {
        return 1;
    }
    const DisplaySnapshot &d = reply.payload.display;
    printf("app_state=%s\n", PhaseName(d.app_state));
    printf("backend[fake] initialized=%u frames=%u (primary dev backend)\n",
           d.fake_initialized, static_cast<unsigned>(d.fake_frames));
    printf("backend[m5] initialized=%u size=%dx%d frames=%u "
           "(transaction shell; panel optically unqualified)\n",
           d.m5_initialized, static_cast<int>(d.m5_w),
           static_cast<int>(d.m5_h), static_cast<unsigned>(d.m5_frames));
    return 0;
}

int CmdFixture(int argc, char **argv) {
    uint8_t backend = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "m5") == 0) {
            backend = 1;
        } else if (strcmp(argv[1], "text") == 0) {
            backend = 2;  // measured text page on the M5 backend
        } else if (strcmp(argv[1], "fake") != 0) {
            printf("error: InvalidArgument: usage fixture [fake|m5|text]\n");
            return 1;
        }
    }
    AppReply reply;
    // A full EPD refresh takes seconds; use a generous bounded timeout.
    const StatusCode status =
        RunConsoleOpWithArg(ConsoleOp::Fixture, backend, &reply, 15000);
    if (status != StatusCode::Ok) {
        printf("fixture result=%s\n", StatusCodeName(status));
        return 1;
    }
    const PlannedPresentSnapshot &pp = reply.payload.present;
    const s3paper::PresentResult &p = pp.present;
    printf("fixture backend=%s id=%u ops_drawn=%u ops_skipped=%u "
           "damage=%d,%d,%d,%d render_us=%u wait_us=%u status=%s\n",
           backend == 2 ? "text-m5" : (backend == 1 ? "m5" : "fake"),
           static_cast<unsigned>(p.frame_id),
           static_cast<unsigned>(p.ops_drawn),
           static_cast<unsigned>(p.ops_skipped), static_cast<int>(p.damage.x),
           static_cast<int>(p.damage.y), static_cast<int>(p.damage.w),
           static_cast<int>(p.damage.h), static_cast<unsigned>(p.render_us),
           static_cast<unsigned>(p.wait_us), StatusCodeName(p.status));
    printf("plan full=%u waveform=%s reason=%s\n", pp.full_refresh,
           s3paper::EpdWaveformName(
               static_cast<s3paper::EpdWaveform>(pp.waveform)),
           s3paper::RefreshReasonName(
               static_cast<s3paper::RefreshReason>(pp.reason)));
    return 0;
}

int CmdRefresh(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Refresh, &reply) != StatusCode::Ok) {
        return 1;
    }
    const RefreshSnapshot &r = reply.payload.refresh;
    printf("policy max_turns=%u max_area=%llu max_elapsed_ms=%lld "
           "merge_distance=%d align_x=%d\n",
           static_cast<unsigned>(r.max_turns_between_full),
           static_cast<unsigned long long>(r.max_partial_area_between_full),
           static_cast<long long>(r.max_elapsed_us_between_full / 1000),
           static_cast<int>(r.merge_distance), static_cast<int>(r.align_x));
    printf("history first_render_done=%u turns_since_full=%u "
           "area_since_full=%llu fulls=%u partials=%u merge_fallbacks=%u "
           "pending_damage=%u\n",
           r.first_render_done, static_cast<unsigned>(r.turns_since_full),
           static_cast<unsigned long long>(r.partial_area_since_full),
           static_cast<unsigned>(r.fulls_total),
           static_cast<unsigned>(r.partials_total),
           static_cast<unsigned>(r.merge_fallbacks),
           static_cast<unsigned>(r.pending_damage));
    return 0;
}

void PrintSoakSnapshot(const SoakSnapshot &s) {
    printf("soak active=%u progress=%u/%u errors=%u fulls=%u partials=%u\n",
           s.active, static_cast<unsigned>(s.completed),
           static_cast<unsigned>(s.target), static_cast<unsigned>(s.errors),
           static_cast<unsigned>(s.fulls), static_cast<unsigned>(s.partials));
    printf("soak heap start=%u now=%u min=%u integrity checks=%u "
           "failures=%u elapsed_ms=%lld\n",
           static_cast<unsigned>(s.heap_free_start),
           static_cast<unsigned>(s.heap_free_now),
           static_cast<unsigned>(s.heap_free_min),
           static_cast<unsigned>(s.integrity_checks),
           static_cast<unsigned>(s.integrity_failures),
           static_cast<long long>(s.elapsed_us / 1000));
    static const char *kWaveformNames[4] = {"Quality", "Text", "Fast",
                                            "Fastest"};
    for (int i = 0; i < 4; ++i) {
        const SoakWaveformStats &w = s.by_waveform[i];
        if (w.count == 0) {
            continue;
        }
        printf("soak waveform[%s] count=%u avg_us=%llu max_us=%u\n",
               kWaveformNames[i], static_cast<unsigned>(w.count),
               static_cast<unsigned long long>(w.total_us / w.count),
               static_cast<unsigned>(w.max_us));
    }
}

int CmdSoak(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "start") == 0) {
        uint32_t steps = 10000;
        if (argc >= 3) {
            const long parsed = strtol(argv[2], nullptr, 10);
            if (parsed <= 0 || parsed > 1000000) {
                printf("error: InvalidArgument: steps must be 1..1000000\n");
                return 1;
            }
            steps = static_cast<uint32_t>(parsed);
        }
        AppReply reply;
        const StatusCode status =
            RunConsoleOpWithArg(ConsoleOp::SoakStart, steps, &reply, 15000);
        if (status != StatusCode::Ok) {
            printf("soak start failed: %s\n", StatusCodeName(status));
            return 1;
        }
        printf("soak started: %u steps (query with 'soak status')\n",
               static_cast<unsigned>(steps));
        return 0;
    }
    if (argc >= 2 && strcmp(argv[1], "status") != 0) {
        printf("error: InvalidArgument: usage soak [start [n]|status]\n");
        return 1;
    }
    AppReply reply;
    // During a soak the owner may be inside a ~1 s full refresh when this
    // event lands; give the reply the same generous bound as fixture.
    if (RunConsoleOpWithArg(ConsoleOp::SoakStatus, 0, &reply, 15000) !=
        StatusCode::Ok) {
        return 1;
    }
    PrintSoakSnapshot(reply.payload.soak);
    return 0;
}

int CmdEvents(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Events, &reply) != StatusCode::Ok) {
        return 1;
    }
    const EventsSnapshot &e = reply.payload.events;
    printf("queue capacity=%u depth=%u high_water=%u\n",
           static_cast<unsigned>(e.queue_capacity),
           static_cast<unsigned>(e.queue_depth_now),
           static_cast<unsigned>(e.queue_high_water));
    for (uint8_t i = 0; i < static_cast<uint8_t>(EventSource::kCount); ++i) {
        printf("source[%s] accepted=%u rejected_sends=%u\n",
               EventSourceName(static_cast<EventSource>(i)),
               static_cast<unsigned>(e.accepted_by_source[i]),
               static_cast<unsigned>(e.dropped_by_source[i]));
    }
    printf("out_of_order_total=%u replies_sent=%u replies_dropped=%u\n",
           static_cast<unsigned>(e.out_of_order_total),
           static_cast<unsigned>(e.replies_sent),
           static_cast<unsigned>(e.replies_dropped));
    return 0;
}

int CmdPing(int, char **) {
    AppReply reply;
    const StatusCode status = RunConsoleOp(ConsoleOp::Ping, &reply);
    if (status == StatusCode::Busy) {
        printf("busy: owner is shutting down\n");
        return 1;
    }
    if (status != StatusCode::Ok) {
        return 1;
    }
    printf("pong round_trip_us=%lld\n",
           static_cast<long long>(esp_timer_get_time() -
                                  reply.payload.echo_monotonic_us));
    return 0;
}

// Stress producer: delivers exactly `count` ordered events, retrying on a
// full queue. Rejected attempts stay visible in the events diagnostics.
void StressProducerTask(void *arg) {
    auto *cfg = static_cast<StressProducerConfig *>(arg);
    for (uint32_t i = 0; i < cfg->count; ++i) {
        AppEvent event = MakeEvent(
            cfg->source == EventSource::StressInput ? AppEventKind::Pointer
                                                    : AppEventKind::ConsoleCommand,
            cfg->source, false);
        if (cfg->source == EventSource::StressInput) {
            event.payload.pointer = {
                .x = static_cast<int32_t>(i % 540),
                .y = static_cast<int32_t>(i % 960),
                .phase = (i % 2 == 0) ? PointerPhase::Down : PointerPhase::Up,
                .pointer_id = 0,
            };
        } else {
            event.payload.console.op = ConsoleOp::Status;
        }
        for (;;) {
            const StatusCode posted = PostEvent(event);
            if (posted == StatusCode::Ok) {
                break;
            }
            cfg->rejected_attempts.fetch_add(1);
            vTaskDelay(1);
        }
        cfg->sent.fetch_add(1);
    }
    cfg->done.store(true);
    vTaskDelete(nullptr);
}

int CmdStress(int argc, char **argv) {
    uint32_t per_source = 500;
    if (argc >= 2) {
        const long parsed = strtol(argv[1], nullptr, 10);
        if (parsed <= 0 || parsed > 100000) {
            printf("error: InvalidArgument: count must be 1..100000\n");
            return 1;
        }
        per_source = static_cast<uint32_t>(parsed);
    }

    // Reset stress counters in the owner first so the report is exact.
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::StressReset, &reply) != StatusCode::Ok) {
        return 1;
    }

    for (auto *cfg : {&s_stress_console, &s_stress_input}) {
        cfg->count = per_source;
        cfg->sent.store(0);
        cfg->rejected_attempts.store(0);
        cfg->done.store(false);
    }
    s_stress_console.source = EventSource::StressConsole;
    s_stress_input.source = EventSource::StressInput;

    const int64_t start_us = esp_timer_get_time();
    xTaskCreatePinnedToCore(StressProducerTask, "stress_con", 4096,
                            &s_stress_console, 4, nullptr, 0);
    xTaskCreatePinnedToCore(StressProducerTask, "stress_inp", 4096,
                            &s_stress_input, 4, nullptr, 0);

    // While the producers hammer the queue, keep issuing real console pings
    // so three producers are demonstrably concurrent.
    uint32_t pings_ok = 0;
    uint32_t pings_failed = 0;
    while (!s_stress_console.done.load() || !s_stress_input.done.load()) {
        AppReply ping_reply;
        if (RunConsoleOp(ConsoleOp::Ping, &ping_reply) == StatusCode::Ok) {
            pings_ok++;
        } else {
            pings_failed++;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
        if (esp_timer_get_time() - start_us > 60'000'000LL) {
            printf("error: Timeout: stress producers did not finish in 60s\n");
            return 1;
        }
    }
    const int64_t elapsed_us = esp_timer_get_time() - start_us;

    if (RunConsoleOp(ConsoleOp::StressReport, &reply) != StatusCode::Ok) {
        return 1;
    }
    const StressSnapshot &s = reply.payload.stress;
    const bool pass = s.console_received == per_source &&
                      s.input_received == per_source && s.out_of_order == 0 &&
                      s.last_console_seq == per_source &&
                      s.last_input_seq == per_source;
    printf("stress sent=%u+%u received=%u+%u out_of_order=%u\n",
           static_cast<unsigned>(per_source),
           static_cast<unsigned>(per_source),
           static_cast<unsigned>(s.console_received),
           static_cast<unsigned>(s.input_received),
           static_cast<unsigned>(s.out_of_order));
    printf("stress last_seq console=%u input=%u rejected_attempts=%u+%u\n",
           static_cast<unsigned>(s.last_console_seq),
           static_cast<unsigned>(s.last_input_seq),
           static_cast<unsigned>(s_stress_console.rejected_attempts.load()),
           static_cast<unsigned>(s_stress_input.rejected_attempts.load()));
    printf("stress concurrent_pings ok=%u failed=%u elapsed_ms=%lld\n",
           static_cast<unsigned>(pings_ok),
           static_cast<unsigned>(pings_failed),
           static_cast<long long>(elapsed_us / 1000));
    printf("stress result=%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}

// Flood: burst that intentionally overflows the queue to prove queue-full
// behavior is explicit and counted, not silent. The burst runs on the owner's
// core at higher priority so the owner cannot drain mid-burst; overflow is
// therefore deterministic once burst > queue capacity.
struct FloodResult {
    uint32_t burst;
    std::atomic<uint32_t> accepted;
    std::atomic<uint32_t> rejected;
    std::atomic<bool> done;
};

FloodResult s_flood;

void FloodTask(void *) {
    for (uint32_t i = 0; i < s_flood.burst; ++i) {
        AppEvent event =
            MakeEvent(AppEventKind::TimerDue, EventSource::Console, false);
        // Flood events carry no producer ordering claim: the rejected ones
        // would otherwise create legitimate-looking sequence gaps.
        event.producer_seq = 0;
        event.payload.timer.timer_id = i;
        if (PostEvent(event) == StatusCode::Ok) {
            s_flood.accepted.fetch_add(1);
        } else {
            s_flood.rejected.fetch_add(1);
        }
    }
    s_flood.done.store(true);
    vTaskDelete(nullptr);
}

int CmdFlood(int argc, char **argv) {
    uint32_t burst = 256;
    if (argc >= 2) {
        const long parsed = strtol(argv[1], nullptr, 10);
        if (parsed <= 0 || parsed > 100000) {
            printf("error: InvalidArgument: count must be 1..100000\n");
            return 1;
        }
        burst = static_cast<uint32_t>(parsed);
    }
    s_flood.burst = burst;
    s_flood.accepted.store(0);
    s_flood.rejected.store(0);
    s_flood.done.store(false);
    // Owner runs on core 1 at priority 5; the flood outranks it there.
    xTaskCreatePinnedToCore(FloodTask, "flood", 4096, nullptr, 6, nullptr, 1);
    const int64_t start_us = esp_timer_get_time();
    while (!s_flood.done.load()) {
        if (esp_timer_get_time() - start_us > 10'000'000LL) {
            printf("error: Timeout: flood task did not finish in 10s\n");
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    printf("flood burst=%u accepted=%u rejected=%u (%s)\n",
           static_cast<unsigned>(burst),
           static_cast<unsigned>(s_flood.accepted.load()),
           static_cast<unsigned>(s_flood.rejected.load()),
           StatusCodeName(StatusCode::CapacityExceeded));
    return 0;
}

int CmdTouch(int argc, char **argv) {
    uint32_t arg = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "on") == 0) {
            arg = 1;
        } else if (strcmp(argv[1], "off") == 0) {
            arg = 2;
        } else if (strcmp(argv[1], "status") != 0) {
            printf("error: InvalidArgument: usage touch [on|off|status]\n");
            return 1;
        }
    }
    AppReply reply;
    // "touch on" initializes the M5 stack (seconds); use the long bound.
    if (RunConsoleOpWithArg(ConsoleOp::Touch, arg, &reply, 15000) !=
        StatusCode::Ok) {
        return 1;
    }
    const TouchSnapshot &t = reply.payload.touch;
    printf("touch enabled=%u samples=%u last=%d,%d last_input_age_ms=%lld\n",
           t.enabled, static_cast<unsigned>(t.samples),
           static_cast<int>(t.last_x), static_cast<int>(t.last_y),
           static_cast<long long>(t.last_input_age_ms));
    printf("events down=%u move=%u up=%u cancel=%u\n",
           static_cast<unsigned>(t.events_by_kind[0]),
           static_cast<unsigned>(t.events_by_kind[1]),
           static_cast<unsigned>(t.events_by_kind[2]),
           static_cast<unsigned>(t.events_by_kind[3]));
    printf("gestures tap=%u longpress=%u swipeL=%u swipeR=%u swipeU=%u "
           "swipeD=%u\n",
           static_cast<unsigned>(t.gestures_by_kind[0]),
           static_cast<unsigned>(t.gestures_by_kind[1]),
           static_cast<unsigned>(t.gestures_by_kind[2]),
           static_cast<unsigned>(t.gestures_by_kind[3]),
           static_cast<unsigned>(t.gestures_by_kind[4]),
           static_cast<unsigned>(t.gestures_by_kind[5]));
    if (t.last_gesture != 0xff) {
        printf("last_gesture=%s at %d,%d\n",
               s3paper::GestureKindName(
                   static_cast<s3paper::GestureKind>(t.last_gesture)),
               static_cast<int>(t.last_gesture_x),
               static_cast<int>(t.last_gesture_y));
    }
    printf("quiet_windows=%u scheduler_pending=%u\n",
           static_cast<unsigned>(t.quiet_windows),
           static_cast<unsigned>(t.scheduler_pending));
    return 0;
}

int CmdReader(int argc, char **argv) {
    uint32_t arg = 0;
    uint32_t arg2 = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "open") == 0) {
            arg = 1;
            if (argc >= 3) {
                const long parsed = strtol(argv[2], nullptr, 10);
                if (parsed < 0 || parsed >= 1000) {
                    printf("error: InvalidArgument: book index 0..999\n");
                    return 1;
                }
                arg = 4;  // open SD library book
                arg2 = static_cast<uint32_t>(parsed);
            }
        } else if (strcmp(argv[1], "next") == 0) {
            arg = 2;
        } else if (strcmp(argv[1], "prev") == 0) {
            arg = 3;
        } else if (strcmp(argv[1], "status") != 0) {
            printf("error: InvalidArgument: usage reader "
                   "[open [n]|next|prev|status]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status =
        RunConsoleOpWithArgs(ConsoleOp::Reader, arg, arg2, &reply, 15000);
    const ReaderSnapshot &r = reply.payload.reader;
    if (status != StatusCode::Ok && status != StatusCode::InvalidArgument) {
        printf("reader op failed: %s\n", StatusCodeName(status));
        return 1;
    }
    if (status == StatusCode::InvalidArgument) {
        printf("reader: %s\n",
               r.at_end ? "at end of book" : "at beginning of book");
    }
    printf("reader open=%u title=\"%s\" source=%s resumed=%u offset=%llu "
           "progress=%u.%u%% lines=%u turns=%u at_end=%u checkpoints=%u\n",
           r.open, r.title, r.source ? "sd" : "embedded", r.resumed,
           static_cast<unsigned long long>(r.byte_offset),
           static_cast<unsigned>(r.progress_permille / 10),
           static_cast<unsigned>(r.progress_permille % 10),
           static_cast<unsigned>(r.line_count),
           static_cast<unsigned>(r.page_turns), r.at_end,
           static_cast<unsigned>(r.checkpoints));
    return 0;
}

void PrintSdSnapshot(const SdSnapshot &s) {
    printf("sd mounted=%u capacity_mib=%u books=%u positions=%u "
           "position_writes=%u write_failures=%u\n",
           s.mounted, static_cast<unsigned>(s.capacity_mib),
           static_cast<unsigned>(s.book_count),
           static_cast<unsigned>(s.position_records),
           static_cast<unsigned>(s.position_writes),
           static_cast<unsigned>(s.position_write_failures));
}

int CmdSd(int argc, char **argv) {
    uint32_t arg = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "mount") == 0) {
            arg = 1;
        } else if (strcmp(argv[1], "unmount") == 0) {
            arg = 2;
        } else if (strcmp(argv[1], "demo") == 0) {
            arg = 3;
        } else if (strcmp(argv[1], "status") != 0) {
            printf("error: InvalidArgument: usage sd "
                   "[mount|unmount|demo|status]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status =
        RunConsoleOpWithArgs(ConsoleOp::Sd, arg, 0, &reply, 15000);
    if (status != StatusCode::Ok) {
        printf("sd op result: %s\n", StatusCodeName(status));
    }
    PrintSdSnapshot(reply.payload.sd);
    return status == StatusCode::Ok ? 0 : 1;
}

int CmdLibrary(int argc, char **argv) {
    uint32_t arg = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "scan") == 0) {
            arg = 1;
        } else if (strcmp(argv[1], "list") != 0) {
            printf("error: InvalidArgument: usage library [scan|list]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status =
        RunConsoleOpWithArgs(ConsoleOp::Library, arg, 0, &reply, 15000);
    if (status != StatusCode::Ok) {
        printf("library op result: %s\n", StatusCodeName(status));
        return 1;
    }
    return 0;
}

int CmdShutdown(int, char **) {
    AppEvent event = MakeEvent(AppEventKind::ShutdownRequest,
                               EventSource::Console, true);
    const StatusCode posted = PostEvent(event);
    if (posted != StatusCode::Ok) {
        printf("error: enqueue failed: %s\n", StatusCodeName(posted));
        return 1;
    }
    AppReply reply;
    if (AwaitReply(s_reply_queue, event.request_id, kReplyTimeoutMs, &reply) !=
        StatusCode::Ok) {
        printf("error: reply wait failed: Timeout\n");
        return 1;
    }
    printf("shutdown acknowledged; interactive commands now report Busy "
           "(reboot to leave this state)\n");
    return 0;
}

void RegisterCommand(const char *name, const char *help,
                     esp_console_cmd_func_t func) {
    const esp_console_cmd_t cmd = {
        .command = name,
        .help = help,
        .hint = nullptr,
        .func = func,
        .argtable = nullptr,
        .func_w_context = nullptr,
        .context = nullptr,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

}  // namespace

void ConsoleStart() {
    s_reply_queue = xQueueCreate(4, sizeof(AppReply));
    configASSERT(s_reply_queue != nullptr);

    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_config =
        ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "reader>";
    repl_config.max_cmdline_length = 128;
    esp_console_dev_usb_serial_jtag_config_t hw_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config,
                                                         &repl_config, &repl));

    RegisterCommand("status", "App state and per-kind event counters",
                    &CmdStatus);
    RegisterCommand("heap", "Internal and PSRAM heap diagnostics", &CmdHeap);
    RegisterCommand("display", "Display backend states (fake and m5)",
                    &CmdDisplay);
    RegisterCommand("fixture",
                    "fixture [fake|m5] - render the phase 2 primitive "
                    "fixture through a backend",
                    &CmdFixture);
    RegisterCommand("refresh",
                    "Refresh planner policy and history diagnostics",
                    &CmdRefresh);
    RegisterCommand("soak",
                    "soak [start [n]|status] - mixed partial/full refresh "
                    "soak on the M5 backend",
                    &CmdSoak);
    RegisterCommand("touch",
                    "touch [on|off|status] - GT911 polling, pointer events, "
                    "gestures, quiet windows",
                    &CmdTouch);
    RegisterCommand("reader",
                    "reader [open [n]|next|prev|status] - read the embedded "
                    "book or SD library book n (touch turns pages)",
                    &CmdReader);
    RegisterCommand("sd", "sd [mount|unmount|demo|status] - microSD card",
                    &CmdSd);
    RegisterCommand("library",
                    "library [scan|list] - scan/list *.txt books on the SD "
                    "card",
                    &CmdLibrary);
    RegisterCommand("events",
                    "Event queue depth, per-source accept/reject, ordering",
                    &CmdEvents);
    RegisterCommand("ping", "Round-trip an event through the owner task",
                    &CmdPing);
    RegisterCommand("stress",
                    "stress [n] - run console+input producers concurrently",
                    &CmdStress);
    RegisterCommand("flood",
                    "flood [n] - burst-post events to prove explicit "
                    "queue-full behavior",
                    &CmdFlood);
    RegisterCommand("shutdown", "Request owner shutdown state", &CmdShutdown);

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    ESP_LOGI(kTag, "console ready on USB Serial/JTAG");
}

}  // namespace reader
