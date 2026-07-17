#include "app_console.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_console.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

#include "app_events.h"
#include "app_owner.h"
#include "s3paper/input.h"

namespace pulp {
namespace {

constexpr uint32_t kReplyTimeoutMs = 500;

QueueHandle_t s_reply_queue = nullptr;
std::atomic<uint32_t> s_next_request_id{1};

AppEvent MakeEvent(AppEventKind kind, EventSource source, bool wants_reply) {
    AppEvent event;
    std::memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.source = source;
    event.producer_seq = NextProducerSeq(source);
    event.request_id = wants_reply ? s_next_request_id.fetch_add(1) : 0;
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

StatusCode RunConsoleOp(ConsoleOp op, uint32_t arg, AppReply *out,
                        uint32_t timeout_ms = kReplyTimeoutMs) {
    return RunConsoleOpWithArgs(op, arg, 0, out, timeout_ms);
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
    if (RunConsoleOp(ConsoleOp::Status, 0, &reply) != StatusCode::Ok) {
        return 1;
    }
    const StatusSnapshot &s = reply.payload.status_snapshot;
    printf("state=%s uptime_ms=%u owner_seq=%u\n", PhaseName(s.app_state),
           static_cast<unsigned>(s.uptime_ms),
           static_cast<unsigned>(s.owner_seq));
    for (uint8_t i = 0; i < static_cast<uint8_t>(AppEventKind::kCount);
         ++i) {
        printf("processed[%s]=%u\n",
               AppEventKindName(static_cast<AppEventKind>(i)),
               static_cast<unsigned>(s.processed_by_kind[i]));
    }
    return 0;
}

int CmdHeap(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Heap, 0, &reply) != StatusCode::Ok) {
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

int CmdEvents(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Events, 0, &reply) != StatusCode::Ok) {
        return 1;
    }
    const EventsSnapshot &e = reply.payload.events;
    printf("queue capacity=%u depth=%u high_water=%u\n",
           static_cast<unsigned>(e.queue_capacity),
           static_cast<unsigned>(e.queue_depth_now),
           static_cast<unsigned>(e.queue_high_water));
    for (uint8_t i = 0; i < static_cast<uint8_t>(EventSource::kCount);
         ++i) {
        printf("source[%s] accepted=%u dropped=%u\n",
               EventSourceName(static_cast<EventSource>(i)),
               static_cast<unsigned>(e.accepted_by_source[i]),
               static_cast<unsigned>(e.dropped_by_source[i]));
    }
    printf("out_of_order=%u replies sent=%u dropped=%u\n",
           static_cast<unsigned>(e.out_of_order_total),
           static_cast<unsigned>(e.replies_sent),
           static_cast<unsigned>(e.replies_dropped));
    return 0;
}

int CmdDisplay(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Display, 0, &reply) != StatusCode::Ok) {
        return 1;
    }
    const DisplaySnapshot &d = reply.payload.display;
    printf("state=%s fake init=%u frames=%u\n", PhaseName(d.app_state),
           d.fake_initialized, static_cast<unsigned>(d.fake_frames));
    printf("m5 init=%u size=%dx%d frames=%u presents=%u\n",
           d.m5_initialized, static_cast<int>(d.m5_w),
           static_cast<int>(d.m5_h), static_cast<unsigned>(d.m5_frames),
           static_cast<unsigned>(d.present_count));
    return 0;
}

int CmdPing(int, char **) {
    AppReply reply;
    if (RunConsoleOp(ConsoleOp::Ping, 0, &reply) != StatusCode::Ok) {
        return 1;
    }
    printf("pong (enqueue-to-reply %lld us)\n",
           static_cast<long long>(esp_timer_get_time() -
                                  reply.payload.echo_monotonic_us));
    return 0;
}

void PrintTouchSnapshot(const TouchSnapshot &t) {
    printf("touch enabled=%u samples=%u quiet_windows=%u\n", t.enabled,
           static_cast<unsigned>(t.samples),
           static_cast<unsigned>(t.quiet_windows));
    printf("pointer down=%u move=%u up=%u cancel=%u last=%d,%d\n",
           static_cast<unsigned>(t.events_by_kind[0]),
           static_cast<unsigned>(t.events_by_kind[1]),
           static_cast<unsigned>(t.events_by_kind[2]),
           static_cast<unsigned>(t.events_by_kind[3]),
           static_cast<int>(t.last_x), static_cast<int>(t.last_y));
    printf("gestures tap=%u long=%u swL=%u swR=%u swU=%u swD=%u\n",
           static_cast<unsigned>(t.gestures_by_kind[0]),
           static_cast<unsigned>(t.gestures_by_kind[1]),
           static_cast<unsigned>(t.gestures_by_kind[2]),
           static_cast<unsigned>(t.gestures_by_kind[3]),
           static_cast<unsigned>(t.gestures_by_kind[4]),
           static_cast<unsigned>(t.gestures_by_kind[5]));
}

int CmdTouch(int argc, char **argv) {
    uint32_t arg = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "on") == 0) {
            arg = 1;
        } else if (strcmp(argv[1], "off") == 0) {
            arg = 2;
        } else if (strcmp(argv[1], "status") != 0) {
            printf("error: usage touch [on|off|status]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status = RunConsoleOp(ConsoleOp::Touch, arg, &reply);
    if (status != StatusCode::Ok) {
        printf("touch op result: %s\n", StatusCodeName(status));
    }
    PrintTouchSnapshot(reply.payload.touch);
    return status == StatusCode::Ok ? 0 : 1;
}

void PrintSdSnapshot(const SdSnapshot &s) {
    printf("sd mounted=%u capacity_mib=%u books=%u positions=%u "
           "position_writes=%u write_failures=%u\n",
           s.mounted, static_cast<unsigned>(s.capacity_mib),
           static_cast<unsigned>(s.book_count),
           static_cast<unsigned>(s.position_records),
           static_cast<unsigned>(s.position_writes),
           static_cast<unsigned>(s.position_write_failures));
    printf("scan cached=%u hashed=%u last_ms=%u catalog_writes=%u\n",
           static_cast<unsigned>(s.scan_cached),
           static_cast<unsigned>(s.scan_hashed),
           static_cast<unsigned>(s.scan_ms),
           static_cast<unsigned>(s.catalog_writes));
}

int CmdSd(int argc, char **argv) {
    uint32_t arg = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "mount") == 0) {
            arg = 1;
        } else if (strcmp(argv[1], "unmount") == 0) {
            arg = 2;
        } else if (strcmp(argv[1], "seed") == 0) {
            arg = 3;
        } else if (strcmp(argv[1], "reload") == 0) {
            arg = 4;
        } else if (strcmp(argv[1], "fault") == 0 && argc >= 4) {
            static const char *kKinds[] = {"positions", "bookmarks",
                                           "catalog", "settings",
                                           "lastbook"};
            static const char *kModes[] = {"flip", "trunc", "del"};
            int kind = -1;
            int mode = -1;
            for (int i = 0; i < 5; ++i) {
                if (strcmp(argv[2], kKinds[i]) == 0) {
                    kind = i;
                }
            }
            for (int i = 0; i < 3; ++i) {
                if (strcmp(argv[3], kModes[i]) == 0) {
                    mode = i;
                }
            }
            if (kind < 0 || mode < 0) {
                printf("error: sd fault <kind> <flip|trunc|del>\n");
                return 1;
            }
            arg = 10 + static_cast<uint32_t>(kind) * 3 +
                  static_cast<uint32_t>(mode);
        } else if (strcmp(argv[1], "status") != 0) {
            printf("error: usage sd [mount|unmount|seed|status|reload|"
                   "fault <kind> <mode>]\n");
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

int CmdSleep(int argc, char **argv) {
    uint32_t arg = 0;
    uint32_t arg2 = 0;
    if (argc >= 2) {
        if (strcmp(argv[1], "deep") == 0 && argc >= 3) {
            arg = 1;
            arg2 = static_cast<uint32_t>(atoi(argv[2]));
        } else if (strcmp(argv[1], "rtc-off") == 0 && argc >= 3) {
            arg = 2;
            arg2 = static_cast<uint32_t>(atoi(argv[2]));
        } else if (strcmp(argv[1], "off") == 0) {
            arg = 3;
        } else if (strcmp(argv[1], "auto") == 0 && argc >= 3) {
            arg = 4;
            arg2 = static_cast<uint32_t>(atoi(argv[2]));
        } else if (strcmp(argv[1], "status") != 0) {
            printf("error: usage sleep [status|deep N|rtc-off N|off|"
                   "auto N]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status =
        RunConsoleOpWithArgs(ConsoleOp::Sleep, arg, arg2, &reply, 30000);
    if (status != StatusCode::Ok) {
        printf("sleep op result: %s\n", StatusCodeName(status));
        return 1;
    }
    const PowerSnapshot &p = reply.payload.power;
    printf("battery=%d%% (%d mV) charging=%u wakeup_cause=%u "
           "reset_reason=%u auto_sleep=%us\n",
           static_cast<int>(p.battery_level),
           static_cast<int>(p.battery_mv), p.charging, p.wakeup_cause,
           p.reset_reason, static_cast<unsigned>(p.auto_sleep_sec));
    return 0;
}

void PrintJsSnapshot(const JsSnapshot &j) {
    printf("js init=%u screen_active=%u arena=%u evals=%u exceptions=%u "
           "dispatches=%u last_error=\"%s\"\n",
           j.initialized, j.screen_active,
           static_cast<unsigned>(j.arena_bytes),
           static_cast<unsigned>(j.evals),
           static_cast<unsigned>(j.exceptions),
           static_cast<unsigned>(j.dispatches), j.last_error);
}

int CmdJs(int argc, char **argv) {
    uint32_t arg = 0;
    uint32_t arg2 = 0;
    if (argc >= 2 && strcmp(argv[1], "status") != 0) {
        if (strcmp(argv[1], "probe") == 0 && argc >= 3) {
            arg = 20 + static_cast<uint32_t>(atoi(argv[2]));
        } else if (strcmp(argv[1], "pulp") == 0) {
            arg = 10;
        } else if (strcmp(argv[1], "tap") == 0 && argc >= 4) {
            arg = 11;
            arg2 = (static_cast<uint32_t>(atoi(argv[2])) << 16) |
                   (static_cast<uint32_t>(atoi(argv[3])) & 0xFFFF);
        } else if (strcmp(argv[1], "swipe") == 0 && argc >= 3) {
            arg = 12;
            arg2 = static_cast<uint32_t>(atoi(argv[2]));
        } else if (strcmp(argv[1], "hits") == 0) {
            arg = 13;
        } else {
            printf("error: usage js [status|probe N|pulp|tap X Y|"
                   "swipe K|hits]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status =
        RunConsoleOpWithArgs(ConsoleOp::Js, arg, arg2, &reply, 15000);
    if (status != StatusCode::Ok) {
        printf("js op result: %s\n", StatusCodeName(status));
    }
    PrintJsSnapshot(reply.payload.js);
    return status == StatusCode::Ok ? 0 : 1;
}

int CmdBuzz(int argc, char **argv) {
    uint32_t arg = 0;
    uint32_t arg2 = 0;
    if (argc >= 2 && strcmp(argv[1], "status") != 0) {
        if (strcmp(argv[1], "beep") == 0) {
            arg = 1;
        } else if (strcmp(argv[1], "stop") == 0) {
            arg = 2;
        } else if (strcmp(argv[1], "tone") == 0 && argc >= 4) {
            arg = 3;
            arg2 = (static_cast<uint32_t>(atoi(argv[2])) << 16) |
                   (static_cast<uint32_t>(atoi(argv[3])) & 0xFFFF);
        } else if (strcmp(argv[1], "melody") == 0) {
            arg = 4;
        } else {
            printf("error: usage buzz [status|beep|stop|tone F MS|"
                   "melody]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status = RunConsoleOpWithArgs(
        ConsoleOp::Buzz, arg, arg2, &reply, kReplyTimeoutMs);
    const BuzzSnapshot &b = reply.payload.buzz;
    printf("buzz init=%u playing=%u melody=%u (%u/%u) tones=%u "
           "result=%s\n",
           b.initialized, b.playing, b.melody_active, b.melody_index,
           b.melody_len, static_cast<unsigned>(b.tones_played),
           StatusCodeName(status));
    return status == StatusCode::Ok ? 0 : 1;
}

int CmdNet(int argc, char **argv) {
    AppEvent event = MakeEvent(AppEventKind::ConsoleCommand,
                               EventSource::Console, true);
    event.payload.console.op = ConsoleOp::Net;
    event.payload.console.arg = 0;
    ConsolePayload &c = event.payload.console;
    if (argc >= 2 && strcmp(argv[1], "status") != 0) {
        if (strcmp(argv[1], "scan") == 0) {
            c.arg = 1;
        } else if (strcmp(argv[1], "join") == 0 && argc >= 3) {
            c.arg = 2;
            snprintf(c.str_a, sizeof(c.str_a), "%s", argv[2]);
            snprintf(c.str_b, sizeof(c.str_b), "%s",
                     argc >= 4 ? argv[3] : "");
        } else if (strcmp(argv[1], "joinsaved") == 0) {
            c.arg = 3;
        } else if (strcmp(argv[1], "save") == 0 && argc >= 4) {
            c.arg = 4;
            snprintf(c.str_a, sizeof(c.str_a), "%s", argv[2]);
            snprintf(c.str_b, sizeof(c.str_b), "%s", argv[3]);
        } else if (strcmp(argv[1], "forget") == 0 && argc >= 3) {
            c.arg = 5;
            snprintf(c.str_a, sizeof(c.str_a), "%s", argv[2]);
        } else if (strcmp(argv[1], "off") == 0) {
            c.arg = 6;
        } else if (strcmp(argv[1], "saved") == 0) {
            c.arg = 7;
        } else if (strcmp(argv[1], "results") == 0) {
            c.arg = 8;
        } else {
            printf("error: usage net [status|scan|results|join SSID "
                   "[PASS]|joinsaved|save SSID PASS|forget SSID|saved|"
                   "off]\n");
            return 1;
        }
    }
    const StatusCode posted = PostEvent(event);
    if (posted != StatusCode::Ok) {
        printf("error: enqueue failed: %s\n", StatusCodeName(posted));
        return 1;
    }
    AppReply reply;
    const StatusCode status =
        AwaitReply(s_reply_queue, event.request_id, 5000, &reply);
    if (status != StatusCode::Ok) {
        printf("net op result: %s\n", StatusCodeName(status));
        return 1;
    }
    const NetSnapshot &n = reply.payload.net;
    static const char *kStates[] = {"off", "idle", "scanning", "joining",
                                    "up"};
    printf("net state=%s ip=%s ssid=\"%s\" rssi=%d scan=%u saved=%u "
           "result=%s\n",
           n.state <= 4 ? kStates[n.state] : "?", n.ip, n.ssid,
           static_cast<int>(n.rssi), static_cast<unsigned>(n.scan_count),
           static_cast<unsigned>(n.saved_count),
           StatusCodeName(reply.status));
    return reply.status == StatusCode::Ok ? 0 : 1;
}

int CmdHttp(int argc, char **argv) {
    AppEvent event = MakeEvent(AppEventKind::ConsoleCommand,
                               EventSource::Console, true);
    event.payload.console.op = ConsoleOp::Http;
    event.payload.console.arg = 0;
    ConsolePayload &c = event.payload.console;
    if (argc >= 2 && strcmp(argv[1], "status") != 0) {
        if (strcmp(argv[1], "get") == 0 && argc >= 3) {
            c.arg = 1;
            snprintf(c.str_a, sizeof(c.str_a), "%s", argv[2]);
            c.arg2 = argc >= 4 ? static_cast<uint32_t>(atoi(argv[3])) : 0;
        } else if (strcmp(argv[1], "abort") == 0) {
            c.arg = 2;
        } else if (strcmp(argv[1], "body") == 0) {
            c.arg = 3;
        } else {
            printf("error: usage http [status|get URL [LIMIT]|body|"
                   "abort]\n");
            return 1;
        }
    }
    const StatusCode posted = PostEvent(event);
    if (posted != StatusCode::Ok) {
        printf("error: enqueue failed: %s\n", StatusCodeName(posted));
        return 1;
    }
    AppReply reply;
    const StatusCode status =
        AwaitReply(s_reply_queue, event.request_id, 5000, &reply);
    if (status != StatusCode::Ok) {
        printf("http op result: %s\n", StatusCodeName(status));
        return 1;
    }
    const HttpSnapshot &h = reply.payload.http;
    printf("http in_flight=%u status=%d len=%u limit=%u url=\"%s\" "
           "result=%s\n",
           h.in_flight, static_cast<int>(h.status),
           static_cast<unsigned>(h.length),
           static_cast<unsigned>(h.limit), h.url,
           StatusCodeName(reply.status));
    return reply.status == StatusCode::Ok ? 0 : 1;
}

int CmdServe(int argc, char **argv) {
    uint32_t arg = 0;
    uint32_t arg2 = 80;
    if (argc >= 2 && strcmp(argv[1], "status") != 0) {
        if (strcmp(argv[1], "start") == 0) {
            arg = 1;
            if (argc >= 3) {
                arg2 = static_cast<uint32_t>(atoi(argv[2]));
            }
        } else if (strcmp(argv[1], "stop") == 0) {
            arg = 2;
        } else if (strcmp(argv[1], "mount") == 0) {
            arg = 3;
        } else {
            printf("error: usage serve [status|start [PORT]|stop|"
                   "mount]\n");
            return 1;
        }
    }
    AppReply reply;
    const StatusCode status = RunConsoleOpWithArgs(
        ConsoleOp::Serve, arg, arg2, &reply, 5000);
    const ServeSnapshot &s = reply.payload.serve;
    printf("serve running=%u port=%u routes=%u static=%u url=\"%s\" "
           "requests=%u busy503=%u timeout503=%u result=%s\n",
           s.running, static_cast<unsigned>(s.port),
           static_cast<unsigned>(s.routes), s.static_mounted, s.url,
           static_cast<unsigned>(s.requests),
           static_cast<unsigned>(s.busy_503),
           static_cast<unsigned>(s.timeout_503), StatusCodeName(status));
    return status == StatusCode::Ok ? 0 : 1;
}

int CmdHome(int, char **) {
    AppReply reply;
    const StatusCode status =
        RunConsoleOp(ConsoleOp::Home, 0, &reply, 15000);
    printf("home present: %s\n", StatusCodeName(status));
    return status == StatusCode::Ok ? 0 : 1;
}

void RegisterCommand(const char *name, const char *help,
                     esp_console_cmd_func_t fn) {
    const esp_console_cmd_t cmd = {
        .command = name,
        .help = help,
        .hint = nullptr,
        .func = fn,
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
    repl_config.prompt = "pulp>";
    repl_config.max_cmdline_length = 160;
    esp_console_dev_usb_serial_jtag_config_t hw_config =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(
        &hw_config, &repl_config, &repl));

    RegisterCommand("status", "App state and per-kind event counters",
                    &CmdStatus);
    RegisterCommand("heap", "Internal and PSRAM heap diagnostics",
                    &CmdHeap);
    RegisterCommand("events", "Event queue and per-source accounting",
                    &CmdEvents);
    RegisterCommand("display", "Backend states and present count",
                    &CmdDisplay);
    RegisterCommand("ping", "Round-trip an event through the owner",
                    &CmdPing);
    RegisterCommand("touch", "touch [on|off|status] - GT911 polling",
                    &CmdTouch);
    RegisterCommand("sd",
                    "sd [mount|unmount|seed|status|reload|fault k m] - "
                    "microSD card",
                    &CmdSd);
    RegisterCommand("sleep",
                    "sleep [status|deep N|rtc-off N|off|auto N] - power",
                    &CmdSleep);
    RegisterCommand("home", "Present the native home page", &CmdHome);
    RegisterCommand("serve",
                    "serve [status|start [PORT]|stop|mount] - web server",
                    &CmdServe);
    RegisterCommand("http",
                    "http [status|get URL [LIMIT]|body|abort] - bounded "
                    "fetch",
                    &CmdHttp);
    RegisterCommand("net",
                    "net [status|scan|join SSID [PASS]|joinsaved|save "
                    "SSID PASS|forget SSID|saved|off] - wifi station",
                    &CmdNet);
    RegisterCommand("buzz",
                    "buzz [status|beep|stop|tone F MS|melody] - GPIO21 "
                    "buzzer",
                    &CmdBuzz);
    RegisterCommand("js",
                    "js [status|probe N|pulp|tap X Y|swipe K] - "
                    "MicroQuickJS runtime",
                    &CmdJs);

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

}  // namespace pulp
