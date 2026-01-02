#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "mquickjs_build.h"

// Just use the standard library without extra functions
// Remove Date.now, performance.now, load, setTimeout, clearTimeout
#define CONFIG_MINIMAL_STDLIB

#include "mqjs_stdlib.c"
