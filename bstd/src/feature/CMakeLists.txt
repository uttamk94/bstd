
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ft_a/inc)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ft_b/inc)
zephyr_library_sources(src/feature.c)
zephyr_library_sources(src/ft_task.c)

add_subdirectory_ifdef(CONFIG_FT_A_ENABLE ft_a)
add_subdirectory_ifdef(CONFIG_FT_A_ENABLE ft_b)
