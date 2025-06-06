cmake_minimum_required(VERSION 3.13)
project(g726)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/g726/src)

add_library(g726 STATIC 
    ${SRC_DIR}/g72x.c
    ${SRC_DIR}/g711.c
    ${SRC_DIR}/g726_16.c
    ${SRC_DIR}/g726_24.c
    ${SRC_DIR}/g726_32.c
    ${SRC_DIR}/g726_40.c
    ${SRC_DIR}/g726.c
)

target_include_directories(g726 PUBLIC
    ${SRC_DIR}
)