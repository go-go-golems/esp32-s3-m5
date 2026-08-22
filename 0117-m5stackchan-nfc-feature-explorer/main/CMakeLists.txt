idf_component_register(
    SRCS
        "app_main.cpp"
        "nfc_explorer.cpp"
        "nfc_console.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        console
        driver
        esp_driver_i2c
        esp_driver_usb_serial_jtag
        nvs_flash
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
