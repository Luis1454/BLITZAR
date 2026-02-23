if(NOT DEFINED GRAVITY_ROOT_DIR OR GRAVITY_ROOT_DIR STREQUAL "")
    message(FATAL_ERROR "GRAVITY_ROOT_DIR is required")
endif()

if(NOT DEFINED GRAVITY_CHECK_BUILD_TARGETS)
    set(GRAVITY_CHECK_BUILD_TARGETS OFF)
endif()

set(_root_cmake "${GRAVITY_ROOT_DIR}/CMakeLists.txt")
if(NOT EXISTS "${_root_cmake}")
    message(FATAL_ERROR "Top-level CMakeLists.txt not found at ${_root_cmake}")
endif()

file(READ "${_root_cmake}" _root_content)

set(_forbidden_cmake_tokens
    "GRAVITY_BUILD_LEGACY_FRONTENDS"
    "GRAVITY_BUILD_SFML_FRONTEND"
    "GRAVITY_BUILD_QT_FRONTEND"
    "GRAVITY_BUILD_CLI_FRONTEND"
    "SFML_FRONTEND_NAME"
    "QT_FRONTEND_NAME"
    "CLI_FRONTEND_NAME"
)

foreach(_token IN LISTS _forbidden_cmake_tokens)
    string(FIND "${_root_content}" "${_token}" _found_index)
    if(NOT _found_index EQUAL -1)
        message(FATAL_ERROR "Forbidden legacy token found in top-level CMakeLists.txt: ${_token}")
    endif()
endforeach()

if(GRAVITY_CHECK_BUILD_TARGETS)
    if(NOT DEFINED GRAVITY_BUILD_DIR OR GRAVITY_BUILD_DIR STREQUAL "")
        message(FATAL_ERROR "GRAVITY_BUILD_DIR is required when GRAVITY_CHECK_BUILD_TARGETS=ON")
    endif()

    set(_build_ninja "${GRAVITY_BUILD_DIR}/build.ninja")
    if(NOT EXISTS "${_build_ninja}")
        message(FATAL_ERROR "build.ninja not found at ${_build_ninja}")
    endif()

    file(READ "${_build_ninja}" _ninja_content)

    set(_forbidden_targets
        "myAppQt"
        "myAppFrontend"
        "myAppCli"
    )
    foreach(_target IN LISTS _forbidden_targets)
        string(FIND "${_ninja_content}" "${_target}" _target_index)
        if(NOT _target_index EQUAL -1)
            message(FATAL_ERROR "Forbidden legacy target found in build.ninja: ${_target}")
        endif()
    endforeach()

    set(_expected_targets
        "myApp.exe"
        "myAppBackend.exe"
        "myAppHeadless.exe"
        "myAppModuleHost.exe"
    )
    foreach(_target IN LISTS _expected_targets)
        string(FIND "${_ninja_content}" "${_target}" _target_index)
        if(_target_index EQUAL -1)
            message(FATAL_ERROR "Expected runtime target missing from build.ninja: ${_target}")
        endif()
    endforeach()
endif()

message(STATUS "Legacy standalone frontend guard check passed.")
