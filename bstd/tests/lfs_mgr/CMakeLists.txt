cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lfs_mgr_test)

target_sources(app PRIVATE
    src/test_lfs_mgr.c
    ../../src/lfs_mgr/src/lfs_mgr.c
)

target_include_directories(app PRIVATE
    ../../src/lfs_mgr/inc
)

# Include test metadata
zephyr_test_metadata(${CMAKE_CURRENT_SOURCE_DIR}/testcase.yml)

