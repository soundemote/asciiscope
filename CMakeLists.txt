cmake_minimum_required(VERSION 3.28)
project(Asciiscope LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/soemdsp/CMakeLists.txt")
    message(FATAL_ERROR "soemdsp not found at libs/soemdsp/. Run: git submodule update --init --recursive")
endif()

add_subdirectory(libs/soemdsp)

add_executable(asciiscope
    src/main.cpp
    src/AnimationScene.cpp
    src/ConsoleRenderer.cpp
    src/DemoSignalInput.cpp
    src/SignalInput.cpp
)

target_include_directories(asciiscope PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(asciiscope PRIVATE
    soemdsp::soemdsp
)
