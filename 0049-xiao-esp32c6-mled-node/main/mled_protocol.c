#include "mled_protocol.h"

#include <string.h>

static inline uint16_t rd_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t rd_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void wr_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
}

static inline void wr_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

bool mled_header_validate(const mled_header_t *h)
{
    return h->magic[0] == (uint8_t)MLED_MAGIC0 && h->magic[1] == (uint8_t)MLED_MAGIC1 && h->magic[2] == (uint8_t)MLED_MAGIC2
        && h->magic[3] == (uint8_t)MLED_MAGIC3 && h->version == MLED_VERSION && h->hdr_len == MLED_HEADER_SIZE;
}

bool mled_header_unpack(mled_header_t *out, const uint8_t *buf, size_t len)
{
    if (!out || !buf || len < MLED_HEADER_SIZE) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    memcpy(out->magic, buf, 4);
    out->version = buf[4];
    out->type = buf[5];
    out->flags = buf[6];
    out->hdr_len = buf[7];
    out->epoch_id = rd_le32(&buf[8]);
    out->msg_id = rd_le32(&buf[12]);
    out->sender_id = rd_le32(&buf[16]);
    out->target = rd_le32(&buf[20]);
    out->execute_at_ms = rd_le32(&buf[24]);
    out->payload_len = rd_le16(&buf[28]);
    out->reserved = rd_le16(&buf[30]);
    return true;
}

void mled_header_pack(uint8_t out_buf[MLED_HEADER_SIZE], const mled_header_t *h)
{
    out_buf[0] = h->magic[0];
    out_buf[1] = h->magic[1];
    out_buf[2] = h->magic[2];
    out_buf[3] = h->magic[3];
    out_buf[4] = h->version;
    out_buf[5] = h->type;
    out_buf[6] = h->flags;
    out_buf[7] = h->hdr_len;
    wr_le32(&out_buf[8], h->epoch_id);
    wr_le32(&out_buf[12], h->msg_id);
    wr_le32(&out_buf[16], h->sender_id);
    wr_le32(&out_buf[20], h->target);
    wr_le32(&out_buf[24], h->execute_at_ms);
    wr_le16(&out_buf[28], h->payload_len);
    wr_le16(&out_buf[30], h->reserved);
}

bool mled_pattern_unpack(mled_pattern_config_t *out, const uint8_t *buf, size_t len)
{
    if (!out || !buf || len < 20) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->pattern_type = buf[0];
    out->brightness_pct = buf[1];
    out->flags = buf[2];
    out->reserved0 = buf[3];
    out->seed = rd_le32(&buf[4]);
    memcpy(out->data, &buf[8], 12);
    return true;
}

void mled_pattern_pack(uint8_t out_buf[20], const mled_pattern_config_t *p)
{
    out_buf[0] = p->pattern_type;
    out_buf[1] = p->brightness_pct;
    out_buf[2] = p->flags;
    out_buf[3] = p->reserved0;
    wr_le32(&out_buf[4], p->seed);
    memcpy(&out_buf[8], p->data, 12);
}

bool mled_cue_prepare_unpack(mled_cue_prepare_t *out, const uint8_t *buf, size_t len)
{
    if (!out || !buf || len < 28) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->cue_id = rd_le32(&buf[0]);
    out->fade_in_ms = rd_le16(&buf[4]);
    out->fade_out_ms = rd_le16(&buf[6]);
    return mled_pattern_unpack(&out->pattern, &buf[8], 20);
}

void mled_cue_prepare_pack(uint8_t out_buf[28], const mled_cue_prepare_t *p)
{
    wr_le32(&out_buf[0], p->cue_id);
    wr_le16(&out_buf[4], p->fade_in_ms);
    wr_le16(&out_buf[6], p->fade_out_ms);
    mled_pattern_pack(&out_buf[8], &p->pattern);
}

bool mled_cue_fire_unpack(mled_cue_fire_t *out, const uint8_t *buf, size_t len)
{
    if (!out || !buf || len < 4) {
        return false;
    }
    out->cue_id = rd_le32(&buf[0]);
    return true;
}

void mled_cue_fire_pack(uint8_t out_buf[4], const mled_cue_fire_t *p)
{
    wr_le32(&out_buf[0], p->cue_id);
}

bool mled_ack_unpack(mled_ack_t *out, const uint8_t *buf, size_t len)
{
    if (!out || !buf || len < 8) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->ack_for_msg_id = rd_le32(&buf[0]);
    out->code = rd_le16(&buf[4]);
    out->reserved = rd_le16(&buf[6]);
    return true;
}

void mled_ack_pack(uint8_t out_buf[8], const mled_ack_t *p)
{
    wr_le32(&out_buf[0], p->ack_for_msg_id);
    wr_le16(&out_buf[4], p->code);
    wr_le16(&out_buf[6], p->reserved);
}

bool mled_time_resp_unpack(mled_time_resp_t *out, const uint8_t *buf, size_t len)
{
    if (!out || !buf || len < 12) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->req_msg_id = rd_le32(&buf[0]);
    out->master_rx_show_ms = rd_le32(&buf[4]);
    out->master_tx_show_ms = rd_le32(&buf[8]);
    return true;
}

void mled_time_resp_pack(uint8_t out_buf[12], const mled_time_resp_t *p)
{
    wr_le32(&out_buf[0], p->req_msg_id);
    wr_le32(&out_buf[4], p->master_rx_show_ms);
    wr_le32(&out_buf[8], p->master_tx_show_ms);
}

bool mled_pong_unpack(mled_pong_t *out, const uint8_t *buf, size_t len)
{
    if (!out || !buf || len < 43) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->uptime_ms = rd_le32(&buf[0]);
    out->rssi_dbm = (int8_t)buf[4];
    out->state_flags = buf[5];
    out->brightness_pct = buf[6];
    out->pattern_type = buf[7];
    out->frame_ms = rd_le16(&buf[8]);
    out->active_cue_id = rd_le32(&buf[10]);
    out->controller_epoch = rd_le32(&buf[14]);
    out->show_ms_now = rd_le32(&buf[18]);
    memcpy(out->name, &buf[22], 16);
    return true;
}

void mled_pong_pack(uint8_t out_buf[43], const mled_pong_t *p)
{
    wr_le32(&out_buf[0], p->uptime_ms);
    out_buf[4] = (uint8_t)p->rssi_dbm;
    out_buf[5] = p->state_flags;
    out_buf[6] = p->brightness_pct;
    out_buf[7] = p->pattern_type;
    wr_le16(&out_buf[8], p->frame_ms);
    wr_le32(&out_buf[10], p->active_cue_id);
    wr_le32(&out_buf[14], p->controller_epoch);
    wr_le32(&out_buf[18], p->show_ms_now);
    memcpy(&out_buf[22], p->name, 16);
    memset(&out_buf[38], 0, 5);
}

