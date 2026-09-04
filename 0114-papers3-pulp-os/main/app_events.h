// Event/command/reply contracts for PULP OS (ESP-51).
//
// Contract rules (inherited from the hardware-proven 0112 architecture):
//  - Every type here is POD. No owned std::string, no borrowed pointers into
//    producer stacks, no JS values. Payload text lives in fixed char arrays.
//  - Producers (console task, touch ticker, timers) only enqueue AppEvent.
//    All model mutation happens in the owner task.
//  - Replies travel through a bounded FreeRTOS queue whose handle is carried
//    in the event. A full reply queue is an explicit counted failure, never
//    a silent block.
#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "s3paper/display_backend.h"
#include "s3paper/status.h"

namespace pulp {

using StatusCode = s3paper::StatusCode;
using s3paper::StatusCodeName;

enum class AppEventKind : uint8_t {
    ConsoleCommand = 0,
    Pointer,
    TimerDue,
    ShutdownRequest,
    // ESP-53 completion mailboxes: a module worker finished; the payload
    // carries the three ints for the JS callback. Result data waits in
    // the module's mailbox (written BEFORE this event was posted; the
    // queue send/receive is the memory barrier).
    ModuleDone,
    kCount,
};

// ESP-53 module ids (one pending completion callback per module).
enum class ModuleId : uint8_t {
    Files = 0,
    Wifi,
    Http,
    Serve,
    Images,  // ESP-54 image upload completion
    Apps,    // ESP-55 app upload completion
    Nav,     // ESP-55 P9 page navigation request
    Mdns,    // ESP-58 mDNS browse completion
    kCount,
};

// Completion-callback kinds delivered as the first CallCb argument.
// Shared JS-visible vocabulary — keep in sync with the intern guide §3.
enum ModuleDoneKind : int32_t {
    kDoneWifiScan = 1,
    kDoneWifiJoin = 2,
    kDoneHttp = 3,
    kDoneServeRequest = 4,
    kDoneFilesList = 10,
    kDoneFilesRead = 11,
    kDoneFilesWrite = 12,
    kDoneFilesAppend = 13,
    kDoneFilesRemove = 14,
    kDoneImagesUpload = 20,  // ESP-54: value = bytes, err = 0 ok
    kDoneAppsUpload = 30,    // ESP-55: value = bytes, err 0 ok/1 card/2 short
    kDoneNavRequest = 40,    // ESP-55 P9: value = 1 go / 2 back / 3 reload
    kDoneMdnsBrowse = 50,    // ESP-58: value = servers found, err 0 ok/1 no wifi
};

const char *AppEventKindName(AppEventKind kind);

enum class EventSource : uint8_t {
    Console = 0,
    Internal,
    kCount,
};

const char *EventSourceName(EventSource source);

enum class ConsoleOp : uint8_t {
    Status = 0,
    Heap,
    Events,
    Display,
    Ping,
    Touch,   // arg: 0 = status, 1 = enable, 2 = disable
    Sd,      // arg: 0 = status, 1 = mount, 2 = unmount, 3 = seed,
             //      4 = reload, 10.. = fault injection (10 + kind*3 + mode)
    Sleep,   // arg: 0 = status, 1 = deep (arg2 s), 2 = rtc-off (arg2 s),
             //      3 = off, 4 = auto-sleep policy (arg2 s, 0 disables)
    Home,    // present the native home page (Phase 4 skeleton)
    Js,      // JS runtime ops (Phase 5+)
    Buzz,    // arg: 0 = status, 1 = beep, 2 = stop,
             //      3 = tone (arg2 = freq<<16 | ms), 4 = demo melody
    Net,     // arg: 0 = status, 1 = scan, 2 = join (str_a/str_b),
             //      3 = join-saved, 4 = save (str_a/str_b),
             //      5 = forget (str_a), 6 = off, 7 = saved-list (printed)
    Http,    // arg: 0 = status, 1 = get (str_a = url, arg2 = limit or 0),
             //      2 = abort, 3 = print body head
    Serve,   // arg: 0 = status, 1 = start (arg2 = port),
             //      2 = stop, 3 = mount /sdcard/www
    Battery, // arg: 0 = status (level/mv/charging printed)  [ESP-54]
    Mdns,    // arg: 0 = status, 1 = announce (arg2 = port),  [ESP-54]
             //      2 = stop
    Images,  // arg: 0 = status, 1 = list, 2 = display (str_a = name), [ESP-54]
             //      3 = remove (str_a = name), 4 = received cb status
    Shot,    // stream the framebuffer as QOI over USB serial [ESP-56]
};

enum class PointerPhase : uint8_t {
    Down = 0,
    Move,
    Up,
    Cancel,
};

struct ConsolePayload {
    ConsoleOp op;
    uint32_t arg;
    uint32_t arg2;
    // Bounded string arguments (ESP-53: net join/save/forget, http get).
    // POD rule kept: fixed arrays copied by value with the event.
    char str_a[128];
    char str_b[65];
};

struct PointerPayload {
    int32_t x;
    int32_t y;
    PointerPhase phase;
    uint8_t pointer_id;
};

struct TimerPayload {
    uint32_t timer_id;
};

struct ModuleDonePayload {
    ModuleId module;
    int32_t kind;   // ModuleDoneKind
    int32_t value;  // count / status code / bytes, per kind
    int32_t err;    // 0 = ok; module-specific error otherwise
};

struct AppEvent {
    AppEventKind kind;
    EventSource source;
    // Monotonic per-source sequence assigned by the producer; the owner
    // verifies it arrives strictly ordered per source.
    uint32_t producer_seq;
    // Correlation id for request/reply commands; 0 when no reply expected.
    uint32_t request_id;
    int64_t monotonic_us;
    // Bounded reply channel; nullptr when the event has no reply. The
    // handle refers to a queue owned by the requesting task and outliving
    // the wait.
    QueueHandle_t reply_queue;
    union {
        ConsolePayload console;
        PointerPayload pointer;
        TimerPayload timer;
        ModuleDonePayload module_done;
    } payload;
};

// Snapshot payloads returned by the owner. Fixed-size, copied by value.
struct StatusSnapshot {
    uint32_t uptime_ms;
    uint32_t owner_seq;
    uint32_t processed_by_kind[static_cast<uint8_t>(AppEventKind::kCount)];
    uint8_t app_state;
};

struct HeapSnapshot {
    uint32_t internal_free;
    uint32_t internal_min_free;
    uint32_t internal_largest_block;
    uint32_t psram_free;
    uint32_t psram_largest_block;
};

struct EventsSnapshot {
    uint32_t queue_capacity;
    uint32_t queue_depth_now;
    uint32_t queue_high_water;
    uint32_t accepted_by_source[static_cast<uint8_t>(EventSource::kCount)];
    uint32_t dropped_by_source[static_cast<uint8_t>(EventSource::kCount)];
    uint32_t out_of_order_total;
    uint32_t replies_sent;
    uint32_t replies_dropped;
};

struct DisplaySnapshot {
    uint8_t app_state;
    uint8_t fake_initialized;
    uint8_t m5_initialized;
    int32_t m5_w;
    int32_t m5_h;
    uint32_t fake_frames;
    uint32_t m5_frames;
    uint32_t present_count;
};

struct TouchSnapshot {
    uint8_t enabled;
    uint32_t samples;
    uint32_t events_by_kind[4];    // s3paper::PointerEventKind order
    uint32_t gestures_by_kind[6];  // s3paper::GestureKind order
    int32_t last_x;
    int32_t last_y;
    uint8_t last_gesture;  // 0xff = none
    int32_t last_gesture_x;
    int32_t last_gesture_y;
    uint32_t quiet_windows;
    int64_t last_input_age_ms;  // -1 = never
};

struct PowerSnapshot {
    int32_t battery_level;  // 0..100, -1 unknown
    int32_t battery_mv;
    uint8_t charging;
    uint8_t wakeup_cause;   // esp_sleep_wakeup_cause_t of this boot
    uint8_t reset_reason;   // esp_reset_reason_t of this boot
    uint32_t auto_sleep_sec;
};

struct SdSnapshot {
    uint8_t mounted;
    uint32_t capacity_mib;
    uint32_t book_count;
    uint32_t position_records;
    uint32_t position_writes;
    uint32_t position_write_failures;
    uint32_t scan_cached;
    uint32_t scan_hashed;
    uint32_t scan_ms;
    uint32_t catalog_writes;
};

struct NetSnapshot {
    uint8_t state;  // 0 off, 1 idle, 2 scanning, 3 joining, 4 up
    char ip[16];
    char ssid[33];
    int32_t rssi;
    uint32_t scan_count;
    uint32_t saved_count;
};

struct HttpSnapshot {
    uint8_t in_flight;
    int32_t status;
    uint32_t length;
    uint32_t limit;
    char url[64];
};

struct ServeSnapshot {
    uint8_t running;
    uint8_t static_mounted;
    uint16_t port;
    uint32_t routes;
    uint32_t requests;
    uint32_t busy_503;
    uint32_t timeout_503;
    char url[32];
};

// ESP-54 mDNS + image gallery snapshots.
struct MdnsSnapshot {
    uint8_t announced;   // 0 off, 1 announced
    uint16_t port;
    char host[24];       // "pulp"
    char url[40];        // "http://pulp.local" or ""
};

struct ImagesSnapshot {
    uint8_t sd_ok;       // 1 = /sdcard/images mounted & writable
    uint32_t count;      // images on the card
    uint32_t last_bytes; // last upload byte count
    uint8_t upload_busy; // 1 = a POST upload is in flight
};

struct BuzzSnapshot {
    uint8_t initialized;
    uint8_t playing;  // 1 = tone or melody currently sounding
    uint8_t melody_active;
    uint8_t melody_len;
    uint8_t melody_index;
    uint32_t tones_played;
};

struct JsSnapshot {
    uint8_t initialized;
    uint8_t screen_active;
    uint32_t arena_bytes;
    uint32_t arena_used;  // ESP-55: JS_GetHeapUsed at snapshot time
    uint32_t loads;         // ESP-55 P3: load() invocations
    uint32_t last_load_ms;  // ESP-55 P3: wall time of the last load()
    uint32_t evals;
    uint32_t exceptions;
    uint32_t dispatches;
    char last_error[48];
};

struct AppReply {
    uint32_t request_id;
    StatusCode status;
    union {
        StatusSnapshot status_snapshot;
        HeapSnapshot heap;
        EventsSnapshot events;
        DisplaySnapshot display;
        TouchSnapshot touch;
        PowerSnapshot power;
        SdSnapshot sd;
        JsSnapshot js;
        BuzzSnapshot buzz;
        NetSnapshot net;
        HttpSnapshot http;
        ServeSnapshot serve;
        MdnsSnapshot mdns;       // ESP-54
        ImagesSnapshot images;  // ESP-54
        int64_t echo_monotonic_us;  // Ping
    } payload;
};

}  // namespace pulp
