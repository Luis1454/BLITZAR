if(NOT DEFINED BLITZAR_CLI)
    message(FATAL_ERROR "BLITZAR_CLI is required")
endif()

if(NOT DEFINED MPIEXEC_EXECUTABLE OR NOT DEFINED MPIEXEC_NUMPROC_FLAG)
    message(FATAL_ERROR "MPIEXEC_EXECUTABLE and MPIEXEC_NUMPROC_FLAG are required")
endif()

if(NOT DEFINED BLITZAR_TEST_ROOT)
    set(BLITZAR_TEST_ROOT "${CMAKE_CURRENT_BINARY_DIR}/blitzar-shard-output-685")
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

function(CheckShardHeader path rank)
    file(READ "${path}" header_hex OFFSET 32 LIMIT 11 HEX)
    string(TOLOWER "${header_hex}" header_hex)

    if(rank EQUAL 0)
        set(rank_hex "00000000")
    elseif(rank EQUAL 1)
        set(rank_hex "01000000")
    else()
        message(FATAL_ERROR "unsupported test rank: ${rank}")
    endif()

    set(expected_prefix "02000000${rank_hex}000101")
    if(NOT header_hex MATCHES "^${expected_prefix}")
        message(FATAL_ERROR "invalid shard header for rank ${rank}: ${path}")
    endif()
endfunction()

file(REMOVE_RECURSE "${BLITZAR_TEST_ROOT}")
file(MAKE_DIRECTORY "${BLITZAR_TEST_ROOT}")

set(RUN_ROOT "${BLITZAR_TEST_ROOT}/distributed")
file(MAKE_DIRECTORY "${RUN_ROOT}")

set(RUN_CONFIG [=[simulation(particle_count=8, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=685, deterministic=true)
run(steps=2)
output(directory="output", every_steps=1, write_initial=true, write_final=true)
]=])

set(CONFIG "${RUN_ROOT}/run.ini")
file(WRITE "${CONFIG}" "${RUN_CONFIG}")

set(MPI_COMMAND
    "${CMAKE_COMMAND}" -E env
    "OMPI_ALLOW_RUN_AS_ROOT=1"
    "OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1"
    "OMPI_MCA_rmaps_base_oversubscribe=1"
    "${MPIEXEC_EXECUTABLE}" "${MPIEXEC_NUMPROC_FLAG}" "2"
)
list(APPEND MPI_COMMAND ${MPIEXEC_PREFLAGS} "${BLITZAR_CLI}" --format json --config "${CONFIG}")
list(APPEND MPI_COMMAND ${MPIEXEC_POSTFLAGS})

RunCommand(first_result first_output first_error ${MPI_COMMAND})

if(NOT first_result EQUAL 0)
    message(FATAL_ERROR "two-rank shard output run failed: ${first_error}")
endif()

if(NOT first_error STREQUAL "")
    message(FATAL_ERROR "two-rank shard output run wrote stderr: ${first_error}")
endif()

set(OUTPUT_ROOT "${RUN_ROOT}/output")
RequireFile("${OUTPUT_ROOT}/manifest.json")
if(NOT IS_DIRECTORY "${OUTPUT_ROOT}/states" OR NOT IS_DIRECTORY "${OUTPUT_ROOT}/diagnostics")
    message(FATAL_ERROR "distributed output directories are incomplete")
endif()

file(READ "${OUTPUT_ROOT}/manifest.json" MANIFEST)
foreach(step IN ITEMS 0 1 2)
    foreach(rank IN ITEMS 0 1)
        set(STEP_TEXT "0000000${step}")
        set(SHARD "state-${STEP_TEXT}.rank-0000000${rank}.bin")
        RequireFile("${OUTPUT_ROOT}/states/${SHARD}")
        CheckShardHeader("${OUTPUT_ROOT}/states/${SHARD}" ${rank})
        if(EXISTS "${OUTPUT_ROOT}/states/${SHARD}.tmp")
            message(FATAL_ERROR "temporary shard remains: ${SHARD}")
        endif()
        string(FIND "${MANIFEST}" "${SHARD}" shard_position)
        if(shard_position EQUAL -1)
            message(FATAL_ERROR "manifest does not reference ${SHARD}")
        endif()
    endforeach()
endforeach()

string(FIND "${MANIFEST}" "\"rank_count\": 2" rank_count_position)
string(FIND "${MANIFEST}" "\"shards\": [" shards_position)
string(FIND "${MANIFEST}" "\"completed_output_count\": 3" count_position)
if(rank_count_position EQUAL -1 OR shards_position EQUAL -1 OR count_position EQUAL -1)
    message(FATAL_ERROR "distributed manifest is missing the shard contract")
endif()

file(GLOB STATE_FILES "${OUTPUT_ROOT}/states/*")
list(LENGTH STATE_FILES STATE_COUNT)
if(NOT STATE_COUNT EQUAL 6)
    message(FATAL_ERROR "expected six rank shards, found ${STATE_COUNT}")
endif()

set(POSTPROCESS_COMMAND "${BLITZAR_CLI}" --post-process "${OUTPUT_ROOT}" --format json)
RunCommand(postprocess_result postprocess_output postprocess_error ${POSTPROCESS_COMMAND})
if(NOT postprocess_result EQUAL 0)
    message(FATAL_ERROR "distributed post-process failed: ${postprocess_error}")
endif()
if(NOT postprocess_error STREQUAL "")
    message(FATAL_ERROR "distributed post-process wrote stderr: ${postprocess_error}")
endif()
RequireFile("${OUTPUT_ROOT}/postProcessing/conservation.csv")
string(FIND "${postprocess_output}" "\"snapshot_count\":3" postprocess_snapshot_position)
if(postprocess_snapshot_position EQUAL -1)
    message(FATAL_ERROR "distributed post-process summary has the wrong snapshot count")
endif()

RunCommand(repeat_result repeat_output repeat_error ${MPI_COMMAND})
if(repeat_result EQUAL 0)
    message(FATAL_ERROR "distributed shard rerun unexpectedly succeeded")
endif()
string(FIND "${repeat_error}" "\"exit_code\":5" repeat_exit_code_position)
string(FIND "${repeat_error}" "\"phase\":\"output-prepare\"" repeat_phase_position)
if(repeat_exit_code_position EQUAL -1 OR repeat_phase_position EQUAL -1)
    message(FATAL_ERROR "distributed rerun did not report output preparation failure: ${repeat_error}")
endif()

set(RESTART_ROOT "${BLITZAR_TEST_ROOT}/restart")
file(MAKE_DIRECTORY "${RESTART_ROOT}")
set(RESTART_CONFIG [=[simulation(particle_count=8, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=685, deterministic=true)
run(steps=3)
restart(directory="../distributed/output", step=1)
output(directory="output", every_steps=1, write_initial=true, write_final=true)
]=])
set(RESTART_FILE "${RESTART_ROOT}/run.ini")
file(WRITE "${RESTART_FILE}" "${RESTART_CONFIG}")

set(RESTART_COMMAND
    "${CMAKE_COMMAND}" -E env
    "OMPI_ALLOW_RUN_AS_ROOT=1"
    "OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1"
    "OMPI_MCA_rmaps_base_oversubscribe=1"
    "${MPIEXEC_EXECUTABLE}" "${MPIEXEC_NUMPROC_FLAG}" "2"
)
list(APPEND RESTART_COMMAND ${MPIEXEC_PREFLAGS} "${BLITZAR_CLI}" --format json --config "${RESTART_FILE}")
list(APPEND RESTART_COMMAND ${MPIEXEC_POSTFLAGS})

RunCommand(restart_result restart_output restart_error ${RESTART_COMMAND})
if(NOT restart_result EQUAL 0)
    message(FATAL_ERROR "distributed restart failed: ${restart_error}")
endif()
if(NOT restart_error STREQUAL "")
    message(FATAL_ERROR "distributed restart wrote stderr: ${restart_error}")
endif()
RequireFile("${RESTART_ROOT}/output/manifest.json")

file(REMOVE_RECURSE "${BLITZAR_TEST_ROOT}")
