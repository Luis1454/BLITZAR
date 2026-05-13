# @file cmake/clang-format-runner.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

# This CMake script runs clang-format on source files.
# Usage:
#   cmake -DMODE=check -DCLANG_FORMAT_EXECUTABLE=<path> -DSOURCE_DIR=<path> -P cmake/clang-format-runner.cmake
#   cmake -DMODE=fix   -DCLANG_FORMAT_EXECUTABLE=<path> -DSOURCE_DIR=<path> -P cmake/clang-format-runner.cmake

if(NOT DEFINED MODE OR NOT DEFINED CLANG_FORMAT_EXECUTABLE OR NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "Usage: cmake -DMODE=check|fix -DCLANG_FORMAT_EXECUTABLE=<path> -DSOURCE_DIR=<path> -P cmake/clang-format-runner.cmake")
endif()

if(NOT MODE MATCHES "^(check|fix)$")
    message(FATAL_ERROR "MODE must be 'check' or 'fix', got: ${MODE}")
endif()

if(NOT EXISTS "${CLANG_FORMAT_EXECUTABLE}")
    message(FATAL_ERROR "clang-format not found at: ${CLANG_FORMAT_EXECUTABLE}")
endif()

if(NOT EXISTS "${SOURCE_DIR}")
    message(FATAL_ERROR "Source directory not found: ${SOURCE_DIR}")
endif()

set(_clang_format_globs
    "${SOURCE_DIR}/engine/**/*.cpp"
    "${SOURCE_DIR}/engine/**/*.hpp"
    "${SOURCE_DIR}/engine/**/*.cu"
    "${SOURCE_DIR}/runtime/**/*.cpp"
    "${SOURCE_DIR}/runtime/**/*.hpp"
    "${SOURCE_DIR}/runtime/**/*.cu"
    "${SOURCE_DIR}/modules/**/*.cpp"
    "${SOURCE_DIR}/modules/**/*.hpp"
    "${SOURCE_DIR}/modules/**/*.cu"
    "${SOURCE_DIR}/apps/**/*.cpp"
    "${SOURCE_DIR}/apps/**/*.hpp"
    "${SOURCE_DIR}/apps/**/*.cu"
    "${SOURCE_DIR}/tests/**/*.cpp"
    "${SOURCE_DIR}/tests/**/*.hpp"
    "${SOURCE_DIR}/tests/**/*.cu"
)

file(GLOB_RECURSE _all_source_files ${_clang_format_globs})

set(_filtered_files)
foreach(_file IN LISTS _all_source_files)
    if(_file MATCHES "[/\\]build[/\\]" OR _file MATCHES "[/\\]build-[^/\\]+[/\\]")
        continue()
    endif()
    list(APPEND _filtered_files "${_file}")
endforeach()

list(LENGTH _filtered_files _file_count)
if(_file_count EQUAL 0)
    message(STATUS "No source files found for clang-format.")
    return()
endif()

message(STATUS "clang-format mode=${MODE}, files=${_file_count}")

set(_failed_files)
set(_processed 0)
foreach(_file IN LISTS _filtered_files)
    if(MODE STREQUAL "check")
        execute_process(
            COMMAND "${CLANG_FORMAT_EXECUTABLE}" -n --Werror "${_file}"
            RESULT_VARIABLE _result
            OUTPUT_QUIET
            ERROR_QUIET
        )
    else()
        execute_process(
            COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i "${_file}"
            RESULT_VARIABLE _result
            OUTPUT_QUIET
            ERROR_QUIET
        )
    endif()

    if(NOT _result EQUAL 0)
        list(APPEND _failed_files "${_file}")
    endif()

    math(EXPR _processed "${_processed} + 1")
endforeach()

list(LENGTH _failed_files _failed_count)
if(_failed_count GREATER 0)
    foreach(_failed IN LISTS _failed_files)
        message(STATUS "clang-format failed: ${_failed}")
    endforeach()
    message(FATAL_ERROR "clang-format ${MODE} failed on ${_failed_count} file(s)")
endif()

message(STATUS "clang-format ${MODE} completed successfully for ${_processed} file(s)")
