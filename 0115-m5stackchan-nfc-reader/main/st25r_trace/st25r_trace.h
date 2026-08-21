/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * Observer-safe ST25R3916 I2C transaction trace ring.
 *
 * Design: design-doc/04-esp-idf-instrumentation-for-arduino-comparable-st25r3916-transport-traces.md
 *
 * This module is deliberately dependency-free (only stdint/stdbool/stddef/string)
 * so it can be unit-tested on the host with a plain C compiler. The caller supplies
 * the monotonic start timestamp (esp_timer_get_time() on target) and the raw
 * esp_err_t result (passed as int32_t). No serial output, no heap, no I2C, and no
 * logging happen inside the record path -- output is deferred to st25r_trace_dump(),
 * which the NFC worker calls only after the transaction phase ends.
 *
 * Key invariants:
 *   - Recording adds only a timestamp read, a struct fill, and a ring index bump.
 *   - The first transport failure is frozen with a 16-event prefix and 16-event
 *     suffix; later diagnostics cannot overwrite that bundle until explicit clear.
 *   - driver_hint is NOT inferred from the public esp_err_t alone (per design S9):
 *     INVALID_STATE/FAIL stay UNKNOWN until driver DEBUG or waveform evidence
 *     annotates them. The stored raw api_result is the ground truth.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Backends (matches design S6) ---- */
typedef enum {
    ST25R_TRACE_BACKEND_NONE         = 0,
    ST25R_TRACE_BACKEND_IDF_HIGH     = 1, /* i2c_master_transmit / transmit_receive */
    ST25R_TRACE_BACKEND_IDF_DEFINED  = 2, /* i2c_master_execute_defined_operations */
    ST25R_TRACE_BACKEND_IDF_LEGACY   = 3, /* legacy/direct controller experiment  */
    ST25R_TRACE_BACKEND_DIRECT_EXP   = 4, /* isolated M5-style direct backend      */
} st25r_trace_backend_t;

/* ---- Transaction kinds ---- */
typedef enum {
    ST25R_TRACE_KIND_NONE       = 0,
    ST25R_TRACE_KIND_WRITE      = 1, /* START addr+W payload STOP          */
    ST25R_TRACE_KIND_READ       = 2, /* START addr+W restart addr+R STOP   */
    ST25R_TRACE_KIND_WRITE_READ = 3, /* transmit_receive (write then read)  */
} st25r_trace_kind_t;

/* ---- Driver hints (do not infer NACK from public error; design S9) ---- */
typedef enum {
    ST25R_DRIVER_HINT_UNKNOWN  = 0,
    ST25R_DRIVER_HINT_DONE     = 1,
    ST25R_DRIVER_HINT_NACK     = 2, /* only set with DEBUG/waveform evidence */
    ST25R_DRIVER_HINT_TIMEOUT  = 3,
    ST25R_DRIVER_HINT_BUS_BUSY = 4,
} st25r_driver_hint_t;

/* ---- Application error classes ---- */
typedef enum {
    ST25R_CLASS_OK                  = 0,
    ST25R_CLASS_HOST_NACK_EVENT     = 1, /* DEBUG-confirmed I2C_EVENT_NACK       */
    ST25R_CLASS_HOST_TIMEOUT        = 2,
    ST25R_CLASS_HOST_NOT_DONE_UNKNOWN = 3, /* INVALID_STATE/FAIL pre-evidence    */
    ST25R_CLASS_READBACK_MISMATCH   = 4,
    ST25R_CLASS_NO_RF_RESPONSE      = 5, /* protocol: no tag answered           */
    ST25R_CLASS_PROTOCOL_FAILURE    = 6, /* protocol: bad SAK/collision/etc    */
} st25r_trace_class_t;

/* ---- Transport operations (one per logical START..STOP transaction) ---- */
typedef enum {
    ST25R_OP_NONE           = 0,
    ST25R_OP_READ_A         = 1, /* read  Space-A register                  */
    ST25R_OP_WRITE_A        = 2, /* write Space-A register                  */
    ST25R_OP_READ_B         = 3, /* read  Space-B register (0xFB prefix)    */
    ST25R_OP_WRITE_B        = 4, /* write Space-B register (0xFB prefix)    */
    ST25R_OP_DIRECT_CMD     = 5, /* direct command, no FIFO data            */
    ST25R_OP_DIRECT_CMD_DATA= 6, /* direct command + payload                */
    ST25R_OP_FIFO_WRITE     = 7, /* load FIFO (0x80 + data)                 */
    ST25R_OP_FIFO_READ      = 8, /* read  FIFO (0x9F)                       */
    ST25R_OP_IRQ_READ       = 9, /* multi-byte interrupt-register read      */
    ST25R_OP_PROBE          = 10,/* i2c_master_probe (diagnostic)          */
    ST25R_OP_BUS_SCAN       = 11,/* i2c bus scan probe (diagnostic)         */
} st25r_trace_op_t;

/* ---- Phases (stable across backends; design S6.2) ---- */
typedef enum {
    ST25R_PHASE_IDLE             = 0,
    ST25R_PHASE_INIT_IDENTITY    = 1,
    ST25R_PHASE_INIT_RESET       = 2,
    ST25R_PHASE_INIT_CONFIG      = 3,
    ST25R_PHASE_INIT_OSCILLATOR  = 4,
    ST25R_PHASE_INIT_ANALOG      = 5,
    ST25R_PHASE_FIELD_ON        = 6,
    ST25R_PHASE_REQUEST_SETUP   = 7,
    ST25R_PHASE_REQUEST_TRANSMIT= 8,
    ST25R_PHASE_IRQ_WAIT        = 9,
    ST25R_PHASE_FIFO_READ       = 10,
    ST25R_PHASE_ANTICOLLISION   = 11,
    ST25R_PHASE_SELECT          = 12,
    ST25R_PHASE_IDENTIFY        = 13,
    ST25R_PHASE_DIAGNOSTIC      = 14,
    ST25R_PHASE_SHUTDOWN        = 15,
} st25r_trace_phase_t;

/* ---- Trace mode ---- */
typedef enum {
    ST25R_TRACE_MODE_OFF     = 0, /* no recording (overhead-free baseline)   */
    ST25R_TRACE_MODE_FAILURE = 1, /* record failures only (cheap monitor)   */
    ST25R_TRACE_MODE_ALL     = 2, /* record everything (full ring + freeze)  */
} st25r_trace_mode_t;

/* ---- Event flags ---- */
#define ST25R_TRACE_FLAG_FIRST_ERROR        0x01
#define ST25R_TRACE_FLAG_RECOVERY_ATTEMPTED 0x02
#define ST25R_TRACE_FLAG_RECOVERY_SUCCEEDED 0x04
#define ST25R_TRACE_FLAG_READBACK_VERIFIED  0x08
#define ST25R_TRACE_FLAG_DIAGNOSTIC_TXN     0x10 /* probe/scan: NOT_FOUND expected */
#define ST25R_TRACE_FLAG_RING_OVERWROTE     0x20 /* set on the overwriting event  */

/* ---- ESP error codes (mirrored from esp_err.h so the module stays host-clean) ---- */
#define ST25R_ESP_OK               0
#define ST25R_ESP_FAIL             (-1)
#define ST25R_ESP_ERR_NO_MEM        0x101
#define ST25R_ESP_ERR_INVALID_ARG   0x102
#define ST25R_ESP_ERR_INVALID_STATE 0x103
#define ST25R_ESP_ERR_INVALID_SIZE  0x104
#define ST25R_ESP_ERR_NOT_FOUND     0x105
#define ST25R_ESP_ERR_TIMEOUT       0x107

/* ---- One recorded transaction (design S6) ---- */
typedef struct {
    uint32_t sequence;      /* monotonic within a clear epoch               */
    uint32_t timestamp_us;  /* started_us low 32 bits (wraps ~71 min)        */
    uint32_t elapsed_us;    /* transaction duration                          */
    uint32_t gap_us;        /* idle gap since prior transaction end          */
    uint16_t write_len;     /* bytes written (incl. key/cmd)                 */
    uint16_t read_len;      /* bytes requested/read                          */
    int32_t  api_result;    /* raw esp_err_t (ground truth)                  */
    uint8_t  backend;       /* st25r_trace_backend_t                         */
    uint8_t  phase;         /* st25r_trace_phase_t (current context)         */
    uint8_t  attempt;       /* request attempt number                        */
    uint8_t  kind;          /* st25r_trace_kind_t                            */
    uint8_t  op;            /* st25r_trace_op_t                              */
    uint8_t  logical_key;   /* raw register/command identity (0x0A)         */
    uint8_t  wire_key;      /* first byte on SDA (0x4A for read 0x0A)       */
    uint8_t  driver_hint;   /* st25r_driver_hint_t (UNKNOWN until evidence) */
    uint8_t  error_class;   /* st25r_trace_class_t                          */
    uint8_t  flags;         /* ST25R_TRACE_FLAG_*                            */
} st25r_trace_event_t;

/* ---- Ring + first-error bundle (design S7) ---- */
#ifndef ST25R_TRACE_CAPACITY
#define ST25R_TRACE_CAPACITY 512
#endif
#ifndef ST25R_TRACE_FIRST_ERROR_PREFIX
#define ST25R_TRACE_FIRST_ERROR_PREFIX 16
#endif
#ifndef ST25R_TRACE_FIRST_ERROR_SUFFIX
#define ST25R_TRACE_FIRST_ERROR_SUFFIX 16
#endif

typedef struct {
    /* circular event ring */
    st25r_trace_event_t events[ST25R_TRACE_CAPACITY];
    uint32_t head;          /* next write index (0..CAPACITY-1)            */
    uint32_t count;         /* valid events currently in ring (<=CAPACITY)*/
    uint32_t overwritten;   /* cumulative events lost to wraparound        */
    uint32_t total_recorded;/* cumulative events ever recorded             */
    uint32_t total_failed;  /* cumulative transport failures recorded      */

    /* frozen first-error bundle */
    bool first_error_set;
    st25r_trace_event_t first_error;
    st25r_trace_event_t prefix[ST25R_TRACE_FIRST_ERROR_PREFIX];
    uint8_t prefix_count;
    st25r_trace_event_t suffix[ST25R_TRACE_FIRST_ERROR_SUFFIX];
    uint8_t suffix_count;
    bool collecting_suffix;

    /* current recording context */
    uint8_t backend;        /* st25r_trace_backend_t (set once per backend) */
    uint8_t phase;          /* st25r_trace_phase_t  (set per NFC phase)     */
    uint8_t attempt;        /* request attempt                              */
    uint8_t mode;           /* st25r_trace_mode_t                           */

    int64_t last_end_us;    /* for gap computation                          */
    bool have_last_end;
    uint32_t next_sequence;
} st25r_trace_store_t;

/* ---- Snapshot info returned by st25r_trace_snapshot() ---- */
typedef struct {
    uint32_t total_recorded;
    uint32_t total_failed;
    uint32_t ring_count;     /* events available in snapshot (<= capacity)  */
    uint32_t overwritten;
    bool     first_error_set;
    uint32_t first_error_seq;
} st25r_trace_snapshot_info_t;

/* ---- First-error bundle (prefix + error + suffix) ---- */
typedef struct {
    bool                  set;
    st25r_trace_event_t   error;
    st25r_trace_event_t   prefix[ST25R_TRACE_FIRST_ERROR_PREFIX];
    uint8_t               prefix_count;
    st25r_trace_event_t   suffix[ST25R_TRACE_FIRST_ERROR_SUFFIX];
    uint8_t               suffix_count;
} st25r_first_error_bundle_t;

/* ===================================================================== */
/* API                                                                   */
/* ===================================================================== */

/* Initialize/reset the store to empty, default mode OFF, backend IDF_HIGH. */
void st25r_trace_init(st25r_trace_store_t *store);

/* Clear the ring, counters, and the first-error bundle (explicit reset). */
void st25r_trace_clear(st25r_trace_store_t *store);

/* Set the current recording context. Call at the start of each NFC phase. */
void st25r_trace_set_context(st25r_trace_store_t *store,
                             st25r_trace_backend_t backend,
                             st25r_trace_phase_t phase,
                             uint8_t attempt);

/* Set the trace mode (OFF/FAILURE/ALL). Default is OFF. */
void st25r_trace_set_mode(st25r_trace_store_t *store, st25r_trace_mode_t mode);
st25r_trace_mode_t st25r_trace_get_mode(const st25r_trace_store_t *store);

/* Record one transaction. Computes driver_hint + error_class internally from
 * api_result (per design S9: never infers NACK from the public error alone).
 *
 * started_us  = monotonic microsecond timestamp captured BEFORE the I2C call.
 * elapsed_us   = (now - started_us) captured AFTER the I2C call by the caller.
 *
 * The caller captures two cheap timer reads (esp_timer_get_time() on target)
 * around the I2C call; both are register reads, no serial/I2C/heap/log between
 * them, so the hot path stays observer-safe. Passing elapsed explicitly (rather
 * than reading a clock inside the module) keeps this unit clock-free and
 * deterministic on the host. gap_us is computed internally from the prior
 * transaction end (started + elapsed).
 *
 * Pass flags = 0 normally; ST25R_TRACE_FLAG_DIAGNOSTIC_TXN for probe/scan
 * (excludes expected NOT_FOUND from first-error freeze). */
void st25r_trace_record(st25r_trace_store_t *store,
                        st25r_trace_op_t op,
                        uint8_t logical_key,
                        uint8_t wire_key,
                        st25r_trace_kind_t kind,
                        uint16_t write_len,
                        uint16_t read_len,
                        int64_t started_us,
                        uint32_t elapsed_us,
                        int32_t api_result,
                        uint8_t flags);

/* Copy up to `capacity` events (chronological) into `output`. Returns count
 * copied; fills `info`. Newest events are kept when the ring has overflowed. */
size_t st25r_trace_snapshot(const st25r_trace_store_t *store,
                            st25r_trace_event_t *output, size_t capacity,
                            st25r_trace_snapshot_info_t *info);

/* Copy the frozen first-error bundle. Returns true if a first error is set. */
bool st25r_trace_first_error(const st25r_trace_store_t *store,
                             st25r_first_error_bundle_t *out);

/* Annotate the frozen first error with a driver hint (e.g. after observing
 * the driver DEBUG "unexpected nack" line) and reclassify its error_class.
 * No-op if no first error is set. */
void st25r_trace_annotate_first_error(st25r_trace_store_t *store,
                                      st25r_driver_hint_t hint);

/* Pretty-print the whole ring in the normalized serial format (design S10).
 * `emit` is the output sink (printf on target; a buffer-writer in host tests). */
typedef void (*st25r_trace_emit_fn)(const char *line, void *arg);
void st25r_trace_dump(const st25r_trace_store_t *store,
                      st25r_trace_emit_fn emit, void *arg);

/* Pretty-print the first-error bundle (prefix + error + suffix). */
void st25r_trace_dump_first_error(const st25r_trace_store_t *store,
                                  st25r_trace_emit_fn emit, void *arg);

/* Status line (one line summary for console). */
void st25r_trace_status(const st25r_trace_store_t *store,
                        st25r_trace_emit_fn emit, void *arg);

/* ---- Name helpers (used by dump; also useful for the UI) ---- */
const char *st25r_trace_op_name(st25r_trace_op_t op);
const char *st25r_trace_kind_name(st25r_trace_kind_t kind);
const char *st25r_trace_hint_name(st25r_driver_hint_t hint);
const char *st25r_trace_class_name(st25r_trace_class_t c);
const char *st25r_trace_phase_name(st25r_trace_phase_t p);
const char *st25r_trace_backend_name(st25r_trace_backend_t b);
const char *st25r_trace_mode_name(st25r_trace_mode_t m);
const char *st25r_trace_err_name(int32_t api_result); /* numeric -> "ESP_..." */

#ifdef __cplusplus
}
#endif
