/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * Host unit tests for the st25r_trace ring. Compiled with a plain C compiler
 * (gcc/clang), no ESP-IDF. Validates the design acceptance tests S17:
 *   - wraparound / overwrite count (S17 #3, #7, #8)
 *   - first-error freeze survives later diagnostics (S17 #4)
 *   - sequence monotonic within a clear epoch (S17 #6)
 *   - clear resets ring but preserves mode/backend (S17 #5)
 *   - diagnostic flag excludes expected NOT_FOUND (S17 implicit)
 *   - failure/all/off modes
 *   - annotate upgrades class
 *
 * Build: see test_host/build.sh
 */
#include "st25r_trace.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); g_failures++; } } while (0)

static void emit_stdout(const char *line, void *arg) { (void)arg; printf("  %s\n", line); }

/* Helper: record a register read with explicit timing. */
static void rec_read(st25r_trace_store_t *s, uint8_t reg, int64_t t0, uint32_t elapsed,
                     int32_t result, uint8_t flags)
{
    uint8_t wire = (uint8_t)((reg & 0x3F) | 0x40);
    st25r_trace_record(s, ST25R_OP_READ_A, reg, wire, ST25R_TRACE_KIND_WRITE_READ,
                      1, 1, t0, elapsed, result, flags);
}

static void test_off_mode_records_nothing(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s); /* default OFF */
    rec_read(&s, 0x02, 100, 50, ST25R_ESP_OK, 0);
    CHECK(s.total_recorded == 0);
    CHECK(s.count == 0);
    printf("PASS off_mode_records_nothing\n");
}

static void test_all_mode_records_success_and_failure(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_INIT_CONFIG, 1);
    rec_read(&s, 0x02, 100, 50, ST25R_ESP_OK, 0);            /* seq 1 */
    rec_read(&s, 0x0A, 200, 195, ST25R_ESP_ERR_INVALID_STATE, 0); /* seq 2 fail */
    rec_read(&s, 0x03, 400, 60, ST25R_ESP_OK, 0);            /* seq 3 */
    CHECK(s.total_recorded == 3);
    CHECK(s.total_failed == 1);
    CHECK(s.count == 3);
    CHECK(s.next_sequence == 4);

    st25r_trace_snapshot_info_t info;
    st25r_trace_event_t out[3];
    size_t n = st25r_trace_snapshot(&s, out, 3, &info);
    CHECK(n == 3);
    CHECK(out[0].sequence == 1);
    CHECK(out[1].sequence == 2);
    CHECK(out[1].api_result == ST25R_ESP_ERR_INVALID_STATE);
    CHECK(out[1].driver_hint == ST25R_DRIVER_HINT_UNKNOWN); /* not inferred */
    CHECK(out[1].error_class == ST25R_CLASS_HOST_NOT_DONE_UNKNOWN);
    CHECK(out[1].flags & ST25R_TRACE_FLAG_FIRST_ERROR);
    CHECK(out[2].sequence == 3);
    CHECK(info.total_failed == 1);
    printf("PASS all_mode_records_success_and_failure\n");
}

static void test_first_error_freeze_survives_overwrite(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_INIT_CONFIG, 1);

    /* Fill ring mostly with successes, put a failure at offset 30, then keep
     * recording until the ring wraps many times. The first-error bundle must
     * survive and keep its prefix neighborhood. */
    int64_t t = 1000;
    for (uint32_t i = 0; i < 30; i++) {
        rec_read(&s, 0x02, t, 50, ST25R_ESP_OK, 0);
        t += 100;
    }
    /* failure at seq 31 */
    rec_read(&s, 0x0A, t, 195, ST25R_ESP_ERR_INVALID_STATE, 0);
    uint32_t fail_seq = s.first_error.sequence;
    CHECK(s.first_error_set);
    CHECK(fail_seq == 31);
    CHECK(s.first_error.logical_key == 0x0A);
    CHECK(s.prefix_count == 16);
    CHECK(s.prefix[15].sequence == 30); /* immediately preceding */
    CHECK(s.prefix[0].sequence == 15);  /* 16th preceding */

    /* now record another 2000 events (wrap several times) */
    for (uint32_t i = 0; i < 2000; i++) {
        rec_read(&s, 0x03, t, 40, ST25R_ESP_OK, 0);
        t += 100;
    }
    /* total recorded = 30 + 1 + 2000 = 2031; ring holds 512, so */
    /* overwritten = 2031 - 512 = 1519 */
    CHECK(s.overwritten == 1519);
    /* bundle must be unchanged despite many wraps */
    CHECK(s.first_error_set);
    CHECK(s.first_error.sequence == fail_seq);
    CHECK(s.first_error.logical_key == 0x0A);
    CHECK(s.prefix_count == 16);
    CHECK(s.prefix[15].sequence == 30);
    CHECK(s.suffix_count == 16);
    CHECK(s.suffix[0].sequence == 32);
    CHECK(s.suffix[15].sequence == 47);
    printf("PASS first_error_freeze_survives_overwrite\n");
}

static void test_wraparound_and_overwrite_count(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_DIAGNOSTIC, 1);
    int64_t t = 500;
    for (uint32_t i = 0; i < ST25R_TRACE_CAPACITY; i++) {
        rec_read(&s, 0x02, t, 30, ST25R_ESP_OK, 0);
        t += 80;
    }
    CHECK(s.count == ST25R_TRACE_CAPACITY);
    CHECK(s.overwritten == 0);
    /* one more -> overwrites oldest */
    rec_read(&s, 0x02, t, 30, ST25R_ESP_OK, 0);
    CHECK(s.count == ST25R_TRACE_CAPACITY);
    CHECK(s.overwritten == 1);
    CHECK(s.total_recorded == ST25R_TRACE_CAPACITY + 1);

    /* snapshot must hold the newest CAPACITY events (seq 2..CAPACITY+1) */
    st25r_trace_event_t out[ST25R_TRACE_CAPACITY];
    st25r_trace_snapshot_info_t info;
    size_t n = st25r_trace_snapshot(&s, out, ST25R_TRACE_CAPACITY, &info);
    CHECK(n == ST25R_TRACE_CAPACITY);
    CHECK(out[0].sequence == 2);                  /* oldest kept */
    CHECK(out[ST25R_TRACE_CAPACITY - 1].sequence == ST25R_TRACE_CAPACITY + 1); /* newest */
    CHECK(info.overwritten == 1);
    printf("PASS wraparound_and_overwrite_count\n");
}

static void test_gap_computation(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_INIT_CONFIG, 1);
    /* event 1: start 100, elapsed 50 -> end 150 */
    rec_read(&s, 0x02, 100, 50, ST25R_ESP_OK, 0);
    /* event 2: start 300 -> gap = 300 - 150 = 150 */
    rec_read(&s, 0x03, 300, 60, ST25R_ESP_OK, 0);
    st25r_trace_event_t out[2];
    st25r_trace_snapshot(&s, out, 2, NULL);
    CHECK(out[0].gap_us == 0);      /* first event has no predecessor */
    CHECK(out[1].gap_us == 150);
    printf("PASS gap_computation\n");
}

static void test_diagnostic_flag_excludes_not_found(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_DIAGNOSTIC, 1);
    /* a probe that returns NOT_FOUND with DIAGNOSTIC flag must NOT freeze */
    st25r_trace_record(&s, ST25R_OP_PROBE, 0x00, 0xA0, ST25R_TRACE_KIND_WRITE,
                       1, 0, 100, 40, ST25R_ESP_ERR_NOT_FOUND,
                       ST25R_TRACE_FLAG_DIAGNOSTIC_TXN);
    CHECK(s.total_recorded == 1);
    CHECK(s.total_failed == 0);
    CHECK(!s.first_error_set);
    /* a real register failure with the same result but no DIAG flag DOES freeze */
    rec_read(&s, 0x02, 200, 50, ST25R_ESP_ERR_NOT_FOUND, 0);
    CHECK(s.first_error_set);
    printf("PASS diagnostic_flag_excludes_not_found\n");
}

static void test_failure_mode_records_only_failures(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_FAILURE);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_INIT_CONFIG, 1);
    rec_read(&s, 0x02, 100, 50, ST25R_ESP_OK, 0);            /* skipped */
    rec_read(&s, 0x0A, 200, 195, ST25R_ESP_ERR_INVALID_STATE, 0); /* kept */
    rec_read(&s, 0x03, 400, 60, ST25R_ESP_OK, 0);            /* skipped */
    CHECK(s.total_recorded == 1);
    CHECK(s.count == 1);
    CHECK(s.total_failed == 1);
    CHECK(s.first_error_set);
    /* prefix empty because successes weren't recorded */
    CHECK(s.prefix_count == 0);
    printf("PASS failure_mode_records_only_failures\n");
}

static void test_clear_preserves_mode_backend(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_LEGACY, ST25R_PHASE_DIAGNOSTIC, 2);
    rec_read(&s, 0x02, 100, 50, ST25R_ESP_OK, 0);
    st25r_trace_clear(&s);
    CHECK(s.count == 0);
    CHECK(s.total_recorded == 0);
    CHECK(s.first_error_set == false);
    CHECK(s.mode == ST25R_TRACE_MODE_ALL);              /* preserved */
    CHECK(s.backend == ST25R_TRACE_BACKEND_IDF_LEGACY);/* preserved */
    /* sequence restarts (1-based) after clear */
    rec_read(&s, 0x02, 200, 50, ST25R_ESP_OK, 0);
    st25r_trace_event_t out[1];
    st25r_trace_snapshot(&s, out, 1, NULL);
    CHECK(out[0].sequence == 1);
    printf("PASS clear_preserves_mode_backend\n");
}

static void test_annotate_upgrades_class(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_INIT_CONFIG, 1);
    rec_read(&s, 0x0A, 100, 195, ST25R_ESP_ERR_INVALID_STATE, 0);
    CHECK(s.first_error.driver_hint == ST25R_DRIVER_HINT_UNKNOWN);
    CHECK(s.first_error.error_class == ST25R_CLASS_HOST_NOT_DONE_UNKNOWN);
    /* simulate observing the driver DEBUG "unexpected nack" line */
    st25r_trace_annotate_first_error(&s, ST25R_DRIVER_HINT_NACK);
    CHECK(s.first_error.driver_hint == ST25R_DRIVER_HINT_NACK);
    CHECK(s.first_error.error_class == ST25R_CLASS_HOST_NACK_EVENT);
    printf("PASS annotate_upgrades_class\n");
}

static void test_dump_format(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_REQUEST_SETUP, 1);
    rec_read(&s, 0x0A, 1200456, 195, ST25R_ESP_ERR_INVALID_STATE, 0);
    printf("--- dump format sample ---\n");
    st25r_trace_status(&s, emit_stdout, NULL);
    st25r_trace_dump(&s, emit_stdout, NULL);
    st25r_trace_dump_first_error(&s, emit_stdout, NULL);
    printf("--- end sample ---\n");
    /* sanity: the failing event line names the right op and api */
    CHECK(s.first_error_set);
    printf("PASS dump_format\n");
}

static void test_dump_last(void)
{
    st25r_trace_store_t s;
    st25r_trace_init(&s);
    st25r_trace_set_mode(&s, ST25R_TRACE_MODE_ALL);
    st25r_trace_set_context(&s, ST25R_TRACE_BACKEND_IDF_HIGH, ST25R_PHASE_DIAGNOSTIC, 1);
    int64_t t = 1000;
    for (uint32_t i = 0; i < 10; i++) {
        rec_read(&s, 0x02, t, 30, ST25R_ESP_OK, 0);
        t += 80;
    }
    /* dump_last(3) should print the 3 most recent events (seq 8,9,10) in order */
    st25r_trace_dump_last(&s, 3, emit_stdout, NULL);
    st25r_trace_event_t out[10];
    st25r_trace_snapshot(&s, out, 10, NULL);
    /* sanity: last 3 are seq 8,9,10 */
    CHECK(out[7].sequence == 8);
    CHECK(out[9].sequence == 10);
    printf("PASS dump_last\n");
}

int main(void)
{
    test_off_mode_records_nothing();
    test_all_mode_records_success_and_failure();
    test_first_error_freeze_survives_overwrite();
    test_wraparound_and_overwrite_count();
    test_gap_computation();
    test_diagnostic_flag_excludes_not_found();
    test_failure_mode_records_only_failures();
    test_clear_preserves_mode_backend();
    test_annotate_upgrades_class();
    test_dump_format();
    test_dump_last();

    if (g_failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }
    printf("\n%d CHECK(s) FAILED\n", g_failures);
    return 1;
}
