# Platform file for PICO (bare-metal ARM). Overrides Windows-GNU link rules
# so the ARM linker is not given Windows-only flags (--out-implib, --major-image-version).

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS FALSE)
set(CMAKE_EXECUTABLE_SUFFIX .elf)

if(CMAKE_HOST_WIN32)
    include(Platform/WindowsPaths)
else()
    include(Platform/UnixPaths)
endif()

# Bare-metal executable link: no Windows implib or image version flags
# (C, CXX, and ASM - boot_stage2 is ASM-only)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_ASM_LINK_EXECUTABLE "<CMAKE_ASM_COMPILER> <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
