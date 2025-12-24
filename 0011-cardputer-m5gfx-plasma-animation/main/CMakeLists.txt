idf_component_register(
    SRCS "hello_world_main.cpp"
    PRIV_REQUIRES spi_flash esp_driver_gpio esp_timer
    REQUIRES M5GFX
    INCLUDE_DIRS ""
)



