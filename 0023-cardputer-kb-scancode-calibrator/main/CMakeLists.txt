idf_component_register(
    SRCS
        "app_main.cpp"
        "wizard.cpp"
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_timer
    REQUIRES M5GFX cardputer_kb
    INCLUDE_DIRS "."
)
