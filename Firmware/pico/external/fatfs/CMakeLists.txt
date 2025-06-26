cmake_minimum_required(VERSION 3.13)
project(fatfs)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/ff15a/source)

add_library(fatfs STATIC 
    # ${SRC_DIR}/diskio.c
    ${SRC_DIR}/ff.c
    ${SRC_DIR}/ffsystem.c
    ${SRC_DIR}/ffunicode.c
)

target_include_directories(fatfs PUBLIC
    ${SRC_DIR}
)