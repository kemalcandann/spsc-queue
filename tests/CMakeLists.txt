add_executable(
    spsc_queue_tests
    spsc_queue_tests.cpp
)

target_include_directories(
    spsc_queue_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(
    spsc_queue_tests
    PRIVATE
        spsc_queue
        project_warnings
        Threads::Threads
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        spsc_queue_tests
        PRIVATE
            -O2
            -g
    )
endif()

add_test(
    NAME spsc_queue_tests
    COMMAND spsc_queue_tests
)