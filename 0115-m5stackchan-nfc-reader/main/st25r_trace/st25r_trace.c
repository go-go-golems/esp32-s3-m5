/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * Observer-safe ST25R3916 I2C transaction trace ring -- implementation.
 *
 * Host-testable: no ESP-IDF, no logging, no clock, no I/O. All output goes
 * through the caller-supplied st25r_trace_emit_fn sink. The caller supplies
 * started_us and elapsed_us so the module never touches a clock.
 */
#include "st25r_trace.h"
#include <string.h>
#include <stdio.h>

/* ===================================================================== */
/* Internal helpers                                                      */
/* ===================================================================== */

static bool is_transport_failure(int32_t api_result, uint8_t flags)
{
    if (api_result == ST25R_ESP_OK) return false;
    /* Probe/scan report ESP_ERR_NOT_FOUND for empty addresses by design. */
    if (flags & ST25R_TRACE_FLAG_DIAGNOSTIC_TXN) return false;
    return true;
}

/* Per design S9: do NOT infer NACK from the public esp_err_t alone.
 * DONE for OK; otherwise UNKNOWN until DEBUG/waveform annotates it. */
static st25r_driver_hint_t classify_hint(int32_t api_result)
{
    if (api_result == ST25R_ESP_OK) return ST25R_DRIVER_HINT_DONE;
    return ST25R_DRIVER_HINT_UNKNOWN;
}

static st25r_trace_class_t classify_class(int32_t api_result, st25r_driver_hint_t hint)
{
    if (api_result == ST25R_ESP_OK) return ST25R_CLASS_OK;
    /* Raw result is ground truth; class stays not-done-unknown until
     * st25r_trace_annotate_first_error() upgrades it from evidence. */
    (void)hint;
    return ST25R_CLASS_HOST_NOT_DONE_UNKNOWN;
}

static void fill_event(st25r_trace_event_t *e, const st25r_trace_store_t *store,
                       st25r_trace_op_t op, uint8_t logical_key, uint8_t wire_key,
                       st25r_trace_kind_t kind, uint16_t wlen, uint16_t rlen,
                       int64_t started_us, uint32_t elapsed_us, uint32_t gap_us,
                       int32_t api_result, uint8_t flags)
{
    memset(e, 0, sizeof(*e));
    e->sequence     = store->next_sequence;
    e->timestamp_us = (uint32_t)started_us;
    e->elapsed_us   = elapsed_us;
    e->gap_us       = gap_us;
    e->write_len    = wlen;
    e->read_len     = rlen;
    e->api_result   = api_result;
    e->backend      = store->backend;
    e->phase        = store->phase;
    e->attempt      = store->attempt;
    e->kind         = (uint8_t)kind;
    e->op           = (uint8_t)op;
    e->logical_key  = logical_key;
    e->wire_key     = wire_key;
    e->driver_hint  = (uint8_t)classify_hint(api_result);
    e->error_class  = (uint8_t)classify_class(api_result, (st25r_driver_hint_t)e->driver_hint);
    e->flags        = flags;
}

/* Push an event into the ring. Returns true if the push overwrote an older
 * event (ring was full), so the caller can set the RING_OVERWROTE flag. */
static bool ring_push(st25r_trace_store_t *store, const st25r_trace_event_t *e)
{
    bool overwrote = false;
    if (store->count >= ST25R_TRACE_CAPACITY) {
        overwrote = true;
        store->overwritten++;
    } else {
        store->count++;
    }
    store->events[store->head] = *e;
    store->head = (store->head + 1) % ST25R_TRACE_CAPACITY;
    return overwrote;
}

/* Read the i-th most recent event (0 = newest, 1 = one before newest, ...).
 * Returns NULL if i >= count. */
static const st25r_trace_event_t *ring_recent(const st25r_trace_store_t *store, uint32_t i)
{
    if (i >= store->count) return NULL;
    /* head points at the next write; newest valid is head-1. */
    int32_t idx = (int32_t)store->head - 1 - (int32_t)i;
    if (idx < 0) idx += ST25R_TRACE_CAPACITY;
    return &store->events[(uint32_t)idx];
}

/* Freeze the prefix: the up-to-PREFIX events immediately preceding the error,
 * copied into chronological order (prefix[0] oldest). */
static void freeze_prefix(st25r_trace_store_t *store)
{
    uint32_t n = store->count;
    if (n > ST25R_TRACE_FIRST_ERROR_PREFIX) n = ST25R_TRACE_FIRST_ERROR_PREFIX;
    store->prefix_count = (uint8_t)n;
    /* Chronological order: prefix[0] = oldest of the captured window,
     * prefix[n-1] = newest (the event immediately before the error).
     * ring_recent(0) is the newest event; ring_recent(n-1) is the oldest. */
    for (uint32_t i = 0; i < n; i++) {
        const st25r_trace_event_t *e = ring_recent(store, n - 1 - i);
        store->prefix[i] = (e) ? *e : store->prefix[i];
    }
}

/* ===================================================================== */
/* API                                                                    */
/* ===================================================================== */

void st25r_trace_init(st25r_trace_store_t *store)
{
    memset(store, 0, sizeof(*store));
    store->mode          = ST25R_TRACE_MODE_OFF;
    store->backend       = ST25R_TRACE_BACKEND_IDF_HIGH;
    store->next_sequence = 1; /* 1-based: first recorded event is seq 1 */
}

void st25r_trace_clear(st25r_trace_store_t *store)
{
    uint8_t backend = store->backend;
    uint8_t mode    = store->mode;
    memset(store, 0, sizeof(*store));
    store->backend       = backend;
    store->mode          = mode;
    store->next_sequence = 1; /* restart sequence epoch */
}

void st25r_trace_set_context(st25r_trace_store_t *store,
                            st25r_trace_backend_t backend,
                            st25r_trace_phase_t phase,
                            uint8_t attempt)
{
    store->backend = (uint8_t)backend;
    store->phase   = (uint8_t)phase;
    store->attempt = attempt;
}

void st25r_trace_set_mode(st25r_trace_store_t *store, st25r_trace_mode_t mode)
{
    store->mode = (uint8_t)mode;
}

st25r_trace_mode_t st25r_trace_get_mode(const st25r_trace_store_t *store)
{
    return (st25r_trace_mode_t)store->mode;
}

void st25r_trace_record(st25r_trace_store_t *store,
                        st25r_trace_op_t op,
                        uint8_t logical_key, uint8_t wire_key,
                        st25r_trace_kind_t kind,
                        uint16_t write_len, uint16_t read_len,
                        int64_t started_us, uint32_t elapsed_us,
                        int32_t api_result, uint8_t flags)
{
    if (store->mode == ST25R_TRACE_MODE_OFF) return;

    const bool failure = is_transport_failure(api_result, flags);
    if (store->mode == ST25R_TRACE_MODE_FAILURE && !failure) return;

    /* gap since prior transaction end (idle gap; design S5.3) */
    uint32_t gap_us = 0;
    if (store->have_last_end) {
        int64_t g = started_us - store->last_end_us;
        if (g < 0) g = 0;
        gap_us = (uint32_t)g;
    }

    st25r_trace_event_t e;
    fill_event(&e, store, op, logical_key, wire_key, kind, write_len, read_len,
               started_us, elapsed_us, gap_us, api_result, flags);

    /* Determine overwrite before pushing so the flag lands on the stored copy. */
    const bool will_overwrite = (store->count >= ST25R_TRACE_CAPACITY);
    if (will_overwrite) e.flags |= ST25R_TRACE_FLAG_RING_OVERWROTE;

    /* First-error freeze happens BEFORE pushing the failing event so the
     * prefix captures exactly the preceding transactions. */
    const bool was_first_error_set = store->first_error_set;
    if (failure && !store->first_error_set) {
        freeze_prefix(store);
        e.flags |= ST25R_TRACE_FLAG_FIRST_ERROR;
        store->first_error = e;
        store->first_error_set = true;
        store->collecting_suffix = true;
        store->suffix_count = 0;
    }

    (void)ring_push(store, &e);
    store->next_sequence++;
    store->total_recorded++;
    if (failure) store->total_failed++;

    /* Suffix = the events AFTER the first error, not the error itself, so we
     * only collect once first_error_set was already true on entry. */
    if (store->collecting_suffix && was_first_error_set &&
        store->suffix_count < ST25R_TRACE_FIRST_ERROR_SUFFIX) {
        store->suffix[store->suffix_count++] = e;
        if (store->suffix_count == ST25R_TRACE_FIRST_ERROR_SUFFIX)
            store->collecting_suffix = false;
    }

    /* Update last end for the next gap. */
    store->last_end_us = started_us + (int64_t)elapsed_us;
    store->have_last_end = true;
}

size_t st25r_trace_snapshot(const st25r_trace_store_t *store,
                            st25r_trace_event_t *output, size_t capacity,
                            st25r_trace_snapshot_info_t *info)
{
    if (info) {
        memset(info, 0, sizeof(*info));
        info->total_recorded = store->total_recorded;
        info->total_failed   = store->total_failed;
        info->ring_count     = store->count;
        info->overwritten    = store->overwritten;
        info->first_error_set = store->first_error_set;
        info->first_error_seq = store->first_error_set ? store->first_error.sequence : 0;
    }
    if (!output || capacity == 0) return 0;

    /* Chronological order: oldest valid first. The oldest valid event is at
     * index `head` when the ring is full, else index 0. */
    uint32_t n = store->count;
    if (n > capacity) n = (uint32_t)capacity;
    uint32_t start = (store->count >= ST25R_TRACE_CAPACITY) ? store->head : 0;
    for (uint32_t i = 0; i < n; i++) {
        output[i] = store->events[(start + i) % ST25R_TRACE_CAPACITY];
    }
    return n;
}

bool st25r_trace_first_error(const st25r_trace_store_t *store,
                             st25r_first_error_bundle_t *out)
{
    if (!out) return store->first_error_set;
    memset(out, 0, sizeof(*out));
    out->set = store->first_error_set;
    if (!store->first_error_set) return false;
    out->error = store->first_error;
    out->prefix_count = store->prefix_count;
    for (uint8_t i = 0; i < store->prefix_count; i++) out->prefix[i] = store->prefix[i];
    out->suffix_count = store->suffix_count;
    for (uint8_t i = 0; i < store->suffix_count; i++) out->suffix[i] = store->suffix[i];
    return true;
}

void st25r_trace_annotate_first_error(st25r_trace_store_t *store,
                                      st25r_driver_hint_t hint)
{
    if (!store->first_error_set) return;
    store->first_error.driver_hint = (uint8_t)hint;
    switch (hint) {
    case ST25R_DRIVER_HINT_NACK:    store->first_error.error_class = (uint8_t)ST25R_CLASS_HOST_NACK_EVENT; break;
    case ST25R_DRIVER_HINT_TIMEOUT: store->first_error.error_class = (uint8_t)ST25R_CLASS_HOST_TIMEOUT; break;
    default: break;
    }
}

/* ===================================================================== */
/* Name helpers                                                           */
/* ===================================================================== */

#define CASE_NAME(x, s) case x: return s
const char *st25r_trace_op_name(st25r_trace_op_t op)
{
    switch (op) {
    CASE_NAME(ST25R_OP_NONE, "none");
    CASE_NAME(ST25R_OP_READ_A, "READ_A");
    CASE_NAME(ST25R_OP_WRITE_A, "WRITE_A");
    CASE_NAME(ST25R_OP_READ_B, "READ_B");
    CASE_NAME(ST25R_OP_WRITE_B, "WRITE_B");
    CASE_NAME(ST25R_OP_DIRECT_CMD, "DIRECT");
    CASE_NAME(ST25R_OP_DIRECT_CMD_DATA, "DIRECT_D");
    CASE_NAME(ST25R_OP_FIFO_WRITE, "FIFO_W");
    CASE_NAME(ST25R_OP_FIFO_READ, "FIFO_R");
    CASE_NAME(ST25R_OP_IRQ_READ, "IRQ_R");
    CASE_NAME(ST25R_OP_PROBE, "PROBE");
    CASE_NAME(ST25R_OP_BUS_SCAN, "SCAN");
    default: return "?op";
    }
}
const char *st25r_trace_kind_name(st25r_trace_kind_t kind)
{
    switch (kind) {
    CASE_NAME(ST25R_TRACE_KIND_NONE, "none");
    CASE_NAME(ST25R_TRACE_KIND_WRITE, "W");
    CASE_NAME(ST25R_TRACE_KIND_READ, "R");
    CASE_NAME(ST25R_TRACE_KIND_WRITE_READ, "WR");
    default: return "?kind";
    }
}
const char *st25r_trace_hint_name(st25r_driver_hint_t h)
{
    switch (h) {
    CASE_NAME(ST25R_DRIVER_HINT_UNKNOWN, "UNKNOWN");
    CASE_NAME(ST25R_DRIVER_HINT_DONE, "DONE");
    CASE_NAME(ST25R_DRIVER_HINT_NACK, "NACK");
    CASE_NAME(ST25R_DRIVER_HINT_TIMEOUT, "TIMEOUT");
    CASE_NAME(ST25R_DRIVER_HINT_BUS_BUSY, "BUSY");
    default: return "?hint";
    }
}
const char *st25r_trace_class_name(st25r_trace_class_t c)
{
    switch (c) {
    CASE_NAME(ST25R_CLASS_OK, "OK");
    CASE_NAME(ST25R_CLASS_HOST_NACK_EVENT, "HOST_NACK");
    CASE_NAME(ST25R_CLASS_HOST_TIMEOUT, "HOST_TIMEOUT");
    CASE_NAME(ST25R_CLASS_HOST_NOT_DONE_UNKNOWN, "NOT_DONE_UNKNOWN");
    CASE_NAME(ST25R_CLASS_READBACK_MISMATCH, "READBACK_MISMATCH");
    CASE_NAME(ST25R_CLASS_NO_RF_RESPONSE, "NO_RF");
    CASE_NAME(ST25R_CLASS_PROTOCOL_FAILURE, "PROTOCOL");
    default: return "?class";
    }
}
const char *st25r_trace_phase_name(st25r_trace_phase_t p)
{
    switch (p) {
    CASE_NAME(ST25R_PHASE_IDLE, "idle");
    CASE_NAME(ST25R_PHASE_INIT_IDENTITY, "init-id");
    CASE_NAME(ST25R_PHASE_INIT_RESET, "init-reset");
    CASE_NAME(ST25R_PHASE_INIT_CONFIG, "init-config");
    CASE_NAME(ST25R_PHASE_INIT_OSCILLATOR, "init-osc");
    CASE_NAME(ST25R_PHASE_INIT_ANALOG, "init-analog");
    CASE_NAME(ST25R_PHASE_FIELD_ON, "field-on");
    CASE_NAME(ST25R_PHASE_REQUEST_SETUP, "req-setup");
    CASE_NAME(ST25R_PHASE_REQUEST_TRANSMIT, "req-tx");
    CASE_NAME(ST25R_PHASE_IRQ_WAIT, "irq-wait");
    CASE_NAME(ST25R_PHASE_FIFO_READ, "fifo-read");
    CASE_NAME(ST25R_PHASE_ANTICOLLISION, "anticoll");
    CASE_NAME(ST25R_PHASE_SELECT, "select");
    CASE_NAME(ST25R_PHASE_IDENTIFY, "identify");
    CASE_NAME(ST25R_PHASE_DIAGNOSTIC, "diag");
    CASE_NAME(ST25R_PHASE_SHUTDOWN, "shutdown");
    default: return "?phase";
    }
}
const char *st25r_trace_backend_name(st25r_trace_backend_t b)
{
    switch (b) {
    CASE_NAME(ST25R_TRACE_BACKEND_NONE, "none");
    CASE_NAME(ST25R_TRACE_BACKEND_IDF_HIGH, "idf-high");
    CASE_NAME(ST25R_TRACE_BACKEND_IDF_DEFINED, "idf-def");
    CASE_NAME(ST25R_TRACE_BACKEND_IDF_LEGACY, "idf-legacy");
    CASE_NAME(ST25R_TRACE_BACKEND_DIRECT_EXP, "direct-exp");
    default: return "?backend";
    }
}
const char *st25r_trace_mode_name(st25r_trace_mode_t m)
{
    switch (m) {
    CASE_NAME(ST25R_TRACE_MODE_OFF, "off");
    CASE_NAME(ST25R_TRACE_MODE_FAILURE, "failure");
    CASE_NAME(ST25R_TRACE_MODE_ALL, "all");
    default: return "?mode";
    }
}
#undef CASE_NAME

const char *st25r_trace_err_name(int32_t api_result)
{
    switch (api_result) {
    case ST25R_ESP_OK:               return "ESP_OK";
    case ST25R_ESP_FAIL:             return "ESP_FAIL";
    case ST25R_ESP_ERR_NO_MEM:        return "ESP_ERR_NO_MEM";
    case ST25R_ESP_ERR_INVALID_ARG:   return "ESP_ERR_INVALID_ARG";
    case ST25R_ESP_ERR_INVALID_STATE: return "ESP_ERR_INVALID_STATE";
    case ST25R_ESP_ERR_INVALID_SIZE:  return "ESP_ERR_INVALID_SIZE";
    case ST25R_ESP_ERR_NOT_FOUND:     return "ESP_ERR_NOT_FOUND";
    case ST25R_ESP_ERR_TIMEOUT:       return "ESP_ERR_TIMEOUT";
    default: return "ESP_ERR_OTHER";
    }
}

/* ===================================================================== */
/* Dump helpers                                                           */
/* ===================================================================== */

static void emit_flags(char *buf, size_t cap, uint8_t flags)
{
    buf[0] = '\0';
    if (!flags) { snprintf(buf, cap, "-"); return; }
    char *p = buf; size_t off = 0;
    const char *sep = "";
#define ADD(s) do { off += (size_t)snprintf(p+off, cap-off, "%s%s", sep, s); sep=","; } while(0)
    if (flags & ST25R_TRACE_FLAG_FIRST_ERROR)        ADD("FIRST_ERROR");
    if (flags & ST25R_TRACE_FLAG_RECOVERY_ATTEMPTED) ADD("RECOVERY");
    if (flags & ST25R_TRACE_FLAG_RECOVERY_SUCCEEDED) ADD("RECOVERY_OK");
    if (flags & ST25R_TRACE_FLAG_READBACK_VERIFIED)  ADD("READBACK_OK");
    if (flags & ST25R_TRACE_FLAG_DIAGNOSTIC_TXN)     ADD("DIAG");
    if (flags & ST25R_TRACE_FLAG_RING_OVERWROTE)     ADD("OVERWROTE");
#undef ADD
    (void)p; (void)off;
}

static void dump_event(const st25r_trace_event_t *e, st25r_trace_emit_fn emit, void *arg, const char *tag)
{
    char fbuf[64];
    emit_flags(fbuf, sizeof(fbuf), e->flags);
    char line[256];
    snprintf(line, sizeof(line),
        "%s seq=%lu t_us=%lu gap_us=%lu elapsed_us=%lu backend=%s phase=%s attempt=%lu "
        "kind=%s op=%s logical=%02lX wire=%02lX wlen=%lu rlen=%lu api=%s hint=%s class=%s flags=%s",
        tag,
        (unsigned long)e->sequence, (unsigned long)e->timestamp_us,
        (unsigned long)e->gap_us, (unsigned long)e->elapsed_us,
        st25r_trace_backend_name((st25r_trace_backend_t)e->backend),
        st25r_trace_phase_name((st25r_trace_phase_t)e->phase), (unsigned long)e->attempt,
        st25r_trace_kind_name((st25r_trace_kind_t)e->kind),
        st25r_trace_op_name((st25r_trace_op_t)e->op),
        (unsigned long)e->logical_key, (unsigned long)e->wire_key,
        (unsigned long)e->write_len, (unsigned long)e->read_len,
        st25r_trace_err_name(e->api_result),
        st25r_trace_hint_name((st25r_driver_hint_t)e->driver_hint),
        st25r_trace_class_name((st25r_trace_class_t)e->error_class), fbuf);
    emit(line, arg);
}

void st25r_trace_status(const st25r_trace_store_t *store,
                        st25r_trace_emit_fn emit, void *arg)
{
    char line[192];
    snprintf(line, sizeof(line),
        "TRACE_STATUS mode=%s backend=%s recorded=%lu failed=%lu ring=%lu/%lu overwritten=%lu first_error=%s%s",
        st25r_trace_mode_name((st25r_trace_mode_t)store->mode),
        st25r_trace_backend_name((st25r_trace_backend_t)store->backend),
        (unsigned long)store->total_recorded, (unsigned long)store->total_failed,
        (unsigned long)store->count, (unsigned long)ST25R_TRACE_CAPACITY,
        (unsigned long)store->overwritten,
        store->first_error_set ? "set " : "none",
        store->first_error_set ? st25r_trace_err_name(store->first_error.api_result) : "");
    emit(line, arg);
}

void st25r_trace_dump(const st25r_trace_store_t *store,
                      st25r_trace_emit_fn emit, void *arg)
{
    char line[192];
    uint32_t failed = store->total_failed;
    uint32_t fe = store->first_error_set ? store->first_error.sequence : 0;
    const char *result = (failed == 0) ? "ok" : "transport-error";
    snprintf(line, sizeof(line),
        "TRACE_BEGIN schema=1 backend=%s mode=%s events=%lu overwritten=%lu failed=%lu first_error_seq=%lu",
        st25r_trace_backend_name((st25r_trace_backend_t)store->backend),
        st25r_trace_mode_name((st25r_trace_mode_t)store->mode),
        (unsigned long)store->count, (unsigned long)store->overwritten,
        (unsigned long)failed, (unsigned long)fe);
    emit(line, arg);

    uint32_t start = (store->count >= ST25R_TRACE_CAPACITY) ? store->head : 0;
    for (uint32_t i = 0; i < store->count; i++) {
        dump_event(&store->events[(start + i) % ST25R_TRACE_CAPACITY], emit, arg, "I2C_TRACE");
    }

    snprintf(line, sizeof(line), "TRACE_END result=%s txns=%lu failed=%lu first_error_seq=%lu",
             result, (unsigned long)store->count, (unsigned long)failed, (unsigned long)fe);
    emit(line, arg);
}

void st25r_trace_dump_last(const st25r_trace_store_t *store, uint32_t last,
                           st25r_trace_emit_fn emit, void *arg)
{
    uint32_t n = store->count;
    if (last < n) n = last;
    uint32_t failed = store->total_failed;
    uint32_t fe = store->first_error_set ? store->first_error.sequence : 0;
    char line[192];
    snprintf(line, sizeof(line),
        "TRACE_BEGIN schema=1 tail=%lu events=%lu overwritten=%lu failed=%lu first_error_seq=%lu",
        (unsigned long)n, (unsigned long)store->count, (unsigned long)store->overwritten,
        (unsigned long)failed, (unsigned long)fe);
    emit(line, arg);
    /* chronological order: oldest of the window first.
     * ring_recent(k) is the k-th most recent; iterate k from (n-1) down to 0. */
    for (uint32_t k = n; k > 0; k--) {
        const st25r_trace_event_t *e = ring_recent(store, k - 1);
        if (e) dump_event(e, emit, arg, "I2C_TRACE");
    }
    const char *result = (failed == 0) ? "ok" : "transport-error";
    snprintf(line, sizeof(line), "TRACE_END result=%s tail=%lu failed=%lu first_error_seq=%lu",
             result, (unsigned long)n, (unsigned long)failed, (unsigned long)fe);
    emit(line, arg);
}

void st25r_trace_dump_first_error(const st25r_trace_store_t *store,
                                  st25r_trace_emit_fn emit, void *arg)
{
    if (!store->first_error_set) { emit("FIRST_ERROR: none", arg); return; }
    char line[128];
    emit("FIRST_ERROR_BEGIN", arg);
    snprintf(line, sizeof(line), "FIRST_ERROR prefix_count=%lu suffix_count=%lu",
             (unsigned long)store->prefix_count, (unsigned long)store->suffix_count);
    emit(line, arg);
    for (uint8_t i = 0; i < store->prefix_count; i++)
        dump_event(&store->prefix[i], emit, arg, "PREFIX");
    dump_event(&store->first_error, emit, arg, "ERROR");
    for (uint8_t i = 0; i < store->suffix_count; i++)
        dump_event(&store->suffix[i], emit, arg, "SUFFIX");
    emit("FIRST_ERROR_END", arg);
}
