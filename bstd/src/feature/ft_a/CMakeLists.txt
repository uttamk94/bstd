
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
zephyr_library_sources(src/ft_a.c)

if (CONFIG_ENCODER_ENABLE) 
    target_link_libraries(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/my_encoder.o
    )
endif()
