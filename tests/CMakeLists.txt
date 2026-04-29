# @file tests/CMakeLists.txt
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

cmake_minimum_required(VERSION 3.24)

get_filename_component(BLITZAR_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    include("${BLITZAR_ROOT_DIR}/cmake/win.cmake")
    project(GravityIntegrationTests LANGUAGES CXX)
endif()
if(COMMAND BLITZAR_populate_windows_toolchain_hints)
    BLITZAR_populate_windows_toolchain_hints()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLITZAR_PROFILE "${BLITZAR_PROFILE}" CACHE STRING "Build profile (dev|prod) for tests lane alignment")
if(NOT BLITZAR_PROFILE)
    set(BLITZAR_PROFILE "dev" CACHE STRING "Build profile (dev|prod) for tests lane alignment" FORCE)
endif()
if(NOT BLITZAR_PROFILE MATCHES "^(dev|prod)$")
    message(FATAL_ERROR "BLITZAR_PROFILE must be one of: dev, prod (got '${BLITZAR_PROFILE}')")
endif()
message(STATUS "Tests profile: BLITZAR_PROFILE=${BLITZAR_PROFILE}")

option(BLITZAR_INTEGRATION_ENABLE_COVERAGE "Enable coverage instrumentation for integration tests" OFF)
option(BLITZAR_INTEGRATION_STRICT_WARNINGS "Treat C++ warnings as errors for integration tests" OFF)
option(
    BLITZAR_INTEGRATION_ENABLE_SANITIZERS
    "Enable address+undefined sanitizers for integration tests (GNU/Clang only)"
    OFF
)
if(BLITZAR_INTEGRATION_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)
endif()

set(BLITZAR_TEST_INCLUDE_DIRS
    "${BLITZAR_ROOT_DIR}"
    "${BLITZAR_ROOT_DIR}/engine/include"
    "${BLITZAR_ROOT_DIR}/runtime/include"
    "${BLITZAR_ROOT_DIR}/engine/include/physics"
    "${BLITZAR_ROOT_DIR}/modules/qt/include"
    "${BLITZAR_ROOT_DIR}/engine/src/physics/cuda/fragments"
)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/setup.cmake")
include("${BLITZAR_ROOT_DIR}/cmake/qt_runtime_deploy.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/targets.cmake")
