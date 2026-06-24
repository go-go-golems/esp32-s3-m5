/*
 * Stub <setjmp.h> for the wasm build.
 *
 * QuickJS's dtoa.c includes <setjmp.h> but NEVER uses setjmp/longjmp/jmp_buf
 * (verified: no call sites in any QuickJS .c). wasi-libc's real setjmp.h emits
 * a #error demanding `-mllvm -wasm-enable-sjlj` + an Exception-Handling-capable
 * engine. Rather than enable the EH proposal (which WAMR's interpreter doesn't
 * need), this stub satisfies the dead include without pulling in EH.
 *
 * Found via -I wasm_overrides (searched before the sysroot).
 */
#ifndef _WASM_STUB_SETJMP_H
#define _WASM_STUB_SETJMP_H
typedef int jmp_buf[4];
static int (setjmp)(jmp_buf b) { (void)b; return 0; }
static void (longjmp)(jmp_buf b, int v) { (void)b; (void)v; for (;;) {}
}
#endif
