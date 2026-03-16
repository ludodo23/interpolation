cmake_minimum_required(VERSION 3.16)
project(PackageTest CXX)

find_package(interpollation CONFIG REQUIRED)

add_executable(example src/example.cpp)
target_link_libraries(example interpollation::interpollation)