idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES cardputer_bsp meshcore_compat nvs_flash
)
