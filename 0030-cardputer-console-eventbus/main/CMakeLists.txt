idf_component_register(
    SRCS
        "app_main.cpp"
    PRIV_REQUIRES
        spi_flash
        esp_event
        esp_timer
        console
    REQUIRES
        M5GFX
        cardputer_kb
    INCLUDE_DIRS
        "."
)

