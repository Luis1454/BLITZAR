if(NOT DEFINED GRAVITY_BUILD_DIR OR GRAVITY_BUILD_DIR STREQUAL "")
    message(FATAL_ERROR "GRAVITY_BUILD_DIR is required")
endif()

if(WIN32)
    set(_exe_suffix ".exe")
else()
    set(_exe_suffix "")
endif()

set(_launcher "${GRAVITY_BUILD_DIR}/myApp${_exe_suffix}")

if(NOT EXISTS "${_launcher}")
    message(FATAL_ERROR "Launcher not found: ${_launcher}")
endif()

function(_run_and_expect _name _expected_code)
    set(options)
    set(oneValueArgs MUST_CONTAIN_STDOUT MUST_CONTAIN_STDERR)
    set(multiValueArgs COMMAND_ARGS)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    execute_process(
        COMMAND "${_launcher}" ${ARG_COMMAND_ARGS}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE _stderr
        TIMEOUT 20
    )

    if(NOT "${_result}" STREQUAL "${_expected_code}")
        message(FATAL_ERROR
            "[${_name}] unexpected exit code: got ${_result}, expected ${_expected_code}\n"
            "stdout:\n${_stdout}\n"
            "stderr:\n${_stderr}")
    endif()

    if(DEFINED ARG_MUST_CONTAIN_STDOUT AND NOT ARG_MUST_CONTAIN_STDOUT STREQUAL "")
        string(FIND "${_stdout}" "${ARG_MUST_CONTAIN_STDOUT}" _stdout_hit)
        if(_stdout_hit EQUAL -1)
            message(FATAL_ERROR
                "[${_name}] missing expected stdout fragment: ${ARG_MUST_CONTAIN_STDOUT}\n"
                "stdout:\n${_stdout}")
        endif()
    endif()

    if(DEFINED ARG_MUST_CONTAIN_STDERR AND NOT ARG_MUST_CONTAIN_STDERR STREQUAL "")
        string(FIND "${_stderr}" "${ARG_MUST_CONTAIN_STDERR}" _stderr_hit)
        if(_stderr_hit EQUAL -1)
            message(FATAL_ERROR
                "[${_name}] missing expected stderr fragment: ${ARG_MUST_CONTAIN_STDERR}\n"
                "stderr:\n${_stderr}")
        endif()
    endif()
endfunction()

_run_and_expect(
    "launcher-help"
    0
    COMMAND_ARGS --help
    MUST_CONTAIN_STDOUT "--mode ui|backend|headless"
)

_run_and_expect(
    "launcher-invalid-mode"
    2
    COMMAND_ARGS --mode nope
    MUST_CONTAIN_STDERR "invalid --mode value"
)

_run_and_expect(
    "launcher-headless-help"
    0
    COMMAND_ARGS --mode headless -- --help
    MUST_CONTAIN_STDOUT "--target-steps"
)

_run_and_expect(
    "launcher-headless-positional-rejected"
    2
    COMMAND_ARGS --mode headless -- 1000
    MUST_CONTAIN_STDERR "unexpected positional argument"
)

_run_and_expect(
    "launcher-backend-unknown-option-rejected"
    2
    COMMAND_ARGS --mode backend -- --foo 1
    MUST_CONTAIN_STDERR "unknown option: --foo"
)

message(STATUS "Launcher contract check passed.")
