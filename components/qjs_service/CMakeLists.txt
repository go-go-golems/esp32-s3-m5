idf_component_register(
    SRCS "qjs_service.cpp"
    INCLUDE_DIRS "include"
    REQUIRES quickjs_native esp_timer heap freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-missing-field-initializers
)
