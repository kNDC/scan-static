cmake_minimum_required(VERSION 3.30)
project(scan VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(target scan)

add_library(${target} INTERFACE)

target_include_directories(${target} INTERFACE include/)

set(test_target scan_tests)

add_executable(${test_target} tests/main.cpp)
target_link_libraries(${test_target} PRIVATE ${target})
