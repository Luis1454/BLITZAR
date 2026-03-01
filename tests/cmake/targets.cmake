file(GLOB GRAVITY_TEST_UNIT_CONFIG_SOURCES CONFIGURE_DEPENDS "${GRAVITY_ROOT_DIR}/tests/unit/config/*.cpp")
file(GLOB GRAVITY_TEST_INT_PROTOCOL_SOURCES CONFIGURE_DEPENDS "${GRAVITY_ROOT_DIR}/tests/int/protocol/*.cpp")
file(GLOB GRAVITY_TEST_INT_BRIDGE_SOURCES CONFIGURE_DEPENDS "${GRAVITY_ROOT_DIR}/tests/int/runtime/bridge*.cpp")
file(GLOB GRAVITY_TEST_INT_RUNTIME_SOURCES CONFIGURE_DEPENDS "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime*.cpp")
file(GLOB GRAVITY_TEST_INT_UI_SOURCES CONFIGURE_DEPENDS "${GRAVITY_ROOT_DIR}/tests/int/ui/*.cpp")

if(GRAVITY_TEST_UNIT_CONFIG_SOURCES)
    gravity_add_gtest(gravityConfigArgsGTests
        LABELS unit
        SOURCES
            ${GRAVITY_TEST_UNIT_CONFIG_SOURCES}
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgs.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsCoreOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsFrontendOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitStateOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsFluidOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
    )
endif()

set(GRAVITY_TEST_BASE_REAL_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/support/backend_harness.cpp"
    "${GRAVITY_ROOT_DIR}/tests/support/backend_harness_runtime.cpp"
    "${GRAVITY_ROOT_DIR}/tests/support/scoped_env_var.cpp"
    ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
    "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
)
set(GRAVITY_TEST_BASE_BRIDGE_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/support/poll_utils.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/frontend/FrontendBackendBridge.cpp"
    ${GRAVITY_TEST_BASE_REAL_SOURCES}
)
set(GRAVITY_TEST_BASE_RUNTIME_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/support/frontend_utils.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/frontend/FrontendRuntime.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/frontend/FrontendCommon.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
    ${GRAVITY_TEST_BASE_BRIDGE_SOURCES}
)

if(GRAVITY_TEST_INT_PROTOCOL_SOURCES)
    gravity_add_gtest(gravityBackendProtocolGTests
        LABELS integration integration_real
        TIMEOUT 30
        BACKEND_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_PROTOCOL_SOURCES}
            ${GRAVITY_TEST_BASE_REAL_SOURCES}
        LIBS
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

if(GRAVITY_TEST_INT_BRIDGE_SOURCES)
    gravity_add_gtest(gravityFrontendBackendBridgeGTests
        LABELS integration integration_real
        TIMEOUT 30
        BACKEND_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_BRIDGE_SOURCES}
            ${GRAVITY_TEST_BASE_BRIDGE_SOURCES}
        LIBS
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

if(GRAVITY_TEST_INT_RUNTIME_SOURCES)
    gravity_add_gtest(gravityFrontendRuntimeGTests
        LABELS contract integration_real
        TIMEOUT 30
        BACKEND_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_RUNTIME_SOURCES}
            ${GRAVITY_TEST_BASE_RUNTIME_SOURCES}
        LIBS
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

if(TARGET Qt6::Widgets AND GRAVITY_TEST_INT_UI_SOURCES)
    gravity_add_gtest(gravityQtMainWindowGTests
        LABELS ui_integration integration_real
        TIMEOUT 45
        BACKEND_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_UI_SOURCES}
            ${GRAVITY_TEST_BASE_RUNTIME_SOURCES}
            "${GRAVITY_ROOT_DIR}/tests/support/qt_test_utils.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/backend/SimulationInitConfig.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/EnergyGraphWidget.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/MainWindow.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/MultiViewWidget.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/ParticleView.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/ParticleViewColor.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/QtViewMath.cpp"
        LIBS
            Qt6::Widgets
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

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
    ARGS
        "ini"
        "--config"
        "${GRAVITY_ROOT_DIR}/simulation.ini"
)
gravity_add_python_check(TST_QLT_REPO_002_GravityMirrorCheck check.py
    ARGS
        "mirror"
        "--root"
        "${GRAVITY_ROOT_DIR}"
)

set(_gravity_check_build_targets OFF)
if(TARGET myApp AND TARGET myAppBackend AND TARGET myAppHeadless AND TARGET myAppModuleHost)
    set(_gravity_check_build_targets ON)
endif()
set(_gravity_no_legacy_args
    "--root"
    "${GRAVITY_ROOT_DIR}"
)
if(_gravity_check_build_targets)
    list(APPEND _gravity_no_legacy_args
        "--check-build-targets"
        "--build-dir"
        "${CMAKE_BINARY_DIR}"
    )
endif()
gravity_add_python_check(TST_QLT_REPO_003_GravityNoLegacyCheck check.py
    ARGS
        "no_legacy"
        ${_gravity_no_legacy_args}
)
gravity_add_python_check(TST_QLT_REPO_004_GravityRepoPolicyCheck check.py
    ARGS
        "repo"
        "--root"
        "${GRAVITY_ROOT_DIR}"
)
gravity_add_python_check(TST_QLT_REPO_006_GravityQualityBaselineCheck check.py
    ARGS
        "quality"
        "--root"
        "${GRAVITY_ROOT_DIR}"
)
gravity_add_python_check(TST_QLT_REPO_007_PrPolicyCheck check.py
    ARGS
        "pr_policy"
        "--root"
        "${GRAVITY_ROOT_DIR}"
)
add_test(
    NAME TST_QLT_REPO_008_PyChecksUnit
    COMMAND ${Python3_EXECUTABLE} -m pytest -q ${GRAVITY_ROOT_DIR}/tests/checks/tests
)
set_tests_properties(TST_QLT_REPO_008_PyChecksUnit PROPERTIES LABELS "integration")
gravity_add_python_check(TST_QLT_REPO_009_PythonQualityGate check.py
    ARGS
        "python_quality"
        "--root"
        "${GRAVITY_ROOT_DIR}"
)
if(TARGET myApp)
    gravity_add_python_check(TST_QLT_REPO_005_GravityLauncherCheck check.py
        ARGS
            "launcher"
            "--build-dir"
            "${CMAKE_BINARY_DIR}"
    )
endif()
