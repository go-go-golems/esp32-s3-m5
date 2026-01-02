idf_component_register(
    SRCS
        "matrix_scanner.cpp"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_gpio esp_timer
)
