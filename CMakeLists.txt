# @file CMakeLists.txt
# @author Luis1454
# @project BLITZAR
# @brief Source artifact for the BLITZAR simulation project.

cmake_minimum_required(VERSION 3.24)

include(cmake/win.cmake)

project(BLITZAR LANGUAGES CUDA CXX)

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
            runtime/src/protocol/ServerProtocol.cpp
            ${BLITZAR_PHYSICS_TEST_SOURCES}
            ${BLITZAR_SERVER_SOURCES}
        )
        configure_BLITZAR_cuda_target(blitzarPhysicsGTests)
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
        configure_BLITZAR_cuda_target(blitzarCoreApiGTests)
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
        runtime/src/protocol/ServerProtocol.cpp
        ${BLITZAR_SERVER_SOURCES}
    )
    configure_BLITZAR_cuda_target(blitzarNumericalValidationTool)
    target_compile_definitions(blitzarNumericalValidationTool
        PRIVATE
            BLITZAR_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_executable(blitzarPerformanceBenchmarkTool
        tests/tools/performance_benchmark_main.cpp
        tests/support/performance_benchmark_tool.cpp
        tests/support/physics_scenario.cpp
        tests/support/physics_test_utils.cpp
        runtime/src/protocol/ServerProtocol.cpp
        ${BLITZAR_SERVER_SOURCES}
    )
    configure_BLITZAR_cuda_target(blitzarPerformanceBenchmarkTool)
    target_compile_definitions(blitzarPerformanceBenchmarkTool
        PRIVATE
            BLITZAR_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_subdirectory(tests)
endif()

