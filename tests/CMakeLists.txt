# -------------------------
# DEPENDENCIES
# -------------------------
find_package(Catch2 REQUIRED)

# -------------------------
# TARGET
# -------------------------
add_executable(test_interpolation
    test_linear.cpp
    test_hermite.cpp
    test_catmull_rom.cpp
)

# Plus besoin de target_include_directories : on linke la target INTERFACE
target_link_libraries(test_interpolation PRIVATE
    interpolation               # ← hérite de include/ via INTERFACE
    Catch2::Catch2WithMain
)

# -------------------------
# FLAGS PROFILING / DEBUG
# -------------------------
if(ENABLE_ASAN)
    target_compile_options(test_interpolation PRIVATE -fsanitize=address -g)
    target_link_options(test_interpolation PRIVATE -fsanitize=address)
endif()

if(ENABLE_COVERAGE)
    target_compile_options(test_interpolation PRIVATE --coverage -O0 -g)
    target_link_options(test_interpolation PRIVATE --coverage)
endif()

if(ENABLE_PERF)
    target_compile_options(test_interpolation PRIVATE -g -fno-omit-frame-pointer)
endif()

# -------------------------
# TRACY
# -------------------------
if(ENABLE_TRACY)
    add_definitions(-DTRACY_ENABLE)
    include(FetchContent)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(tracy)
    target_link_libraries(test_interpolation PRIVATE Tracy::TracyClient)
endif()

# -------------------------
# TESTS (CTest + Catch2)
# -------------------------
include(CTest)
include(Catch)
catch_discover_tests(test_interpolation)