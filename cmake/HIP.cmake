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

    if(BLITZAR_HIP_COMPILER)
        if(NOT DEFINED CMAKE_HIP_COMPILER)
            set(CMAKE_HIP_COMPILER "${BLITZAR_HIP_COMPILER}"
                CACHE FILEPATH "HIP compiler used by BLITZAR")
        endif()
        if(DEFINED ENV{HIP_PLATFORM} AND NOT DEFINED CMAKE_HIP_PLATFORM)
            set(CMAKE_HIP_PLATFORM "$ENV{HIP_PLATFORM}" CACHE STRING
                "HIP platform selected by the environment")
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
            if(TARGET hip::host)
                list(APPEND BLITZAR_HIP_LINK_TARGETS hip::host)
            endif()
            if(TARGET hip::device)
                list(APPEND BLITZAR_HIP_LINK_TARGETS hip::device)
            elseif(TARGET hip-lang::device)
                list(APPEND BLITZAR_HIP_LINK_TARGETS hip-lang::device)
            endif()
            set(BLITZAR_HIP_LINK_TARGETS "${BLITZAR_HIP_LINK_TARGETS}"
                CACHE INTERNAL "HIP runtime targets linked by BLITZAR" FORCE)
            message(STATUS "BLITZAR HIP backend enabled with ${CMAKE_HIP_COMPILER}")
        elseif(_blitzar_hip_mode STREQUAL "ON")
            message(FATAL_ERROR
                "BLITZAR_HIP_MODE=ON could not configure a HIP compiler")
        else()
            message(STATUS "HIP compiler is unavailable; using the CPU backend")
        endif()
    elseif(_blitzar_hip_mode STREQUAL "ON")
        message(FATAL_ERROR
            "BLITZAR_HIP_MODE=ON requires hipcc in PATH or ROCm/HIP_PATH")
    else()
        message(STATUS "HIP not detected; using the CPU backend")
    endif()
endif()

function(blitzar_enable_hip target)
    if(NOT BLITZAR_HIP_ENABLED)
        return()
    endif()
    target_compile_definitions(${target} PRIVATE BLITZAR_HAS_HIP=1)
    if(BLITZAR_HIP_LINK_TARGETS)
        target_link_libraries(${target} PUBLIC ${BLITZAR_HIP_LINK_TARGETS})
    endif()
endfunction()
