#include "ppa_client.h"

#include <algorithm>
#include <cstring>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#include "ppa_proto.h"
#include "wifi_mgr.h"

namespace {
const char *TAG = "ppa_client";

// Timing constants validated by the prototype (design doc §4.5).
constexpr uint32_t kAckTimeoutMs = 2500;
constexpr int kMaxAttempts = 5;
constexpr uint32_t kBusyBackoffMs = 500;
constexpr uint32_t kDiscoveryIntervalMs = 20000;
constexpr uint32_t kDiscoveryExpiryMs = 60000; // 3 missed refresh cycles
constexpr uint32_t kSocketPollMs = 50;

struct Found {
    uint32_t uid;
    uint32_t ip; // network byte order
    int64_t last_seen_ms;
};

// Per-action recall state machine: SEND -> AWAIT_ACK -> (OK | BUSY_BACKOFF -> SEND | FAIL)
struct RecallSlot {
    PpaAction action;
    uint32_t ip = 0;
    uint16_t seq = 0;
    int attempts = 0;
    int64_t deadline_ms = 0;
    int64_t backoff_until_ms = 0;
    bool awaiting = false;
    bool done = false;
    bool ok = false;
};

struct RecallJob {
    bool active = false;
    int scene_index = -1;
    std::vector<RecallSlot> slots;
};

SemaphoreHandle_t s_mutex = nullptr;
std::vector<PpaScene> s_scenes;
std::vector<Found> s_found;
QueueHandle_t s_event_out = nullptr;
QueueHandle_t s_cmd_queue = nullptr; // int scene_index
int s_socket = -1;
uint16_t s_seq = 0x0100;
RecallJob s_job;

int64_t now_ms() { return esp_timer_get_time() / 1000; }

uint16_t next_seq() { return ++s_seq; }

void send_to(uint32_t ip_nbo, const uint8_t *buf, size_t len) {
    sockaddr_in dst = {};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(PPA_PORT);
    dst.sin_addr.s_addr = ip_nbo;
    sendto(s_socket, buf, len, 0, reinterpret_cast<sockaddr *>(&dst), sizeof(dst));
}

void send_ping(uint32_t ip_nbo) {
    uint8_t buf[PPA_PING_LEN];
    ppa_encode_ping(buf, next_seq());
    send_to(ip_nbo, buf, sizeof(buf));
}

void broadcast_discovery() {
    uint32_t local = wifi_mgr_sta_ip();
    if (local == 0) return;
    send_ping(local | htonl(0x000000FFu)); // /24 directed broadcast
    send_ping(INADDR_BROADCAST);
}

// Locked helpers -------------------------------------------------------------

uint32_t ip_for_action_locked(const PpaAction &a) {
    if (a.uid == 0) return a.fixed_ip;
    for (const auto &f : s_found)
        if (f.uid == a.uid) return f.ip;
    return 0;
}

void expire_found_locked() {
    const int64_t cutoff = now_ms() - kDiscoveryExpiryMs;
    s_found.erase(std::remove_if(s_found.begin(), s_found.end(),
                                 [cutoff](const Found &f) { return f.last_seen_ms < cutoff; }),
                  s_found.end());
}

void post_event(const PpaEvent &ev) {
    if (s_event_out != nullptr) xQueueSend(s_event_out, &ev, 0);
}

// Recall engine ---------------------------------------------------------------

void start_recall(int scene_index) {
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_job = RecallJob{};
    if (scene_index < 0 || scene_index >= static_cast<int>(s_scenes.size())) {
        xSemaphoreGive(s_mutex);
        return;
    }
    s_job.active = true;
    s_job.scene_index = scene_index;
    for (const auto &a : s_scenes[scene_index].actions) {
        RecallSlot slot;
        slot.action = a;
        slot.ip = ip_for_action_locked(a);
        if (slot.ip == 0) { // module unknown: fail immediately
            slot.done = true;
            slot.ok = false;
        }
        s_job.slots.push_back(slot);
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "recall scene %d (%d actions)", scene_index,
             static_cast<int>(s_job.slots.size()));
}

void finish_job_if_done() {
    if (!s_job.active) return;
    int done = 0, ok = 0;
    for (const auto &s : s_job.slots) {
        if (s.done) done++;
        if (s.ok) ok++;
    }
    if (done == static_cast<int>(s_job.slots.size())) {
        const bool all_ok = ok == done && done > 0;
        ESP_LOGI(TAG, "recall done: %d/%d ok", ok, done);
        post_event({PpaEventType::kRecallDone, s_job.scene_index, ok,
                    static_cast<int>(s_job.slots.size()), all_ok});
        s_job.active = false;
    }
}

void drive_recall() {
    if (!s_job.active) return;
    const int64_t now = now_ms();
    for (auto &slot : s_job.slots) {
        if (slot.done) continue;
        if (slot.awaiting) {
            if (now >= slot.deadline_ms) slot.awaiting = false; // timeout -> resend
            else continue;
        }
        if (now < slot.backoff_until_ms) continue;
        if (slot.attempts >= kMaxAttempts) {
            slot.done = true;
            slot.ok = false;
            continue;
        }
        slot.attempts++;
        slot.seq = next_seq();
        uint8_t buf[PPA_RECALL_LEN];
        ppa_encode_recall(buf, slot.seq, slot.action.preset_id, slot.action.preset_sub);
        send_to(slot.ip, buf, sizeof(buf));
        slot.awaiting = true;
        slot.deadline_ms = now + kAckTimeoutMs;
    }
    finish_job_if_done();
}

void handle_packet(const uint8_t *buf, size_t len, uint32_t from_ip) {
    ppa_header_t hdr;
    if (!ppa_decode_header(buf, len, &hdr)) return;
    const uint8_t kind = static_cast<uint8_t>(hdr.status & 0xFF);

    // Ping replies (0x01 and 0x09 both mean "present", prototype quirk).
    if (hdr.type == PPA_TYPE_PING && (kind == PPA_KIND_OK || kind == PPA_KIND_ERROR)) {
        xSemaphoreTake(s_mutex, portMAX_DELAY);
        bool known = false;
        for (auto &f : s_found) {
            if (f.uid == hdr.uid) {
                f.ip = from_ip;
                f.last_seen_ms = now_ms();
                known = true;
            }
        }
        if (!known) {
            s_found.push_back({hdr.uid, from_ip, now_ms()});
            ESP_LOGI(TAG, "discovered uid=%08lx", static_cast<unsigned long>(hdr.uid));
        }
        xSemaphoreGive(s_mutex);
        post_event({PpaEventType::kDiscoveryUpdate, -1, 0, 0, false});
    }

    // Match recall replies to awaiting slots.
    if (!s_job.active) return;
    for (auto &slot : s_job.slots) {
        if (slot.done || !slot.awaiting || slot.seq != hdr.seq || slot.ip != from_ip) continue;
        switch (ppa_classify_reply(&hdr, buf, len)) {
        case PPA_REPLY_OK:
            slot.done = true;
            slot.ok = true;
            slot.awaiting = false;
            break;
        case PPA_REPLY_BUSY:
            slot.awaiting = false;
            slot.backoff_until_ms = now_ms() + kBusyBackoffMs;
            break;
        case PPA_REPLY_ERROR:
            slot.done = true;
            slot.ok = false;
            slot.awaiting = false;
            break;
        case PPA_REPLY_WAIT:
        case PPA_REPLY_OTHER:
            break; // keep waiting within the deadline
        }
        int done = 0;
        for (const auto &s : s_job.slots)
            if (s.done && s.ok) done++;
        post_event({PpaEventType::kRecallProgress, s_job.scene_index, done,
                    static_cast<int>(s_job.slots.size()), false});
    }
    finish_job_if_done();
}

void ppa_task(void *) {
    // Wait for STA connectivity before binding.
    while (wifi_mgr_state() != WifiState::kConnected) vTaskDelay(pdMS_TO_TICKS(250));

    s_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    int bcast = 1;
    setsockopt(s_socket, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));
    sockaddr_in local = {};
    local.sin_family = AF_INET;
    local.sin_port = htons(PPA_PORT);
    local.sin_addr.s_addr = INADDR_ANY;
    if (bind(s_socket, reinterpret_cast<sockaddr *>(&local), sizeof(local)) != 0) {
        ESP_LOGE(TAG, "bind :%d failed errno=%d", PPA_PORT, errno);
        vTaskDelete(nullptr);
        return;
    }
    ESP_LOGI(TAG, "listening on :%d", PPA_PORT);
    broadcast_discovery();
    int64_t last_discovery = now_ms();

    while (true) {
        // Commands (scene recalls) are non-blocking.
        int scene_index;
        while (xQueueReceive(s_cmd_queue, &scene_index, 0) == pdTRUE) start_recall(scene_index);

        // Pump the socket with a bounded wait.
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s_socket, &rfds);
        timeval tv = {0, kSocketPollMs * 1000};
        if (select(s_socket + 1, &rfds, nullptr, nullptr, &tv) > 0) {
            uint8_t buf[64];
            sockaddr_in from = {};
            socklen_t from_len = sizeof(from);
            int n = recvfrom(s_socket, buf, sizeof(buf), 0,
                             reinterpret_cast<sockaddr *>(&from), &from_len);
            if (n >= PPA_HEADER_LEN) handle_packet(buf, n, from.sin_addr.s_addr);
        }

        drive_recall();

        if (now_ms() - last_discovery > kDiscoveryIntervalMs) {
            last_discovery = now_ms();
            xSemaphoreTake(s_mutex, portMAX_DELAY);
            expire_found_locked();
            xSemaphoreGive(s_mutex);
            broadcast_discovery();
            post_event({PpaEventType::kDiscoveryUpdate, -1, 0, 0, false});
        }
    }
}
} // namespace

bool ppa_client_start(QueueHandle_t event_out) {
    s_event_out = event_out;
    s_mutex = xSemaphoreCreateMutex();
    s_cmd_queue = xQueueCreate(4, sizeof(int));
    if (s_mutex == nullptr || s_cmd_queue == nullptr) return false;
    return xTaskCreatePinnedToCore(ppa_task, "ppa_client", 6144, nullptr, 6, nullptr, 0) == pdPASS;
}

void ppa_client_set_scenes(const std::vector<PpaScene> &scenes) {
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_scenes = scenes;
    xSemaphoreGive(s_mutex);
}

void ppa_client_request_recall(int scene_index) {
    if (s_cmd_queue != nullptr) xQueueSend(s_cmd_queue, &scene_index, 0);
}

int ppa_client_online_count(int scene_index) {
    int online = 0;
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (scene_index >= 0 && scene_index < static_cast<int>(s_scenes.size())) {
        for (const auto &a : s_scenes[scene_index].actions)
            if (ip_for_action_locked(a) != 0) online++;
    }
    xSemaphoreGive(s_mutex);
    return online;
}
