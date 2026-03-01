set(APP_NAME myApp)
set(HEADLESS_NAME myAppHeadless)
set(BACKEND_DAEMON_NAME myAppBackend)
set(MODULE_HOST_NAME myAppModuleHost)
set(FRONTEND_MODULE_CLI_NAME gravityFrontendModuleCli)
set(FRONTEND_MODULE_ECHO_NAME gravityFrontendModuleEcho)
set(FRONTEND_MODULE_GUI_PROXY_NAME gravityFrontendModuleGuiProxy)
set(FRONTEND_MODULE_QT_INPROC_NAME gravityFrontendModuleQtInProc)

option(GRAVITY_BUILD_BACKEND_DAEMON "Build dedicated backend daemon service" ON)
option(GRAVITY_BUILD_HEADLESS_BINARY "Build headless simulation binary (myAppHeadless)" ON)
option(GRAVITY_BUILD_FRONTEND_MODULE_HOST "Build dynamic module host frontend" ON)
option(GRAVITY_BUILD_FRONTEND_MODULES "Build sample dynamic frontend modules" ON)
option(GRAVITY_BUILD_TESTS "Build deterministic physics regression tests" ON)
option(GRAVITY_PROFILE_LOGS "Print backend timings each update" OFF)
option(GRAVITY_STRICT_WARNINGS "Treat project C++ warnings as errors" OFF)
set(GRAVITY_PROFILE "dev" CACHE STRING "Build profile: dev or prod")
set_property(CACHE GRAVITY_PROFILE PROPERTY STRINGS dev prod)
option(
    GRAVITY_SUPPRESS_KNOWN_CUDA_TOOLCHAIN_WARNINGS
    "Suppress known noisy NVCC/EDG warnings from system/third-party headers on Windows toolchains"
    ON
)

string(TOLOWER "${GRAVITY_PROFILE}" GRAVITY_PROFILE)
if(NOT GRAVITY_PROFILE STREQUAL "dev" AND NOT GRAVITY_PROFILE STREQUAL "prod")
    message(FATAL_ERROR "GRAVITY_PROFILE must be one of: dev, prod")
endif()

if(GRAVITY_PROFILE STREQUAL "prod")
    set(GRAVITY_BUILD_FRONTEND_MODULE_HOST OFF CACHE BOOL "Build dynamic module host frontend" FORCE)
    set(GRAVITY_BUILD_FRONTEND_MODULES OFF CACHE BOOL "Build sample dynamic frontend modules" FORCE)
    set(GRAVITY_STRICT_WARNINGS ON CACHE BOOL "Treat project C++ warnings as errors" FORCE)
    set(GRAVITY_PROFILE_LOGS OFF CACHE BOOL "Print backend timings each update" FORCE)
    message(STATUS "GRAVITY_PROFILE=prod: dynamic frontend modules disabled for deterministic critical path")
else()
    message(STATUS "GRAVITY_PROFILE=dev: dynamic frontend modules allowed")
endif()
