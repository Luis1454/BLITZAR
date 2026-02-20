cmake_minimum_required(VERSION 3.24)
project(GravityHostedTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GRAVITY_HOSTED_ENABLE_COVERAGE "Enable coverage instrumentation for hosted tests" OFF)
if(GRAVITY_HOSTED_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    endif()
endif()

get_filename_component(GRAVITY_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

enable_testing()

find_package(GTest CONFIG QUIET)
if(NOT TARGET GTest::gtest_main)
    include(FetchContent)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(gravityHostedConfigArgsGTests
    "${GRAVITY_ROOT_DIR}/tests/config/config_args_regression.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationArgs.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationConfig.cpp"
)
target_include_directories(gravityHostedConfigArgsGTests
    PRIVATE
        "${GRAVITY_ROOT_DIR}"
        "${GRAVITY_ROOT_DIR}/include"
)
target_link_libraries(gravityHostedConfigArgsGTests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(gravityHostedConfigArgsGTests)

add_test(
    NAME GravityConfigValidation
    COMMAND ${CMAKE_COMMAND}
        -DGRAVITY_CONFIG_PATH:FILEPATH=${GRAVITY_ROOT_DIR}/simulation.ini
        -P ${GRAVITY_ROOT_DIR}/tests/config/validate_simulation_ini.cmake
)
