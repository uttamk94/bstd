
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
zephyr_library_sources(
    src/commu.c
    src/data_commu.c
    src/clnt_a.c
    src/clnt_b.c
    src/ca_cpa_msg.c
    )