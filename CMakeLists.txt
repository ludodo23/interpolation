cmake_minimum_required(VERSION 3.16)
project(interpolation CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -------------------------
# OPTIONS (pilotées par Conan ou manuellement)
# -------------------------
option(ENABLE_TRACY    "Enable Tracy profiler"    OFF)
option(ENABLE_ASAN     "Enable AddressSanitizer"  OFF)
option(ENABLE_COVERAGE "Enable coverage"          OFF)
option(ENABLE_PERF     "Enable perf-friendly flags" OFF)

# -------------------------
# LIB HEADER-ONLY
# -------------------------
add_library(interpolation INTERFACE)
target_include_directories(interpolation INTERFACE include/)

# -------------------------
# TESTS
# -------------------------
enable_testing()
add_subdirectory(tests)