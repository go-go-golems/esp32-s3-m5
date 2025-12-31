idf_component_register(
    SRCS
        "ble_temp_logger_main.c"
    PRIV_REQUIRES
        bt
        nvs_flash
        esp_timer
        esp_driver_gpio
        esp_driver_uart
    INCLUDE_DIRS
        ""
)


