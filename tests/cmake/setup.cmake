if(NOT DEFINED GRAVITY_GRAPHICS_SOURCES)
    set(GRAVITY_GRAPHICS_SOURCES
        "${GRAVITY_ROOT_DIR}/engine/src/graphics/ViewMath.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/graphics/ColorPipeline.cpp"
    )
endif()

if(NOT DEFINED GRAVITY_RUNTIME_PROTOCOL_SOURCES)
    set(GRAVITY_RUNTIME_PROTOCOL_SOURCES
        "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodec.cpp"
        "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParse.cpp"
        "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseStatus.cpp"
        "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseSnapshot.cpp"
        "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecReadNumber.cpp"
        "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerClient.cpp"
        "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerProtocol.cpp"
    )
endif()

if(NOT DEFINED GRAVITY_SERVER_SOURCES)
    if(WIN32)
        set(GRAVITY_ENV_UTILS_SOURCES
            "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtilsWin.cpp"
        )
    else()
        set(GRAVITY_ENV_UTILS_SOURCES
            "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtilsPosix.cpp"
        )
    endif()
    set(GRAVITY_SERVER_SOURCES
        ${GRAVITY_ENV_UTILS_SOURCES}
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgs.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsCoreOptions.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsClientOptions.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitOptions.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitStateOptions.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsFluidOptions.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/server/SimulationServer.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/physics/cuda/ParticleSystem.cu"
    )
endif()

include("${CMAKE_CURRENT_LIST_DIR}/windows_paths.cmake")
include("${GRAVITY_ROOT_DIR}/cmake/rust.cmake")
gravity_ensure_rust_runtime_target()

function(gravity_apply_test_quality_flags target_name)
    if(GRAVITY_INTEGRATION_STRICT_WARNINGS)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /W4 /WX /permissive-)
        else()
            target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
    endif()

    if(GRAVITY_INTEGRATION_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer -fsanitize=address,undefined)
        target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

if(COMMAND gravity_ensure_gtest)
    gravity_ensure_gtest()
else()
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
endif()

foreach(_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
    if(TARGET ${_gtest_target})
        gravity_apply_test_paths(${_gtest_target})
    endif()
endforeach()

include(GoogleTest)
find_package(Qt6 COMPONENTS Widgets QUIET)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

enable_testing()

set(GRAVITY_TEST_PLATFORM_TARGET gravityPlatform)
if(NOT TARGET ${GRAVITY_TEST_PLATFORM_TARGET})
    set(GRAVITY_TEST_PLATFORM_TARGET gravityIntegrationPlatform)
    set(_platform_sources
        "${GRAVITY_ROOT_DIR}/engine/src/platform/PlatformErrors.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/platform/common/DynamicLibraryCommon.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/platform/common/PlatformProcessCommon.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/platform/common/PlatformProcessCommonImpl.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/platform/common/SocketPlatformCommon.cpp"
    )
    if(WIN32)
        list(APPEND _platform_sources
            "${GRAVITY_ROOT_DIR}/engine/src/platform/win/DynamicLibraryWin.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/platform/win/PlatformPathsWin.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/platform/win/PlatformProcessWin.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/platform/win/SocketPlatformWin.cpp"
        )
    else()
        list(APPEND _platform_sources
            "${GRAVITY_ROOT_DIR}/engine/src/platform/posix/DynamicLibraryPosix.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/platform/posix/PlatformPathsPosix.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/platform/posix/PlatformProcessPosix.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/platform/posix/SocketPlatformPosix.cpp"
        )
    endif()

    add_library(${GRAVITY_TEST_PLATFORM_TARGET} STATIC ${_platform_sources})
    target_include_directories(${GRAVITY_TEST_PLATFORM_TARGET} PUBLIC ${GRAVITY_TEST_INCLUDE_DIRS})
    target_compile_definitions(${GRAVITY_TEST_PLATFORM_TARGET}
        PUBLIC
            $<$<BOOL:${WIN32}>:NOMINMAX>
            GRAVITY_CLIENT_MODULE_EXPORT_ATTR=
            GRAVITY_HD_DEVICE=
            GRAVITY_HD_HOST=
            $<$<STREQUAL:${GRAVITY_PROFILE},prod>:GRAVITY_PROFILE_PROD=1>
            $<$<NOT:$<STREQUAL:${GRAVITY_PROFILE},prod>>:GRAVITY_PROFILE_DEV=1>
            $<$<STREQUAL:${GRAVITY_PROFILE},prod>:GRAVITY_PROFILE_IS_PROD=1>
            $<$<NOT:$<STREQUAL:${GRAVITY_PROFILE},prod>>:GRAVITY_PROFILE_IS_PROD=0>
            $<$<STREQUAL:${GRAVITY_PROFILE},prod>:GRAVITY_PROFILE_IS_DEV=0>
            $<$<NOT:$<STREQUAL:${GRAVITY_PROFILE},prod>>:GRAVITY_PROFILE_IS_DEV=1>
    )
    gravity_apply_test_paths(${GRAVITY_TEST_PLATFORM_TARGET})
    gravity_apply_test_quality_flags(${GRAVITY_TEST_PLATFORM_TARGET})
    if(WIN32)
        target_link_libraries(${GRAVITY_TEST_PLATFORM_TARGET} PUBLIC ws2_32)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(${GRAVITY_TEST_PLATFORM_TARGET} PUBLIC dl)
    endif()
endif()

function(gravity_add_gtest target_name)
    set(options SERVER_LOCK)
    set(oneValueArgs TIMEOUT)
    set(multiValueArgs LABELS SOURCES LIBS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${target_name} ${ARG_SOURCES})
    target_include_directories(${target_name} PRIVATE ${GRAVITY_TEST_INCLUDE_DIRS})
    target_compile_definitions(${target_name}
        PRIVATE
            $<$<BOOL:${WIN32}>:NOMINMAX>
            GRAVITY_CLIENT_MODULE_EXPORT_ATTR=
            GRAVITY_HD_DEVICE=
            GRAVITY_HD_HOST=
            $<$<STREQUAL:${GRAVITY_PROFILE},prod>:GRAVITY_PROFILE_PROD=1>
            $<$<NOT:$<STREQUAL:${GRAVITY_PROFILE},prod>>:GRAVITY_PROFILE_DEV=1>
            $<$<STREQUAL:${GRAVITY_PROFILE},prod>:GRAVITY_PROFILE_IS_PROD=1>
            $<$<NOT:$<STREQUAL:${GRAVITY_PROFILE},prod>>:GRAVITY_PROFILE_IS_PROD=0>
            $<$<STREQUAL:${GRAVITY_PROFILE},prod>:GRAVITY_PROFILE_IS_DEV=0>
            $<$<NOT:$<STREQUAL:${GRAVITY_PROFILE},prod>>:GRAVITY_PROFILE_IS_DEV=1>
    )
    target_link_libraries(${target_name} PRIVATE GTest::gtest_main ${ARG_LIBS})
    gravity_apply_test_paths(${target_name})
    gravity_apply_test_quality_flags(${target_name})

    set(_props "")
    if(ARG_LABELS)
        list(APPEND _props LABELS ${ARG_LABELS})
    endif()
    if(ARG_SERVER_LOCK)
        list(APPEND _props RESOURCE_LOCK gravity_server_daemon)
    endif()
    if(ARG_TIMEOUT)
        list(APPEND _props TIMEOUT "${ARG_TIMEOUT}")
    endif()

    if(_props)
        gtest_discover_tests(${target_name} PROPERTIES ${_props})
    else()
        gtest_discover_tests(${target_name})
    endif()
endfunction()
