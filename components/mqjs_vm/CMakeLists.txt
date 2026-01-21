idf_component_register(
    SRCS
        "mqjs_vm.cpp"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        mquickjs
        esp_timer
)

