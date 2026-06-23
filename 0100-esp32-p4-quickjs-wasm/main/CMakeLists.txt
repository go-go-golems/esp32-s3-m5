# main/CMakeLists.txt for 0100.
# Currently registers a minimal buildable console stub.
#
# === INTERN: after building quickjs.wasm and porting the WAMR sources, replace this with ===
#
# idf_component_register(
#     SRCS
#         "app_main.cpp"
#         "wasm_runtime_service.cpp"
#         "wasm_host_api.cpp"
#         "wasm_runner.cpp"
#         "js_command.cpp"
#     INCLUDE_DIRS "."
#     REQUIRES
#         espressif__wasm-micro-runtime
#         console
#         pthread
#     EMBED_FILES
#         "quickjs.wasm"
# )

idf_component_register(
    SRCS
        "app_main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        console
)
