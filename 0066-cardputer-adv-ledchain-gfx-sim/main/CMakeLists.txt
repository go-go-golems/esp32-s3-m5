idf_component_register(
    SRCS
        "app_main.cpp"
        "sim_console.cpp"
        "sim_engine.cpp"
        "sim_ui.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        M5GFX
        console
        ws281x
)
