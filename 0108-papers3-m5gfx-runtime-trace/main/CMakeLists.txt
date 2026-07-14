idf_component_register(
    SRCS
        "app_main.cpp"
        "epd_trace_runtime.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        M5Unified
        console
        esp_driver_usb_serial_jtag
        esp_psram
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    PAPERS3_MATRIX_CELL="TRACE-D"
    PAPERS3_M5GFX_SHA="ad9b814264d4e2000e9f30070002310bbccaffc9"
    PAPERS3_M5UNIFIED_SHA="b1ffcc677014ed8bd01e5a1f240736ae654bfe12"
)
