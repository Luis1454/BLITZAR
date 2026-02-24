cmake_minimum_required(VERSION 3.24)

option(
    GRAVITY_WINDOWS_AUTODETECT_TOOLCHAIN
    "Autodetect MSVC/Windows SDK paths and mutate PATH/INCLUDE/LIB during configure (Windows only)"
    ON
)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain file")
    endif()
endif()

if(WIN32)
    set(VCPKG_APPLOCAL_DEPS OFF)
endif()

if(WIN32 AND GRAVITY_WINDOWS_AUTODETECT_TOOLCHAIN)
    file(GLOB _msvc_roots "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/*")
    if(_msvc_roots)
        list(SORT _msvc_roots COMPARE NATURAL ORDER DESCENDING)
        list(GET _msvc_roots 0 _msvc_root)
        set(_msvc_host_bin "${_msvc_root}/bin/Hostx64/x64")
        set(GRAVITY_MSVC_LIB_DIR "${_msvc_root}/lib/x64")
        set(GRAVITY_MSVC_INCLUDE_DIR "${_msvc_root}/include")
        set(ENV{PATH} "${_msvc_host_bin};$ENV{PATH}")
        if(EXISTS "${_msvc_host_bin}/cl.exe" AND NOT DEFINED CMAKE_CUDA_HOST_COMPILER)
            set(CMAKE_CUDA_HOST_COMPILER "${_msvc_host_bin}/cl.exe" CACHE FILEPATH "MSVC host compiler for nvcc" FORCE)
        endif()
        if(EXISTS "${_msvc_root}/include")
            set(ENV{INCLUDE} "${_msvc_root}/include;$ENV{INCLUDE}")
        endif()
        if(EXISTS "${_msvc_root}/lib/x64")
            set(ENV{LIB} "${_msvc_root}/lib/x64;$ENV{LIB}")
            set(ENV{LIBPATH} "${_msvc_root}/lib/x64;$ENV{LIBPATH}")
        endif()
    endif()

    file(GLOB _winsdk_x64_bins "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
    if(_winsdk_x64_bins)
        list(SORT _winsdk_x64_bins COMPARE NATURAL ORDER DESCENDING)
        list(GET _winsdk_x64_bins 0 _winsdk_x64_bin)
        set(ENV{PATH} "${_winsdk_x64_bin};$ENV{PATH}")
        if(EXISTS "${_winsdk_x64_bin}/rc.exe" AND NOT DEFINED CMAKE_RC_COMPILER)
            set(CMAKE_RC_COMPILER "${_winsdk_x64_bin}/rc.exe" CACHE FILEPATH "Windows SDK resource compiler" FORCE)
        endif()
        if(EXISTS "${_winsdk_x64_bin}/mt.exe" AND NOT DEFINED CMAKE_MT)
            set(CMAKE_MT "${_winsdk_x64_bin}/mt.exe" CACHE FILEPATH "Windows SDK manifest tool" FORCE)
        endif()
    endif()

    file(GLOB _winsdk_include_versions "C:/Program Files (x86)/Windows Kits/10/Include/*")
    if(_winsdk_include_versions)
        list(SORT _winsdk_include_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET _winsdk_include_versions 0 _winsdk_include_root)
        set(GRAVITY_WINSDK_INCLUDE_UCRT "${_winsdk_include_root}/ucrt")
        set(GRAVITY_WINSDK_INCLUDE_UM "${_winsdk_include_root}/um")
        set(GRAVITY_WINSDK_INCLUDE_SHARED "${_winsdk_include_root}/shared")
        set(GRAVITY_WINSDK_INCLUDE_WINRT "${_winsdk_include_root}/winrt")
        set(GRAVITY_WINSDK_INCLUDE_CPPWINRT "${_winsdk_include_root}/cppwinrt")
        set(ENV{INCLUDE} "${_winsdk_include_root}/ucrt;${_winsdk_include_root}/um;${_winsdk_include_root}/shared;${_winsdk_include_root}/winrt;${_winsdk_include_root}/cppwinrt;$ENV{INCLUDE}")
    endif()

    file(GLOB _winsdk_lib_versions "C:/Program Files (x86)/Windows Kits/10/Lib/*")
    if(_winsdk_lib_versions)
        list(SORT _winsdk_lib_versions COMPARE NATURAL ORDER DESCENDING)
        list(GET _winsdk_lib_versions 0 _winsdk_lib_root)
        set(GRAVITY_WINSDK_UCRT_LIB_DIR "${_winsdk_lib_root}/ucrt/x64")
        set(GRAVITY_WINSDK_UM_LIB_DIR "${_winsdk_lib_root}/um/x64")
        set(ENV{LIB} "${_winsdk_lib_root}/ucrt/x64;${_winsdk_lib_root}/um/x64;$ENV{LIB}")
        set(ENV{LIBPATH} "${_winsdk_lib_root}/ucrt/x64;${_winsdk_lib_root}/um/x64;$ENV{LIBPATH}")
    endif()
endif()

project(CUDA_GRAVITY_SIMULATION LANGUAGES CUDA CXX)

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
option(
    GRAVITY_SUPPRESS_KNOWN_CUDA_TOOLCHAIN_WARNINGS
    "Suppress known noisy NVCC/EDG warnings from system/third-party headers on Windows toolchains"
    ON
)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

find_package(CUDAToolkit REQUIRED)

function(gravity_populate_windows_toolchain_hints)
    if(NOT WIN32)
        return()
    endif()

    if((NOT DEFINED GRAVITY_MSVC_INCLUDE_DIR OR NOT EXISTS "${GRAVITY_MSVC_INCLUDE_DIR}")
       OR (NOT DEFINED GRAVITY_MSVC_LIB_DIR OR NOT EXISTS "${GRAVITY_MSVC_LIB_DIR}"))
        if(DEFINED CMAKE_CXX_COMPILER AND EXISTS "${CMAKE_CXX_COMPILER}")
            get_filename_component(_gravity_cxx_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
            get_filename_component(_gravity_msvc_host_dir "${_gravity_cxx_compiler_dir}" DIRECTORY)
            get_filename_component(_gravity_msvc_bin_dir "${_gravity_msvc_host_dir}" DIRECTORY)
            get_filename_component(_gravity_msvc_root "${_gravity_msvc_bin_dir}" DIRECTORY)
            if(EXISTS "${_gravity_msvc_root}/include")
                set(GRAVITY_MSVC_INCLUDE_DIR "${_gravity_msvc_root}/include" CACHE PATH "MSVC include directory" FORCE)
            endif()
            if(EXISTS "${_gravity_msvc_root}/lib/x64")
                set(GRAVITY_MSVC_LIB_DIR "${_gravity_msvc_root}/lib/x64" CACHE PATH "MSVC x64 library directory" FORCE)
            endif()
        endif()
    endif()

    if((NOT DEFINED GRAVITY_WINSDK_INCLUDE_UCRT OR NOT EXISTS "${GRAVITY_WINSDK_INCLUDE_UCRT}")
       OR (NOT DEFINED GRAVITY_WINSDK_UM_LIB_DIR OR NOT EXISTS "${GRAVITY_WINSDK_UM_LIB_DIR}"))
        file(GLOB _gravity_winsdk_include_versions "C:/Program Files (x86)/Windows Kits/10/Include/*")
        if(_gravity_winsdk_include_versions)
            list(SORT _gravity_winsdk_include_versions COMPARE NATURAL ORDER DESCENDING)
            list(GET _gravity_winsdk_include_versions 0 _gravity_winsdk_include_root)
            if(EXISTS "${_gravity_winsdk_include_root}/ucrt")
                set(GRAVITY_WINSDK_INCLUDE_UCRT "${_gravity_winsdk_include_root}/ucrt" CACHE PATH "Windows SDK UCRT include dir" FORCE)
            endif()
            if(EXISTS "${_gravity_winsdk_include_root}/um")
                set(GRAVITY_WINSDK_INCLUDE_UM "${_gravity_winsdk_include_root}/um" CACHE PATH "Windows SDK UM include dir" FORCE)
            endif()
            if(EXISTS "${_gravity_winsdk_include_root}/shared")
                set(GRAVITY_WINSDK_INCLUDE_SHARED "${_gravity_winsdk_include_root}/shared" CACHE PATH "Windows SDK shared include dir" FORCE)
            endif()
            if(EXISTS "${_gravity_winsdk_include_root}/winrt")
                set(GRAVITY_WINSDK_INCLUDE_WINRT "${_gravity_winsdk_include_root}/winrt" CACHE PATH "Windows SDK WinRT include dir" FORCE)
            endif()
            if(EXISTS "${_gravity_winsdk_include_root}/cppwinrt")
                set(GRAVITY_WINSDK_INCLUDE_CPPWINRT "${_gravity_winsdk_include_root}/cppwinrt" CACHE PATH "Windows SDK C++/WinRT include dir" FORCE)
            endif()
        endif()

        file(GLOB _gravity_winsdk_lib_versions "C:/Program Files (x86)/Windows Kits/10/Lib/*")
        if(_gravity_winsdk_lib_versions)
            list(SORT _gravity_winsdk_lib_versions COMPARE NATURAL ORDER DESCENDING)
            list(GET _gravity_winsdk_lib_versions 0 _gravity_winsdk_lib_root)
            if(EXISTS "${_gravity_winsdk_lib_root}/ucrt/x64")
                set(GRAVITY_WINSDK_UCRT_LIB_DIR "${_gravity_winsdk_lib_root}/ucrt/x64" CACHE PATH "Windows SDK UCRT x64 lib dir" FORCE)
            endif()
            if(EXISTS "${_gravity_winsdk_lib_root}/um/x64")
                set(GRAVITY_WINSDK_UM_LIB_DIR "${_gravity_winsdk_lib_root}/um/x64" CACHE PATH "Windows SDK UM x64 lib dir" FORCE)
            endif()
        endif()
    endif()

    if(MSVC AND (NOT DEFINED GRAVITY_MSVC_INCLUDE_DIR OR NOT EXISTS "${GRAVITY_MSVC_INCLUDE_DIR}"))
        message(FATAL_ERROR
            "MSVC standard include directory could not be resolved. "
            "Set GRAVITY_MSVC_INCLUDE_DIR manually or run configure from a VS Developer prompt.")
    endif()
endfunction()

gravity_populate_windows_toolchain_hints()

set(GRAVITY_BACKEND_SOURCES
    src/cpp/sim/SimulationArgs.cpp
    src/cpp/sim/SimulationBackend.cpp
    src/cpp/sim/SimulationConfig.cpp
    src/cpp/sim/SimulationModes.cpp
    src/cpp/sim/TextParse.cpp
    src/cuda/core/ParticleSystem.cu
)

function(gravity_collect_existing_paths out_var)
    set(_result "")
    foreach(_path IN LISTS ARGN)
        if(NOT "${_path}" STREQUAL "" AND EXISTS "${_path}")
            list(APPEND _result "${_path}")
        endif()
    endforeach()
    set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()

function(configure_gravity_target target_name)
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/fragments
    )

    if(WIN32)
        gravity_collect_existing_paths(
            _gravity_windows_system_includes
            "${GRAVITY_MSVC_INCLUDE_DIR}"
            "${GRAVITY_WINSDK_INCLUDE_UCRT}"
            "${GRAVITY_WINSDK_INCLUDE_UM}"
            "${GRAVITY_WINSDK_INCLUDE_SHARED}"
            "${GRAVITY_WINSDK_INCLUDE_WINRT}"
            "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}"
        )
        if(_gravity_windows_system_includes)
            target_include_directories(${target_name}
                SYSTEM PRIVATE
                    ${_gravity_windows_system_includes}
            )
        endif()
    endif()

    set_target_properties(${target_name}
        PROPERTIES
            CUDA_SEPARABLE_COMPILATION ON
    )

    target_link_libraries(${target_name}
        PRIVATE
            CUDA::cudart
    )
    if(TARGET gravityPlatform)
        target_link_libraries(${target_name}
            PRIVATE
                gravityPlatform
        )
    endif()

    if(GRAVITY_PROFILE_LOGS)
        target_compile_definitions(${target_name}
            PRIVATE
                GRAVITY_PROFILE_LOGS=1
        )
    endif()

    if(MSVC)
        target_compile_options(${target_name}
            PRIVATE
                $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:__cplusplus>
        )
        if(GRAVITY_SUPPRESS_KNOWN_CUDA_TOOLCHAIN_WARNINGS)
            target_compile_options(${target_name}
                PRIVATE
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=128>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=186>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=1388>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=1394>
            )
        endif()
    endif()

    if(WIN32)
        gravity_collect_existing_paths(
            _gravity_windows_link_dirs
            "${GRAVITY_MSVC_LIB_DIR}"
            "${GRAVITY_WINSDK_UCRT_LIB_DIR}"
            "${GRAVITY_WINSDK_UM_LIB_DIR}"
        )
        if(_gravity_windows_link_dirs)
            target_link_directories(${target_name}
                PRIVATE
                    ${_gravity_windows_link_dirs}
            )
        endif()
    endif()
endfunction()

function(configure_gravity_cpp_target target_name)
    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/fragments
    )

    if(WIN32)
        gravity_collect_existing_paths(
            _gravity_windows_system_includes
            "${GRAVITY_MSVC_INCLUDE_DIR}"
            "${GRAVITY_WINSDK_INCLUDE_UCRT}"
            "${GRAVITY_WINSDK_INCLUDE_UM}"
            "${GRAVITY_WINSDK_INCLUDE_SHARED}"
            "${GRAVITY_WINSDK_INCLUDE_WINRT}"
            "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}"
        )
        if(_gravity_windows_system_includes)
            target_include_directories(${target_name}
                SYSTEM PRIVATE
                    ${_gravity_windows_system_includes}
            )
        endif()
        gravity_collect_existing_paths(
            _gravity_windows_link_dirs
            "${GRAVITY_MSVC_LIB_DIR}"
            "${GRAVITY_WINSDK_UCRT_LIB_DIR}"
            "${GRAVITY_WINSDK_UM_LIB_DIR}"
        )
        if(_gravity_windows_link_dirs)
            target_link_directories(${target_name}
                PRIVATE
                    ${_gravity_windows_link_dirs}
            )
        endif()
    endif()

    if(TARGET gravityPlatform AND NOT "${target_name}" STREQUAL "gravityPlatform")
        target_link_libraries(${target_name}
            PRIVATE
                gravityPlatform
        )
    endif()
endfunction()

set(GRAVITY_PLATFORM_SOURCES
    src/cpp/platform/DynamicLibraryCommon.cpp
    src/cpp/platform/PlatformProcessCommon.cpp
    src/cpp/platform/PlatformProcessCommonImpl.cpp
    src/cpp/platform/SocketPlatformCommon.cpp
)
if(WIN32)
    list(APPEND GRAVITY_PLATFORM_SOURCES
        src/cpp/platform/win/DynamicLibraryWin.cpp
        src/cpp/platform/win/PlatformPathsWin.cpp
        src/cpp/platform/win/PlatformProcessWin.cpp
        src/cpp/platform/win/SocketPlatformWin.cpp
    )
elseif(APPLE)
    list(APPEND GRAVITY_PLATFORM_SOURCES
        src/cpp/platform/posix/DynamicLibraryPosix.cpp
        src/cpp/platform/posix/PlatformPathsPosix.cpp
        src/cpp/platform/posix/PlatformProcessPosix.cpp
        src/cpp/platform/posix/SocketPlatformPosix.cpp
    )
else()
    list(APPEND GRAVITY_PLATFORM_SOURCES
        src/cpp/platform/posix/DynamicLibraryPosix.cpp
        src/cpp/platform/posix/PlatformPathsPosix.cpp
        src/cpp/platform/posix/PlatformProcessPosix.cpp
        src/cpp/platform/posix/SocketPlatformPosix.cpp
    )
endif()

add_library(gravityPlatform STATIC
    ${GRAVITY_PLATFORM_SOURCES}
)
configure_gravity_cpp_target(gravityPlatform)
target_compile_features(gravityPlatform PRIVATE cxx_std_17)
if(APPLE)
    target_compile_definitions(gravityPlatform
        PRIVATE
            GRAVITY_PLATFORM_DYLIB_EXT=".dylib"
    )
endif()
if(WIN32)
    target_link_libraries(gravityPlatform
        PUBLIC
            ws2_32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(gravityPlatform
        PUBLIC
            dl
    )
endif()

add_executable(${APP_NAME}
    src/cpp/app/main_launcher.cpp
)
configure_gravity_cpp_target(${APP_NAME})
target_compile_features(${APP_NAME} PRIVATE cxx_std_17)

if(GRAVITY_BUILD_HEADLESS_BINARY)
    add_executable(${HEADLESS_NAME}
        src/cuda/app/main_headless.cu
        ${GRAVITY_BACKEND_SOURCES}
    )
    configure_gravity_target(${HEADLESS_NAME})
endif()

if(GRAVITY_BUILD_BACKEND_DAEMON)
    add_executable(${BACKEND_DAEMON_NAME}
        src/cpp/app/main_backend.cpp
        src/cpp/sim/BackendServer.cpp
        ${GRAVITY_BACKEND_SOURCES}
    )
    configure_gravity_target(${BACKEND_DAEMON_NAME})
endif()

if(GRAVITY_BUILD_FRONTEND_MODULE_HOST)
    add_executable(${MODULE_HOST_NAME}
        src/cpp/app/main_module_host.cpp
        src/cpp/sim/FrontendModuleHandle.cpp
    )
    configure_gravity_cpp_target(${MODULE_HOST_NAME})
    target_compile_features(${MODULE_HOST_NAME} PRIVATE cxx_std_17)
endif()

if(GRAVITY_BUILD_FRONTEND_MODULES)
    add_library(${FRONTEND_MODULE_CLI_NAME} MODULE
        src/cpp/plugins/module_cli.cpp
        src/cpp/sim/BackendClient.cpp
        src/cpp/sim/TextParse.cpp
    )
    configure_gravity_cpp_target(${FRONTEND_MODULE_CLI_NAME})
    target_compile_features(${FRONTEND_MODULE_CLI_NAME} PRIVATE cxx_std_17)

    add_library(${FRONTEND_MODULE_ECHO_NAME} MODULE
        src/cpp/plugins/module_echo.cpp
    )
    configure_gravity_cpp_target(${FRONTEND_MODULE_ECHO_NAME})
    target_compile_features(${FRONTEND_MODULE_ECHO_NAME} PRIVATE cxx_std_17)

    add_library(${FRONTEND_MODULE_GUI_PROXY_NAME} MODULE
        src/cpp/plugins/module_gui_proxy.cpp
    )
    configure_gravity_cpp_target(${FRONTEND_MODULE_GUI_PROXY_NAME})
    target_compile_features(${FRONTEND_MODULE_GUI_PROXY_NAME} PRIVATE cxx_std_17)
endif()

if(GRAVITY_BUILD_FRONTEND_MODULES)
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if(NOT Qt6_FOUND AND WIN32)
        file(GLOB _qt_local_roots "C:/Qt/*/msvc*_64")
        list(SORT _qt_local_roots COMPARE NATURAL ORDER DESCENDING)
        foreach(_qt_root IN LISTS _qt_local_roots)
            if(EXISTS "${_qt_root}/lib/cmake/Qt6/Qt6Config.cmake")
                list(PREPEND CMAKE_PREFIX_PATH "${_qt_root}")
                break()
            endif()
        endforeach()
        find_package(Qt6 QUIET COMPONENTS Widgets)
    endif()

    if(Qt6_FOUND)
        add_library(${FRONTEND_MODULE_QT_INPROC_NAME} MODULE
            src/cpp/plugins/module_qt_inproc.cpp
            src/cpp/sim/BackendClient.cpp
            src/cpp/sim/FrontendBackendBridge.cpp
            src/cpp/sim/FrontendCommon.cpp
            src/cpp/sim/LocalBackendFactory.cpp
            src/cpp/sim/FrontendRuntime.cpp
            src/cpp/sim/SimulationConfig.cpp
            src/cpp/sim/TextParse.cpp
            src/cpp/ui/EnergyGraphWidget.cpp
            src/cpp/ui/MainWindow.cpp
            src/cpp/ui/MultiViewWidget.cpp
            src/cpp/ui/ParticleView.cpp
            src/cpp/ui/QtViewMath.cpp
            ${GRAVITY_BACKEND_SOURCES}
        )
        configure_gravity_target(${FRONTEND_MODULE_QT_INPROC_NAME})
        target_link_libraries(${FRONTEND_MODULE_QT_INPROC_NAME}
            PRIVATE
                Qt6::Widgets
        )
    else()
        message(STATUS "Qt6 not found. Qt in-process frontend module is disabled.")
    endif()
endif()

if(GRAVITY_BUILD_TESTS)
    enable_testing()

    if(WIN32)
        gravity_collect_existing_paths(
            _gravity_windows_system_includes
            "${GRAVITY_MSVC_INCLUDE_DIR}"
            "${GRAVITY_WINSDK_INCLUDE_UCRT}"
            "${GRAVITY_WINSDK_INCLUDE_UM}"
            "${GRAVITY_WINSDK_INCLUDE_SHARED}"
            "${GRAVITY_WINSDK_INCLUDE_WINRT}"
            "${GRAVITY_WINSDK_INCLUDE_CPPWINRT}"
        )
        gravity_collect_existing_paths(
            _gravity_windows_link_dirs
            "${GRAVITY_MSVC_LIB_DIR}"
            "${GRAVITY_WINSDK_UCRT_LIB_DIR}"
            "${GRAVITY_WINSDK_UM_LIB_DIR}"
        )
    endif()

    add_test(
        NAME GravityConfigValidation
        COMMAND ${CMAKE_COMMAND}
            -DGRAVITY_CONFIG_PATH:FILEPATH=${CMAKE_CURRENT_SOURCE_DIR}/simulation.ini
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/config/validate_simulation_ini.cmake
    )
    set_tests_properties(GravityConfigValidation PROPERTIES LABELS "integration")

    add_test(
        NAME GravityNoLegacyFrontendTargets
        COMMAND ${CMAKE_COMMAND}
            -DGRAVITY_ROOT_DIR:PATH=${CMAKE_CURRENT_SOURCE_DIR}
            -DGRAVITY_BUILD_DIR:PATH=${CMAKE_BINARY_DIR}
            -DGRAVITY_CHECK_BUILD_TARGETS:BOOL=ON
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/validate_no_legacy_frontends.cmake
    )
    set_tests_properties(GravityNoLegacyFrontendTargets PROPERTIES LABELS "integration")

    add_test(
        NAME GravityLauncherContract
        COMMAND ${CMAKE_COMMAND}
            -DGRAVITY_BUILD_DIR:PATH=${CMAKE_BINARY_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/validate_launcher_contract.cmake
    )
    set_tests_properties(GravityLauncherContract PROPERTIES LABELS "integration")

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

        if(WIN32)
            foreach(_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
                if(TARGET ${_gtest_target} AND _gravity_windows_system_includes)
                    target_include_directories(${_gtest_target}
                        SYSTEM PRIVATE
                            ${_gravity_windows_system_includes}
                    )
                endif()
            endforeach()
        endif()
    endif()

    add_executable(gravityPhysicsGTests
        tests/physics_regression.cpp
        ${GRAVITY_BACKEND_SOURCES}
    )
    configure_gravity_target(gravityPhysicsGTests)
    target_compile_definitions(gravityPhysicsGTests
        PRIVATE
            GRAVITY_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(gravityPhysicsGTests
        PRIVATE
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(gravityPhysicsGTests PROPERTIES LABELS "gpu")

    add_executable(gravityConfigArgsGTests
        tests/config/config_args_regression.cpp
        src/cpp/sim/SimulationArgs.cpp
        src/cpp/sim/SimulationConfig.cpp
        src/cpp/sim/SimulationModes.cpp
        src/cpp/sim/TextParse.cpp
    )
    target_include_directories(gravityConfigArgsGTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(gravityConfigArgsGTests
        PRIVATE
            GTest::gtest_main
    )
    if(WIN32)
        if(_gravity_windows_system_includes)
            target_include_directories(gravityConfigArgsGTests
                SYSTEM PRIVATE
                    ${_gravity_windows_system_includes}
            )
        endif()
        if(_gravity_windows_link_dirs)
            target_link_directories(gravityConfigArgsGTests
                PRIVATE
                    ${_gravity_windows_link_dirs}
            )
        endif()
    endif()
    gtest_discover_tests(gravityConfigArgsGTests PROPERTIES LABELS "integration")

    add_executable(gravityBackendProtocolGTests
        tests/integration/backend_protocol_regression.cpp
        tests/integration/RealBackendHarness.cpp
        src/cpp/sim/BackendClient.cpp
        src/cpp/sim/TextParse.cpp
    )
    target_include_directories(gravityBackendProtocolGTests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(gravityBackendProtocolGTests
        PRIVATE
            GTest::gtest_main
            gravityPlatform
    )
    if(WIN32)
        if(_gravity_windows_system_includes)
            target_include_directories(gravityBackendProtocolGTests
                SYSTEM PRIVATE
                    ${_gravity_windows_system_includes}
            )
        endif()
        if(_gravity_windows_link_dirs)
            target_link_directories(gravityBackendProtocolGTests
                PRIVATE
                    ${_gravity_windows_link_dirs}
            )
        endif()
    endif()
    gtest_discover_tests(gravityBackendProtocolGTests
        PROPERTIES
            LABELS "integration\\;integration_real"
            RESOURCE_LOCK gravity_backend_daemon
            TIMEOUT 30
    )

    add_executable(gravityFrontendBackendBridgeGTests
        tests/integration/frontend_backend_bridge_regression.cpp
        tests/integration/RealBackendHarness.cpp
        src/cpp/sim/FrontendBackendBridge.cpp
        src/cpp/sim/BackendClient.cpp
        src/cpp/sim/TextParse.cpp
    )
    configure_gravity_cpp_target(gravityFrontendBackendBridgeGTests)
    target_link_libraries(gravityFrontendBackendBridgeGTests
        PRIVATE
            GTest::gtest_main
    )
    gtest_discover_tests(gravityFrontendBackendBridgeGTests
        PROPERTIES
            LABELS "integration\\;integration_real"
            RESOURCE_LOCK gravity_backend_daemon
            TIMEOUT 30
    )

    add_executable(gravityFrontendRuntimeContractGTests
        tests/integration/frontend_runtime_contract_regression.cpp
        tests/integration/RealBackendHarness.cpp
        src/cpp/sim/FrontendRuntime.cpp
        src/cpp/sim/FrontendBackendBridge.cpp
        src/cpp/sim/BackendClient.cpp
        src/cpp/sim/TextParse.cpp
    )
    configure_gravity_cpp_target(gravityFrontendRuntimeContractGTests)
    target_link_libraries(gravityFrontendRuntimeContractGTests
        PRIVATE
            GTest::gtest_main
    )
    gtest_discover_tests(gravityFrontendRuntimeContractGTests
        PROPERTIES
            LABELS "contract\\;integration_real"
            RESOURCE_LOCK gravity_backend_daemon
            TIMEOUT 30
    )

    if(TARGET Qt6::Widgets AND GRAVITY_BUILD_FRONTEND_MODULES)
    add_executable(gravityQtMainWindowIntegrationGTests
        tests/integration/qt_mainwindow_integration.cpp
        tests/integration/RealBackendHarness.cpp
        src/cpp/sim/FrontendRuntime.cpp
        src/cpp/sim/FrontendBackendBridge.cpp
        src/cpp/sim/BackendClient.cpp
        src/cpp/sim/FrontendCommon.cpp
        src/cpp/sim/SimulationConfig.cpp
        src/cpp/sim/SimulationModes.cpp
        src/cpp/sim/TextParse.cpp
        src/cpp/ui/EnergyGraphWidget.cpp
            src/cpp/ui/MainWindow.cpp
            src/cpp/ui/MultiViewWidget.cpp
            src/cpp/ui/ParticleView.cpp
            src/cpp/ui/QtViewMath.cpp
        )
        target_include_directories(gravityQtMainWindowIntegrationGTests
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        target_link_libraries(gravityQtMainWindowIntegrationGTests
            PRIVATE
                GTest::gtest_main
                Qt6::Widgets
                gravityPlatform
        )
        if(WIN32)
            if(_gravity_windows_system_includes)
                target_include_directories(gravityQtMainWindowIntegrationGTests
                    SYSTEM PRIVATE
                        ${_gravity_windows_system_includes}
                )
            endif()
            if(_gravity_windows_link_dirs)
                target_link_directories(gravityQtMainWindowIntegrationGTests
                    PRIVATE
                        ${_gravity_windows_link_dirs}
                )
            endif()
        endif()
        gtest_discover_tests(gravityQtMainWindowIntegrationGTests
            DISCOVERY_MODE PRE_TEST
            PROPERTIES
                LABELS "ui_integration\\;integration_real"
                RESOURCE_LOCK gravity_backend_daemon
                TIMEOUT 45
        )
    endif()
endif()

