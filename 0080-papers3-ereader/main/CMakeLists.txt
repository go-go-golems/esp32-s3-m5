idf_component_register(
    SRCS
        "app_main.cpp"
        "ereader_app.cpp"
        "ereader_console.cpp"
        "book_store.cpp"
        "paginator.cpp"
        "bookmark_store.cpp"
        "layout_engine.cpp"
        "widget_renderer.cpp"
        "dirty_tracker.cpp"
        "bitmap_font.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        M5Unified
        console
        esp_driver_usb_serial_jtag
    PRIV_REQUIRES
        spiffs
)

spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)
