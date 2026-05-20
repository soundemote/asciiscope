cmake_minimum_required(VERSION 3.15)
project(AsciiScopeChaos LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Inspect and target the newly formed libs subfolder map
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/soemdsp/CMakeLists.txt")
    message(FATAL_ERROR "soemdsp not found at libs/soemdsp/. Please ensure the SourceTree submodule step completed.")
endif()
add_subdirectory(libs/soemdsp)

# 2. Map the application executable binary components
add_executable(orbiting_chaos
    src/main.cpp
    include/asciiscope/AsciiOscilloscope.hpp
    include/asciiscope/src/AsciiOscilloscope.cpp
)

# 3. Expose your custom include tree path parameters
target_include_directories(orbiting_chaos PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 4. Link cleanly against the raw soemdsp engine targets
target_link_libraries(orbiting_chaos PRIVATE 
    soemdsp
)