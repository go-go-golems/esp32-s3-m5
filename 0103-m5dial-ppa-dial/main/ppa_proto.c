#include "ppa_proto.h"

#include <string.h>

// Header layout (all little-endian):
//  0: type, 1: version(0x01), 2-3: status, 4-7: uid, 8-9: seq, 10: comp, 11: res
static void build_header(uint8_t *b, uint8_t type, uint16_t status, uint16_t seq,
                         uint8_t comp, uint8_t res) {
    b[0] = type;
    b[1] = 0x01;
    b[2] = (uint8_t)(status & 0xFF);
    b[3] = (uint8_t)(status >> 8);
    b[4] = b[5] = b[6] = b[7] = 0; // uid: 0 in requests
    b[8] = (uint8_t)(seq & 0xFF);
    b[9] = (uint8_t)(seq >> 8);
    b[10] = comp;
    b[11] = res;
}

size_t ppa_encode_ping(uint8_t buf[PPA_PING_LEN], uint16_t seq) {
    memset(buf, 0, PPA_PING_LEN);
    build_header(buf, PPA_TYPE_PING, 0x0006, seq, 0xFE, 0x00);
    return PPA_PING_LEN;
}

size_t ppa_encode_recall(uint8_t buf[PPA_RECALL_LEN], uint16_t seq,
                         uint8_t preset_id, uint8_t preset_sub) {
    memset(buf, 0, PPA_RECALL_LEN);
    build_header(buf, PPA_TYPE_RECALL, 0x0102, seq, 0xFF, 0x01);
    buf[12] = 0x00;
    buf[13] = preset_id;
    buf[14] = preset_sub;
    return PPA_RECALL_LEN;
}

bool ppa_decode_header(const uint8_t *buf, size_t len, ppa_header_t *out) {
    if (len < PPA_HEADER_LEN) return false;
    out->type = buf[0];
    out->status = (uint16_t)(buf[2] | (buf[3] << 8));
    out->uid = (uint32_t)buf[4] | ((uint32_t)buf[5] << 8) | ((uint32_t)buf[6] << 16) |
               ((uint32_t)buf[7] << 24);
    out->seq = (uint16_t)(buf[8] | (buf[9] << 8));
    out->comp = buf[10];
    out->res = buf[11];
    return true;
}

ppa_reply_kind_t ppa_classify_reply(const ppa_header_t *hdr, const uint8_t *buf, size_t len) {
    const uint8_t kind = (uint8_t)(hdr->status & 0xFF);
    switch (kind) {
    case PPA_KIND_OK:
        return PPA_REPLY_OK;
    case PPA_KIND_ERROR:
        if (len > PPA_HEADER_LEN && buf[12] == PPA_ERR_SUB_BUSY) return PPA_REPLY_BUSY;
        return PPA_REPLY_ERROR;
    case PPA_KIND_WAIT:
        return PPA_REPLY_WAIT;
    default:
        return PPA_REPLY_OTHER;
    }
}
