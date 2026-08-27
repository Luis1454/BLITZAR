if(NOT DEFINED BLITZAR_CLI)
    message(FATAL_ERROR "BLITZAR_CLI is required")
endif()

execute_process(
    COMMAND "${BLITZAR_CLI}" --invalid
    RESULT_VARIABLE result
    OUTPUT_VARIABLE standard_output
    ERROR_VARIABLE standard_error
)

if(NOT result EQUAL 2)
    message(FATAL_ERROR "unexpected usage exit code: ${result}")
endif()

if(NOT standard_output STREQUAL "")
    message(FATAL_ERROR "usage wrote to stdout: ${standard_output}")
endif()

set(expected_error [=[{"schema_version":1,"status":1,"phase":"usage","exit_code":2,"message":"invalid argument"}
]=])

if(NOT standard_error STREQUAL expected_error)
    message(FATAL_ERROR "unexpected usage stderr: ${standard_error}")
endif()
