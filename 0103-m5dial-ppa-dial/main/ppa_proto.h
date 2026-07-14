// PPA UDP wire protocol (reconstructed; see ticket M5DIAL-PPA-CONTROL design doc §4).
// Pure encode/decode — no sockets, host-testable.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPA_PORT 5001

#define PPA_TYPE_PING 0x00
#define PPA_TYPE_RECALL 0x04

// Low byte of reply status.
#define PPA_KIND_OK 0x01
#define PPA_KIND_ERROR 0x09
#define PPA_KIND_WAIT 0x41
// Payload byte 12 of a PPA_KIND_ERROR reply that means "busy, retry later".
#define PPA_ERR_SUB_BUSY 0x03

#define PPA_PING_LEN 16
#define PPA_RECALL_LEN 18
#define PPA_HEADER_LEN 12

typedef struct {
    uint8_t type;
    uint16_t status;
    uint32_t uid;
    uint16_t seq;
    uint8_t comp;
    uint8_t res;
} ppa_header_t;

size_t ppa_encode_ping(uint8_t buf[PPA_PING_LEN], uint16_t seq);
size_t ppa_encode_recall(uint8_t buf[PPA_RECALL_LEN], uint16_t seq,
                         uint8_t preset_id, uint8_t preset_sub);
bool ppa_decode_header(const uint8_t *buf, size_t len, ppa_header_t *out);

// Classify a reply for a recall we are waiting on.
typedef enum {
    PPA_REPLY_OK,
    PPA_REPLY_BUSY,
    PPA_REPLY_ERROR,
    PPA_REPLY_WAIT,
    PPA_REPLY_OTHER,
} ppa_reply_kind_t;

ppa_reply_kind_t ppa_classify_reply(const ppa_header_t *hdr, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif
