# @file tests/cmake/setup.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

if(NOT DEFINED BLITZAR_GRAPHICS_SOURCES)
    set(BLITZAR_GRAPHICS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/graphics/GfxViewMath.cpp"
        "${BLITZAR_ROOT_DIR}/engine/graphics/GfxColorPipeline.cpp"
    )
endif()

if(NOT DEFINED BLITZAR_RUNTIME_PROTOCOL_SOURCES)
    set(BLITZAR_RUNTIME_PROTOCOL_SOURCES
        "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/PtcJsonCodec.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcParser.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcStatus.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcSnapshot.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcNumber.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/protocol/client/PtcClient.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/protocol/PtcProtocol.cpp"
    )
endif()

if(NOT DEFINED BLITZAR_RUNTIME_COMMAND_SOURCES)
    set(BLITZAR_RUNTIME_COMMAND_SOURCES
        "${BLITZAR_ROOT_DIR}/runtime/command/execution/CmdBatchRunner.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/command/catalog/CmdCatalog.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/command/execution/CmdExecutor.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/command/parsing/CmdParser.cpp"
        "${BLITZAR_ROOT_DIR}/runtime/command/transport/CmdTransport.cpp"
    )
endif()

if(NOT DEFINED BLITZAR_SERVER_SOURCES)
    set(BLITZAR_TEST_COMMON_SOURCES
        ${BLITZAR_BATCH_MODULE_SOURCES}
        ${BLITZAR_CONFIG_SOURCES}
        ${BLITZAR_SERVER_MODULE_SOURCES}
        ${BLITZAR_SERVER_RUNTIME_SOURCES}
        ${BLITZAR_PHYSICS_CORE_SOURCES}
        ${BLITZAR_PHYSICS_TREEPM_SOURCES}
        ${BLITZAR_PHYSICS_FMM_SOURCES}
    )
    if(BLITZAR_ENABLE_CUDA)
        set(BLITZAR_SERVER_SOURCES
            ${BLITZAR_TEST_COMMON_SOURCES}
            ${BLITZAR_PHYSICS_CUDA_DEVICE_SOURCES}
        )
    else()
        set(BLITZAR_SERVER_SOURCES
            ${BLITZAR_TEST_COMMON_SOURCES}
            ${BLITZAR_PHYSICS_CUDA_HOST_SOURCES}
            ${BLITZAR_PHYSICS_OCTREE_SOURCES}
        )
    endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/windows_paths.cmake")
include("${BLITZAR_ROOT_DIR}/cmake/qt_paths.cmake")
include("${BLITZAR_ROOT_DIR}/cmake/rust.cmake")
BLITZAR_ensure_rust_runtime_target()
set(BLITZAR_TEST_RUST_LIBS "")
if(TARGET blitzarRustRuntime)
    set(BLITZAR_TEST_RUST_LIBS blitzarRustRuntime)
endif()

function(BLITZAR_apply_test_quality_flags target_name)
    if(BLITZAR_INTEGRATION_STRICT_WARNINGS)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /W4 /WX /permissive-)
        else()
            target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Werror)
        endif()
    endif()

    if(BLITZAR_INTEGRATION_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer -fsanitize=address,undefined)
        target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

if(COMMAND BLITZAR_ensure_gtest)
    BLITZAR_ensure_gtest()
else()
    if(EXISTS "${CMAKE_BINARY_DIR}/lib/gtest.lib"
       AND EXISTS "${CMAKE_BINARY_DIR}/lib/gtest_main.lib"
       AND EXISTS "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include")
        add_library(GTest::gtest STATIC IMPORTED)
        set_target_properties(GTest::gtest PROPERTIES
            IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib/gtest.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include"
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include"
        )

        add_library(GTest::gtest_main STATIC IMPORTED)
        set_target_properties(GTest::gtest_main PROPERTIES
            IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/lib/gtest_main.lib"
            INTERFACE_LINK_LIBRARIES "GTest::gtest"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include"
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/_deps/googletest-src/googletest/include"
        )
    else()
    find_package(GTest CONFIG QUIET)
    if(NOT TARGET GTest::gtest_main)
        include(FetchContent)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(_BLITZAR_gtest_cmake_args "")
        if(WIN32
           AND CMAKE_GENERATOR MATCHES "Ninja"
           AND CMAKE_VERSION VERSION_GREATER_EQUAL 4.2
           AND CMAKE_VERSION VERSION_LESS 4.3)
            list(APPEND _BLITZAR_gtest_cmake_args
                "-DCMAKE_C_COMPILER_FORCED=ON"
                "-DCMAKE_CXX_COMPILER_FORCED=ON"
            )
        endif()
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            CMAKE_ARGS ${_BLITZAR_gtest_cmake_args}
        )
        FetchContent_MakeAvailable(googletest)
    endif()
    endif()
endif()

foreach(_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
    if(TARGET ${_gtest_target})
        BLITZAR_apply_test_paths(${_gtest_target})
    endif()
endforeach()

include(GoogleTest)
BLITZAR_find_qt6_widgets()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

enable_testing()

set(BLITZAR_TEST_PLATFORM_TARGET blitzarPlatform)
if(NOT TARGET ${BLITZAR_TEST_PLATFORM_TARGET})
    set(BLITZAR_TEST_PLATFORM_TARGET blitzarIntegrationPlatform)
    set(_platform_sources
        "${BLITZAR_ROOT_DIR}/engine/platform/errors/PltErrors.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/dynamic_library/PltDynamicLibrary.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcess.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcessImpl.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/socket/PltSocket.cpp"
    )
    if(WIN32)
        list(APPEND _platform_sources
            "${BLITZAR_ROOT_DIR}/engine/platform/dynamic_library/PltDynamicLibraryWin.cpp"
            "${BLITZAR_ROOT_DIR}/engine/platform/paths/PltPathsWin.cpp"
            "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcessWin.cpp"
            "${BLITZAR_ROOT_DIR}/engine/platform/socket/PltSocketWin.cpp"
        )
    else()
        list(APPEND _platform_sources
            "${BLITZAR_ROOT_DIR}/engine/platform/dynamic_library/PltDynamicLibraryPosix.cpp"
            "${BLITZAR_ROOT_DIR}/engine/platform/paths/PltPathsPosix.cpp"
            "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcessPosix.cpp"
            "${BLITZAR_ROOT_DIR}/engine/platform/socket/PltSocketPosix.cpp"
        )
    endif()

    add_library(${BLITZAR_TEST_PLATFORM_TARGET} STATIC ${_platform_sources})
    target_include_directories(${BLITZAR_TEST_PLATFORM_TARGET} PUBLIC ${BLITZAR_TEST_INCLUDE_DIRS})
    target_compile_definitions(${BLITZAR_TEST_PLATFORM_TARGET}
        PUBLIC
            $<$<BOOL:${WIN32}>:NOMINMAX>
            BLITZAR_CLIENT_MODULE_EXPORT_ATTR=
            BLITZAR_HD_DEVICE=
            BLITZAR_HD_HOST=
            $<$<STREQUAL:${BLITZAR_PROFILE},prod>:BLITZAR_PROFILE_PROD=1>
            $<$<NOT:$<STREQUAL:${BLITZAR_PROFILE},prod>>:BLITZAR_PROFILE_DEV=1>
            $<$<STREQUAL:${BLITZAR_PROFILE},prod>:BLITZAR_PROFILE_IS_PROD=1>
            $<$<NOT:$<STREQUAL:${BLITZAR_PROFILE},prod>>:BLITZAR_PROFILE_IS_PROD=0>
            $<$<STREQUAL:${BLITZAR_PROFILE},prod>:BLITZAR_PROFILE_IS_DEV=0>
            $<$<NOT:$<STREQUAL:${BLITZAR_PROFILE},prod>>:BLITZAR_PROFILE_IS_DEV=1>
    )
    BLITZAR_apply_test_paths(${BLITZAR_TEST_PLATFORM_TARGET})
    BLITZAR_apply_test_quality_flags(${BLITZAR_TEST_PLATFORM_TARGET})
    if(WIN32)
        target_link_libraries(${BLITZAR_TEST_PLATFORM_TARGET} PUBLIC ws2_32)
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(${BLITZAR_TEST_PLATFORM_TARGET} PUBLIC dl)
    endif()
endif()

function(BLITZAR_add_gtest target_name)
    set(options SERVER_LOCK)
    set(oneValueArgs TIMEOUT)
    set(multiValueArgs LABELS SOURCES LIBS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${target_name} ${ARG_SOURCES})
    target_include_directories(${target_name} PRIVATE ${BLITZAR_TEST_INCLUDE_DIRS})
    target_compile_definitions(${target_name}
        PRIVATE
            $<$<BOOL:${WIN32}>:NOMINMAX>
            BLITZAR_CLIENT_MODULE_EXPORT_ATTR=
            BLITZAR_HD_DEVICE=
            BLITZAR_HD_HOST=
            $<$<STREQUAL:${BLITZAR_PROFILE},prod>:BLITZAR_PROFILE_PROD=1>
            $<$<NOT:$<STREQUAL:${BLITZAR_PROFILE},prod>>:BLITZAR_PROFILE_DEV=1>
            $<$<STREQUAL:${BLITZAR_PROFILE},prod>:BLITZAR_PROFILE_IS_PROD=1>
            $<$<NOT:$<STREQUAL:${BLITZAR_PROFILE},prod>>:BLITZAR_PROFILE_IS_PROD=0>
            $<$<STREQUAL:${BLITZAR_PROFILE},prod>:BLITZAR_PROFILE_IS_DEV=0>
            $<$<NOT:$<STREQUAL:${BLITZAR_PROFILE},prod>>:BLITZAR_PROFILE_IS_DEV=1>
    )
    target_link_libraries(${target_name} PRIVATE GTest::gtest_main ${ARG_LIBS})
    BLITZAR_apply_test_paths(${target_name})
    BLITZAR_apply_test_quality_flags(${target_name})

    set(_props "")
    if(ARG_LABELS)
        list(APPEND _props LABELS ${ARG_LABELS})
    endif()
    if(ARG_SERVER_LOCK)
        list(APPEND _props RESOURCE_LOCK BLITZAR_server_daemon)
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
