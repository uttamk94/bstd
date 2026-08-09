zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
zephyr_library_sources(src/sensor.c)
zephyr_library_sources(src/sensor_gpio.c)