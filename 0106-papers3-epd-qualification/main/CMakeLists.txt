idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        M5Unified
        console
        esp_driver_usb_serial_jtag
        esp_psram
)

foreach(name PAPERS3_MATRIX_CELL PAPERS3_M5GFX_SHA PAPERS3_M5UNIFIED_SHA)
    if(DEFINED ENV{${name}} AND NOT "$ENV{${name}}" STREQUAL "")
        target_compile_definitions(${COMPONENT_LIB} PRIVATE ${name}="$ENV{${name}}")
    endif()
endforeach()
