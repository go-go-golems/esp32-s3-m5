idf_component_register(
    SRCS
        "app_main.cpp"
        "console_adapter.cpp"
        "demo_profiles.cpp"
        "demo_messages.cpp"
        "nvs_mode_store.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        gogolem_nfc
        gogolem_nfc_engine
        console
        esp_driver_i2c
        esp_driver_usb_serial_jtag
        esp_system
        nvs_flash
        freertos
)
