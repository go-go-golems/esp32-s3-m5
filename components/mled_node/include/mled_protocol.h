#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MLED_MAGIC0 'M'
#define MLED_MAGIC1 'L'
#define MLED_MAGIC2 'E'
#define MLED_MAGIC3 'D'

#define MLED_VERSION 1
#define MLED_HEADER_SIZE 32

#define MLED_MULTICAST_GROUP "239.255.32.6"
#define MLED_MULTICAST_PORT 4626
#define MLED_MULTICAST_TTL 1

typedef enum {
    MLED_MSG_BEACON = 0x01,
    MLED_MSG_HELLO = 0x02,
    MLED_MSG_TIME_REQ = 0x03,
    MLED_MSG_TIME_RESP = 0x04,
    MLED_MSG_CUE_PREPARE = 0x10,
    MLED_MSG_CUE_FIRE = 0x11,
    MLED_MSG_CUE_CANCEL = 0x12,
    MLED_MSG_PING = 0x20,
    MLED_MSG_PONG = 0x21,
    MLED_MSG_ACK = 0x22,
} mled_msg_type_t;

typedef enum {
    MLED_TARGET_ALL = 0,
    MLED_TARGET_NODE = 1,
    MLED_TARGET_GROUP = 2,
} mled_target_mode_t;

typedef enum {
    MLED_PATTERN_OFF = 0,
    MLED_PATTERN_RAINBOW = 1,
    MLED_PATTERN_CHASE = 2,
    MLED_PATTERN_BREATHING = 3,
    MLED_PATTERN_SPARKLE = 4,
} mled_pattern_type_t;

#define MLED_FLAG_ACK_REQ 0x04

typedef struct {
    uint8_t magic[4];
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint8_t hdr_len;
    uint32_t epoch_id;
    uint32_t msg_id;
    uint32_t sender_id;
    uint32_t target;
    uint32_t execute_at_ms;
    uint16_t payload_len;
    uint16_t reserved;
} mled_header_t;

typedef struct {
    uint8_t pattern_type;
    uint8_t brightness_pct;
    uint8_t flags;
    uint8_t reserved0;
    uint32_t seed;
    uint8_t data[12];
} mled_pattern_config_t;

typedef struct {
    uint32_t cue_id;
    uint16_t fade_in_ms;
    uint16_t fade_out_ms;
    mled_pattern_config_t pattern;
} mled_cue_prepare_t;

typedef struct {
    uint32_t cue_id;
} mled_cue_fire_t;

typedef struct {
    uint32_t uptime_ms;
    int8_t rssi_dbm;
    uint8_t state_flags;
    uint8_t brightness_pct;
    uint8_t pattern_type;
    uint16_t frame_ms;
    uint32_t active_cue_id;
    uint32_t controller_epoch;
    uint32_t show_ms_now;
    char name[16];
} mled_pong_t;

typedef struct {
    uint32_t ack_for_msg_id;
    uint16_t code;
    uint16_t reserved;
} mled_ack_t;

typedef struct {
    uint32_t req_msg_id;
    uint32_t master_rx_show_ms;
    uint32_t master_tx_show_ms;
} mled_time_resp_t;

static inline mled_target_mode_t mled_header_target_mode(const mled_header_t *h)
{
    return (mled_target_mode_t)(h->flags & 0x03);
}

static inline bool mled_header_ack_req(const mled_header_t *h)
{
    return (h->flags & MLED_FLAG_ACK_REQ) != 0;
}

bool mled_header_unpack(mled_header_t *out, const uint8_t *buf, size_t len);
void mled_header_pack(uint8_t out_buf[MLED_HEADER_SIZE], const mled_header_t *h);
bool mled_header_validate(const mled_header_t *h);

bool mled_pattern_unpack(mled_pattern_config_t *out, const uint8_t *buf, size_t len);
void mled_pattern_pack(uint8_t out_buf[20], const mled_pattern_config_t *p);

bool mled_cue_prepare_unpack(mled_cue_prepare_t *out, const uint8_t *buf, size_t len);
void mled_cue_prepare_pack(uint8_t out_buf[28], const mled_cue_prepare_t *p);

bool mled_cue_fire_unpack(mled_cue_fire_t *out, const uint8_t *buf, size_t len);
void mled_cue_fire_pack(uint8_t out_buf[4], const mled_cue_fire_t *p);

bool mled_pong_unpack(mled_pong_t *out, const uint8_t *buf, size_t len);
void mled_pong_pack(uint8_t out_buf[43], const mled_pong_t *p);

bool mled_ack_unpack(mled_ack_t *out, const uint8_t *buf, size_t len);
void mled_ack_pack(uint8_t out_buf[8], const mled_ack_t *p);

bool mled_time_resp_unpack(mled_time_resp_t *out, const uint8_t *buf, size_t len);
void mled_time_resp_pack(uint8_t out_buf[12], const mled_time_resp_t *p);

#ifdef __cplusplus
}
#endif

