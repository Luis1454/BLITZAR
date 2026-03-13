cmake_minimum_required(VERSION 3.24)

include(cmake/win.cmake)

project(CUDA_GRAVITY_SIMULATION LANGUAGES CUDA CXX)

include(cmake/targets.cmake)

if(GRAVITY_BUILD_TESTS)
    enable_testing()
    gravity_ensure_gtest()
    include(GoogleTest)

    file(GLOB GRAVITY_PHYSICS_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/physics/*.cpp"
    )
    if(GRAVITY_PHYSICS_TEST_SOURCES)
        add_executable(gravityPhysicsGTests
            tests/support/physics_scenario.cpp
            ${GRAVITY_PHYSICS_TEST_SOURCES}
            ${GRAVITY_SERVER_SOURCES}
        )
        configure_gravity_cuda_target(gravityPhysicsGTests)
        target_compile_definitions(gravityPhysicsGTests
            PRIVATE
                GRAVITY_ENABLE_GTEST_FIXTURE=1
                GRAVITY_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        )
        target_link_libraries(gravityPhysicsGTests
            PRIVATE
                GTest::gtest_main
        )
        gtest_discover_tests(gravityPhysicsGTests PROPERTIES LABELS "gpu")
    endif()

    add_executable(gravityNumericalValidationTool
        tests/tools/numerical_validation_main.cpp
        tests/support/numerical_validation_tool.cpp
        tests/support/physics_scenario.cpp
        ${GRAVITY_SERVER_SOURCES}
    )
    configure_gravity_cuda_target(gravityNumericalValidationTool)
    target_compile_definitions(gravityNumericalValidationTool
        PRIVATE
            GRAVITY_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_subdirectory(tests)
endif()

