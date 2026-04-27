# File: cmake/core.cmake
# Purpose: CMake build orchestration for BLITZAR targets and tooling.

include("${CMAKE_CURRENT_LIST_DIR}/core/profile.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/core/toolchain.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/core/targets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/core/clang-format.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/qt_runtime_deploy.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/rust.cmake")
