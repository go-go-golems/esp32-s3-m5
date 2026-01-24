idf_component_register(
    SRCS
        "matrix_scanner.cpp"
        "tca8418.c"
        "unified_scanner.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_gpio esp_driver_i2c esp_timer
)
