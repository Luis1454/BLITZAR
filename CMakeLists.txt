cmake_minimum_required(VERSION 3.24)

include(cmake/win.cmake)

project(CUDA_GRAVITY_SIMULATION LANGUAGES CUDA CXX)

include(cmake/targets.cmake)

if(GRAVITY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

