idf_component_register(
    SRCS
        "app_main.c"
        "matrix_console.c"
        "max7219.c"
    PRIV_REQUIRES
        spi_flash
        console
        esp_driver_gpio
        esp_driver_spi
    INCLUDE_DIRS
        "."
)

