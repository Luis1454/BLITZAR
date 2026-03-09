function(gravity_apply_windows_paths target_name)
    if(NOT WIN32)
        return()
    endif()
    if(GRAVITY_WINDOWS_SYSTEM_INCLUDES)
        target_include_directories(${target_name} SYSTEM PRIVATE ${GRAVITY_WINDOWS_SYSTEM_INCLUDES})
    endif()
    if(GRAVITY_WINDOWS_LINK_DIRS)
        target_link_directories(${target_name} PRIVATE ${GRAVITY_WINDOWS_LINK_DIRS})
    endif()
endfunction()

function(gravity_apply_strict_warnings target_name)
    if(NOT GRAVITY_STRICT_WARNINGS)
        return()
    endif()
    if(MSVC)
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX /permissive->)
    else()
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -Werror>)
    endif()
endfunction()

function(configure_gravity_cpp_target target_name)
    target_include_directories(${target_name} PRIVATE ${GRAVITY_PROJECT_INCLUDE_DIRS})
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    target_compile_definitions(${target_name}
        PRIVATE
            GRAVITY_FRONTEND_MODULE_EXPORT_ATTR=
            GRAVITY_HD_DEVICE=
            GRAVITY_HD_HOST=
    )
    gravity_apply_strict_warnings(${target_name})
    gravity_apply_windows_paths(${target_name})

    if(TARGET gravityPlatform AND NOT "${target_name}" STREQUAL "gravityPlatform")
        target_link_libraries(${target_name} PRIVATE gravityPlatform)
    endif()

    if(GRAVITY_PROFILE_LOGS)
        target_compile_definitions(${target_name} PRIVATE GRAVITY_PROFILE_LOGS=1)
    else()
        target_compile_definitions(${target_name} PRIVATE GRAVITY_PROFILE_LOGS=0)
    endif()
    if(GRAVITY_PROFILE STREQUAL "prod")
        target_compile_definitions(${target_name} PRIVATE GRAVITY_PROFILE_PROD=1)
    else()
        target_compile_definitions(${target_name} PRIVATE GRAVITY_PROFILE_DEV=1)
    endif()
endfunction()

function(configure_gravity_cuda_target target_name)
    configure_gravity_cpp_target(${target_name})
    set_target_properties(${target_name} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    target_compile_definitions(${target_name}
        PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:GRAVITY_HD_HOST=__host__>
            $<$<COMPILE_LANGUAGE:CUDA>:GRAVITY_HD_DEVICE=__device__>
    )
    target_link_libraries(${target_name} PRIVATE CUDA::cudart)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:__cplusplus>)
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
endfunction()

function(gravity_ensure_gtest)
    if(TARGET GTest::gtest_main)
        return()
    endif()
    find_package(GTest CONFIG QUIET)
    if(TARGET GTest::gtest_main)
        return()
    endif()

    include(FetchContent)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
    FetchContent_MakeAvailable(googletest)

    if(WIN32 AND GRAVITY_WINDOWS_SYSTEM_INCLUDES)
        foreach(_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
            if(TARGET ${_gtest_target})
                target_include_directories(${_gtest_target} SYSTEM PRIVATE ${GRAVITY_WINDOWS_SYSTEM_INCLUDES})
            endif()
        endforeach()
    endif()
endfunction()
