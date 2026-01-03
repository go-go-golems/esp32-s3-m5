idf_component_register(
    SRCS
        "app_main.cpp"
        "action_registry.cpp"
        "command_palette.cpp"
        "console_repl.cpp"
        "control_plane.cpp"
        "demo_manager.cpp"
        "demo_menu.cpp"
        "demo_basics.cpp"
        "demo_pomodoro.cpp"
        "demo_split_console.cpp"
        "demo_system_monitor.cpp"
        "demo_file_browser.cpp"
        "demo_file_viewer.cpp"
        "input_keyboard.cpp"
        "lvgl_port_cardputer_kb.cpp"
        "lvgl_port_m5gfx.cpp"
        "sdcard_fatfs.cpp"
        "screenshot_png.cpp"
    PRIV_REQUIRES esp_timer console esp_driver_usb_serial_jtag fatfs sdmmc esp_driver_sdspi esp_driver_spi
    REQUIRES M5GFX cardputer_kb lvgl
    INCLUDE_DIRS "."
)
