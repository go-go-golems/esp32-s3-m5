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

JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc,
                      JSValue *argv);
JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc,
                    JSValue *argv);
JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc,
                           JSValue *argv);
JSValue js_millis(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue js_s3_version(JSContext *ctx, JSValue *this_val, int argc,
                      JSValue *argv);
JSValue js_s3_reset(JSContext *ctx, JSValue *this_val, int argc,
                    JSValue *argv);
JSValue js_s3_text(JSContext *ctx, JSValue *this_val, int argc,
                   JSValue *argv);
JSValue js_s3_row(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_s3_col(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_s3_spacer(JSContext *ctx, JSValue *this_val, int argc,
                     JSValue *argv);
JSValue js_s3_divider(JSContext *ctx, JSValue *this_val, int argc,
                      JSValue *argv);
JSValue js_s3_progress(JSContext *ctx, JSValue *this_val, int argc,
                       JSValue *argv);
JSValue js_s3_list(JSContext *ctx, JSValue *this_val, int argc,
                   JSValue *argv);
JSValue js_s3_region(JSContext *ctx, JSValue *this_val, int argc,
                     JSValue *argv);
JSValue js_s3_add_child(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_set_text(JSContext *ctx, JSValue *this_val, int argc,
                       JSValue *argv);
JSValue js_s3_set_progress(JSContext *ctx, JSValue *this_val, int argc,
                           JSValue *argv);
JSValue js_s3_config(JSContext *ctx, JSValue *this_val, int argc,
                     JSValue *argv);
JSValue js_s3_present(JSContext *ctx, JSValue *this_val, int argc,
                      JSValue *argv);
JSValue js_s3_library_count(JSContext *ctx, JSValue *this_val, int argc,
                            JSValue *argv);
JSValue js_s3_library_line(JSContext *ctx, JSValue *this_val, int argc,
                           JSValue *argv);
JSValue js_s3_embedded_line(JSContext *ctx, JSValue *this_val, int argc,
                            JSValue *argv);
JSValue js_s3_open_book(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_book_open(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_book_title(JSContext *ctx, JSValue *this_val, int argc,
                         JSValue *argv);
JSValue js_s3_book_line_count(JSContext *ctx, JSValue *this_val, int argc,
                              JSValue *argv);
JSValue js_s3_book_line(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_book_next(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_book_prev(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_book_progress(JSContext *ctx, JSValue *this_val, int argc,
                            JSValue *argv);
JSValue js_s3_timer_start(JSContext *ctx, JSValue *this_val, int argc,
                          JSValue *argv);
JSValue js_s3_timer_stop(JSContext *ctx, JSValue *this_val, int argc,
                         JSValue *argv);
JSValue js_s3_store_get(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_store_set(JSContext *ctx, JSValue *this_val, int argc,
                        JSValue *argv);
JSValue js_s3_append_postcard(JSContext *ctx, JSValue *this_val, int argc,
                              JSValue *argv);

#ifdef __cplusplus
}
#endif
