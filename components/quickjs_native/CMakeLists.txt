# Native upstream QuickJS engine component for ESP-IDF.
#
# This component intentionally builds only the core engine sources. It excludes
# quickjs-libc.c for the first firmware milestone so the ESP32 host API remains
# explicit and small.

file(READ "${CMAKE_CURRENT_LIST_DIR}/quickjs/VERSION" QUICKJS_NATIVE_VERSION)
string(STRIP "${QUICKJS_NATIVE_VERSION}" QUICKJS_NATIVE_VERSION)

idf_component_register(
    SRCS
        "quickjs/quickjs.c"
        "quickjs/cutils.c"
        "quickjs/dtoa.c"
        "quickjs/libregexp.c"
        "quickjs/libunicode.c"
    INCLUDE_DIRS
        "quickjs"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CONFIG_VERSION="${QUICKJS_NATIVE_VERSION}"
)

# QuickJS is third-party C code. Keep warning suppression local to this
# component so application code can still build with the normal ESP-IDF checks.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -include ${CMAKE_CURRENT_LIST_DIR}/quickjs_espidf_compat.h
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-format
    -Wno-type-limits
    -Wno-maybe-uninitialized
    -Wno-implicit-fallthrough
    -Wno-error=incompatible-pointer-types
)
