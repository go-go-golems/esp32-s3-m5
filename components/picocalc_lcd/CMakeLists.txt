idf_component_register(
    SRCS "picocalc_lcd.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_driver_spi heap
)
