# @file tests/cmake/targets.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

# Explicit source manifests for all test target compilation groups.
# Rationale: file(GLOB) masks build-truth and prevents reviewers from verifying
# test scope at a glance. All sources are listed explicitly; add new files here
# when adding new test cases.
# Issue: Closes #287

set(BLITZAR_TEST_UNIT_CONFIG_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/unit/config/args_cli.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/args_cli_usage.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/args_io.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/args_io_directive.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/args_io_validation.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/args_octree.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/env_utils.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/init_plan_edges.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/scenario_validation.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/scenario_validation_edges.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/scenario_validation_init_modes.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/config/simulation_profile.cpp"
)

set(BLITZAR_TEST_UNIT_PROTOCOL_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/unit/protocol/json_codec.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/protocol/json_codec_parse.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/protocol/json_codec_parse_edge_cases.cpp"
)

set(BLITZAR_TEST_UNIT_MODULE_CLI_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_batch_runner.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_catalog.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_executor.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_executor_flows.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_executor_flows_profile_export.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_parser.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_parser_expansion_a.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/command_parser_expansion_b.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/server_ops_validation.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_cli/text_and_commands.cpp"
)

set(BLITZAR_TEST_UNIT_CLIENT_HOST_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/unit/module_client/client_common.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_client/cli_args_text.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_client/client_module_boundary.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/module_client/platform_adapters.cpp"
)

set(BLITZAR_TEST_UNIT_PHYSICS_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/TST_PHY_ProfileTests.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/checkpoint.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/export_queue.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/extended_solvers.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/force_law_policy.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/multibody.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/octree_criteria.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/orbit.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/reentrancy.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/runtime_reload.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/solver.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/physics/substep_policy.cpp"
)

set(BLITZAR_TEST_UNIT_UI_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/unit/ui/qt_mainwindow_controller.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/ui/qt_mainwindow_presenter.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/ui/qt_octree_overlay.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/ui/qt_throughput_advisor.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/ui/qt_workspace_layout_store.cpp"
    "${BLITZAR_ROOT_DIR}/tests/unit/ui/TST_UNT_UI_UiEnums.cpp"
)

set(BLITZAR_TEST_INT_PROTOCOL_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/int/protocol/connect.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/protocol/control.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/protocol/replay.cpp"
)

set(BLITZAR_TEST_INT_BRIDGE_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/int/runtime/bridge.cpp"
)

set(BLITZAR_TEST_INT_RUNTIME_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/int/runtime/runtime.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/runtime/runtime_draw_cap.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/runtime/runtime_reload.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/runtime/runtime_unit_like.cpp"
)

set(BLITZAR_TEST_INT_UI_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/int/ui/qt_window.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/ui/qt_window_visual.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/ui/qt_workspace_controls.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/ui/qt_workspace_overlay_controls.cpp"
    "${BLITZAR_ROOT_DIR}/tests/int/ui/qt_workspace_runtime_controls.cpp"
)

include("${CMAKE_CURRENT_LIST_DIR}/targets_gtests.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/targets_repo_checks.cmake")
