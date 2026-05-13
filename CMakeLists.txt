# @file CMakeLists.txt
# @author Luis1454
# @project BLITZAR
# @brief Source artifact for the BLITZAR simulation project.

cmake_minimum_required(VERSION 3.24)

include(cmake/win.cmake)
include(cmake/dependencies.cmake)

find_program(BLITZAR_NVCC_EXECUTABLE
    NAMES nvcc
    HINTS
        "$ENV{CUDA_HOME}/bin"
        "$ENV{CUDA_PATH}/bin"
        "/usr/local/cuda/bin"
        "/usr/local/cuda-13.2/bin"
)
if(BLITZAR_NVCC_EXECUTABLE AND
   (NOT CMAKE_CUDA_COMPILER OR CMAKE_CUDA_COMPILER MATCHES "NOTFOUND$"))
    set(BLITZAR_DETECTED_CUDA_COMPILER "${BLITZAR_NVCC_EXECUTABLE}")
endif()

project(BLITZAR LANGUAGES CXX)

# Find OpenMP for parallelization support
find_package(OpenMP REQUIRED)

# Detect CUDA toolkit optionally. If found, enable CUDA-aware targets.
include(CheckLanguage)
find_package(CUDAToolkit QUIET)
option(BLITZAR_ALLOW_UNSUPPORTED_CUDA_HOST_COMPILER
    "Allow nvcc to use a host compiler newer than NVIDIA officially supports" OFF)
set(BLITZAR_CUDA_HOST_COMPILER_SUPPORTED ON)
set(BLITZAR_CUDA_HOST_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
set(BLITZAR_CUDA_HOST_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_CUDA_HOST_COMPILER)
    execute_process(
        COMMAND "${CMAKE_CUDA_HOST_COMPILER}" -dumpfullversion -dumpversion
        OUTPUT_VARIABLE BLITZAR_CUDA_HOST_COMPILER_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(BLITZAR_CUDA_HOST_COMPILER_ID "GNU")
endif()
if(BLITZAR_CUDA_HOST_COMPILER_ID STREQUAL "GNU" AND
   BLITZAR_CUDA_HOST_COMPILER_VERSION VERSION_GREATER_EQUAL 16 AND
   NOT BLITZAR_ALLOW_UNSUPPORTED_CUDA_HOST_COMPILER)
    set(BLITZAR_CUDA_HOST_COMPILER_SUPPORTED OFF)
    unset(CMAKE_CUDA_COMPILER CACHE)
    message(WARNING
        "CUDA toolkit detected, but GCC ${BLITZAR_CUDA_HOST_COMPILER_VERSION} is newer than the "
        "host compiler supported by this CUDA toolkit; building without CUDA support. "
        "Install/select GCC 15 or older, or configure with "
        "-DBLITZAR_ALLOW_UNSUPPORTED_CUDA_HOST_COMPILER=ON to enable GPU targets.")
elseif(BLITZAR_CUDA_HOST_COMPILER_ID STREQUAL "GNU" AND
       BLITZAR_CUDA_HOST_COMPILER_VERSION VERSION_GREATER_EQUAL 16)
    if(NOT CMAKE_CUDA_FLAGS MATCHES "(^| )-allow-unsupported-compiler( |$)")
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler"
            CACHE STRING "CUDA compiler flags" FORCE)
    endif()
    message(WARNING
        "CUDA toolkit detected with unsupported GCC ${BLITZAR_CUDA_HOST_COMPILER_VERSION}; "
        "continuing because BLITZAR_ALLOW_UNSUPPORTED_CUDA_HOST_COMPILER=ON.")
endif()
if(BLITZAR_CUDA_HOST_COMPILER_SUPPORTED AND BLITZAR_DETECTED_CUDA_COMPILER AND
   (NOT CMAKE_CUDA_COMPILER OR CMAKE_CUDA_COMPILER MATCHES "NOTFOUND$"))
    set(CMAKE_CUDA_COMPILER "${BLITZAR_DETECTED_CUDA_COMPILER}" CACHE FILEPATH "CUDA compiler" FORCE)
endif()
if(BLITZAR_CUDA_HOST_COMPILER_SUPPORTED)
    check_language(CUDA)
endif()
if(CUDAToolkit_FOUND AND CMAKE_CUDA_COMPILER)
    message(STATUS "CUDA toolkit detected; enabling CUDA targets")
    enable_language(CUDA)
    set(BLITZAR_ENABLE_CUDA ON)
elseif(CUDAToolkit_FOUND)
    message(STATUS "CUDA toolkit detected but CUDA compiler unavailable; building without CUDA support")
    set(BLITZAR_ENABLE_CUDA OFF)
    set(BLITZAR_BUILD_TESTS OFF CACHE BOOL "Build deterministic physics regression tests" FORCE)
else()
    message(STATUS "CUDA toolkit not found; building without CUDA support")
    set(BLITZAR_ENABLE_CUDA OFF)
    set(BLITZAR_BUILD_TESTS OFF CACHE BOOL "Build deterministic physics regression tests" FORCE)
endif()

include(cmake/targets.cmake)

if(BLITZAR_BUILD_TESTS)
    enable_testing()
    BLITZAR_ensure_gtest()
    include(GoogleTest)

    set(BLITZAR_PHYSICS_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/TST_PHY_ProfileTests.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/checkpoint.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/export_queue.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/extended_solvers.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/force_law_policy.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/multibody.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/octree_criteria.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/octree_gpu_stability.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/orbit.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/reentrancy.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/runtime_reload.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/solver.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/substep_policy.cpp"
    )
    set(BLITZAR_CORE_API_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/ffi/core_api.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/ffi/vtk_conformance.cpp"
    )
    if(BLITZAR_PHYSICS_TEST_SOURCES)
        add_executable(blitzarPhysicsGTests
            tests/support/physics_scenario.cpp
            tests/support/physics_test_utils.cpp
            runtime/src/protocol/Protocol.cpp
            ${BLITZAR_PHYSICS_TEST_SOURCES}
            ${BLITZAR_SERVER_SOURCES}
        )
        if(BLITZAR_ENABLE_CUDA)
            configure_BLITZAR_cuda_target(blitzarPhysicsGTests)
        else()
            configure_BLITZAR_cpp_target(blitzarPhysicsGTests)
        endif()
        target_compile_definitions(blitzarPhysicsGTests
            PRIVATE
                BLITZAR_ENABLE_GTEST_FIXTURE=1
                BLITZAR_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )
        target_link_libraries(blitzarPhysicsGTests
            PRIVATE
                GTest::gtest_main
        )
        gtest_discover_tests(blitzarPhysicsGTests PROPERTIES LABELS "gpu")
    endif()

    if(BLITZAR_CORE_API_TEST_SOURCES)
        add_executable(blitzarCoreApiGTests
            ${BLITZAR_CORE_API_TEST_SOURCES}
        )
        if(BLITZAR_ENABLE_CUDA)
            configure_BLITZAR_cuda_target(blitzarCoreApiGTests)
        else()
            configure_BLITZAR_cpp_target(blitzarCoreApiGTests)
        endif()
        target_link_libraries(blitzarCoreApiGTests
            PRIVATE
                GTest::gtest_main
                blitzarCoreFfi
        )
        gtest_discover_tests(blitzarCoreApiGTests PROPERTIES LABELS "unit")
    endif()

    add_executable(blitzarNumericalValidationTool
        tests/tools/numerical_validation_main.cpp
        tests/support/numerical_validation_tool.cpp
        tests/support/physics_scenario.cpp
        tests/support/physics_test_utils.cpp
        runtime/src/protocol/Protocol.cpp
        ${BLITZAR_SERVER_SOURCES}
    )
    if(BLITZAR_ENABLE_CUDA)
        configure_BLITZAR_cuda_target(blitzarNumericalValidationTool)
    else()
        configure_BLITZAR_cpp_target(blitzarNumericalValidationTool)
    endif()
    target_compile_definitions(blitzarNumericalValidationTool
        PRIVATE
            BLITZAR_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_executable(blitzarPerformanceBenchmarkTool
        tests/tools/performance_benchmark_main.cpp
        tests/support/performance_benchmark_tool.cpp
        tests/support/physics_scenario.cpp
        tests/support/physics_test_utils.cpp
        runtime/src/protocol/Protocol.cpp
        ${BLITZAR_SERVER_SOURCES}
    )
    if(BLITZAR_ENABLE_CUDA)
        configure_BLITZAR_cuda_target(blitzarPerformanceBenchmarkTool)
    else()
        configure_BLITZAR_cpp_target(blitzarPerformanceBenchmarkTool)
    endif()
    target_compile_definitions(blitzarPerformanceBenchmarkTool
        PRIVATE
            BLITZAR_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_subdirectory(tests)
endif()
