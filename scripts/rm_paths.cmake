if(NOT DEFINED TARGETS OR "${TARGETS}" STREQUAL "")
    message(FATAL_ERROR "TARGETS must be provided (semicolon-separated list).")
endif()

if(NOT DEFINED RETRIES OR "${RETRIES}" STREQUAL "")
    set(RETRIES 4)
endif()

if(NOT DEFINED SLEEP_SECONDS OR "${SLEEP_SECONDS}" STREQUAL "")
    set(SLEEP_SECONDS 1)
endif()

set(_pending ${TARGETS})
set(_attempt 1)

while(_attempt LESS_EQUAL RETRIES)
    set(_next_pending "")
    foreach(_path IN LISTS _pending)
        if(EXISTS "${_path}")
            file(REMOVE_RECURSE "${_path}")
            if(EXISTS "${_path}")
                list(APPEND _next_pending "${_path}")
            endif()
        endif()
    endforeach()

    list(LENGTH _next_pending _remaining_count)
    if(_remaining_count EQUAL 0)
        set(_pending "")
        break()
    endif()

    if(_attempt LESS RETRIES)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep "${SLEEP_SECONDS}")
    endif()

    set(_pending ${_next_pending})
    math(EXPR _attempt "${_attempt} + 1")
endwhile()

if(_pending)
    string(JOIN ", " _remaining ${_pending})
    message(FATAL_ERROR
        "Failed to remove: ${_remaining}. "
        "Close processes using these paths (IDE terminals, running binaries, debugger) and retry.")
endif()
