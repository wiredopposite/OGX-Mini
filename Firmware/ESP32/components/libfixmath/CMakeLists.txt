cmake_minimum_required(VERSION 3.5)

set(LIBFIXMATH_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../external/libfixmath) 

if (NOT EXISTS ${LIBFIXMATH_ROOT})
    message(FATAL_ERROR "External directory not found: ${LIBFIXMATH_ROOT}")
else()
    message(STATUS "Found libfixmath at ${LIBFIXMATH_ROOT}")
endif()

file(GLOB SRCS ${LIBFIXMATH_ROOT}/libfixmath/*.c)

idf_component_register(
    SRCS
        ${SRCS}
    INCLUDE_DIRS
        ${LIBFIXMATH_ROOT}
        ${LIBFIXMATH_ROOT}/libfixmath
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    FIXMATH_FAST_SIN
    FIXMATH_NO_64BIT
    FIXMATH_NO_CACHE
    FIXMATH_NO_HARD_DIVISION
    FIXMATH_NO_OVERFLOW
)