# Explicit source manifests for all test target compilation groups.
# Rationale: file(GLOB) masks build-truth and prevents reviewers from verifying
# test scope at a glance. All sources are listed explicitly; add new files here
# when adding new test cases.
# Issue: Closes #287

set(GRAVITY_TEST_UNIT_CONFIG_SOURCES
    "/tests/unit/config/args_cli.cpp"
    "/tests/unit/config/args_cli_usage.cpp"
    "/tests/unit/config/args_io.cpp"
    "/tests/unit/config/args_io_directive.cpp"
    "/tests/unit/config/args_io_validation.cpp"
    "/tests/unit/config/args_octree.cpp"
)

set(GRAVITY_TEST_UNIT_PROTOCOL_SOURCES
    "/tests/unit/protocol/json_codec.cpp"
)

set(GRAVITY_TEST_UNIT_MODULE_CLI_SOURCES
    "/tests/unit/module_cli/command_executor.cpp"
    "/tests/unit/module_cli/command_parser.cpp"
    "/tests/unit/module_cli/server_ops_validation.cpp"
    "/tests/unit/module_cli/text_and_commands.cpp"
)

set(GRAVITY_TEST_UNIT_CLIENT_HOST_SOURCES
    "/tests/unit/module_client/cli_args_text.cpp"
    "/tests/unit/module_client/client_module_boundary.cpp"
)

set(GRAVITY_TEST_UNIT_PHYSICS_SOURCES
    "/tests/unit/physics/TST_PHY_ProfileTests.cpp"
    "/tests/unit/physics/checkpoint.cpp"
    "/tests/unit/physics/export_queue.cpp"
    "/tests/unit/physics/extended_solvers.cpp"
    "/tests/unit/physics/force_law_policy.cpp"
    "/tests/unit/physics/multibody.cpp"
    "/tests/unit/physics/octree_criteria.cpp"
    "/tests/unit/physics/orbit.cpp"
    "/tests/unit/physics/reentrancy.cpp"
    "/tests/unit/physics/runtime_reload.cpp"
    "/tests/unit/physics/solver.cpp"
    "/tests/unit/physics/substep_policy.cpp"
)

set(GRAVITY_TEST_UNIT_UI_SOURCES
    "/tests/unit/ui/qt_mainwindow_controller.cpp"
    "/tests/unit/ui/qt_mainwindow_presenter.cpp"
    "/tests/unit/ui/qt_octree_overlay.cpp"
    "/tests/unit/ui/qt_throughput_advisor.cpp"
    "/tests/unit/ui/qt_workspace_layout_store.cpp"
)

set(GRAVITY_TEST_INT_PROTOCOL_SOURCES
    "/tests/int/protocol/connect.cpp"
    "/tests/int/protocol/control.cpp"
)

set(GRAVITY_TEST_INT_BRIDGE_SOURCES
    "/tests/int/runtime/bridge.cpp"
)

set(GRAVITY_TEST_INT_RUNTIME_SOURCES
    "/tests/int/runtime/runtime.cpp"
    "/tests/int/runtime/runtime_draw_cap.cpp"
    "/tests/int/runtime/runtime_reload.cpp"
)

set(GRAVITY_TEST_INT_UI_SOURCES
    "/tests/int/ui/qt_window.cpp"
    "/tests/int/ui/qt_window_visual.cpp"
    "/tests/int/ui/qt_workspace_controls.cpp"
    "/tests/int/ui/qt_workspace_overlay_controls.cpp"
    "/tests/int/ui/qt_workspace_runtime_controls.cpp"
)

include("/targets_gtests.cmake")
include("/targets_repo_checks.cmake")
