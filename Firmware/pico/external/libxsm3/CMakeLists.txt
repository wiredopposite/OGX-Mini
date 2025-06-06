cmake_minimum_required(VERSION 3.13)
project(libxsm3)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/libxsm3)

file(GLOB_RECURSE LIBXSM3_SOURCES "${SRC_DIR}/*.c")

add_library(libxsm3 STATIC ${LIBXSM3_SOURCES})

target_include_directories(libxsm3 PUBLIC
    ${SRC_DIR}
)