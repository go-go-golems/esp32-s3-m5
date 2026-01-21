#include "mled_node.h"

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "mled_protocol.h"
#include "mled_time.h"

static const char *TAG = "mlednode_node";

#define MLEDNODE_MAX_CUES 64
#define MLEDNODE_MAX_PENDING_FIRES 64
#define MLEDNODE_MAX_TIME_REQS 16

typedef struct {
    bool valid;
    uint32_t cue_id;
    mled_pattern_config_t pattern;
    uint16_t fade_in_ms;
    uint16_t fade_out_ms;
} cue_entry_t;

typedef struct {
    uint32_t cue_id;
    uint32_t execute_at_ms;
} pending_fire_t;

typedef struct {
    bool valid;
    uint32_t req_id;
    uint32_t t0_local_ms;
} time_req_entry_t;

typedef struct {
    TaskHandle_t task;
    bool started;
    bool stop;

    int fd;

    uint32_t node_id;
    char name[17];

    uint32_t controller_epoch;
    struct sockaddr_in controller_addr;
    bool controller_addr_valid;

    int32_t show_time_offset_ms;
    bool time_synced;
    const char *last_time_sync_method;
    int32_t last_time_req_rtt_ms;
    uint32_t last_time_req_local_ms;

    uint8_t current_pattern;
    uint8_t brightness_pct;
    uint16_t frame_ms;
    uint32_t active_cue_id;

    cue_entry_t cues[MLEDNODE_MAX_CUES];
    pending_fire_t fires[MLEDNODE_MAX_PENDING_FIRES];
    uint32_t fires_len;

    uint32_t time_req_counter;
    time_req_entry_t time_reqs[MLEDNODE_MAX_TIME_REQS];
} node_ctx_t;

static node_ctx_t s_ctx = {0};

static uint32_t fnv1a32(const uint8_t *data, size_t n)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        h ^= data[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t compute_node_id(void)
{
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        return 1;
    }
    uint32_t id = fnv1a32(mac, sizeof(mac));
    if (id == 0) id = 1;
    return id;
}

uint32_t mled_node_id(void)
{
    return s_ctx.node_id;
}

static uint32_t show_ms(void)
{
    const uint32_t local = mled_time_local_ms();
    return (uint32_t)(local + (uint32_t)s_ctx.show_time_offset_ms);
}

static void cues_clear(void)
{
    for (size_t i = 0; i < MLEDNODE_MAX_CUES; i++) {
        s_ctx.cues[i].valid = false;
    }
}

static cue_entry_t *cue_find(uint32_t cue_id)
{
    for (size_t i = 0; i < MLEDNODE_MAX_CUES; i++) {
        if (s_ctx.cues[i].valid && s_ctx.cues[i].cue_id == cue_id) {
            return &s_ctx.cues[i];
        }
    }
    return NULL;
}

static cue_entry_t *cue_put(const mled_cue_prepare_t *p)
{
    cue_entry_t *existing = cue_find(p->cue_id);
    if (existing) {
        existing->pattern = p->pattern;
        existing->fade_in_ms = p->fade_in_ms;
        existing->fade_out_ms = p->fade_out_ms;
        return existing;
    }

    for (size_t i = 0; i < MLEDNODE_MAX_CUES; i++) {
        if (!s_ctx.cues[i].valid) {
            s_ctx.cues[i].valid = true;
            s_ctx.cues[i].cue_id = p->cue_id;
            s_ctx.cues[i].pattern = p->pattern;
            s_ctx.cues[i].fade_in_ms = p->fade_in_ms;
            s_ctx.cues[i].fade_out_ms = p->fade_out_ms;
            return &s_ctx.cues[i];
        }
    }

    return NULL;
}

static void fires_add(uint32_t cue_id, uint32_t execute_at_ms)
{
    if (s_ctx.fires_len >= MLEDNODE_MAX_PENDING_FIRES) {
        return;
    }
    s_ctx.fires[s_ctx.fires_len].cue_id = cue_id;
    s_ctx.fires[s_ctx.fires_len].execute_at_ms = execute_at_ms;
    s_ctx.fires_len++;
}

static void fires_remove_cue(uint32_t cue_id)
{
    uint32_t w = 0;
    for (uint32_t r = 0; r < s_ctx.fires_len; r++) {
        if (s_ctx.fires[r].cue_id == cue_id) {
            continue;
        }
        s_ctx.fires[w++] = s_ctx.fires[r];
    }
    s_ctx.fires_len = w;
}

static int next_due_timeout_ms(void)
{
    if (s_ctx.fires_len == 0) {
        return 500;
    }

    const uint32_t now = show_ms();
    int32_t best = 500;
    for (uint32_t i = 0; i < s_ctx.fires_len; i++) {
        const uint32_t t = s_ctx.fires[i].execute_at_ms;
        const int32_t dt = mled_time_u32_duration(now, t);
        if (dt <= 0) {
            return 0;
        }
        if (dt < best) {
            best = dt;
        }
    }
    if (best < 0) best = 0;
    if (best > 500) best = 500;
    return (int)best;
}

static void apply_cue(uint32_t cue_id)
{
    cue_entry_t *entry = cue_find(cue_id);
    if (!entry) {
        ESP_LOGW(TAG, "fire cue=%" PRIu32 " but no prepare stored", cue_id);
        return;
    }

    s_ctx.current_pattern = entry->pattern.pattern_type;
    s_ctx.brightness_pct = entry->pattern.brightness_pct ? entry->pattern.brightness_pct : 1;
    s_ctx.active_cue_id = cue_id;

    ESP_LOGI(TAG,
             "APPLY cue=%" PRIu32 " pattern=%u bri=%u%% show_ms=%" PRIu32,
             cue_id,
             (unsigned)s_ctx.current_pattern,
             (unsigned)s_ctx.brightness_pct,
             show_ms());
}

static void process_due_fires(void)
{
    if (s_ctx.fires_len == 0) {
        return;
    }

    const uint32_t now = show_ms();
    uint32_t w = 0;
    for (uint32_t r = 0; r < s_ctx.fires_len; r++) {
        if (mled_time_is_due(now, s_ctx.fires[r].execute_at_ms)) {
            apply_cue(s_ctx.fires[r].cue_id);
            continue;
        }
        s_ctx.fires[w++] = s_ctx.fires[r];
    }
    s_ctx.fires_len = w;
}

static void build_header(mled_header_t *h, uint8_t type)
{
    memset(h, 0, sizeof(*h));
    h->magic[0] = (uint8_t)MLED_MAGIC0;
    h->magic[1] = (uint8_t)MLED_MAGIC1;
    h->magic[2] = (uint8_t)MLED_MAGIC2;
    h->magic[3] = (uint8_t)MLED_MAGIC3;
    h->version = MLED_VERSION;
    h->hdr_len = MLED_HEADER_SIZE;
    h->type = type;
}

static int send_unicast(const struct sockaddr_in *dest, const void *buf, size_t len)
{
    const int r = sendto(s_ctx.fd, buf, len, 0, (const struct sockaddr *)dest, sizeof(*dest));
    return (r == (int)len) ? 0 : -1;
}

static int wifi_rssi_dbm(void)
{
    wifi_ap_record_t ap = {};
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        return 0;
    }
    return (int)ap.rssi;
}

static int send_pong(const struct sockaddr_in *dest, uint32_t msg_id)
{
    uint8_t pkt[MLED_HEADER_SIZE + 43];

    mled_pong_t pong = {0};
    pong.uptime_ms = mled_time_local_ms();
    const int rssi = wifi_rssi_dbm();
    if (rssi < -128) {
        pong.rssi_dbm = -128;
    } else if (rssi > 127) {
        pong.rssi_dbm = 127;
    } else {
        pong.rssi_dbm = (int8_t)rssi;
    }

    uint8_t state_flags = 0x01; // running
    if (s_ctx.time_synced) state_flags |= 0x04;
    pong.state_flags = state_flags;
    pong.brightness_pct = s_ctx.brightness_pct;
    pong.pattern_type = s_ctx.current_pattern;
    pong.frame_ms = s_ctx.frame_ms;
    pong.active_cue_id = s_ctx.active_cue_id;
    pong.controller_epoch = s_ctx.controller_epoch;
    pong.show_ms_now = show_ms();
    memset(pong.name, 0, sizeof(pong.name));
    memcpy(pong.name, s_ctx.name, 16);

    uint8_t payload[43];
    mled_pong_pack(payload, &pong);

    mled_header_t h;
    build_header(&h, MLED_MSG_PONG);
    h.epoch_id = s_ctx.controller_epoch;
    h.msg_id = msg_id;
    h.sender_id = s_ctx.node_id;
    h.payload_len = 43;

    mled_header_pack(pkt, &h);
    memcpy(pkt + MLED_HEADER_SIZE, payload, 43);

    return send_unicast(dest, pkt, sizeof(pkt));
}

static int send_ack(const struct sockaddr_in *dest, uint32_t ack_for_msg_id, uint16_t code)
{
    uint8_t pkt[MLED_HEADER_SIZE + 8];

    mled_ack_t ack = {0};
    ack.ack_for_msg_id = ack_for_msg_id;
    ack.code = code;

    uint8_t payload[8];
    mled_ack_pack(payload, &ack);

    mled_header_t h;
    build_header(&h, MLED_MSG_ACK);
    h.epoch_id = s_ctx.controller_epoch;
    h.msg_id = 0;
    h.sender_id = s_ctx.node_id;
    h.payload_len = 8;

    mled_header_pack(pkt, &h);
    memcpy(pkt + MLED_HEADER_SIZE, payload, 8);

    return send_unicast(dest, pkt, sizeof(pkt));
}

static bool time_req_store(uint32_t req_id, uint32_t t0_local_ms)
{
    for (size_t i = 0; i < MLEDNODE_MAX_TIME_REQS; i++) {
        if (!s_ctx.time_reqs[i].valid) {
            s_ctx.time_reqs[i].valid = true;
            s_ctx.time_reqs[i].req_id = req_id;
            s_ctx.time_reqs[i].t0_local_ms = t0_local_ms;
            return true;
        }
    }
    return false;
}

static bool time_req_take(uint32_t req_id, uint32_t *out_t0)
{
    for (size_t i = 0; i < MLEDNODE_MAX_TIME_REQS; i++) {
        if (s_ctx.time_reqs[i].valid && s_ctx.time_reqs[i].req_id == req_id) {
            *out_t0 = s_ctx.time_reqs[i].t0_local_ms;
            s_ctx.time_reqs[i].valid = false;
            return true;
        }
    }
    return false;
}

static bool send_time_req(void)
{
#if !CONFIG_MLEDNODE_TIME_REQ_ENABLE
    return false;
#else
    if (!s_ctx.controller_addr_valid) {
        return false;
    }

    const uint32_t now = mled_time_local_ms();
    if (mled_time_u32_duration(s_ctx.last_time_req_local_ms, now) < 500) {
        return false;
    }
    s_ctx.last_time_req_local_ms = now;

    s_ctx.time_req_counter++;
    const uint32_t req_id = s_ctx.time_req_counter;
    const uint32_t t0 = mled_time_local_ms();
    if (!time_req_store(req_id, t0)) {
        return false;
    }

    uint8_t pkt[MLED_HEADER_SIZE];
    mled_header_t h;
    build_header(&h, MLED_MSG_TIME_REQ);
    h.epoch_id = s_ctx.controller_epoch;
    h.msg_id = req_id;
    h.sender_id = s_ctx.node_id;
    h.payload_len = 0;
    mled_header_pack(pkt, &h);

    if (send_unicast(&s_ctx.controller_addr, pkt, sizeof(pkt)) != 0) {
        return false;
    }

    ESP_LOGD(TAG, "sent TIME_REQ req_id=%" PRIu32, req_id);
    return true;
#endif
}

static void handle_time_resp(const uint8_t *payload, uint32_t payload_len)
{
    mled_time_resp_t resp = {0};
    if (!mled_time_resp_unpack(&resp, payload, payload_len)) {
        return;
    }

    uint32_t t0 = 0;
    if (!time_req_take(resp.req_msg_id, &t0)) {
        return;
    }

    const uint32_t t3 = mled_time_local_ms();
    const int32_t rtt = mled_time_u32_duration(t0, t3);
    const int32_t srv_proc = mled_time_u32_duration(resp.master_rx_show_ms, resp.master_tx_show_ms);

    int32_t effective = rtt - srv_proc;
    if (effective < 0) effective = 0;
    const uint32_t one_way = (uint32_t)(effective / 2);
    const uint32_t estimated_master_at_t3 = resp.master_tx_show_ms + one_way;

    s_ctx.show_time_offset_ms = mled_time_u32_diff(estimated_master_at_t3, t3);
    s_ctx.time_synced = true;
    s_ctx.last_time_sync_method = "TIME_REQ";
    s_ctx.last_time_req_rtt_ms = rtt;

    ESP_LOGI(TAG, "TIME_RESP req_id=%" PRIu32 " rtt=%" PRIi32 "ms new_offset=%" PRIi32 "ms", resp.req_msg_id, rtt, s_ctx.show_time_offset_ms);
}

static void handle_beacon(const mled_header_t *hdr, const struct sockaddr_in *src)
{
    const bool epoch_changed = (s_ctx.controller_epoch == 0) || (hdr->epoch_id != s_ctx.controller_epoch);
    if (epoch_changed) {
        s_ctx.controller_epoch = hdr->epoch_id;
        memcpy(&s_ctx.controller_addr, src, sizeof(*src));
        s_ctx.controller_addr_valid = true;
        cues_clear();
        s_ctx.fires_len = 0;
        s_ctx.active_cue_id = 0;
    } else {
        memcpy(&s_ctx.controller_addr, src, sizeof(*src));
        s_ctx.controller_addr_valid = true;
    }

    const uint32_t master_show = hdr->execute_at_ms;
    const uint32_t local = mled_time_local_ms();
    s_ctx.show_time_offset_ms = mled_time_u32_diff(master_show, local);
    s_ctx.time_synced = true;
    s_ctx.last_time_sync_method = "BEACON";

    char ip[32];
    inet_ntoa_r(src->sin_addr, ip, sizeof(ip));
    ESP_LOGD(TAG, "BEACON epoch=%" PRIu32 " from %s:%u offset=%" PRIi32 "ms", s_ctx.controller_epoch, ip, ntohs(src->sin_port), s_ctx.show_time_offset_ms);

    (void)send_time_req();
}

static void handle_ping(const mled_header_t *hdr, const struct sockaddr_in *src)
{
    (void)send_pong(src, hdr->msg_id);
}

static void handle_cue_prepare(const mled_header_t *hdr, const uint8_t *payload, uint32_t payload_len, const struct sockaddr_in *src)
{
    if (mled_header_target_mode(hdr) == MLED_TARGET_NODE && hdr->target != s_ctx.node_id) {
        return;
    }

    mled_cue_prepare_t cp = {0};
    if (!mled_cue_prepare_unpack(&cp, payload, payload_len)) {
        if (mled_header_ack_req(hdr)) {
            (void)send_ack(src, hdr->msg_id, 1);
        }
        return;
    }

    if (!cue_put(&cp)) {
        if (mled_header_ack_req(hdr)) {
            (void)send_ack(src, hdr->msg_id, 2);
        }
        return;
    }

    if (mled_header_ack_req(hdr)) {
        (void)send_ack(src, hdr->msg_id, 0);
    }

    ESP_LOGI(TAG, "CUE_PREPARE cue=%" PRIu32 " pattern=%u bri=%u%%", cp.cue_id, (unsigned)cp.pattern.pattern_type, (unsigned)cp.pattern.brightness_pct);
}

static void handle_cue_fire(const mled_header_t *hdr, const uint8_t *payload, uint32_t payload_len)
{
    mled_cue_fire_t cf = {0};
    if (!mled_cue_fire_unpack(&cf, payload, payload_len)) {
        return;
    }
    fires_add(cf.cue_id, hdr->execute_at_ms);
    ESP_LOGI(TAG, "CUE_FIRE cue=%" PRIu32 " execute_at=%" PRIu32, cf.cue_id, hdr->execute_at_ms);
}

static void handle_cue_cancel(const uint8_t *payload, uint32_t payload_len)
{
    mled_cue_fire_t cf = {0};
    if (!mled_cue_fire_unpack(&cf, payload, payload_len)) {
        return;
    }
    for (size_t i = 0; i < MLEDNODE_MAX_CUES; i++) {
        if (s_ctx.cues[i].valid && s_ctx.cues[i].cue_id == cf.cue_id) {
            s_ctx.cues[i].valid = false;
        }
    }
    fires_remove_cue(cf.cue_id);
    if (s_ctx.active_cue_id == cf.cue_id) {
        s_ctx.active_cue_id = 0;
    }
    ESP_LOGI(TAG, "CUE_CANCEL cue=%" PRIu32, cf.cue_id);
}

static int open_socket(void)
{
    const int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) {
        return -1;
    }

    const int opt = 1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in bind_addr = {0};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(MLED_MULTICAST_PORT);
    if (bind(fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) != 0) {
        close(fd);
        return -1;
    }

    struct ip_mreq mreq = {0};
    mreq.imr_multiaddr.s_addr = inet_addr(MLED_MULTICAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != 0) {
        close(fd);
        return -1;
    }

    const int ttl = MLED_MULTICAST_TTL;
    (void)setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    return fd;
}

static void node_task_main(void *arg)
{
    (void)arg;

    s_ctx.fd = open_socket();
    if (s_ctx.fd < 0) {
        ESP_LOGE(TAG, "open_socket failed: errno=%d", errno);
        s_ctx.task = NULL;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "node started: id=%" PRIu32 " name=%s group=%s:%d", s_ctx.node_id, s_ctx.name, MLED_MULTICAST_GROUP, MLED_MULTICAST_PORT);

    uint8_t buf[2048];
    while (!s_ctx.stop) {
        process_due_fires();
        const int timeout_ms = next_due_timeout_ms();

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s_ctx.fd, &rfds);

        struct timeval tv = {
            .tv_sec = timeout_ms / 1000,
            .tv_usec = (timeout_ms % 1000) * 1000,
        };

        const int r = select(s_ctx.fd + 1, &rfds, NULL, NULL, &tv);
        if (r <= 0) {
            continue;
        }

        struct sockaddr_in src = {0};
        socklen_t slen = sizeof(src);
        const int nread = recvfrom(s_ctx.fd, buf, sizeof(buf), 0, (struct sockaddr *)&src, &slen);
        if (nread <= 0) {
            continue;
        }

        mled_header_t hdr = {0};
        if (!mled_header_unpack(&hdr, buf, (size_t)nread)) {
            continue;
        }
        if (!mled_header_validate(&hdr)) {
            continue;
        }

        const uint8_t *payload = buf + MLED_HEADER_SIZE;
        const uint32_t payload_len = hdr.payload_len;
        if (MLED_HEADER_SIZE + payload_len > (uint32_t)nread) {
            continue;
        }

        if (hdr.type == MLED_MSG_BEACON) {
            handle_beacon(&hdr, &src);
            continue;
        }
        if (hdr.type == MLED_MSG_PING) {
            handle_ping(&hdr, &src);
            continue;
        }

        if (hdr.epoch_id != s_ctx.controller_epoch || s_ctx.controller_epoch == 0) {
            continue;
        }

        if (hdr.type == MLED_MSG_TIME_RESP) {
            handle_time_resp(payload, payload_len);
        } else if (hdr.type == MLED_MSG_CUE_PREPARE) {
            handle_cue_prepare(&hdr, payload, payload_len, &src);
        } else if (hdr.type == MLED_MSG_CUE_FIRE) {
            handle_cue_fire(&hdr, payload, payload_len);
        } else if (hdr.type == MLED_MSG_CUE_CANCEL) {
            handle_cue_cancel(payload, payload_len);
        } else {
            // ignore
        }

        process_due_fires();
    }

    close(s_ctx.fd);
    s_ctx.fd = -1;
    s_ctx.task = NULL;
    vTaskDelete(NULL);
}

void mled_node_start(void)
{
    if (s_ctx.started) {
        return;
    }

    s_ctx = (node_ctx_t){0};
    s_ctx.started = true;
    s_ctx.stop = false;
    s_ctx.fd = -1;
    s_ctx.node_id = compute_node_id();
    s_ctx.controller_epoch = 0;
    s_ctx.controller_addr_valid = false;
    s_ctx.show_time_offset_ms = 0;
    s_ctx.time_synced = false;
    s_ctx.last_time_sync_method = NULL;
    s_ctx.last_time_req_rtt_ms = 0;
    s_ctx.last_time_req_local_ms = 0;
    s_ctx.current_pattern = MLED_PATTERN_OFF;
    s_ctx.brightness_pct = 100;
    s_ctx.frame_ms = 20;
    s_ctx.active_cue_id = 0;
    s_ctx.fires_len = 0;
    s_ctx.time_req_counter = 0;

    memset(s_ctx.name, 0, sizeof(s_ctx.name));
    strlcpy(s_ctx.name, CONFIG_MLEDNODE_NODE_NAME, sizeof(s_ctx.name));
    cues_clear();

    BaseType_t ok = xTaskCreate(&node_task_main, "mled_node", 4096, NULL, 5, &s_ctx.task);
    if (ok != pdPASS) {
        s_ctx.started = false;
        ESP_LOGE(TAG, "xTaskCreate failed");
    }
}

void mled_node_stop(void)
{
    s_ctx.stop = true;
}
