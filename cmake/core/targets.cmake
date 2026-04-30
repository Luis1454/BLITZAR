# @file cmake/core/targets.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

function(BLITZAR_apply_windows_paths target_name)
    if(NOT WIN32)
        return()
    endif()
    if(BLITZAR_WINDOWS_SYSTEM_INCLUDES)
        target_include_directories(${target_name} SYSTEM PRIVATE ${BLITZAR_WINDOWS_SYSTEM_INCLUDES})
    endif()
    if(BLITZAR_WINDOWS_LINK_DIRS)
        target_link_directories(${target_name} PRIVATE ${BLITZAR_WINDOWS_LINK_DIRS})
    endif()
endfunction()

function(BLITZAR_apply_strict_warnings target_name)
    if(NOT BLITZAR_STRICT_WARNINGS)
        return()
    endif()
    if(MSVC)
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /permissive->)
    else()
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Werror>)
    endif()
endfunction()

function(configure_BLITZAR_cpp_target target_name)
    target_include_directories(${target_name} PRIVATE ${BLITZAR_PROJECT_INCLUDE_DIRS})
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_compile_definitions(${target_name}
        PRIVATE
            $<$<BOOL:${WIN32}>:NOMINMAX>
            BLITZAR_CLIENT_MODULE_EXPORT_ATTR=
            BLITZAR_HD_DEVICE=
            BLITZAR_HD_HOST=
    )
    BLITZAR_apply_strict_warnings(${target_name})
    BLITZAR_apply_windows_paths(${target_name})

    if(TARGET blitzarPlatform AND NOT "${target_name}" STREQUAL "blitzarPlatform")
        target_link_libraries(${target_name} PRIVATE blitzarPlatform)
    endif()

    if(BLITZAR_PROFILE_LOGS)
        target_compile_definitions(${target_name} PRIVATE BLITZAR_PROFILE_LOGS=1)
    else()
        target_compile_definitions(${target_name} PRIVATE BLITZAR_PROFILE_LOGS=0)
    endif()
    if(BLITZAR_PROFILE STREQUAL "prod")
        target_compile_definitions(${target_name} PRIVATE BLITZAR_PROFILE_PROD=1 BLITZAR_PROFILE_IS_PROD=1 BLITZAR_PROFILE_IS_DEV=0)
    else()
        target_compile_definitions(${target_name} PRIVATE BLITZAR_PROFILE_DEV=1 BLITZAR_PROFILE_IS_PROD=0 BLITZAR_PROFILE_IS_DEV=1)
    endif()
endfunction()

function(configure_BLITZAR_cuda_target target_name)
    configure_BLITZAR_cpp_target(${target_name})
    set_target_properties(${target_name} PROPERTIES CUDA_SEPARABLE_COMPILATION OFF)
    target_compile_definitions(${target_name}
        PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:BLITZAR_HD_HOST=__host__>
            $<$<COMPILE_LANGUAGE:CUDA>:BLITZAR_HD_DEVICE=__device__>
    )
    target_link_libraries(${target_name} PRIVATE CUDA::cudart)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:__cplusplus>)
        if(BLITZAR_SUPPRESS_KNOWN_CUDA_TOOLCHAIN_WARNINGS)
            target_compile_options(${target_name}
                PRIVATE
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=128>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=186>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=1388>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=1394>
            )
        endif()
    endif()
endfunction()

function(BLITZAR_ensure_gtest)
    if(TARGET GTest::gtest_main)
        return()
    endif()

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
        return()
    endif()

    find_package(GTest CONFIG QUIET)
    if(TARGET GTest::gtest_main)
        return()
    endif()

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
    if(_BLITZAR_gtest_cmake_args)
        set_property(DIRECTORY PROPERTY EP_UPDATE_DISCONNECTED ON)
        set(FETCHCONTENT_UPDATES_DISCONNECTED_GOOGLETEST ON)
        set(FETCHCONTENT_QUIET OFF)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            CMAKE_ARGS ${_BLITZAR_gtest_cmake_args}
        )
    else()
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        )
    endif()
    FetchContent_MakeAvailable(googletest)

    if(WIN32 AND BLITZAR_WINDOWS_SYSTEM_INCLUDES)
        foreach(_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
            if(TARGET ${_gtest_target})
                target_include_directories(${_gtest_target} SYSTEM PRIVATE ${BLITZAR_WINDOWS_SYSTEM_INCLUDES})
            endif()
        endforeach()
    endif()
endfunction()
