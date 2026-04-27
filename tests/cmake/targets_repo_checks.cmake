# @file tests/cmake/targets_repo_checks.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

function(gravity_add_python_check test_name check_script)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_test(
        NAME ${test_name}
        COMMAND ${Python3_EXECUTABLE}
            ${GRAVITY_ROOT_DIR}/tests/checks/${check_script}
            ${ARG_ARGS}
    )
    set_tests_properties(${test_name} PROPERTIES LABELS "integration")
endfunction()

gravity_add_python_check(TST_QLT_REPO_001_GravityIniCheck check.py
    ARGS "ini" "--config" "${GRAVITY_ROOT_DIR}/simulation.ini"
)
gravity_add_python_check(TST_QLT_REPO_002_GravityMirrorCheck check.py
    ARGS "mirror" "--root" "${GRAVITY_ROOT_DIR}"
)

set(_gravity_check_build_targets OFF)
if(TARGET blitzar AND TARGET blitzar-server AND TARGET blitzar-headless AND TARGET blitzar-client)
    set(_gravity_check_build_targets ON)
endif()
set(_gravity_no_legacy_args "--root" "${GRAVITY_ROOT_DIR}")
if(_gravity_check_build_targets)
    list(APPEND _gravity_no_legacy_args "--check-build-targets" "--build-dir" "${CMAKE_BINARY_DIR}")
endif()

gravity_add_python_check(TST_QLT_REPO_003_GravityNoLegacyCheck check.py
    ARGS "no_legacy" ${_gravity_no_legacy_args}
)
gravity_add_python_check(TST_QLT_REPO_004_GravityRepoPolicyCheck check.py
    ARGS "repo" "--root" "${GRAVITY_ROOT_DIR}"
)
gravity_add_python_check(TST_QLT_REPO_006_GravityQualityBaselineCheck check.py
    ARGS "quality" "--root" "${GRAVITY_ROOT_DIR}"
)
gravity_add_python_check(TST_QLT_REPO_007_PrPolicyCheck check.py
    ARGS "pr_policy" "--root" "${GRAVITY_ROOT_DIR}"
)

add_test(
    NAME TST_QLT_REPO_008_PyChecksUnit
    COMMAND ${Python3_EXECUTABLE} -m pytest
        --basetemp
        ${CMAKE_BINARY_DIR}/pytest-basetemp-tst-qlt-repo-008
        -q
        ${GRAVITY_ROOT_DIR}/tests/checks/suites
    WORKING_DIRECTORY ${GRAVITY_ROOT_DIR}
)
set_tests_properties(TST_QLT_REPO_008_PyChecksUnit PROPERTIES LABELS "integration")

gravity_add_python_check(TST_QLT_REPO_009_PythonQualityGate check.py
    ARGS "python_quality" "--root" "${GRAVITY_ROOT_DIR}"
)
if(TARGET blitzar)
    gravity_add_python_check(TST_QLT_REPO_005_GravityLauncherCheck check.py
        ARGS "launcher" "--build-dir" "${CMAKE_BINARY_DIR}"
    )
endif()
