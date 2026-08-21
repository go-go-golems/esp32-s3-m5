/*
 * SPDX-FileCopyrightText: 2026 (ESP-60-M5STACKCHAN-NFC)
 * SPDX-License-Identifier: MIT
 *
 * esp_console commands for the NFC reader.
 */
#include "nfc_console.h"
#include "st25r3916.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_console.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_bus_handle_t s_i2c_bus = NULL;

/* emit sink: print one trace line to the console */
static void emit_printf(const char *line, void *arg)
{
    (void)arg;
    printf("%s\n", line);
}

static void print_picc(const nfc_picc_t *p)
{
    printf("PICC: UID=");
    for (uint8_t i = 0; i < p->uid_len; i++) {
        printf("%02X%s", p->uid[i], (i + 1 < p->uid_len) ? ":" : "");
    }
    printf(" ATQA=%04X SAK=%02X type=%s\n", p->atqa, p->sak, p->type_str);
}

/* nfc-scan : I2C bus scan (reuse the firmware's i2c_master_probe pattern). */
static int cmd_scan(int argc, char **argv)
{
    if (!s_i2c_bus) { printf("I2C bus not initialized\n"); return 1; }
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            uint8_t a = (uint8_t)(i + j);
            esp_err_t r = i2c_master_probe(s_i2c_bus, a, pdMS_TO_TICKS(200));
            if (r == ESP_OK)      printf("%02x ", a);
            else if (r == ESP_ERR_TIMEOUT) printf("UU ");
            else                  printf("-- ");
        }
        printf("\n");
    }
    return 0;
}

/* nfc-probe : read ST25R3916 IC identity. */
static int cmd_probe(int argc, char **argv)
{
    st25r3916_id_t id;
    esp_err_t e = st25r3916_read_id(&id);
    if (e != ESP_OK) { printf("probe failed: %s\n", esp_err_to_name(e)); return 1; }
    printf("ST25R3916 type=0x%02X rev=0x%02X (%s)\n", id.type, id.revision,
           (id.type == 0x05) ? "ST25R3916/7 OK" : "WRONG TYPE");
    return 0;
}

/* nfc-field on|off */
static int cmd_field(int argc, char **argv)
{
    if (argc < 2) { printf("usage: nfc-field on|off\n"); return 1; }
    if (strcmp(argv[1], "on") == 0) {
        esp_err_t e = st25r3916_field_on();
        printf("field on: %s\n", esp_err_to_name(e));
    } else if (strcmp(argv[1], "off") == 0) {
        esp_err_t e = st25r3916_field_off();
        printf("field off: %s\n", esp_err_to_name(e));
    } else {
        printf("usage: nfc-field on|off\n"); return 1;
    }
    return 0;
}

/* nfc-read : poll once for an ISO14443-A tag.
 * Options: --attempts N  retry up to N polls (default 1). */
static int cmd_read(int argc, char **argv)
{
    int attempts = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--attempts") == 0 && i + 1 < argc) {
            attempts = atoi(argv[++i]);
            if (attempts < 1) attempts = 1;
        }
    }
    bool any_found = false;
    for (int a = 1; a <= attempts; a++) {
        nfc_picc_t p;
        esp_err_t e = st25r3916_poll_nfca(&p);
        if (e == ESP_OK) {
            print_picc(&p);
            any_found = true;
            return 0;
        } else if (e == ESP_ERR_NOT_FOUND) {
            printf("[%d/%d] no tag\n", a, attempts);
        } else {
            printf("[%d/%d] read error: %s\n", a, attempts, esp_err_to_name(e));
            st25r3916_debug_dump();
        }
        if (a < attempts) vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (!any_found) {
        /* surface transport health so a no-tag result is not confused with a
         * transport failure */
        st25r_trace_store_t *t = st25r3916_trace();
        if (t && t->total_failed > 0) {
            printf("transport: %lu failed transaction(s) during read -- see 'nfc-trace first-error'\n",
                   (unsigned long)t->total_failed);
        }
    }
    return 1;
}

/* nfc-reqa : loop REQA/WUPA and print every result (find the coil by sweeping tag).
 * Alternates REQA and WUPA each iteration (WUPA wakes halted tags). Prints
 * ATQA=xxxx when a tag answers, "." otherwise. */
static int cmd_reqa(int argc, char **argv)
{
    st25r3916_field_on();
    printf("REQA/WUPA loop (sweep tag over body). Reset to stop.\n");
    for (int i = 0; i < 200; i++) {  /* ~30s at 150ms */
        uint16_t atqa = 0;
        esp_err_t e = (i % 2) ? st25r3916_wupa(&atqa) : st25r3916_reqa(&atqa);
        if (e == ESP_OK)      printf("ATQA=%04X <== tag!\n", atqa);
        else if (e == ESP_ERR_NOT_FOUND) printf(".\n");
        else                  printf("wake err: %s\n", esp_err_to_name(e));
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    return 0;
}

/* nfc-regs : dump key ST25R3916 registers. */
static int cmd_regs(int argc, char **argv)
{
    st25r3916_debug_dump();
    return 0;
}

/* nfc-cap : measure antenna capacitance (is the coil connected?). */
static int cmd_cap(int argc, char **argv)
{
    uint8_t c = st25r3916_measure_capacitance();
    printf("cap=%3u (non-zero/stable => antenna coil connected; 0 => feed open)\n", c);
    return 0;
}

/* nfc-dump : dump all Space-A registers for expert comparison. */
static int cmd_dump(int argc, char **argv)
{
    st25r3916_dump_all();
    return 0;
}

/* nfc-sweep : field on + repeatedly measure RF amplitude (find the coil).
 * Slide the tag over the body; the value spikes when the tag is over the coil. */
static int cmd_sweep(int argc, char **argv)
{
    st25r3916_force_field_on();
    st25r3916_set_tx_rx(true);  /* enable receiver for amplitude measurement */
    printf("sweeping (Ctrl-C/reset to stop). Higher = tag loading the field.\n");
    for (int i = 0; i < 200; i++) {  /* ~30s at 150ms */
        uint8_t a = st25r3916_measure_amplitude();
        printf("amp=%3u ", a);
        for (uint8_t k = 0; k < (a / 4); k++) putchar('#');
        printf("\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    return 0;
}

/* nfc-poll : continuously poll (until Ctrl-C / device reset). */
static int cmd_poll(int argc, char **argv)
{
    printf("polling (place/remove a tag). Reset device to stop.\n");
    nfc_picc_t last;
    bool have_last = false;
    memset(&last, 0, sizeof(last));
    while (1) {
        nfc_picc_t p;
        esp_err_t e = st25r3916_poll_nfca(&p);
        if (e == ESP_OK) {
            /* Print only on change to avoid spam. */
            bool same = have_last &&
                        p.uid_len == last.uid_len &&
                        memcmp(p.uid, last.uid, p.uid_len) == 0 &&
                        p.atqa == last.atqa && p.sak == last.sak;
            if (!same) {
                print_picc(&p);
                last = p;
                have_last = true;
            }
        } else if (e == ESP_ERR_NOT_FOUND) {
            if (have_last) {
                printf("tag removed\n");
                have_last = false;
                memset(&last, 0, sizeof(last));
            }
        } else {
            printf("poll error: %s\n", esp_err_to_name(e));
        }
        vTaskDelay(pdMS_TO_TICKS(400));
    }
    return 0;
}

/* nfc-trace : observer-safe transaction trace ring control.
 * Subcommands:
 *   status           one-line summary
 *   dump [--last N]  print whole ring (or last N) in normalized format
 *   first-error      print the frozen first-error bundle (prefix+error+suffix)
 *   clear            reset ring + counters + first-error bundle
 *   mode off|failure|all   set recording mode
 *   annotate nack|timeout  mark the frozen first error from driver DEBUG evidence
 */
static int cmd_trace(int argc, char **argv)
{
    st25r_trace_store_t *t = st25r3916_trace();
    if (!t) { printf("trace not available\n"); return 1; }
    if (argc < 2) { printf("usage: nfc-trace status|dump|first-error|clear|mode|annotate\n"); return 1; }
    const char *sub = argv[1];

    if (strcmp(sub, "status") == 0) {
        st25r_trace_status(t, emit_printf, NULL);
        return 0;
    }
    if (strcmp(sub, "dump") == 0) {
        /* optional --last N: print only the N most recent events */
        int last = 0;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--last") == 0 && i + 1 < argc) last = atoi(argv[++i]);
        }
        if (last > 0) st25r_trace_dump_last(t, (uint32_t)last, emit_printf, NULL);
        else          st25r_trace_dump(t, emit_printf, NULL);
        return 0;
    }
    if (strcmp(sub, "first-error") == 0) {
        st25r_trace_dump_first_error(t, emit_printf, NULL);
        return 0;
    }
    if (strcmp(sub, "clear") == 0) {
        st25r_trace_clear(t);
        printf("trace cleared\n");
        return 0;
    }
    if (strcmp(sub, "mode") == 0) {
        if (argc < 3) { printf("current mode: %s\n", st25r_trace_mode_name(st25r_trace_get_mode(t))); return 0; }
        if (strcmp(argv[2], "off") == 0) st25r_trace_set_mode(t, ST25R_TRACE_MODE_OFF);
        else if (strcmp(argv[2], "failure") == 0) st25r_trace_set_mode(t, ST25R_TRACE_MODE_FAILURE);
        else if (strcmp(argv[2], "all") == 0) st25r_trace_set_mode(t, ST25R_TRACE_MODE_ALL);
        else { printf("usage: nfc-trace mode off|failure|all\n"); return 1; }
        printf("mode set: %s\n", st25r_trace_mode_name(st25r_trace_get_mode(t)));
        return 0;
    }
    if (strcmp(sub, "annotate") == 0) {
        if (argc < 3) { printf("usage: nfc-trace annotate nack|timeout\n"); return 1; }
        if (strcmp(argv[2], "nack") == 0) st25r_trace_annotate_first_error(t, ST25R_DRIVER_HINT_NACK);
        else if (strcmp(argv[2], "timeout") == 0) st25r_trace_annotate_first_error(t, ST25R_DRIVER_HINT_TIMEOUT);
        else { printf("usage: nfc-trace annotate nack|timeout\n"); return 1; }
        st25r_trace_dump_first_error(t, emit_printf, NULL);
        return 0;
    }
    printf("unknown nfc-trace subcommand: %s\n", sub);
    return 1;
}

static void reg(const char *cmd, const char *help, esp_console_cmd_func_t func, const char *hint)
{
    esp_console_cmd_t c = {0};
    c.command = cmd;
    c.help = help;
    c.func = func;
    c.hint = hint;
    esp_console_cmd_register(&c);
}

void nfc_console_register(i2c_master_bus_handle_t i2c_bus)
{
    s_i2c_bus = i2c_bus;
    reg("nfc-scan",  "I2C bus scan (0x00-0x7f)",        cmd_scan,  NULL);
    reg("nfc-probe", "Read ST25R3916 IC identity",      cmd_probe, NULL);
    reg("nfc-field", "RF field on|off",                 cmd_field,  "on|off");
    reg("nfc-read",  "Poll one ISO14443-A tag",         cmd_read,   NULL);
    reg("nfc-poll",  "Continuously poll for tags",      cmd_poll,   NULL);
    reg("nfc-regs",  "Dump key ST25R3916 registers",   cmd_regs,   NULL);
    reg("nfc-sweep", "Field on + measure RF amplitude (find coil)", cmd_sweep, NULL);
    reg("nfc-reqa", "Loop REQA, print ATQA on tag (find coil)", cmd_reqa, NULL);
    reg("nfc-cap",  "Measure antenna capacitance (coil connected?)", cmd_cap, NULL);
    reg("nfc-dump", "Dump all Space-A registers (expert compare)", cmd_dump, NULL);
    reg("nfc-trace", "Transaction trace ring: status|dump|first-error|clear|mode|annotate", cmd_trace, NULL);
}
