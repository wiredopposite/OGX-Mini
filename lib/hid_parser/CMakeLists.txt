add_library(hid_parser 
src/hid_parser.c
src/hid_parser.h
)

target_include_directories(hid_parser INTERFACE 
src
)