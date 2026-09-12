if(NOT DEFINED BLITZAR_CLI)
    message(FATAL_ERROR "BLITZAR_CLI is required")
endif()

execute_process(
    COMMAND "${BLITZAR_CLI}" --format json --invalid
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

execute_process(
    COMMAND "${BLITZAR_CLI}" --format invalid --config missing.ini
    RESULT_VARIABLE invalid_format_result
    OUTPUT_VARIABLE invalid_format_output
    ERROR_VARIABLE invalid_format_error
)

if(NOT invalid_format_result EQUAL 2)
    message(FATAL_ERROR "unexpected invalid-format exit code: ${invalid_format_result}")
endif()

if(NOT invalid_format_output STREQUAL "")
    message(FATAL_ERROR "invalid format wrote to stdout: ${invalid_format_output}")
endif()

set(expected_invalid_format_error [=[BLITZAR error
  phase:        usage
  status:       invalid argument
  message:      invalid argument
  exit code:    2
]=])

if(NOT invalid_format_error STREQUAL expected_invalid_format_error)
    message(FATAL_ERROR "unexpected invalid-format stderr: ${invalid_format_error}")
endif()
