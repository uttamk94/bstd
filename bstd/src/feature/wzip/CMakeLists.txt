
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

zephyr_library_sources(
    src/compression.c
    src/golomb_rice.c
    src/tlv_container.c
    src/pattern_matcher.c
)
