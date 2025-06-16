cmake_minimum_required(VERSION 3.5)

set(LIBFIXMATH_ROOT ${CMAKE_CURRENT_LIST_DIR}/libfixmath) 

if(NOT EXISTS ${LIBFIXMATH_ROOT}/libfixmath)
    message(FATAL_ERROR "libfixmath directory not found at ${LIBFIXMATH_ROOT}. Please ensure the library is present.")
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