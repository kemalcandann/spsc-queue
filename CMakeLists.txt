cmake_minimum_required(VERSION 3.20)

project(
    spsc_low_latency
    VERSION 1.0
    DESCRIPTION "SPSC lock-free queue and low-latency benchmark"
    LANGUAGES CXX
)

# ------------------------------------------------------------
# Global C++ configuration
# ------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# Default build type for single-config generators
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Build type"
        FORCE
    )

    set_property(
        CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel
    )
endif()

# ------------------------------------------------------------
# Common compiler warnings
# ------------------------------------------------------------

add_library(project_warnings INTERFACE)

# target_compile_options(
#     project_warnings
#     INTERFACE
#         -Wall
#         -Wextra
#         -Wpedantic
#         -Wconversion
#         -Wshadow
#         -Wundef
#         -Wdouble-promotion
# )

# ------------------------------------------------------------
# Platform checks
# ------------------------------------------------------------

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR
        "This project currently requires Linux."
    )
endif()

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    message(FATAL_ERROR
        "This benchmark currently requires x86-64."
    )
endif()

# ------------------------------------------------------------
# CPU / TSC library
# ------------------------------------------------------------

add_library(spsc_platform STATIC
    src/cpu_affinity.cpp
    src/tsc.cpp
)

target_include_directories(
    spsc_platform
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    spsc_platform
    PUBLIC
        project_warnings
        Threads::Threads
)

# ------------------------------------------------------------
# SPSC queue library
#
# Header-only because SPSCQueue is a template.
# ------------------------------------------------------------

add_library(spsc_queue INTERFACE)

target_include_directories(
    spsc_queue
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    spsc_queue
    INTERFACE
        project_warnings
)

# ------------------------------------------------------------
# Benchmark executable
# ------------------------------------------------------------

add_executable(
    spsc_benchmark

    benchmark/main.cpp

    benchmark/latency_benchmark.cpp
    benchmark/throughput_benchmark.cpp
)

target_include_directories(
    spsc_benchmark
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/benchmark
)

target_link_libraries(
    spsc_benchmark
    PRIVATE
        spsc_queue
        spsc_platform
        project_warnings
        Threads::Threads
)

# ------------------------------------------------------------
# Optimisation flags
# ------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        spsc_benchmark
        PRIVATE
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:-march=native>
            $<$<CONFIG:Release>:-DNDEBUG>
    )
endif()


# ------------------------------------------------------------
# Optional tests
# ------------------------------------------------------------

option(
    BUILD_TESTS
    "Build correctness tests"
    ON
)

if(BUILD_TESTS)
    enable_testing()

    add_subdirectory(tests)
endif()

# ------------------------------------------------------------
# Useful build information
# ------------------------------------------------------------

message(STATUS "==========================================")
message(STATUS "SPSC Low-Latency Project")
message(STATUS "==========================================")
message(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++       : C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler   : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "System     : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor  : ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "==========================================")