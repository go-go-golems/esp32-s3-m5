/* Stdlib definition for the 0112 reader's s3paper JS ABI (Phase 12).
 *
 * Host-only generator input: build with tools/js/gen_s3_stdlib.sh, which
 * emits main/js_stdlib.h and components/mquickjs/mquickjs_atom.h. All
 * function references are stringified names; implementations live in
 * main/app_js.cpp (extern "C") via main/js_stdlib_table.c.
 */
#include <stdio.h>
#include <stdint.h>

#include "mquickjs_build.h"

#define CONFIG_S3 1
#include "mqjs_stdlib_s3.c"
