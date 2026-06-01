idf_component_register(
    SRCS
        "app_main.c"
        "picocalc_keyboard.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        console
        driver
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_spi
        esp_driver_uart
        esp_psram
        esp_timer
        heap
        spi_flash
)
