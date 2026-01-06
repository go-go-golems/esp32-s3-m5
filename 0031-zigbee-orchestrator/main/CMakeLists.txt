set(srcs
    "app_main.c"
    "gw_bus.c"
    "gw_console.c"
    "gw_console_cmds.c"
    "gw_registry.c"
    "gw_req_id.c"
    "gw_zb.c"
    "gw_zb_app_signal.c"
)

idf_component_register(
    SRCS ${srcs}
    PRIV_REQUIRES
        esp_event
        esp_app_format
        console
        esp_timer
        nvs_flash
        zb_host
    INCLUDE_DIRS
        "."
)
