cmake_minimum_required(VERSION 3.25)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/plan/manifest.json" BLITZAR_PLAN_MANIFEST)
string(JSON BLITZAR_PRODUCT_VERSION
    GET "${BLITZAR_PLAN_MANIFEST}" product_version)
string(JSON BLITZAR_PLAN_VERSION
    GET "${BLITZAR_PLAN_MANIFEST}" plan_version)
if(NOT BLITZAR_PRODUCT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "plan/manifest.json product_version must be semantic version")
endif()
if(NOT BLITZAR_PLAN_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "plan/manifest.json plan_version must be semantic version")
endif()

project(BLITZAR VERSION ${BLITZAR_PRODUCT_VERSION} LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/HIP.cmake)
include(cmake/MPI.cmake)

option(BLITZAR_BUILD_TESTS "Build BLITZAR tests" ON)
option(BLITZAR_BUILD_CLI "Build the BLITZAR command-line application" ON)
option(BLITZAR_BUILD_EXAMPLES "Build SDK consumption examples" ON)
option(BLITZAR_BUILD_SHARED "Build BLITZAR as a shared library" OFF)
option(BLITZAR_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan" OFF)
option(BLITZAR_ENABLE_OPENMP "Enable CPU OpenMP parallelism" ON)

if(BLITZAR_ENABLE_SANITIZERS AND MSVC)
    message(FATAL_ERROR "BLITZAR_ENABLE_SANITIZERS requires a Clang/GNU toolchain")
endif()

if(BLITZAR_BUILD_SHARED AND BLITZAR_BUILD_TESTS)
    message(FATAL_ERROR
        "BLITZAR_BUILD_TESTS targets internal symbols; use the static build for them")
endif()

if(BLITZAR_ENABLE_OPENMP)
    find_package(OpenMP REQUIRED COMPONENTS CXX)
endif()
find_package(Threads REQUIRED)

function(blitzar_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C>:/W4>"
            "$<$<COMPILE_LANGUAGE:C>:/WX>"
            "$<$<COMPILE_LANGUAGE:CXX>:/W4>"
            "$<$<COMPILE_LANGUAGE:CXX>:/WX>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/W4>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/WX>"
        )
    else()
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:C>:-Wall>"
            "$<$<COMPILE_LANGUAGE:C>:-Wextra>"
            "$<$<COMPILE_LANGUAGE:C>:-Wpedantic>"
            "$<$<COMPILE_LANGUAGE:C>:-Werror>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>"
            "$<$<COMPILE_LANGUAGE:CXX>:-Werror>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wextra>"
            "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Werror>"
        )
    endif()
endfunction()

include(cmake/Library.cmake)
include(cmake/Package.cmake)
include(cmake/Targets.cmake)

if(BLITZAR_BUILD_TESTS)
    include(cmake/Tests.cmake)
endif()
