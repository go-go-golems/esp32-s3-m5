#pragma once

/* ESP-IDF/native QuickJS compatibility shim.
 *
 * QuickJS expects a malloc_usable_size() declaration on some libc targets.
 * ESP-IDF provides the symbol through its heap/newlib integration, but the
 * declaration is not visible to QuickJS by default in this build.
 */
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t malloc_usable_size(void *ptr);

#ifdef __cplusplus
}
#endif
