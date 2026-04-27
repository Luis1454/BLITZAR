# @file tests/cmake/targets.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

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
    "${GRAVITY_ROOT_DIR}/tests/unit/config/env_utils.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/init_plan_edges.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/scenario_validation.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/scenario_validation_edges.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/scenario_validation_init_modes.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/config/simulation_profile.cpp"
)

set(GRAVITY_TEST_UNIT_PROTOCOL_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/protocol/json_codec.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/protocol/json_codec_parse.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/protocol/json_codec_parse_edge_cases.cpp"
)

set(GRAVITY_TEST_UNIT_MODULE_CLI_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_batch_runner.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_catalog.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_executor.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_executor_flows.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_executor_flows_profile_export.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_parser.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_parser_expansion_a.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/command_parser_expansion_b.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/server_ops_validation.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_cli/text_and_commands.cpp"
)

set(GRAVITY_TEST_UNIT_CLIENT_HOST_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/unit/module_client/client_common.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_client/cli_args_text.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_client/client_module_boundary.cpp"
    "${GRAVITY_ROOT_DIR}/tests/unit/module_client/platform_adapters.cpp"
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
    "${GRAVITY_ROOT_DIR}/tests/int/protocol/replay.cpp"
)

set(GRAVITY_TEST_INT_BRIDGE_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/bridge.cpp"
)

set(GRAVITY_TEST_INT_RUNTIME_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime_draw_cap.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime_reload.cpp"
    "${GRAVITY_ROOT_DIR}/tests/int/runtime/runtime_unit_like.cpp"
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
