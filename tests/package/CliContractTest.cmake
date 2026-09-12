cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED BLITZAR_SOURCE_DIR OR NOT IS_DIRECTORY "${BLITZAR_SOURCE_DIR}")
    message(FATAL_ERROR "BLITZAR_SOURCE_DIR must name the source tree")
endif()
if(NOT DEFINED BLITZAR_BUILD_DIR OR NOT IS_DIRECTORY "${BLITZAR_BUILD_DIR}")
    message(FATAL_ERROR "BLITZAR_BUILD_DIR must name the test build tree")
endif()

set(probe_dir "${BLITZAR_BUILD_DIR}/shared-cli-contract")
file(REMOVE_RECURSE "${probe_dir}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${BLITZAR_SOURCE_DIR}" -B "${probe_dir}"
        -DBLITZAR_BUILD_SHARED=ON
        -DBLITZAR_BUILD_CLI=ON
        -DBLITZAR_BUILD_TESTS=OFF
        -DBLITZAR_BUILD_EXAMPLES=OFF
        -DBLITZAR_ENABLE_OPENMP=OFF
        -DBLITZAR_HIP_MODE=OFF
        -DBLITZAR_MPI_MODE=OFF
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error)

if(configure_result EQUAL 0)
    message(FATAL_ERROR "shared library and internal CLI unexpectedly configured together")
endif()

set(configure_log "${configure_output}\n${configure_error}")
if(NOT configure_log MATCHES "BLITZAR_BUILD_CLI requires the static library")
    message(FATAL_ERROR
        "shared/CLI configuration failed for an unexpected reason:\n${configure_log}")
endif()
