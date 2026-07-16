/* Prototypes pairing the generated JS stdlib table (main/js_stdlib.h)
 * with the binding implementations in app_js.cpp. C linkage: included by
 * both the C table TU and the C++ implementation. */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "mquickjs.h"

/* User class ids for this firmware's stdlib. JS_CLASS_COUNT sizes the
 * generated finalizer table and MUST cover every user class. */
#define JS_CLASS_WIDGET (JS_CLASS_USER + 0)
#define JS_CLASS_PAGE   (JS_CLASS_USER + 1)
#define JS_CLASS_COUNT  (JS_CLASS_USER + 2)

#define PULP_JS_FN(name) \
    JSValue name(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)

PULP_JS_FN(js_print);
PULP_JS_FN(js_gc);
PULP_JS_FN(js_load);
PULP_JS_FN(js_setTimeout);
PULP_JS_FN(js_clearTimeout);
PULP_JS_FN(js_date_now);
PULP_JS_FN(js_performance_now);
PULP_JS_FN(js_millis);

PULP_JS_FN(js_pulp_abi_version);
PULP_JS_FN(js_pulp_reset_tree);
PULP_JS_FN(js_pulp_text);
PULP_JS_FN(js_pulp_row);
PULP_JS_FN(js_pulp_col);
PULP_JS_FN(js_pulp_spacer);
PULP_JS_FN(js_pulp_divider);
PULP_JS_FN(js_pulp_progress_bar);
PULP_JS_FN(js_pulp_list);
PULP_JS_FN(js_pulp_region);
PULP_JS_FN(js_pulp_page);

PULP_JS_FN(js_widget_ctor);
PULP_JS_FN(js_w_pad);
PULP_JS_FN(js_w_gap);
PULP_JS_FN(js_w_main_align);
PULP_JS_FN(js_w_cross_align);
PULP_JS_FN(js_w_width);
PULP_JS_FN(js_w_height);
PULP_JS_FN(js_w_flex);
PULP_JS_FN(js_w_font);
PULP_JS_FN(js_w_size);
PULP_JS_FN(js_w_gray);
PULP_JS_FN(js_w_center);
PULP_JS_FN(js_w_align);
PULP_JS_FN(js_w_invert);
PULP_JS_FN(js_w_dep);
PULP_JS_FN(js_w_hit);
PULP_JS_FN(js_w_add);
PULP_JS_FN(js_w_set);
PULP_JS_FN(js_w_progress);
PULP_JS_FN(js_w_on_tap);
PULP_JS_FN(js_w_every);
PULP_JS_FN(js_w_quiet);
PULP_JS_FN(js_w_line);
PULP_JS_FN(js_w_disc);
PULP_JS_FN(js_w_ring);
PULP_JS_FN(js_w_box);
PULP_JS_FN(js_w_paint);
PULP_JS_FN(js_w_wipe);
PULP_JS_FN(js_pulp_canvas);
void js_widget_finalizer(JSContext *ctx, void *opaque);

PULP_JS_FN(js_page_ctor);
PULP_JS_FN(js_p_header);
PULP_JS_FN(js_p_content);
PULP_JS_FN(js_p_footer);
PULP_JS_FN(js_p_overlay);
PULP_JS_FN(js_p_on);
PULP_JS_FN(js_p_every);
PULP_JS_FN(js_p_show);
PULP_JS_FN(js_p_update);
void js_page_finalizer(JSContext *ctx, void *opaque);

PULP_JS_FN(js_paper_home);
PULP_JS_FN(js_paper_sleep_image);
PULP_JS_FN(js_paper_refresh_turns);
PULP_JS_FN(js_paper_version);

PULP_JS_FN(js_pulp_book_open);
PULP_JS_FN(js_pulp_book_title);
PULP_JS_FN(js_pulp_book_line_count);
PULP_JS_FN(js_pulp_book_line);
PULP_JS_FN(js_pulp_book_next);
PULP_JS_FN(js_pulp_book_prev);
PULP_JS_FN(js_pulp_book_progress);

PULP_JS_FN(js_pulp_library_count);
PULP_JS_FN(js_pulp_library_line);
PULP_JS_FN(js_pulp_library_rescan);
PULP_JS_FN(js_pulp_store_get);
PULP_JS_FN(js_pulp_store_set);
PULP_JS_FN(js_pulp_append_postcard);
PULP_JS_FN(js_pulp_battery_level);

/* ESP-53 connectivity + peripherals. */
PULP_JS_FN(js_buzzer_tone);
PULP_JS_FN(js_buzzer_beep);
PULP_JS_FN(js_buzzer_stop);
PULP_JS_FN(js_buzzer_melody);

PULP_JS_FN(js_files_exists);
PULP_JS_FN(js_files_list);
PULP_JS_FN(js_files_read);
PULP_JS_FN(js_files_write);
PULP_JS_FN(js_files_append);
PULP_JS_FN(js_files_remove);
PULP_JS_FN(js_files_name);
PULP_JS_FN(js_files_size);
PULP_JS_FN(js_files_is_dir);
PULP_JS_FN(js_files_line);
PULP_JS_FN(js_files_line_count);

PULP_JS_FN(js_wifi_status);
PULP_JS_FN(js_wifi_ip);
PULP_JS_FN(js_wifi_ssid_current);
PULP_JS_FN(js_wifi_rssi_current);
PULP_JS_FN(js_wifi_scan);
PULP_JS_FN(js_wifi_count);
PULP_JS_FN(js_wifi_ssid);
PULP_JS_FN(js_wifi_rssi);
PULP_JS_FN(js_wifi_secure);
PULP_JS_FN(js_wifi_join);
PULP_JS_FN(js_wifi_join_saved);
PULP_JS_FN(js_wifi_save);
PULP_JS_FN(js_wifi_forget);
PULP_JS_FN(js_wifi_saved_count);
PULP_JS_FN(js_wifi_saved_ssid);
PULP_JS_FN(js_wifi_off);

#ifdef __cplusplus
}
#endif
