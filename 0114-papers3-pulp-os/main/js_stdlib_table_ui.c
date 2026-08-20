/* ESP-55 P9: the UI stdlib — the sandbox for browser page contexts.
 *
 * Shares the generated ROM object table (identical atoms, classes and
 * prototypes: page scripts compile/parse exactly like apps) but swaps the
 * C function table so every non-UI native is js_ui_denied. The engine
 * dispatches native calls through ctx->c_function_table[idx], so the
 * denial is enforced at the call layer, not by JS discipline.
 *
 * MAINTENANCE RULE: every new native added to tools/js/pulp_stdlib.c or
 * mqjs_stdlib_pulp.c MUST be added to the deny list below unless pages
 * are explicitly allowed to call it. Default is DENY.
 */
#include "app_js_bindings.h"

static JSValue js_ui_denied(JSContext *ctx, JSValue *this_val, int argc,
                            JSValue *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    return JS_ThrowTypeError(ctx, "not available to pages");
}

/* engine escape hatches */
#define js_load js_ui_denied
#define js_pulp_reset_tree js_ui_denied
/* paper policy (version stays readable) */
#define js_paper_home js_ui_denied
#define js_paper_sleep_image js_ui_denied
#define js_paper_refresh_turns js_ui_denied
/* books / library / persistence */
#define js_pulp_book_open js_ui_denied
#define js_pulp_book_title js_ui_denied
#define js_pulp_book_line_count js_ui_denied
#define js_pulp_book_line js_ui_denied
#define js_pulp_book_next js_ui_denied
#define js_pulp_book_prev js_ui_denied
#define js_pulp_book_progress js_ui_denied
#define js_pulp_library_count js_ui_denied
#define js_pulp_library_line js_ui_denied
#define js_pulp_library_rescan js_ui_denied
#define js_pulp_store_get js_ui_denied
#define js_pulp_store_set js_ui_denied
#define js_pulp_append_postcard js_ui_denied
#define js_pulp_battery_level js_ui_denied
/* peripherals */
#define js_buzzer_tone js_ui_denied
#define js_buzzer_beep js_ui_denied
#define js_buzzer_stop js_ui_denied
#define js_buzzer_melody js_ui_denied
#define js_battery_level js_ui_denied
#define js_battery_mv js_ui_denied
#define js_battery_charging js_ui_denied
#define js_battery_status_text js_ui_denied
/* filesystem */
#define js_files_exists js_ui_denied
#define js_files_list js_ui_denied
#define js_files_read js_ui_denied
#define js_files_write js_ui_denied
#define js_files_append js_ui_denied
#define js_files_remove js_ui_denied
#define js_files_name js_ui_denied
#define js_files_size js_ui_denied
#define js_files_is_dir js_ui_denied
#define js_files_line js_ui_denied
#define js_files_line_count js_ui_denied
/* network */
#define js_wifi_status js_ui_denied
#define js_wifi_ip js_ui_denied
#define js_wifi_ssid_current js_ui_denied
#define js_wifi_rssi_current js_ui_denied
#define js_wifi_scan js_ui_denied
#define js_wifi_count js_ui_denied
#define js_wifi_ssid js_ui_denied
#define js_wifi_rssi js_ui_denied
#define js_wifi_secure js_ui_denied
#define js_wifi_join js_ui_denied
#define js_wifi_join_saved js_ui_denied
#define js_wifi_save js_ui_denied
#define js_wifi_forget js_ui_denied
#define js_wifi_saved_count js_ui_denied
#define js_wifi_saved_ssid js_ui_denied
#define js_wifi_off js_ui_denied
#define js_http_get js_ui_denied
#define js_http_header js_ui_denied
#define js_http_limit js_ui_denied
#define js_http_done js_ui_denied
#define js_http_send js_ui_denied
#define js_http_abort js_ui_denied
#define js_http_status js_ui_denied
#define js_http_length js_ui_denied
#define js_http_body js_ui_denied
#define js_http_body_line js_ui_denied
#define js_http_body_line_count js_ui_denied
#define js_serve_get js_ui_denied
#define js_serve_handle js_ui_denied
#define js_serve_text js_ui_denied
#define js_serve_json js_ui_denied
#define js_serve_status js_ui_denied
#define js_serve_query js_ui_denied
#define js_serve_files js_ui_denied
#define js_serve_start js_ui_denied
#define js_serve_stop js_ui_denied
#define js_serve_url js_ui_denied
#define js_mdns_status js_ui_denied
#define js_mdns_host js_ui_denied
#define js_mdns_url js_ui_denied
/* images + apps + browser control */
#define js_images_count js_ui_denied
#define js_images_name js_ui_denied
#define js_images_display js_ui_denied
#define js_images_remove js_ui_denied
#define js_images_received js_ui_denied
#define js_apps_count js_ui_denied
#define js_apps_name js_ui_denied
#define js_apps_copy js_ui_denied
#define js_apps_write_text js_ui_denied
#define js_apps_received js_ui_denied
#define js_apps_upload_name js_ui_denied
#define js_browser_run js_ui_denied
#define js_browser_close js_ui_denied
#define js_browser_watch js_ui_denied
#define js_browser_nav_url js_ui_denied
#define js_browser_nav_kind js_ui_denied

/* Emit the whole generated stdlib under a new name. */
#define js_stdlib js_stdlib_ui
#include "js_stdlib.h"
