if(NOT DEFINED BLITZAR_CLI)
    message(FATAL_ERROR "BLITZAR_CLI is required")
endif()

if(NOT DEFINED MPIEXEC_EXECUTABLE OR NOT DEFINED MPIEXEC_NUMPROC_FLAG)
    message(FATAL_ERROR "MPIEXEC_EXECUTABLE and MPIEXEC_NUMPROC_FLAG are required")
endif()

if(NOT DEFINED BLITZAR_TEST_ROOT)
    set(BLITZAR_TEST_ROOT "${CMAKE_CURRENT_BINARY_DIR}/blitzar-output-boundary-649")
endif()

function(RunCommand result_var output_var error_var)
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE command_result
        OUTPUT_VARIABLE command_output
        ERROR_VARIABLE command_error
    )

    set(${result_var} "${command_result}" PARENT_SCOPE)
    set(${output_var} "${command_output}" PARENT_SCOPE)
    set(${error_var} "${command_error}" PARENT_SCOPE)
endfunction()

function(RequireFile path)
    if(NOT EXISTS "${path}" OR IS_DIRECTORY "${path}")
        message(FATAL_ERROR "expected regular file: ${path}")
    endif()
endfunction()

function(CompareFile relative_path)
    set(direct_path "${BLITZAR_DIRECT_ROOT}/output/${relative_path}")
    set(single_path "${BLITZAR_SINGLE_ROOT}/output/${relative_path}")

    RequireFile("${direct_path}")
    RequireFile("${single_path}")

    file(SHA256 "${direct_path}" direct_hash)
    file(SHA256 "${single_path}" single_hash)

    if(NOT direct_hash STREQUAL single_hash)
        message(FATAL_ERROR "single-rank MPI output differs for ${relative_path}")
    endif()
endfunction()

file(REMOVE_RECURSE "${BLITZAR_TEST_ROOT}")
file(MAKE_DIRECTORY "${BLITZAR_TEST_ROOT}")

set(BLITZAR_DIRECT_ROOT "${BLITZAR_TEST_ROOT}/direct")
set(BLITZAR_SINGLE_ROOT "${BLITZAR_TEST_ROOT}/single")
set(BLITZAR_DISTRIBUTED_ROOT "${BLITZAR_TEST_ROOT}/distributed")
file(MAKE_DIRECTORY
    "${BLITZAR_DIRECT_ROOT}"
    "${BLITZAR_SINGLE_ROOT}"
    "${BLITZAR_DISTRIBUTED_ROOT}"
)

set(RUN_CONFIG [=[simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=649, deterministic=true)
run(steps=2)
output(directory="output", every_steps=1, write_initial=true, write_final=true)
]=])

set(DIRECT_CONFIG "${BLITZAR_DIRECT_ROOT}/run.ini")
set(SINGLE_CONFIG "${BLITZAR_SINGLE_ROOT}/run.ini")
set(DISTRIBUTED_CONFIG "${BLITZAR_DISTRIBUTED_ROOT}/run.ini")
file(WRITE "${DIRECT_CONFIG}" "${RUN_CONFIG}")
file(WRITE "${SINGLE_CONFIG}" "${RUN_CONFIG}")
file(WRITE "${DISTRIBUTED_CONFIG}" "${RUN_CONFIG}")

RunCommand(direct_result direct_output direct_error
    "${BLITZAR_CLI}" --config "${DIRECT_CONFIG}"
)

if(NOT direct_result EQUAL 0)
    message(FATAL_ERROR "direct CLI output run failed: ${direct_error}")
endif()

if(NOT direct_error STREQUAL "")
    message(FATAL_ERROR "direct CLI output run wrote stderr: ${direct_error}")
endif()

set(MPI_COMMAND
    "${CMAKE_COMMAND}" -E env
    "OMPI_ALLOW_RUN_AS_ROOT=1"
    "OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1"
    "OMPI_MCA_rmaps_base_oversubscribe=1"
    "${MPIEXEC_EXECUTABLE}" "${MPIEXEC_NUMPROC_FLAG}" "1"
)
list(APPEND MPI_COMMAND ${MPIEXEC_PREFLAGS} "${BLITZAR_CLI}" --config "${SINGLE_CONFIG}")
list(APPEND MPI_COMMAND ${MPIEXEC_POSTFLAGS})

RunCommand(single_result single_output single_error ${MPI_COMMAND})

if(NOT single_result EQUAL 0)
    message(FATAL_ERROR "single-rank MPI output run failed: ${single_error}")
endif()

if(NOT single_error STREQUAL "")
    message(FATAL_ERROR "single-rank MPI output run wrote stderr: ${single_error}")
endif()

set(OUTPUT_FILES
    manifest.json
    states/state-00000000.bin
    states/state-00000001.bin
    states/state-00000002.bin
)
foreach(relative_file IN LISTS OUTPUT_FILES)
    CompareFile("${relative_file}")
endforeach()

set(MPI_COMMAND
    "${CMAKE_COMMAND}" -E env
    "OMPI_ALLOW_RUN_AS_ROOT=1"
    "OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1"
    "OMPI_MCA_rmaps_base_oversubscribe=1"
    "${MPIEXEC_EXECUTABLE}" "${MPIEXEC_NUMPROC_FLAG}" "2"
)
list(APPEND MPI_COMMAND ${MPIEXEC_PREFLAGS} "${BLITZAR_CLI}" --config "${DISTRIBUTED_CONFIG}")
list(APPEND MPI_COMMAND ${MPIEXEC_POSTFLAGS})

RunCommand(distributed_result distributed_output distributed_error ${MPI_COMMAND})

if(distributed_result EQUAL 0)
    message(FATAL_ERROR "multi-rank output unexpectedly succeeded")
endif()

string(FIND "${distributed_error}" "\"status\":5" status_position)
string(FIND "${distributed_error}" "\"phase\":\"output-topology\"" phase_position)

if(status_position EQUAL -1 OR phase_position EQUAL -1)
    message(FATAL_ERROR "multi-rank output did not report output-topology unsupported: ${distributed_error}")
endif()

if(EXISTS "${BLITZAR_DISTRIBUTED_ROOT}/output")
    message(FATAL_ERROR "multi-rank output created an output directory before rejection")
endif()

file(REMOVE_RECURSE "${BLITZAR_TEST_ROOT}")
