set(BLITZAR_HIP_MODE "AUTO" CACHE STRING
    "HIP backend mode: AUTO, ON, or OFF")
set_property(CACHE BLITZAR_HIP_MODE PROPERTY STRINGS AUTO ON OFF)
set(BLITZAR_ENABLE_HIP OFF CACHE BOOL
    "Force-enable the HIP backend; use BLITZAR_HIP_MODE for new projects")

string(TOUPPER "${BLITZAR_HIP_MODE}" _blitzar_hip_mode)
if(BLITZAR_ENABLE_HIP)
    set(_blitzar_hip_mode ON)
endif()
if(NOT _blitzar_hip_mode STREQUAL "AUTO" AND
   NOT _blitzar_hip_mode STREQUAL "ON" AND
   NOT _blitzar_hip_mode STREQUAL "OFF")
    message(FATAL_ERROR
        "BLITZAR_HIP_MODE must be AUTO, ON, or OFF")
endif()

set(BLITZAR_HIP_ENABLED OFF CACHE INTERNAL
    "Whether the optional HIP backend is enabled" FORCE)
set(BLITZAR_HIP_LANGUAGE "" CACHE INTERNAL
    "CMake language used to compile the HIP backend" FORCE)
set(BLITZAR_HIP_NATIVE_CUDA OFF CACHE INTERNAL
    "Whether NVIDIA uses CUDA without HIP headers" FORCE)
set(BLITZAR_HIP_LINK_TARGETS "" CACHE INTERNAL
    "HIP runtime targets linked by BLITZAR" FORCE)

if(NOT _blitzar_hip_mode STREQUAL "OFF")
    set(_blitzar_hip_hints)
    if(DEFINED ENV{ROCM_PATH})
        list(APPEND _blitzar_hip_hints "$ENV{ROCM_PATH}")
    endif()
    if(DEFINED ENV{HIP_PATH})
        list(APPEND _blitzar_hip_hints "$ENV{HIP_PATH}")
    endif()

    find_program(BLITZAR_HIP_COMPILER
        NAMES hipcc
        HINTS ${_blitzar_hip_hints}
        PATH_SUFFIXES bin)

    set(_blitzar_hip_platform "")
    if(DEFINED CMAKE_HIP_PLATFORM)
        set(_blitzar_hip_platform "${CMAKE_HIP_PLATFORM}")
    elseif(DEFINED ENV{HIP_PLATFORM})
        set(_blitzar_hip_platform "$ENV{HIP_PLATFORM}")
    elseif(BLITZAR_HIP_COMPILER)
        find_program(_blitzar_hipconfig
            NAMES hipconfig
            HINTS ${_blitzar_hip_hints}
            PATH_SUFFIXES bin)
        if(_blitzar_hipconfig)
            execute_process(
                COMMAND "${_blitzar_hipconfig}" --platform
                OUTPUT_VARIABLE _blitzar_hip_platform
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
        endif()
    endif()
    string(TOLOWER "${_blitzar_hip_platform}" _blitzar_hip_platform)
    if(NOT _blitzar_hip_platform STREQUAL "" AND
       NOT _blitzar_hip_platform STREQUAL "amd" AND
       NOT _blitzar_hip_platform STREQUAL "nvidia")
        message(FATAL_ERROR
            "HIP_PLATFORM must be amd or nvidia when HIP is enabled")
    endif()
    if(NOT _blitzar_hip_platform STREQUAL "")
        set(CMAKE_HIP_PLATFORM "${_blitzar_hip_platform}" CACHE STRING
            "HIP platform selected by BLITZAR" FORCE)
    endif()

    if(_blitzar_hip_platform STREQUAL "nvidia")
        include(CheckLanguage)
        find_program(_blitzar_cuda_compiler
            NAMES nvcc
            HINTS $ENV{CUDA_PATH} $ENV{CUDA_HOME}
            PATH_SUFFIXES bin)
        if(_blitzar_cuda_compiler)
            set(CMAKE_CUDA_COMPILER "${_blitzar_cuda_compiler}"
                CACHE FILEPATH "CUDA compiler used for NVIDIA HIP")
            if(DEFINED CMAKE_HIP_ARCHITECTURES AND
               NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
                set(CMAKE_CUDA_ARCHITECTURES "${CMAKE_HIP_ARCHITECTURES}"
                    CACHE STRING "CUDA architectures used for HIP")
            endif()
            set(CMAKE_CUDA_STANDARD 20 CACHE STRING
                "CUDA language standard used for HIP")
            set(CMAKE_CUDA_STANDARD_REQUIRED ON)
            set(CMAKE_CUDA_EXTENSIONS OFF)
            check_language(CUDA)
            if(CMAKE_CUDA_COMPILER)
                enable_language(CUDA)
                find_package(CUDAToolkit REQUIRED)

                set(_blitzar_hip_native_cuda ON)
                if(BLITZAR_HIP_COMPILER)
                    find_package(hip CONFIG QUIET HINTS ${_blitzar_hip_hints})
                    if(hip_FOUND)
                        set(_blitzar_hip_native_cuda OFF)
                    endif()
                endif()

                set(BLITZAR_HIP_ENABLED ON CACHE INTERNAL
                    "Whether the optional HIP backend is enabled" FORCE)
                set(BLITZAR_HIP_LANGUAGE "CUDA" CACHE INTERNAL
                    "CMake language used to compile the HIP backend" FORCE)
                set(BLITZAR_HIP_NATIVE_CUDA "${_blitzar_hip_native_cuda}"
                    CACHE INTERNAL
                    "Whether NVIDIA uses CUDA without HIP headers" FORCE)
                if(TARGET CUDA::cudart)
                    list(APPEND BLITZAR_HIP_LINK_TARGETS CUDA::cudart)
                endif()
                if(NOT _blitzar_hip_native_cuda AND TARGET hip::host)
                    list(APPEND BLITZAR_HIP_LINK_TARGETS hip::host)
                endif()
                set(BLITZAR_HIP_LINK_TARGETS
                    "${BLITZAR_HIP_LINK_TARGETS}" CACHE INTERNAL
                    "HIP runtime targets linked by BLITZAR" FORCE)
                if(_blitzar_hip_native_cuda)
                    message(STATUS
                        "BLITZAR native CUDA backend enabled with "
                        "${CMAKE_CUDA_COMPILER} (HIP headers not required)")
                else()
                    message(STATUS
                        "BLITZAR HIP backend enabled with HIP headers and "
                        "${CMAKE_CUDA_COMPILER}")
                endif()
            elseif(_blitzar_hip_mode STREQUAL "ON")
                message(FATAL_ERROR
                    "BLITZAR_HIP_MODE=ON could not configure nvcc")
            else()
                message(STATUS "nvcc is unavailable; using the CPU backend")
            endif()
        elseif(_blitzar_hip_mode STREQUAL "ON")
            message(FATAL_ERROR
                "BLITZAR_HIP_MODE=ON requires nvcc for HIP_PLATFORM=nvidia")
        else()
            message(STATUS "nvcc is unavailable; using the CPU backend")
        endif()
    elseif(_blitzar_hip_platform STREQUAL "amd")
        if(BLITZAR_HIP_COMPILER)
            if(NOT DEFINED CMAKE_HIP_COMPILER)
                find_program(_blitzar_hip_clang
                    NAMES clang++ clang++-17
                    HINTS $ENV{HIP_PATH}/bin $ENV{ROCM_PATH}/llvm/bin)
                if(_blitzar_hip_clang)
                    set(CMAKE_HIP_COMPILER "${_blitzar_hip_clang}"
                        CACHE FILEPATH "HIP compiler used by BLITZAR")
                endif()
            endif()
            if(DEFINED CMAKE_HIP_COMPILER AND
               CMAKE_HIP_COMPILER MATCHES "hipcc")
                unset(CMAKE_HIP_COMPILER CACHE)
            endif()
            set(CMAKE_HIP_STANDARD 20 CACHE STRING "HIP language standard")
            set(CMAKE_HIP_STANDARD_REQUIRED ON)
            set(CMAKE_HIP_EXTENSIONS OFF)
            include(CheckLanguage)
            check_language(HIP)
            if(CMAKE_HIP_COMPILER)
                enable_language(HIP)
                find_package(hip CONFIG QUIET HINTS ${_blitzar_hip_hints})
                if(NOT hip_FOUND)
                    find_package(HIP CONFIG QUIET HINTS ${_blitzar_hip_hints})
                endif()
                set(BLITZAR_HIP_ENABLED ON CACHE INTERNAL
                    "Whether the optional HIP backend is enabled" FORCE)
                set(BLITZAR_HIP_LANGUAGE "HIP" CACHE INTERNAL
                    "CMake language used to compile the HIP backend" FORCE)
                if(TARGET hip::host)
                    list(APPEND BLITZAR_HIP_LINK_TARGETS hip::host)
                endif()
                if(TARGET hip::device)
                    list(APPEND BLITZAR_HIP_LINK_TARGETS hip::device)
                elseif(TARGET hip-lang::device)
                    list(APPEND BLITZAR_HIP_LINK_TARGETS hip-lang::device)
                endif()
                set(BLITZAR_HIP_LINK_TARGETS
                    "${BLITZAR_HIP_LINK_TARGETS}" CACHE INTERNAL
                    "HIP runtime targets linked by BLITZAR" FORCE)
                message(STATUS
                    "BLITZAR HIP backend enabled with ${CMAKE_HIP_COMPILER}")
            elseif(_blitzar_hip_mode STREQUAL "ON")
                message(FATAL_ERROR
                    "BLITZAR_HIP_MODE=ON could not configure a HIP compiler")
            else()
                message(STATUS
                    "HIP compiler is unavailable; using the CPU backend")
            endif()
        elseif(_blitzar_hip_mode STREQUAL "ON")
            message(FATAL_ERROR
                "BLITZAR_HIP_MODE=ON requires hipcc for HIP_PLATFORM=amd")
        else()
            message(STATUS "hipcc is unavailable; using the CPU backend")
        endif()
    elseif(_blitzar_hip_mode STREQUAL "ON")
        message(FATAL_ERROR
            "BLITZAR_HIP_MODE=ON requires HIP_PLATFORM=amd or nvidia")
    else()
        message(STATUS "HIP platform is not selected; using the CPU backend")
    endif()
endif()

function(blitzar_enable_hip target)
    if(NOT BLITZAR_HIP_ENABLED)
        return()
    endif()
    target_compile_definitions(${target} PRIVATE BLITZAR_HAS_HIP=1)
    if(CMAKE_HIP_PLATFORM STREQUAL "nvidia")
        target_compile_definitions(${target} PRIVATE
            __HIP_PLATFORM_NVCC__
            __HIP_PLATFORM_NVIDIA__
        )
    endif()
    if(BLITZAR_HIP_NATIVE_CUDA)
        target_compile_definitions(${target} PRIVATE
            BLITZAR_HIP_NATIVE_CUDA=1
        )
    endif()
    if(hip_INCLUDE_DIRS)
        target_include_directories(${target} SYSTEM PRIVATE ${hip_INCLUDE_DIRS})
    endif()
    if(BLITZAR_HIP_LINK_TARGETS)
        target_link_libraries(${target} PUBLIC ${BLITZAR_HIP_LINK_TARGETS})
    endif()
endfunction()
