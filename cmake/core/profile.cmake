# @file cmake/core/profile.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

set(APP_NAME blitzar)
set(HEADLESS_NAME blitzar-headless)
set(SERVER_DAEMON_NAME blitzar-server)
set(CLIENT_HOST_NAME blitzar-client)
set(WEB_GATEWAY_NAME blitzar-web-gateway)
set(CLIENT_MODULE_CLI_NAME blitzarClientModuleCli)
set(CLIENT_MODULE_ECHO_NAME blitzarClientModuleEcho)
set(CLIENT_MODULE_GUI_PROXY_NAME blitzarClientModuleGuiProxy)
set(CLIENT_MODULE_QT_INPROC_NAME blitzarClientModuleQtInProc)

option(BLITZAR_BUILD_SERVER_DAEMON "Build dedicated server daemon service" ON)
option(BLITZAR_BUILD_HEADLESS_BINARY "Build headless simulation binary (blitzar-headless)" ON)
option(BLITZAR_BUILD_CLIENT_HOST "Build dynamic client host" ON)
option(BLITZAR_BUILD_WEB_GATEWAY "Build Rust WebSocket/HTTP gateway" ON)
option(BLITZAR_BUILD_CLIENT_MODULES "Build sample dynamic client modules" ON)
option(BLITZAR_BUILD_TESTS "Build deterministic physics regression tests" ON)
option(BLITZAR_PROFILE_LOGS "Print server timings each update" OFF)
option(BLITZAR_STRICT_WARNINGS "Treat project C++ warnings as errors" OFF)
set(BLITZAR_PROFILE "dev" CACHE STRING "Build profile: dev or prod")
set_property(CACHE BLITZAR_PROFILE PROPERTY STRINGS dev prod)
option(
    BLITZAR_SUPPRESS_KNOWN_CUDA_TOOLCHAIN_WARNINGS
    "Suppress known noisy NVCC/EDG warnings from system/third-party headers on Windows toolchains"
    ON
)

string(TOLOWER "${BLITZAR_PROFILE}" BLITZAR_PROFILE)
if(NOT BLITZAR_PROFILE STREQUAL "dev" AND NOT BLITZAR_PROFILE STREQUAL "prod")
    message(FATAL_ERROR "BLITZAR_PROFILE must be one of: dev, prod")
endif()

if(BLITZAR_PROFILE STREQUAL "prod")
    set(BLITZAR_BUILD_CLIENT_HOST OFF CACHE BOOL "Build dynamic client host" FORCE)
    set(BLITZAR_BUILD_CLIENT_MODULES OFF CACHE BOOL "Build sample dynamic client modules" FORCE)
    set(BLITZAR_STRICT_WARNINGS ON CACHE BOOL "Treat project C++ warnings as errors" FORCE)
    set(BLITZAR_PROFILE_LOGS OFF CACHE BOOL "Print server timings each update" FORCE)
    message(STATUS "BLITZAR_PROFILE=prod: dynamic client modules disabled for deterministic critical path")
else()
    message(STATUS "BLITZAR_PROFILE=dev: dynamic client modules allowed")
endif()
