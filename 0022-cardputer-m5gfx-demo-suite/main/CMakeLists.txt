idf_component_register(
    SRCS
        "app_main.cpp"
        "demo_a1_hud.cpp"
        "demo_b2_perf.cpp"
        "demo_b3_screenshot.cpp"
        "demo_plasma.cpp"
        "demo_primitives.cpp"
        "input_keyboard.cpp"
        "screenshot_png.cpp"
        "ui_console.cpp"
        "ui_hud.cpp"
        "ui_list_view.cpp"
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_timer esp_driver_usb_serial_jtag
    REQUIRES M5GFX cardputer_kb
    INCLUDE_DIRS "."
)
