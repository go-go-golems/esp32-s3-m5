/* native_test.c — native (non-wasm) sanity check for the qjs_init/qjs_eval wrapper.
 * Provides host_print/host_millis/host_gpio_write (which wasm_main.c imports in
 * the wasm build) and a main() that drives the same eval path. */
#include <stdio.h>
#include <string.h>
#include "quickjs.h"

extern void qjs_init(void);
extern int qjs_eval(const char *src, int len);

void host_print(const char *s) { fputs(s, stdout); fflush(stdout); }
int host_millis(void) { return 0; }
void host_gpio_write(int p, int v) { (void)p; (void)v; }

int main(int argc, char **argv)
{
    const char *src = argc > 1 ? argv[1] : "print(1+2)";
    qjs_init();
    int r = qjs_eval(src, (int)strlen(src));
    printf("\n[native] qjs_eval returned %d\n", r);
    return r == 0 ? 0 : 1;
}
