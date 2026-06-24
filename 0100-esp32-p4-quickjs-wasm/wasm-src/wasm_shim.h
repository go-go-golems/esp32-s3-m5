/*
 * wasm_shim.h — declarations for symbols QuickJS expects but that are not
 * exposed in the standard wasi-libc headers QuickJS includes.
 * Force-included into every TU via -include so the declarations are visible
 * without patching vendored QuickJS sources.
 *
 * - malloc_usable_size: glibc/musl extension used by QuickJS's default
 *   allocator for memory accounting only. wasi-libc's dlmalloc already
 *   DEFINES this (in libc.a) but does not declare it in a header QuickJS
 *   pulls in, so we only forward-declare it here (no definition).
 */
#ifndef WASM_SHIM_H
#define WASM_SHIM_H
#include <stddef.h>
size_t malloc_usable_size(void *ptr);
#endif
