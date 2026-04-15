
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
zephyr_library_sources(
    src/ble.c
    src/adv.c
    src/data_svc.c
    src/conn.c
    src/llog_svc.c
    src/ble_task.c
    )