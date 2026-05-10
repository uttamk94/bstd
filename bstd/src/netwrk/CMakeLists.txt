
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
zephyr_library_sources(
    src/netwrk.c
    src/netwrk_main.c
    src/wifi_http.c
    src/netwrk_task.c
    )