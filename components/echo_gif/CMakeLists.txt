idf_component_register(
    SRCS
        "src/gif_storage.cpp"
        "src/gif_registry.cpp"
        "src/gif_player.cpp"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        esp_partition
        fatfs
        spi_flash
        esp_timer
        log
        animatedgif
        M5GFX
)


