# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/_deps/picotool-src"
  "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/_deps/picotool-build"
  "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/_deps"
  "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/picotool/tmp"
  "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/picotool/src/picotoolBuild-stamp"
  "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/picotool/src"
  "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/picotool/src/picotoolBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/picotool/src/picotoolBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/weebrobot/Desktop/projects/OGX-Mini/Firmware/pico/build/picotool/src/picotoolBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
