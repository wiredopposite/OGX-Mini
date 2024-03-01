add_library(CRC32 
src/CRC32.cpp
src/CRC32.h
)

target_include_directories(CRC32 INTERFACE 
src
)