# Explicit source manifests for all test target compilation groups.
# Rationale: file(GLOB) masks build-truth and prevents reviewers from verifying
# test scope at a glance. All sources are listed explicitly; add new files here
# when adding new test cases.
# Issue: Closes #287

set(GRAVITY_TEST_UNIT_CONFIG_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/config/args_cli.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/args_cli_usage.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/args_io.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/args_io_directive.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/args_io_validation.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/args_octree.cpp"
)

set(GRAVITY_TEST_UNIT_PROTOCOL_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/protocol/json_codec.cpp"
)

set(GRAVITY_TEST_UNIT_MODULE_CLI_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_executor.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_parser.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/server_ops_validation.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/text_and_commands.cpp"
)

set(GRAVITY_TEST_UNIT_CLIENT_HOST_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/module_client/cli_args_text.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_client/client_module_boundary.cpp"
)

set(GRAVITY_TEST_UNIT_PHYSICS_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/TST_PHY_ProfileTests.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/checkpoint.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/export_queue.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/extended_solvers.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/force_law_policy.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/multibody.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/octree_criteria.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/orbit.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/reentrancy.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/runtime_reload.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/solver.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/physics/substep_policy.cpp"
)

set(GRAVITY_TEST_UNIT_UI_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/ui/qt_mainwindow_controller.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/ui/qt_mainwindow_presenter.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/ui/qt_octree_overlay.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/ui/qt_throughput_advisor.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/ui/qt_workspace_layout_store.cpp"
)

set(GRAVITY_TEST_INT_PROTOCOL_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/int/protocol/connect.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/protocol/control.cpp"
)

set(GRAVITY_TEST_INT_BRIDGE_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/bridge.cpp"
)

set(GRAVITY_TEST_INT_RUNTIME_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime_draw_cap.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime_reload.cpp"
)

set(GRAVITY_TEST_INT_UI_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/int/ui/qt_window.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/ui/qt_window_visual.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/ui/qt_workspace_controls.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/ui/qt_workspace_overlay_controls.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/ui/qt_workspace_runtime_controls.cpp"
)

include("${CMAKE_CURRENT_LIST_DIR}/targets_gtests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/targets_repo_checks.cmake")
