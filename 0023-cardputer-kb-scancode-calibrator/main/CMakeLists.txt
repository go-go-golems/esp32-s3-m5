idf_component_register(
    SRCS
        "app_main.cpp"
        "kb_scan.cpp"
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_timer
    REQUIRES M5GFX
    INCLUDE_DIRS "."
)

