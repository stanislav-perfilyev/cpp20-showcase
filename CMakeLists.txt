cmake_minimum_required(VERSION 3.25)
project(cpp20_showcase VERSION 1.0.0 LANGUAGES CXX)

# ── ccache (faster rebuilds) ──────────────────────────────────────────────────
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}")
endif()

# ── C++ standard ───────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Export compile commands (clangd, clang-tidy) ───────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Compiler warnings (senior standard) ───────────────────────
add_compile_options(
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-function -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 /permissive->
)

# ── FetchContent ───────────────────────────────────────────────
include(FetchContent)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK   ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.8.3
    GIT_SHALLOW    TRUE
)
set(BENCHMARK_ENABLE_TESTING          OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL          OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES   ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# ── Testing ────────────────────────────────────────────────────
enable_testing()
include(GoogleTest)

# ── Subdirectories ─────────────────────────────────────────────
add_subdirectory(01_concepts)
add_subdirectory(02_ranges)
add_subdirectory(03_coroutines)
add_subdirectory(04_modules)
add_subdirectory(05_jthread)
add_subdirectory(06_lockfree)
add_subdirectory(07_atomic_model)
add_subdirectory(08_format)
add_subdirectory(09_mdspan)
add_subdirectory(benchmarks)
