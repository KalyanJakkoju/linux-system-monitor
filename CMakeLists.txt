cmake_minimum_required(VERSION 3.16)
project(linux_system_monitor
    VERSION 1.0.0
    DESCRIPTION "Linux System Resource Monitor and CPU Scheduling Simulator in C++17"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and options
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wformat=2
        -Wshadow
        -Wnon-virtual-dtor
    )
endif()

# Core static library
add_library(sysmon_core STATIC
    src/cpu_monitor.cpp
    src/memory_monitor.cpp
    src/process_monitor.cpp
    src/system_monitor.cpp
    src/process_model.cpp
    src/scheduler.cpp
    src/cli.cpp
)

target_include_directories(sysmon_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main command-line application
add_executable(system_monitor
    src/main.cpp
)

target_link_libraries(system_monitor PRIVATE
    sysmon_core
)

# Unit test executable
add_executable(run_unit_tests
    tests/test_main.cpp
    tests/test_cpu_metrics.cpp
    tests/test_memory_metrics.cpp
    tests/test_process_parsing.cpp
    tests/test_scheduler.cpp
)

target_include_directories(run_unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(run_unit_tests PRIVATE
    sysmon_core
)

enable_testing()
add_test(NAME AllUnitTests COMMAND run_unit_tests)
