cmake_minimum_required(VERSION 3.16)
project(test_interpolation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Catch2 REQUIRED)

add_executable(test_interpolation
    test_linear.cpp
    test_hermite.cpp
    test_catmull_rom.cpp
)

target_include_directories(test_interpolation PRIVATE ../include)
target_link_libraries(test_interpolation PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(test_interpolation)
