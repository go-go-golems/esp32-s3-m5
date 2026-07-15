idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    EMBED_FILES "fixtures/reader_page.bin"
    REQUIRES
        console
        epd_painter
        esp_driver_usb_serial_jtag
        esp_psram
        mbedtls
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wno-missing-field-initializers
    -Werror=return-type
    -Werror=uninitialized
)
