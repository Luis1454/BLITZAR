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
    if(MSVC)
        # Keep debug symbols per object file to avoid shared-PDB contention in parallel builds.
        set_property(TARGET ${target_name} PROPERTY MSVC_DEBUG_INFORMATION_FORMAT Embedded)
    endif()
    target_compile_definitions(${target_name}
        PRIVATE
            $<$<BOOL:${WIN32}>:NOMINMAX>
            BLITZAR_ENABLE_CUDA=$<IF:$<BOOL:${BLITZAR_ENABLE_CUDA}>,1,0>
            BLITZAR_CLIENT_MODULE_EXPORT_ATTR=
            $<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:BLITZAR_HD_DEVICE=>
            $<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:BLITZAR_HD_HOST=>
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
    if(NOT TARGET CUDA::cufft)
        message(FATAL_ERROR "CUDA FFT library (CUDA::cufft) is required for BLITZAR CUDA targets")
    endif()
    target_link_libraries(${target_name} PRIVATE CUDA::cufft)
    if(TARGET CUDA::cuda_driver)
        target_link_libraries(${target_name} PRIVATE CUDA::cuda_driver)
        target_compile_definitions(${target_name} PRIVATE BLITZAR_HAS_CUDA_DRIVER=1)
    else()
        target_compile_definitions(${target_name} PRIVATE BLITZAR_HAS_CUDA_DRIVER=0)
    endif()
    if(TARGET CUDA::nvrtc)
        target_link_libraries(${target_name} PRIVATE CUDA::nvrtc)
        target_compile_definitions(${target_name} PRIVATE BLITZAR_HAS_NVRTC=1)
    else()
        target_compile_definitions(${target_name} PRIVATE BLITZAR_HAS_NVRTC=0)
    endif()
    if(MSVC AND CUDAToolkit_BIN_DIR)
        # MSVC does not inherit CUDA's device-runtime directory from nvcc.
        get_filename_component(_blitzar_cuda_root "${CUDAToolkit_BIN_DIR}" DIRECTORY)
        set(_blitzar_cuda_library_dir "${_blitzar_cuda_root}/lib/x64")
        if(EXISTS "${_blitzar_cuda_library_dir}/cudadevrt.lib")
            target_link_directories(${target_name} PRIVATE "${_blitzar_cuda_library_dir}")
        endif()
    endif()
    if(MSVC)
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:__cplusplus>)
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/openmp>)
        if(BLITZAR_SUPPRESS_KNOWN_CUDA_TOOLCHAIN_WARNINGS)
            target_compile_options(${target_name}
                PRIVATE
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=128>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=186>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=1388>
                    $<$<COMPILE_LANGUAGE:CUDA>:--diag-suppress=1394>
            )
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fopenmp>)
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
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    FetchContent_MakeAvailable(googletest)

    if(WIN32 AND BLITZAR_WINDOWS_SYSTEM_INCLUDES)
        foreach(_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
            if(TARGET ${_gtest_target})
                target_include_directories(${_gtest_target} SYSTEM PRIVATE ${BLITZAR_WINDOWS_SYSTEM_INCLUDES})
            endif()
        endforeach()
    endif()
endfunction()
