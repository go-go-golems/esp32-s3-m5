idf_component_register(
    SRCS
        "main.c"
        "ws281x_encoder.c"
    PRIV_REQUIRES
        esp_driver_rmt
    INCLUDE_DIRS
        "."
)

