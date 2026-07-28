/* Stdlib definition for PULP OS v2 (ESP-51): the native builder API.
 *
 * Host-only generator input: compiled together with mquickjs_build.c into
 * pulp_stdlib_gen, which emits main/js_stdlib.h (32-bit table) and
 * components/mquickjs/mquickjs_atom.h. Function references here are
 * stringified symbol names — the real implementations live on the device
 * in main/app_js.cpp (extern "C").
 *
 * Regenerate with tools/js/gen_pulp_stdlib.sh after ANY edit here, then
 * rebuild bytecode apps (they are atom-coupled).
 */
#include <stdio.h>
#include <stdint.h>

#include "mquickjs_build.h"

/* v2 design (intern guide §7): builder objects ARE native class instances.
   opaque = packed s3paper WidgetHandle (generation<<16|index)+1; fluent
   methods are ROM prototype entries returning *this_val; the finalizer is
   a no-op (the tree owns nodes); staleness is generation-checked on every
   method and throws TypeError("stale widget handle"). */

static const JSPropDef js_widget_proto[] = {
    JS_CFUNC_DEF("pad", 4, js_w_pad),
    JS_CFUNC_DEF("gap", 1, js_w_gap),
    JS_CFUNC_DEF("mainAlign", 1, js_w_main_align),
    JS_CFUNC_DEF("crossAlign", 1, js_w_cross_align),
    JS_CFUNC_DEF("width", 1, js_w_width),
    JS_CFUNC_DEF("height", 1, js_w_height),
    JS_CFUNC_DEF("flex", 1, js_w_flex),
    JS_CFUNC_DEF("font", 1, js_w_font),
    JS_CFUNC_DEF("size", 1, js_w_size),
    JS_CFUNC_DEF("gray", 1, js_w_gray),
    JS_CFUNC_DEF("center", 0, js_w_center),
    JS_CFUNC_DEF("align", 1, js_w_align),
    JS_CFUNC_DEF("invert", 0, js_w_invert),
    JS_CFUNC_DEF("dep", 1, js_w_dep),
    JS_CFUNC_DEF("hit", 2, js_w_hit),
    JS_CFUNC_DEF("add", 1, js_w_add),
    JS_CFUNC_DEF("set", 1, js_w_set),
    JS_CFUNC_DEF("progress", 1, js_w_progress),
    JS_CFUNC_DEF("onTap", 1, js_w_on_tap),
    JS_CFUNC_DEF("every", 2, js_w_every),
    JS_CFUNC_DEF("quiet", 0, js_w_quiet),
    /* Canvas-kind methods (ESP-52): freehand command list. */
    JS_CFUNC_DEF("line", 6, js_w_line),
    JS_CFUNC_DEF("disc", 4, js_w_disc),
    JS_CFUNC_DEF("ring", 5, js_w_ring),
    JS_CFUNC_DEF("box", 6, js_w_box),
    JS_CFUNC_DEF("paint", 5, js_w_paint),
    JS_CFUNC_DEF("wipe", 0, js_w_wipe),
    JS_PROP_END,
};

static const JSClassDef js_widget_class =
    JS_CLASS_DEF("Widget", 0, js_widget_ctor, JS_CLASS_WIDGET,
                 NULL, js_widget_proto, NULL, js_widget_finalizer);

/* Page: opaque PageId into the native page table. Retained by name;
   show() presents (full/partial), update() runs a diff-only present. */
static const JSPropDef js_page_proto[] = {
    JS_CFUNC_DEF("header", 1, js_p_header),
    JS_CFUNC_DEF("content", 1, js_p_content),
    JS_CFUNC_DEF("footer", 1, js_p_footer),
    JS_CFUNC_DEF("overlay", 1, js_p_overlay),
    JS_CFUNC_DEF("on", 2, js_p_on),
    JS_CFUNC_DEF("every", 1, js_p_every),
    JS_CFUNC_DEF("show", 1, js_p_show),
    JS_CFUNC_DEF("update", 0, js_p_update),
    JS_PROP_END,
};

static const JSClassDef js_page_class =
    JS_CLASS_DEF("Page", 1, js_page_ctor, JS_CLASS_PAGE,
                 NULL, js_page_proto, NULL, js_page_finalizer);

/* paper singleton: device-level policy. Parameter setters, never
   lambdas that would execute inside layout/present. */
static const JSPropDef js_paper[] = {
    JS_CFUNC_DEF("home", 1, js_paper_home),
    JS_CFUNC_DEF("sleepImage", 1, js_paper_sleep_image),
    JS_CFUNC_DEF("refreshTurns", 1, js_paper_refresh_turns),
    JS_CFUNC_DEF("version", 0, js_paper_version),
    JS_PROP_END,
};

static const JSClassDef js_paper_obj = JS_OBJECT_DEF("Paper", js_paper);

/* buzzer singleton (ESP-53): GPIO21 LEDC chimes. Synchronous verbs; the
   owner tick times note stops, so nothing here blocks. */
static const JSPropDef js_buzzer[] = {
    JS_CFUNC_DEF("tone", 2, js_buzzer_tone),
    JS_CFUNC_DEF("beep", 0, js_buzzer_beep),
    JS_CFUNC_DEF("stop", 0, js_buzzer_stop),
    JS_CFUNC_DEF("melody", 1, js_buzzer_melody),
    JS_PROP_END,
};

static const JSClassDef js_buzzer_obj = JS_OBJECT_DEF("Buzzer", js_buzzer);

/* files singleton (ESP-53): bounded SD access rooted at /sdcard. Async
   verbs take (path, [body,] fn) and complete as fn(kind, value, err);
   accessors read the native mailboxes. */
static const JSPropDef js_files[] = {
    JS_CFUNC_DEF("exists", 1, js_files_exists),
    JS_CFUNC_DEF("list", 2, js_files_list),
    JS_CFUNC_DEF("read", 2, js_files_read),
    JS_CFUNC_DEF("write", 3, js_files_write),
    JS_CFUNC_DEF("append", 3, js_files_append),
    JS_CFUNC_DEF("remove", 2, js_files_remove),
    JS_CFUNC_DEF("name", 1, js_files_name),
    JS_CFUNC_DEF("size", 1, js_files_size),
    JS_CFUNC_DEF("isDir", 1, js_files_is_dir),
    JS_CFUNC_DEF("line", 1, js_files_line),
    JS_CFUNC_DEF("lineCount", 0, js_files_line_count),
    JS_PROP_END,
};

static const JSClassDef js_files_obj = JS_OBJECT_DEF("Files", js_files);

/* wifi singleton (ESP-53): station verbs with one completion callback,
   scan mailbox + saved-credential accessors. Radio off at boot; lazy. */
static const JSPropDef js_wifi[] = {
    JS_CFUNC_DEF("status", 0, js_wifi_status),
    JS_CFUNC_DEF("ip", 0, js_wifi_ip),
    JS_CFUNC_DEF("ssidCurrent", 0, js_wifi_ssid_current),
    JS_CFUNC_DEF("rssiCurrent", 0, js_wifi_rssi_current),
    JS_CFUNC_DEF("scan", 1, js_wifi_scan),
    JS_CFUNC_DEF("count", 0, js_wifi_count),
    JS_CFUNC_DEF("ssid", 1, js_wifi_ssid),
    JS_CFUNC_DEF("rssi", 1, js_wifi_rssi),
    JS_CFUNC_DEF("secure", 1, js_wifi_secure),
    JS_CFUNC_DEF("join", 3, js_wifi_join),
    JS_CFUNC_DEF("joinSaved", 1, js_wifi_join_saved),
    JS_CFUNC_DEF("save", 2, js_wifi_save),
    JS_CFUNC_DEF("forget", 1, js_wifi_forget),
    JS_CFUNC_DEF("savedCount", 0, js_wifi_saved_count),
    JS_CFUNC_DEF("savedSsid", 1, js_wifi_saved_ssid),
    JS_CFUNC_DEF("off", 0, js_wifi_off),
    JS_PROP_END,
};

static const JSClassDef js_wifi_obj = JS_OBJECT_DEF("Wifi", js_wifi);

/* http singleton (ESP-53): fetch builder with a terminal verb, per the
   express taste — get/header/limit/done chain, send() launches. */
static const JSPropDef js_http[] = {
    JS_CFUNC_DEF("get", 1, js_http_get),
    JS_CFUNC_DEF("header", 2, js_http_header),
    JS_CFUNC_DEF("limit", 1, js_http_limit),
    JS_CFUNC_DEF("done", 1, js_http_done),
    JS_CFUNC_DEF("send", 0, js_http_send),
    JS_CFUNC_DEF("abort", 0, js_http_abort),
    JS_CFUNC_DEF("status", 0, js_http_status),
    JS_CFUNC_DEF("length", 0, js_http_length),
    JS_CFUNC_DEF("body", 0, js_http_body),
    JS_CFUNC_DEF("bodyLine", 1, js_http_body_line),
    JS_CFUNC_DEF("bodyLineCount", 0, js_http_body_line_count),
    JS_PROP_END,
};

static const JSClassDef js_http_obj = JS_OBJECT_DEF("Http", js_http);

/* serve singleton (ESP-53): JS routes with an express-style builder
   (get(path).handle(fn)); handlers are synchronous and return a token
   from text()/json()/status(). The HOST owns the listener lifecycle. */
static const JSPropDef js_serve[] = {
    JS_CFUNC_DEF("get", 1, js_serve_get),
    JS_CFUNC_DEF("handle", 1, js_serve_handle),
    JS_CFUNC_DEF("text", 1, js_serve_text),
    JS_CFUNC_DEF("json", 1, js_serve_json),
    JS_CFUNC_DEF("status", 1, js_serve_status),
    JS_CFUNC_DEF("query", 1, js_serve_query),
    JS_CFUNC_DEF("files", 2, js_serve_files),
    JS_CFUNC_DEF("start", 1, js_serve_start),
    JS_CFUNC_DEF("stop", 0, js_serve_stop),
    JS_CFUNC_DEF("url", 0, js_serve_url),
    JS_PROP_END,
};

static const JSClassDef js_serve_obj = JS_OBJECT_DEF("Serve", js_serve);

/* battery singleton (ESP-54): surfaces charging + mv already read by
   s3paper::PowerRead(). Sync, owner-only, thin wrappers; batteryLevel()
   remains the historical alias global. */
static const JSPropDef js_battery[] = {
    JS_CFUNC_DEF("level", 0, js_battery_level),
    JS_CFUNC_DEF("mv", 0, js_battery_mv),
    JS_CFUNC_DEF("charging", 0, js_battery_charging),
    JS_CFUNC_DEF("statusText", 0, js_battery_status_text),
    JS_PROP_END,
};

static const JSClassDef js_battery_obj = JS_OBJECT_DEF("Battery", js_battery);

/* mdns singleton (ESP-54): read-only accessors. pulp.local is announced as
   a side effect of serve.start() and withdrawn by serve.stop()/wifi.off(). */
static const JSPropDef js_mdns[] = {
    JS_CFUNC_DEF("status", 0, js_mdns_status),
    JS_CFUNC_DEF("host", 0, js_mdns_host),
    JS_CFUNC_DEF("url", 0, js_mdns_url),
    JS_PROP_END,
};

static const JSClassDef js_mdns_obj = JS_OBJECT_DEF("Mdns", js_mdns);

/* images singleton (ESP-54): gallery catalog + display + upload callback.
   count/name/remove are sync; received(fn) registers the upload-completion
   callback (the module-cb mailbox pattern). */
static const JSPropDef js_images[] = {
    JS_CFUNC_DEF("count", 0, js_images_count),
    JS_CFUNC_DEF("name", 1, js_images_name),
    JS_CFUNC_DEF("display", 1, js_images_display),
    JS_CFUNC_DEF("remove", 1, js_images_remove),
    JS_CFUNC_DEF("received", 1, js_images_received),
    JS_PROP_END,
};

static const JSClassDef js_images_obj = JS_OBJECT_DEF("Images", js_images);

#define CONFIG_PULP 1
#include "mqjs_stdlib_pulp.c"
