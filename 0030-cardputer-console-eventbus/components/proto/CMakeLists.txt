include(FetchContent)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    FetchContent_Declare(
        nanopb
        GIT_REPOSITORY https://github.com/nanopb/nanopb.git
        GIT_TAG        0.4.9.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(nanopb)

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${nanopb_SOURCE_DIR}/extra)
    find_package(Nanopb REQUIRED)
endif()

idf_component_register()

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    nanopb_generate_cpp(TARGET proto RELPATH defs
        defs/demo_bus.proto
    )

    target_link_libraries(${COMPONENT_LIB} INTERFACE proto)
endif()

