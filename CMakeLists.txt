cmake_minimum_required(VERSION 3.15)
project(AsciiScopeChaos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Pull in the external soemdsp targets from your libs folder
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/soemdsp/CMakeLists.txt")
    message(FATAL_ERROR "soemdsp not found at libs/soemdsp/. Did you run the git clone step?")
endif()
add_subdirectory(libs/soemdsp)

# 2. Define the binary executable mapping to your exact file tracks
add_executable(orbiting_chaos
    src/main.cpp
    include/asciiscope/AsciiOscilloscope.hpp
    include/asciiscope/src/AsciiOscilloscope.cpp
)

# 3. Expose your include root directory cleanly
target_include_directories(orbiting_chaos PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 4. Link against the soemdsp math engine
target_link_libraries(orbiting_chaos PRIVATE 
    soemdsp
)