idf_component_register(
    SRCS
        "app_main.cpp"
        "input_keyboard.cpp"
        "ui_list_view.cpp"
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_timer
    REQUIRES M5GFX
    INCLUDE_DIRS "."
)

