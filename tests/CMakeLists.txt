# File: tests/CMakeLists.txt
# Purpose: Verification coverage for the BLITZAR quality gate.

cmake_minimum_required(VERSION 3.24)

get_filename_component(GRAVITY_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include("${GRAVITY_ROOT_DIR}/cmake/win.cmake")
    project(GravityIntegrationTests LANGUAGES CXX)
endif()
if(COMMAND gravity_populate_windows_toolchain_hints)
    gravity_populate_windows_toolchain_hints()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GRAVITY_PROFILE "${GRAVITY_PROFILE}" CACHE STRING "Build profile (dev|prod) for tests lane alignment")
if(NOT GRAVITY_PROFILE)
    set(GRAVITY_PROFILE "dev" CACHE STRING "Build profile (dev|prod) for tests lane alignment" FORCE)
endif()
if(NOT GRAVITY_PROFILE MATCHES "^(dev|prod)$")
    message(FATAL_ERROR "GRAVITY_PROFILE must be one of: dev, prod (got '${GRAVITY_PROFILE}')")
endif()
message(STATUS "Tests profile: GRAVITY_PROFILE=${GRAVITY_PROFILE}")

option(GRAVITY_INTEGRATION_ENABLE_COVERAGE "Enable coverage instrumentation for integration tests" OFF)
option(GRAVITY_INTEGRATION_STRICT_WARNINGS "Treat C++ warnings as errors for integration tests" OFF)
option(
    GRAVITY_INTEGRATION_ENABLE_SANITIZERS
    "Enable address+undefined sanitizers for integration tests (GNU/Clang only)"
    OFF
)
if(GRAVITY_INTEGRATION_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)
endif()

set(GRAVITY_TEST_INCLUDE_DIRS
    "${GRAVITY_ROOT_DIR}"
    "${GRAVITY_ROOT_DIR}/engine/include"
    "${GRAVITY_ROOT_DIR}/runtime/include"
    "${GRAVITY_ROOT_DIR}/engine/include/physics"
    "${GRAVITY_ROOT_DIR}/modules/qt/include"
    "${GRAVITY_ROOT_DIR}/engine/src/physics/cuda/fragments"
)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/setup.cmake")
include("${GRAVITY_ROOT_DIR}/cmake/qt_runtime_deploy.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/targets.cmake")
