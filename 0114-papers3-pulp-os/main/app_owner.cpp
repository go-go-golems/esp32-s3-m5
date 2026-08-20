#include "app_owner.h"

#include <atomic>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

#include "app_book_seed.h"
#include "app_buzzer.h"
#include "app_home.h"
#include "app_images.h"
#include "app_input.h"
#include "app_js.h"
#include "app_power.h"
#include "app_shot.h"
#include "net_http.h"
#include "net_mdns.h"
#include "net_serve.h"
#include "net_wifi.h"
#include "s3paper_runtime/runtime.h"
#include "s3paper_storage/storage.h"

namespace pulp {
namespace {

const char *kTag = "owner";

constexpr uint8_t kSourceCount = static_cast<uint8_t>(EventSource::kCount);
constexpr uint8_t kKindCount = static_cast<uint8_t>(AppEventKind::kCount);

enum class Phase : uint8_t {
    Booting = 0,
    Ready,
    ShuttingDown,
};

struct AppState {
    Phase phase = Phase::Booting;
    int64_t boot_us = 0;
    uint32_t owner_seq = 0;
    uint32_t processed_by_kind[kKindCount] = {};
    uint32_t last_seq_by_source[kSourceCount] = {};
    uint32_t out_of_order_by_source[kSourceCount] = {};
    uint32_t accepted_by_source[kSourceCount] = {};
    uint32_t queue_high_water = 0;
    uint32_t replies_sent = 0;
    uint32_t replies_dropped = 0;
};

AppState s_state;
QueueHandle_t s_event_queue = nullptr;
TaskHandle_t s_owner_task = nullptr;

// Producer-side instrumentation: enqueue attempts that never reached the
// owner, the only values legitimately written outside the owner task.
std::atomic<uint32_t> s_dropped_by_source[kSourceCount];
std::atomic<uint32_t> s_next_seq_by_source[kSourceCount];

void SendReply(const AppEvent &event, const AppReply &reply) {
    AssertOwner();
    if (event.reply_queue == nullptr) {
        return;
    }
    if (xQueueSend(event.reply_queue, &reply, 0) == pdTRUE) {
        s_state.replies_sent++;
    } else {
        s_state.replies_dropped++;
    }
}

AppReply MakeReply(const AppEvent &event, StatusCode status) {
    AppReply reply;
    std::memset(&reply, 0, sizeof(reply));
    reply.request_id = event.request_id;
    reply.status = status;
    return reply;
}

void FillSdSnapshot(SdSnapshot *out) {
    s3paper_storage::StorageStats stats;
    s3paper_storage::GetStats(&stats);
    std::memset(out, 0, sizeof(*out));
    out->mounted = stats.mounted;
    out->capacity_mib = stats.capacity_mib;
    out->book_count = stats.book_count;
    out->position_records = stats.position_records;
    out->position_writes = stats.position_writes;
    out->position_write_failures = stats.position_write_failures;
    out->scan_cached = stats.scan_cached;
    out->scan_hashed = stats.scan_hashed;
    out->scan_ms = stats.scan_ms;
    out->catalog_writes = stats.catalog_writes;
}

void HandleConsoleCommand(const AppEvent &event) {
    AssertOwner();
    AppReply reply = MakeReply(event, StatusCode::Ok);
    switch (event.payload.console.op) {
        case ConsoleOp::Status: {
            StatusSnapshot &s = reply.payload.status_snapshot;
            s.uptime_ms = static_cast<uint32_t>(
                (esp_timer_get_time() - s_state.boot_us) / 1000);
            s.owner_seq = s_state.owner_seq;
            for (uint8_t i = 0; i < kKindCount; ++i) {
                s.processed_by_kind[i] = s_state.processed_by_kind[i];
            }
            s.app_state = static_cast<uint8_t>(s_state.phase);
            break;
        }
        case ConsoleOp::Heap: {
            HeapSnapshot &h = reply.payload.heap;
            h.internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            h.internal_min_free =
                heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
            h.internal_largest_block =
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
            h.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            h.psram_largest_block =
                heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
            break;
        }
        case ConsoleOp::Events: {
            EventsSnapshot &e = reply.payload.events;
            e.queue_capacity = kEventQueueCapacity;
            e.queue_depth_now = static_cast<uint32_t>(
                uxQueueMessagesWaiting(s_event_queue));
            e.queue_high_water = s_state.queue_high_water;
            uint32_t out_of_order_total = 0;
            for (uint8_t i = 0; i < kSourceCount; ++i) {
                e.accepted_by_source[i] = s_state.accepted_by_source[i];
                e.dropped_by_source[i] =
                    s_dropped_by_source[i].load(std::memory_order_relaxed);
                out_of_order_total += s_state.out_of_order_by_source[i];
            }
            e.out_of_order_total = out_of_order_total;
            e.replies_sent = s_state.replies_sent;
            e.replies_dropped = s_state.replies_dropped;
            break;
        }
        case ConsoleOp::Display: {
            DisplaySnapshot &d = reply.payload.display;
            const s3paper::BackendState fake =
                s3paper_runtime::FakeBackendState();
            const s3paper::BackendState m5 =
                s3paper_runtime::M5BackendState();
            d.app_state = static_cast<uint8_t>(s_state.phase);
            d.fake_initialized = fake.initialized ? 1 : 0;
            d.m5_initialized = m5.initialized ? 1 : 0;
            d.m5_w = m5.physical_size.w;
            d.m5_h = m5.physical_size.h;
            d.fake_frames = fake.frames_presented;
            d.m5_frames = m5.frames_presented;
            d.present_count = s3paper_runtime::PresentCount();
            break;
        }
        case ConsoleOp::Ping:
            if (s_state.phase != Phase::Ready) {
                reply.status = StatusCode::Busy;
            }
            reply.payload.echo_monotonic_us = event.monotonic_us;
            break;
        case ConsoleOp::Touch: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = TouchEnable();
                    break;
                case 2:
                    TouchDisable();
                    break;
                default:
                    break;
            }
            FillTouchSnapshot(&reply.payload.touch);
            break;
        }
        case ConsoleOp::Sd: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = s3paper_storage::StorageMount();
                    break;
                case 2:
                    reply.status = s3paper_storage::StorageUnmount();
                    break;
                case 3:
                    reply.status = s3paper_storage::StorageWriteSeedBook();
                    break;
                case 4:
                    s3paper_storage::DebugReloadState();
                    break;
                default:
                    if (event.payload.console.arg >= 10) {
                        const uint32_t packed =
                            event.payload.console.arg - 10;
                        reply.status = s3paper_storage::
                            DebugCorruptStateFile(packed / 3, packed % 3);
                    }
                    break;
            }
            FillSdSnapshot(&reply.payload.sd);
            break;
        }
        case ConsoleOp::Sleep: {
            const uint32_t arg = event.payload.console.arg;
            const uint32_t seconds = event.payload.console.arg2;
            if (arg == 0) {
                FillPowerSnapshot(&reply.payload.power);
            } else if (arg == 4) {
                PowerSetAutoSleep(seconds);
                FillPowerSnapshot(&reply.payload.power);
            } else if (arg <= 3) {
                if ((arg == 1 || arg == 2) &&
                    (seconds == 0 || seconds > 86400)) {
                    reply.status = StatusCode::InvalidArgument;
                    break;
                }
                // The reply must leave before power drops: send it now,
                // then run the quiesce sequence (does not return).
                SendReply(event, reply);
                reply.status =
                    PowerSleep(static_cast<SleepMode>(arg), seconds);
                ESP_LOGE(kTag, "sleep failed: %s",
                         StatusCodeName(reply.status));
                return;  // already replied
            } else {
                reply.status = StatusCode::InvalidArgument;
            }
            break;
        }
        case ConsoleOp::Home:
            reply.status = HomeShowNative();
            break;
        case ConsoleOp::Js: {
            const uint32_t arg = event.payload.console.arg;
            if (arg == 0) {
                (void)JsInit();
            } else if (arg >= 20 && arg <= 49) {
                reply.status = JsRunProbe(arg - 20);
            } else if (arg == 10) {
                reply.status = JsRunPulp();
            } else if (arg == 11) {
                const uint32_t xy = event.payload.console.arg2;
                reply.status = JsSyntheticGesture(
                    0, static_cast<int32_t>(xy >> 16),
                    static_cast<int32_t>(xy & 0xFFFF));
            } else if (arg == 12) {
                reply.status = JsSyntheticGesture(
                    event.payload.console.arg2, 270, 480);
            } else if (arg == 13) {
                JsPrintHits();
            } else if (arg == 14) {
                reply.status = JsRunMeasure();  // ESP-55 Phase 0
            } else {
                reply.status = StatusCode::InvalidArgument;
            }
            FillJsSnapshot(&reply.payload.js);
            break;
        }
        case ConsoleOp::Buzz: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = BuzzerBeep();
                    break;
                case 2:
                    BuzzerStop();
                    break;
                case 3:
                    reply.status = BuzzerTone(
                        static_cast<int32_t>(
                            event.payload.console.arg2 >> 16),
                        static_cast<int32_t>(
                            event.payload.console.arg2 & 0xFFFF));
                    break;
                case 4:
                    // Rising triad with a rest: exercises tone, rest, and
                    // sequencing in one audible gate.
                    reply.status = BuzzerMelody(
                        "880:120,0:40,1109:120,0:40,1319:200");
                    break;
                default:
                    break;
            }
            FillBuzzSnapshot(&reply.payload.buzz);
            break;
        }
        case ConsoleOp::Net: {
            const ConsolePayload &c = event.payload.console;
            switch (c.arg) {
                case 1:
                    reply.status = WifiScan();
                    break;
                case 2:
                    reply.status = WifiJoin(c.str_a, c.str_b);
                    break;
                case 3:
                    reply.status = WifiJoinSaved();
                    break;
                case 4:
                    reply.status =
                        s3paper_storage::WifiCredsSet(c.str_a, c.str_b);
                    break;
                case 5:
                    reply.status =
                        s3paper_storage::WifiCredsForget(c.str_a);
                    break;
                case 6:
                    reply.status = WifiOff();
                    break;
                case 7: {
                    const uint32_t n = s3paper_storage::WifiCredsCount();
                    for (uint32_t i = 0; i < n; ++i) {
                        const s3paper_storage::WifiCredential *cred =
                            s3paper_storage::WifiCredsGetRanked(i);
                        printf("net saved[%u]: %s last_ok=%u\n",
                               static_cast<unsigned>(i), cred->ssid,
                               static_cast<unsigned>(cred->last_ok));
                    }
                    break;
                }
                case 8:
                    for (uint32_t i = 0; i < WifiScanCount(); ++i) {
                        printf("net ap[%u]: \"%s\" rssi=%d secure=%d\n",
                               static_cast<unsigned>(i), WifiScanSsid(i),
                               static_cast<int>(WifiScanRssi(i)),
                               static_cast<int>(WifiScanSecure(i)));
                    }
                    break;
                default:
                    break;
            }
            FillNetSnapshot(&reply.payload.net);
            break;
        }
        case ConsoleOp::Http: {
            const ConsolePayload &c = event.payload.console;
            switch (c.arg) {
                case 1:
                    reply.status = HttpBegin(c.str_a);
                    if (reply.status == StatusCode::Ok && c.arg2 != 0) {
                        reply.status = HttpLimit(c.arg2);
                    }
                    if (reply.status == StatusCode::Ok) {
                        reply.status = HttpSend();
                    }
                    break;
                case 2:
                    HttpAbort();
                    break;
                case 3: {
                    // Body head for console eyes (bounded).
                    const uint32_t n = HttpLineCount();
                    for (uint32_t i = 0; i < n && i < 8; ++i) {
                        uint32_t len = 0;
                        const char *line = HttpLine(i, &len);
                        printf("http body[%u]: %.*s\n",
                               static_cast<unsigned>(i),
                               static_cast<int>(len > 120 ? 120 : len),
                               line);
                    }
                    break;
                }
                default:
                    break;
            }
            FillHttpSnapshot(&reply.payload.http);
            break;
        }
        case ConsoleOp::Serve: {
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = ServeStart(static_cast<uint16_t>(
                        event.payload.console.arg2));
                    break;
                case 2:
                    reply.status = ServeStop();
                    break;
                case 3:
                    reply.status = ServeFilesMount("/", "/sdcard/www");
                    break;
                default:
                    break;
            }
            FillServeSnapshot(&reply.payload.serve);
            break;
        }
        case ConsoleOp::Shot: {
            uint32_t len = 0;
            reply.status = ShotToConsole(&len) ? StatusCode::Ok
                                               : StatusCode::CorruptData;
            break;
        }
        case ConsoleOp::Battery: {
            // arg 0 = status snapshot (level/mv/charging). Battery has no
            // verbs beyond read; the JS singleton is the rich surface.
            FillPowerSnapshot(&reply.payload.power);
            break;
        }
        case ConsoleOp::Mdns: {
            // ESP-54 P2: arg 0 = status, 1 = announce (arg2 = port), 2 = stop.
            switch (event.payload.console.arg) {
                case 1:
                    reply.status = MdnsAnnounce(static_cast<uint16_t>(
                        event.payload.console.arg2));
                    break;
                case 2:
                    reply.status = MdnsStop();
                    break;
                default:
                    break;
            }
            FillMdnsSnapshot(&reply.payload.mdns);
            break;
        }
        case ConsoleOp::Images: {
            // ESP-54 P3: arg 0 = status, 1 = list, 2 = display (str_a),
            // 3 = remove (str_a), 4 = received-cb status.
            const ConsolePayload &c = event.payload.console;
            switch (c.arg) {
                case 1:
                    ImagesPrintCatalog();
                    break;
                case 2:
                    reply.status = ImagesDisplay(c.str_a);
                    break;
                case 3:
                    reply.status = ImagesRemove(c.str_a);
                    break;
                default:
                    break;
            }
            FillImagesSnapshot(&reply.payload.images);
            break;
        }
    }
    SendReply(event, reply);
}

void HandleEvent(const AppEvent &event) {
    AssertOwner();
    const uint8_t source = static_cast<uint8_t>(event.source);
    const uint8_t kind = static_cast<uint8_t>(event.kind);
    if (source >= kSourceCount || kind >= kKindCount) {
        ESP_LOGW(kTag, "discarding malformed event kind=%u source=%u", kind,
                 source);
        return;
    }
    s_state.owner_seq++;
    s_state.processed_by_kind[kind]++;
    s_state.accepted_by_source[source]++;
    if (event.producer_seq != 0) {
        if (event.producer_seq <= s_state.last_seq_by_source[source]) {
            s_state.out_of_order_by_source[source]++;
            ESP_LOGW(kTag, "out-of-order event source=%s seq=%u last=%u",
                     EventSourceName(event.source),
                     static_cast<unsigned>(event.producer_seq),
                     static_cast<unsigned>(
                         s_state.last_seq_by_source[source]));
        }
        s_state.last_seq_by_source[source] = event.producer_seq;
    }

    switch (event.kind) {
        case AppEventKind::ConsoleCommand:
            HandleConsoleCommand(event);
            break;
        case AppEventKind::Pointer:
            break;
        case AppEventKind::TimerDue:
            if (event.payload.timer.timer_id == kTouchTimerId) {
                InputHandleTick();
            }
            break;
        case AppEventKind::ModuleDone: {
            int32_t value = event.payload.module_done.value;
            int32_t err = event.payload.module_done.err;
            if (event.payload.module_done.module == ModuleId::Serve) {
                // Serve requests carry route callbacks of their own; the
                // single-slot module-cb path does not apply.
                ServeOwnerDispatch(value, err);
                break;
            }
            if (event.payload.module_done.module == ModuleId::Wifi &&
                !WifiOwnerOnModuleDone(event.payload.module_done.kind,
                                       &value, &err)) {
                break;  // consumed by the joinSaved sequencer
            }
            JsModuleDone(event.payload.module_done.module,
                         event.payload.module_done.kind, value, err);
            break;
        }
        case AppEventKind::ShutdownRequest:
            s_state.phase = Phase::ShuttingDown;
            s3paper_storage::StorageFlushNow();
            ESP_LOGI(kTag, "shutdown requested");
            SendReply(event, MakeReply(event, StatusCode::Ok));
            break;
        default:
            break;
    }
}

void TickHooks() {
    const int64_t now = esp_timer_get_time();
    s3paper_storage::StorageFlushIfDue(now);
    JsTimerTick(now);
    BuzzerTick(now);
    WifiTick(now);
    PowerAutoTick(now);
}

void OwnerTask(void *) {
    s_state.boot_us = esp_timer_get_time();
    PowerLogBootCause();
    // All display/frame/storage state initializes inside the owner task so
    // no other task ever touches it. Boot flow: runtime -> storage mount
    // -> native home page -> touch on.
    // ESP-54: a full 540x960 .g4 image is ~253 KiB and the bitmap op
    // copies it into the frame arena (GlyphRun pattern). Raise the arena
    // capacity so a full image + the usual text payloads fit. PSRAM is
    // 8 MB; 320 KiB leaves ample headroom. Must be the FIRST RuntimeInit
    // (it is idempotent; a later default call would be a no-op).
    {
        s3paper_runtime::RuntimeConfig cfg{};
        cfg.arena_capacity = 320 * 1024;
        s3paper_runtime::RuntimeInit(cfg);
    }
    {
        // SPI bus is shared with the display: the pre-mount hook guarantees
        // M5GFX owns the bus before the SD mount (ESP-50 diary S9).
        s3paper_storage::StorageConfig config{};
        config.pre_mount = &s3paper_runtime::EnsureM5Init;
        config.seed_path = kSeedBookPath;
        config.seed_text = kSeedBookText;
        config.seed_len = sizeof(kSeedBookText) - 1;
        s3paper_storage::StorageConfigure(config);
        const StatusCode mounted = s3paper_storage::StorageMount();
        if (mounted != StatusCode::Ok) {
            ESP_LOGW(kTag, "SD mount: %s (running cardless)",
                     StatusCodeName(mounted));
        } else {
            // Cyrillic sample onto the shelf (never overwrites).
            (void)s3paper_storage::StorageWriteSeedBook();
        }
    }
    InputServiceInit();
    // Product boot: straight into the JS launcher (clean full render);
    // the native home page is the fallback when the script platform is
    // unavailable (owner keeps running either way).
    const StatusCode pulp = JsRunPulp();
    if (pulp != StatusCode::Ok) {
        ESP_LOGW(kTag, "pulp boot failed: %s (native fallback)",
                 StatusCodeName(pulp));
        const StatusCode home = HomeShowNative();
        if (home != StatusCode::Ok) {
            ESP_LOGE(kTag, "home present failed: %s",
                     StatusCodeName(home));
        }
    }
    (void)TouchEnable();
    s_state.phase = Phase::Ready;
    ESP_LOGI(kTag, "owner task ready; queue capacity=%u",
             static_cast<unsigned>(kEventQueueCapacity));
    for (;;) {
        AppEvent event;
        // Bounded wait so deferred work runs even when no events arrive.
        if (xQueueReceive(s_event_queue, &event, pdMS_TO_TICKS(500)) !=
            pdTRUE) {
            TickHooks();
            continue;
        }
        const uint32_t depth = static_cast<uint32_t>(
            uxQueueMessagesWaiting(s_event_queue)) + 1;
        if (depth > s_state.queue_high_water) {
            s_state.queue_high_water = depth;
        }
        HandleEvent(event);
        TickHooks();
    }
}

}  // namespace

const char *AppEventKindName(AppEventKind kind) {
    switch (kind) {
        case AppEventKind::ConsoleCommand: return "ConsoleCommand";
        case AppEventKind::Pointer: return "Pointer";
        case AppEventKind::TimerDue: return "TimerDue";
        case AppEventKind::ShutdownRequest: return "ShutdownRequest";
        case AppEventKind::ModuleDone: return "ModuleDone";
        case AppEventKind::kCount: break;
    }
    return "Unknown";
}

const char *EventSourceName(EventSource source) {
    switch (source) {
        case EventSource::Console: return "console";
        case EventSource::Internal: return "internal";
        case EventSource::kCount: break;
    }
    return "unknown";
}

void AssertOwner() {
    if (xTaskGetCurrentTaskHandle() != s_owner_task) {
        ESP_LOGE(kTag, "app state touched outside owner task");
        abort();
    }
}

void OwnerStart() {
    if (s_owner_task != nullptr) {
        return;
    }
    s_event_queue = xQueueCreate(kEventQueueCapacity, sizeof(AppEvent));
    configASSERT(s_event_queue != nullptr);
    const BaseType_t ok = xTaskCreatePinnedToCore(
        OwnerTask, "ui_owner", 8192, nullptr, 5, &s_owner_task, 1);
    configASSERT(ok == pdPASS);
}

StatusCode PostEvent(const AppEvent &event) {
    if (s_event_queue == nullptr ||
        static_cast<uint8_t>(event.source) >= kSourceCount ||
        static_cast<uint8_t>(event.kind) >= kKindCount) {
        return StatusCode::InvalidArgument;
    }
    if (xQueueSend(s_event_queue, &event, 0) == pdTRUE) {
        return StatusCode::Ok;
    }
    s_dropped_by_source[static_cast<uint8_t>(event.source)].fetch_add(
        1, std::memory_order_relaxed);
    return StatusCode::CapacityExceeded;
}

StatusCode PostModuleDone(ModuleId module, int32_t kind, int32_t value,
                          int32_t err) {
    AppEvent event;
    std::memset(&event, 0, sizeof(event));
    event.kind = AppEventKind::ModuleDone;
    event.source = EventSource::Internal;
    event.producer_seq = NextProducerSeq(EventSource::Internal);
    event.monotonic_us = esp_timer_get_time();
    event.payload.module_done.module = module;
    event.payload.module_done.kind = kind;
    event.payload.module_done.value = value;
    event.payload.module_done.err = err;
    return PostEvent(event);
}

uint32_t NextProducerSeq(EventSource source) {
    return s_next_seq_by_source[static_cast<uint8_t>(source)].fetch_add(
               1, std::memory_order_relaxed) +
           1;
}

StatusCode AwaitReply(QueueHandle_t reply_queue, uint32_t request_id,
                      uint32_t timeout_ms, AppReply *out) {
    const TickType_t deadline =
        xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    for (;;) {
        const TickType_t now = xTaskGetTickCount();
        const TickType_t remaining = (deadline > now) ? (deadline - now) : 0;
        AppReply reply;
        if (xQueueReceive(reply_queue, &reply, remaining) != pdTRUE) {
            return StatusCode::Timeout;
        }
        if (reply.request_id == request_id) {
            *out = reply;
            return StatusCode::Ok;
        }
        // Stale reply from an earlier timed-out request; keep waiting.
    }
}

}  // namespace pulp
