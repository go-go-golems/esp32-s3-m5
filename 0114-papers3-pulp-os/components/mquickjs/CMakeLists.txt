idf_component_register(
    SRCS "mquickjs.c" "cutils.c" "dtoa.c" "libm.c" "mquickjs_build.c"
    INCLUDE_DIRS "."
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-function -Wno-unused-variable -Wno-format -Wno-type-limits)
