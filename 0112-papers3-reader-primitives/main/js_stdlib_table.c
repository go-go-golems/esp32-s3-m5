/* Instantiates the generated JS stdlib table. The generated header
 * references the binding functions by bare name, so their prototypes must
 * be visible first (implementations are extern "C" in app_js.cpp). */
#include "app_js_bindings.h"

#include "js_stdlib.h"
