idf_component_register(
    SRCS
        "hello_world_main.cpp"
        "uart_rx_heartbeat.cpp"
        "control_plane.cpp"
        "gif_registry.cpp"
        "gif_player.cpp"
        "console_repl.cpp"
        "backlight.cpp"
        "display_hal.cpp"
        "gif_storage.cpp"
    PRIV_REQUIRES
        spi_flash
        esp_driver_gpio
        esp_driver_spi
        esp_driver_i2c
        esp_timer
        console
        esp_partition
        wear_levelling
        fatfs
    REQUIRES
        M5GFX
        animatedgif
    INCLUDE_DIRS ""
)


