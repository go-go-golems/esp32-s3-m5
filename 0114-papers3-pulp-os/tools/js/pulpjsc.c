/* pulpjsc: host compiler for the reader's JS apps (task ibe5).
 *
 * Compiles an ES5 source file against the SAME s3 stdlib (atoms!) the
 * device runs, emits relocated 32-bit bytecode as a C header for
 * EMBED-style inclusion. Pinned to the runtime by construction: it links
 * the vendored engine sources from components/mquickjs directly.
 *
 * Build and run via tools/js/build_bytecode_apps.sh. The stub functions
 * exist only to satisfy the stdlib table's symbol references; a
 * compilation context never calls them (mqjs.c: "the actual content of
 * the stdlib does not matter... the atoms for the parsing are defined").
 *
 * usage: pulpjsc <in.js> <out.h> <symbol>
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mquickjs.h"

#define STUB(name)                                                         \
    static JSValue name(JSContext *ctx, JSValue *this_val, int argc,       \
                        JSValue *argv) {                                   \
        (void)ctx; (void)this_val; (void)argc; (void)argv;                 \
        return JS_UNDEFINED;                                               \
    }

STUB(js_print)
STUB(js_gc)
STUB(js_load)
STUB(js_setTimeout)
STUB(js_clearTimeout)
STUB(js_date_now)
STUB(js_performance_now)
STUB(js_millis)
STUB(js_pulp_abi_version)
STUB(js_pulp_reset_tree)
STUB(js_pulp_text)
STUB(js_pulp_row)
STUB(js_pulp_col)
STUB(js_pulp_spacer)
STUB(js_pulp_divider)
STUB(js_pulp_progress_bar)
STUB(js_pulp_list)
STUB(js_pulp_region)
STUB(js_pulp_page)
STUB(js_widget_ctor)
STUB(js_w_pad)
STUB(js_w_gap)
STUB(js_w_main_align)
STUB(js_w_cross_align)
STUB(js_w_width)
STUB(js_w_height)
STUB(js_w_flex)
STUB(js_w_font)
STUB(js_w_size)
STUB(js_w_gray)
STUB(js_w_center)
STUB(js_w_align)
STUB(js_w_invert)
STUB(js_w_dep)
STUB(js_w_hit)
STUB(js_w_add)
STUB(js_w_set)
STUB(js_w_progress)
STUB(js_w_on_tap)
STUB(js_w_every)
STUB(js_w_quiet)
STUB(js_w_line)
STUB(js_w_disc)
STUB(js_w_ring)
STUB(js_w_box)
STUB(js_w_paint)
STUB(js_w_wipe)
STUB(js_pulp_canvas)
STUB(js_page_ctor)
STUB(js_p_header)
STUB(js_p_content)
STUB(js_p_footer)
STUB(js_p_overlay)
STUB(js_p_on)
STUB(js_p_every)
STUB(js_p_show)
STUB(js_p_update)
STUB(js_paper_home)
STUB(js_paper_sleep_image)
STUB(js_paper_refresh_turns)
STUB(js_paper_version)
STUB(js_pulp_book_open)
STUB(js_pulp_book_title)
STUB(js_pulp_book_line_count)
STUB(js_pulp_book_line)
STUB(js_pulp_book_next)
STUB(js_pulp_book_prev)
STUB(js_pulp_book_progress)
STUB(js_pulp_library_count)
STUB(js_pulp_library_line)
STUB(js_pulp_library_rescan)
STUB(js_pulp_store_get)
STUB(js_pulp_store_set)
STUB(js_pulp_append_postcard)
STUB(js_pulp_battery_level)
STUB(js_buzzer_tone)
STUB(js_buzzer_beep)
STUB(js_buzzer_stop)
STUB(js_buzzer_melody)
STUB(js_files_exists)
STUB(js_files_list)
STUB(js_files_read)
STUB(js_files_write)
STUB(js_files_append)
STUB(js_files_remove)
STUB(js_files_name)
STUB(js_files_size)
STUB(js_files_is_dir)
STUB(js_files_line)
STUB(js_files_line_count)
STUB(js_wifi_status)
STUB(js_wifi_ip)
STUB(js_wifi_ssid_current)
STUB(js_wifi_rssi_current)
STUB(js_wifi_scan)
STUB(js_wifi_count)
STUB(js_wifi_ssid)
STUB(js_wifi_rssi)
STUB(js_wifi_secure)
STUB(js_wifi_join)
STUB(js_wifi_join_saved)
STUB(js_wifi_save)
STUB(js_wifi_forget)
STUB(js_wifi_saved_count)
STUB(js_wifi_saved_ssid)
STUB(js_wifi_off)
STUB(js_http_get)
STUB(js_http_header)
STUB(js_http_limit)
STUB(js_http_done)
STUB(js_http_send)
STUB(js_http_abort)
STUB(js_http_status)
STUB(js_http_length)
STUB(js_http_body)
STUB(js_http_body_line)
STUB(js_http_body_line_count)
STUB(js_serve_get)
STUB(js_serve_handle)
STUB(js_serve_text)
STUB(js_serve_json)
STUB(js_serve_status)
STUB(js_serve_query)
STUB(js_serve_files)
STUB(js_serve_start)
STUB(js_serve_stop)
STUB(js_serve_url)
STUB(js_battery_level)
STUB(js_battery_mv)
STUB(js_battery_charging)
STUB(js_battery_status_text)
STUB(js_mdns_status)
STUB(js_mdns_host)
STUB(js_mdns_url)
STUB(js_images_count)
STUB(js_images_name)
STUB(js_images_display)
STUB(js_images_remove)
STUB(js_images_received)

static void js_widget_finalizer(JSContext *ctx, void *opaque) {
    (void)ctx; (void)opaque;
}
static void js_page_finalizer(JSContext *ctx, void *opaque) {
    (void)ctx; (void)opaque;
}

/* User class ids (device values; only the names matter to the table).
   JS_CLASS_COUNT sizes the generated finalizer table. */
#define JS_CLASS_WIDGET (JS_CLASS_USER + 0)
#define JS_CLASS_PAGE   (JS_CLASS_USER + 1)
#define JS_CLASS_COUNT  (JS_CLASS_USER + 2)

/* 64-bit host build of the SAME stdlib definition (generated without
   -m32 by build_bytecode_apps.sh). */
#include "pulp_stdlib_host.h"

static void log_to_stderr(void *opaque, const void *buf, size_t len) {
    (void)opaque;
    fwrite(buf, 1, len, stderr);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: pulpjsc <in.js> <out.h> <symbol>\n");
        return 1;
    }
#if JSW != 8
    fprintf(stderr, "pulpjsc must be built as a 64-bit host tool\n");
    return 1;
#else
    FILE *in = fopen(argv[1], "rb");
    if (in == NULL) {
        perror(argv[1]);
        return 1;
    }
    fseek(in, 0, SEEK_END);
    const long src_len = ftell(in);
    fseek(in, 0, SEEK_SET);
    char *src = malloc((size_t)src_len + 1);
    if (fread(src, 1, (size_t)src_len, in) != (size_t)src_len) {
        fprintf(stderr, "short read\n");
        return 1;
    }
    src[src_len] = '\0';
    fclose(in);

    const size_t mem_size = 4u << 20;
    void *mem = malloc(mem_size);
    JSContext *ctx = JS_NewContext2(mem, mem_size, &js_stdlib, 1);
    if (ctx == NULL) {
        fprintf(stderr, "JS_NewContext2 failed\n");
        return 1;
    }
    JS_SetLogFunc(ctx, log_to_stderr);
    const JSValue parsed =
        JS_Parse(ctx, src, (size_t)src_len, argv[1], JS_EVAL_STRIP_COL);
    if (JS_IsException(parsed)) {
        const JSValue err = JS_GetException(ctx);
        JS_PrintValueF(ctx, err, JS_DUMP_LONG);
        fprintf(stderr, "\nparse failed: %s\n", argv[1]);
        return 1;
    }
    JSBytecodeHeader32 hdr;
    const uint8_t *data = NULL;
    uint32_t data_len = 0;
    if (JS_PrepareBytecode64to32(ctx, &hdr, &data, &data_len, parsed) != 0) {
        fprintf(stderr, "64->32 bytecode conversion failed\n");
        return 1;
    }

    FILE *out = fopen(argv[2], "wb");
    if (out == NULL) {
        perror(argv[2]);
        return 1;
    }
    fprintf(out,
            "/* generated by tools/js/pulpjsc.c from %s - do not edit */\n"
            "#pragma once\n\n"
            "static const unsigned char %s[] = {\n",
            argv[1], argv[3]);
    const uint8_t *hdr_bytes = (const uint8_t *)&hdr;
    uint32_t col = 0;
    for (uint32_t i = 0; i < sizeof(hdr); ++i) {
        fprintf(out, "0x%02x,%s", hdr_bytes[i],
                (++col % 12) == 0 ? "\n" : " ");
    }
    for (uint32_t i = 0; i < data_len; ++i) {
        fprintf(out, "0x%02x,%s", data[i], (++col % 12) == 0 ? "\n" : " ");
    }
    fprintf(out, "\n};\n");
    fclose(out);
    fprintf(stderr, "%s: %u bytecode bytes -> %s\n", argv[1],
            (unsigned)(sizeof(hdr) + data_len), argv[2]);
    JS_FreeContext(ctx);
    free(mem);
    free(src);
    return 0;
#endif
}
