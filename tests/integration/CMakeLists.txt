cmake_minimum_required(VERSION 3.24)
project(GravityIntegrationTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GRAVITY_INTEGRATION_ENABLE_COVERAGE "Enable coverage instrumentation for integration tests" OFF)
if(GRAVITY_INTEGRATION_ENABLE_COVERAGE)
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

add_executable(gravityIntegrationConfigArgsGTests
    "${GRAVITY_ROOT_DIR}/tests/config/config_args_regression.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationArgs.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationConfig.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationModes.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/TextParse.cpp"
)
target_include_directories(gravityIntegrationConfigArgsGTests
    PRIVATE
        "${GRAVITY_ROOT_DIR}"
        "${GRAVITY_ROOT_DIR}/include"
)
target_link_libraries(gravityIntegrationConfigArgsGTests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(gravityIntegrationConfigArgsGTests)

find_package(Qt6 COMPONENTS Widgets QUIET)

set(GRAVITY_INTEGRATION_PLATFORM_SOURCES
    "${GRAVITY_ROOT_DIR}/src/cpp/platform/DynamicLibraryCommon.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/platform/PlatformProcessCommon.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/platform/PlatformProcessCommonImpl.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/platform/SocketPlatformCommon.cpp"
)
if(WIN32)
    list(APPEND GRAVITY_INTEGRATION_PLATFORM_SOURCES
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/win/DynamicLibraryWin.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/win/PlatformPathsWin.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/win/PlatformProcessWin.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/win/SocketPlatformWin.cpp"
    )
elseif(APPLE)
    list(APPEND GRAVITY_INTEGRATION_PLATFORM_SOURCES
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/DynamicLibraryPosix.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/PlatformPathsPosix.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/PlatformPathsApple.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/PlatformProcessPosix.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/SocketPlatformPosix.cpp"
    )
elseif(UNIX)
    list(APPEND GRAVITY_INTEGRATION_PLATFORM_SOURCES
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/DynamicLibraryPosix.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/PlatformPathsPosix.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/PlatformProcessPosix.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/platform/posix/SocketPlatformPosix.cpp"
    )
endif()

add_library(gravityIntegrationPlatform STATIC ${GRAVITY_INTEGRATION_PLATFORM_SOURCES})
target_include_directories(gravityIntegrationPlatform
    PUBLIC
        "${GRAVITY_ROOT_DIR}"
        "${GRAVITY_ROOT_DIR}/include"
        "${GRAVITY_ROOT_DIR}/fragments"
)
if(WIN32)
    target_link_libraries(gravityIntegrationPlatform PUBLIC ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(gravityIntegrationPlatform PUBLIC dl)
endif()

add_executable(gravityIntegrationBackendProtocolGTests
    "${GRAVITY_ROOT_DIR}/tests/integration/backend_protocol_regression.cpp"
    "${GRAVITY_ROOT_DIR}/tests/integration/RealBackendHarness.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/BackendClient.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/TextParse.cpp"
)
target_include_directories(gravityIntegrationBackendProtocolGTests
    PRIVATE
        "${GRAVITY_ROOT_DIR}"
        "${GRAVITY_ROOT_DIR}/include"
)
target_link_libraries(gravityIntegrationBackendProtocolGTests
    PRIVATE
        GTest::gtest_main
        gravityIntegrationPlatform
)
gtest_discover_tests(gravityIntegrationBackendProtocolGTests
    PROPERTIES
        LABELS "integration\\;integration_real"
        RESOURCE_LOCK gravity_backend_daemon
        TIMEOUT 30
)

add_executable(gravityIntegrationFrontendBackendBridgeGTests
    "${GRAVITY_ROOT_DIR}/tests/integration/frontend_backend_bridge_regression.cpp"
    "${GRAVITY_ROOT_DIR}/tests/integration/RealBackendHarness.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/FrontendBackendBridge.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/BackendClient.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/TextParse.cpp"
)
target_include_directories(gravityIntegrationFrontendBackendBridgeGTests
    PRIVATE
        "${GRAVITY_ROOT_DIR}"
        "${GRAVITY_ROOT_DIR}/include"
)
target_link_libraries(gravityIntegrationFrontendBackendBridgeGTests
    PRIVATE
        GTest::gtest_main
        gravityIntegrationPlatform
)
gtest_discover_tests(gravityIntegrationFrontendBackendBridgeGTests
    PROPERTIES
        LABELS "integration\\;integration_real"
        RESOURCE_LOCK gravity_backend_daemon
        TIMEOUT 30
)

add_executable(gravityIntegrationFrontendRuntimeContractGTests
    "${GRAVITY_ROOT_DIR}/tests/integration/frontend_runtime_contract_regression.cpp"
    "${GRAVITY_ROOT_DIR}/tests/integration/RealBackendHarness.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/FrontendRuntime.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/FrontendBackendBridge.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/BackendClient.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/FrontendCommon.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationConfig.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationModes.cpp"
    "${GRAVITY_ROOT_DIR}/src/cpp/sim/TextParse.cpp"
)
target_include_directories(gravityIntegrationFrontendRuntimeContractGTests
    PRIVATE
        "${GRAVITY_ROOT_DIR}"
        "${GRAVITY_ROOT_DIR}/include"
)
target_link_libraries(gravityIntegrationFrontendRuntimeContractGTests
    PRIVATE
        GTest::gtest_main
        gravityIntegrationPlatform
)
gtest_discover_tests(gravityIntegrationFrontendRuntimeContractGTests
    PROPERTIES
        LABELS "contract\\;integration_real"
        RESOURCE_LOCK gravity_backend_daemon
        TIMEOUT 30
)

if(TARGET Qt6::Widgets)
    add_executable(gravityIntegrationQtMainWindowIntegrationGTests
        "${GRAVITY_ROOT_DIR}/tests/integration/qt_mainwindow_integration.cpp"
        "${GRAVITY_ROOT_DIR}/tests/integration/RealBackendHarness.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/sim/FrontendRuntime.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/sim/FrontendBackendBridge.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/sim/BackendClient.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/sim/FrontendCommon.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationConfig.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/sim/SimulationModes.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/sim/TextParse.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/ui/EnergyGraphWidget.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/ui/MainWindow.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/ui/MultiViewWidget.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/ui/ParticleView.cpp"
        "${GRAVITY_ROOT_DIR}/src/cpp/ui/QtViewMath.cpp"
    )
    target_include_directories(gravityIntegrationQtMainWindowIntegrationGTests
        PRIVATE
            "${GRAVITY_ROOT_DIR}"
            "${GRAVITY_ROOT_DIR}/include"
    )
    target_link_libraries(gravityIntegrationQtMainWindowIntegrationGTests
        PRIVATE
            GTest::gtest_main
            Qt6::Widgets
            gravityIntegrationPlatform
    )
    gtest_discover_tests(gravityIntegrationQtMainWindowIntegrationGTests
        PROPERTIES
            LABELS "ui_integration\\;integration_real"
            RESOURCE_LOCK gravity_backend_daemon
            TIMEOUT 45
    )
endif()

add_test(
    NAME GravityConfigValidation
    COMMAND ${CMAKE_COMMAND}
        -DGRAVITY_CONFIG_PATH:FILEPATH=${GRAVITY_ROOT_DIR}/simulation.ini
        -P ${GRAVITY_ROOT_DIR}/tests/config/validate_simulation_ini.cmake
)

add_test(
    NAME GravityNoLegacyFrontendTargets
    COMMAND ${CMAKE_COMMAND}
        -DGRAVITY_ROOT_DIR:PATH=${GRAVITY_ROOT_DIR}
        -DGRAVITY_CHECK_BUILD_TARGETS:BOOL=OFF
        -P ${GRAVITY_ROOT_DIR}/tests/integration/validate_no_legacy_frontends.cmake
)

