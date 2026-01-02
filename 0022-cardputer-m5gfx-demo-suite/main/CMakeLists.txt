idf_component_register(
    SRCS
        "app_main.cpp"
        "input_keyboard.cpp"
        "screenshot_png.cpp"
        "ui_console.cpp"
        "ui_hud.cpp"
        "ui_list_view.cpp"
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_timer esp_driver_usb_serial_jtag
    REQUIRES M5GFX cardputer_kb
    INCLUDE_DIRS "."
)
