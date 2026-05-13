# @file tests/cmake/targets_gtests.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

if(WIN32)
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_win.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Base.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Win.cpp"
    )
else()
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_posix.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Base.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Posix.cpp"
    )
endif()

if(BLITZAR_TEST_UNIT_CONFIG_SOURCES)
    BLITZAR_add_gtest(blitzarConfigArgsGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_CONFIG_SOURCES}
            ${BLITZAR_TEST_ENV_UTILS_SOURCES}
            
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/Main.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/Parse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/CoreOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/ClientOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/InitOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/InitStateOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/FluidOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Main.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Apply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Entries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Performance.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Main.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Scenario.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Physics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Render.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Config.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Write.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/StreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/ValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/core/Config.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/modes/Normalize.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/text/Parse.cpp"
    )
endif()

if(BLITZAR_TEST_UNIT_PROTOCOL_SOURCES)
    BLITZAR_add_gtest(blitzarProtocolCodecGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_PROTOCOL_SOURCES}
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/JsonCodec.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Parser.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Status.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Snapshot.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/codec/parser/Number.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/Protocol.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/text/Parse.cpp"
    )
endif()

set(BLITZAR_TEST_UNIT_MODULE_SOURCES
    ${BLITZAR_TEST_UNIT_MODULE_CLI_SOURCES}
    ${BLITZAR_TEST_UNIT_CLIENT_HOST_SOURCES}
)
if(BLITZAR_TEST_UNIT_MODULE_SOURCES)
    BLITZAR_add_gtest(blitzarClientCliHostGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_MODULE_SOURCES}
            "${BLITZAR_ROOT_DIR}/modules/cli/State.cpp"
            "${BLITZAR_ROOT_DIR}/modules/cli/Text.cpp"
            "${BLITZAR_ROOT_DIR}/modules/cli/ServerOps.cpp"
            "${BLITZAR_ROOT_DIR}/modules/cli/Commands.cpp"
            ${BLITZAR_RUNTIME_COMMAND_SOURCES}
            "${BLITZAR_ROOT_DIR}/apps/client-host/src/Cli.cpp"
            "${BLITZAR_ROOT_DIR}/apps/client-host/src/CliArgs.cpp"
            "${BLITZAR_ROOT_DIR}/apps/client-host/src/ModuleOps.cpp"
            "${BLITZAR_ROOT_DIR}/apps/client-host/src/CliText.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/args/Parse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Main.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Apply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Entries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Performance.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Main.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Scenario.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Physics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Render.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Config.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Write.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/StreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/directive/ValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/core/Config.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/modes/Normalize.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/diagnostics/ErrorBuffer.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/module/Api.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/module/Boundary.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/module/Hash.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/module/Handle.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/module/Load.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/module/Manifest.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/common/ClientCommon.cpp"
            ${BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE}
            ${BLITZAR_ENV_UTILS_SOURCES}
            ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
            "${BLITZAR_ROOT_DIR}/engine/src/config/text/Parse.cpp"
        LIBS
            ${BLITZAR_TEST_RUST_LIBS}
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()
set(BLITZAR_TEST_BASE_REAL_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/server_harness.cpp"
    "${BLITZAR_ROOT_DIR}/tests/support/server_harness_runtime.cpp"
    "${BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE}"
    ${BLITZAR_TEST_ENV_UTILS_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/Parse.cpp"
    ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/src/config/text/Parse.cpp"
)
set(BLITZAR_TEST_BASE_BRIDGE_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/poll_utils.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/runtime/BridgeState.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/runtime/Bridge.cpp"
    ${BLITZAR_TEST_BASE_REAL_SOURCES}
)
set(BLITZAR_TEST_BASE_RUNTIME_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/client_utils.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/runtime/Runtime.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/common/ClientCommon.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Main.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Apply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Entries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Performance.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Main.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Scenario.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Physics.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/validation/Render.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Config.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/Write.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/StreamWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/directive/ValueFormatter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/core/Config.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/modes/Normalize.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
    ${BLITZAR_TEST_BASE_BRIDGE_SOURCES}
)
set(BLITZAR_TEST_BASE_QT_LOGIC_SOURCES
    ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
    "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/Controller.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/Presenter.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/Octree.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/support/types/Enums.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/support/performance/Throughput.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/support/storage/LayoutStore.cpp"
)

if(BLITZAR_TEST_INT_PROTOCOL_SOURCES)
    BLITZAR_add_gtest(blitzarServerProtocolGTests
        LABELS integration integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_PROTOCOL_SOURCES}
            ${BLITZAR_TEST_BASE_REAL_SOURCES}
        LIBS
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()

if(TARGET blitzarRustRuntime AND BLITZAR_TEST_INT_BRIDGE_SOURCES)
    BLITZAR_add_gtest(blitzarBridgeGTests
        LABELS integration integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_BRIDGE_SOURCES}
            ${BLITZAR_TEST_BASE_BRIDGE_SOURCES}
        LIBS
            ${BLITZAR_TEST_RUST_LIBS}
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()

if(TARGET blitzarRustRuntime AND BLITZAR_TEST_INT_RUNTIME_SOURCES)
    BLITZAR_add_gtest(blitzarRuntimeGTests
        LABELS contract integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_RUNTIME_SOURCES}
            ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
        LIBS
            ${BLITZAR_TEST_RUST_LIBS}
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()

if(TARGET blitzarRustRuntime AND BLITZAR_TEST_UNIT_UI_SOURCES)
    BLITZAR_add_gtest(blitzarQtUiLogicGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_UI_SOURCES}
            ${BLITZAR_TEST_BASE_QT_LOGIC_SOURCES}
        LIBS
            ${BLITZAR_TEST_RUST_LIBS}
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()

if(TARGET Qt6::Widgets AND TARGET blitzarRustRuntime AND BLITZAR_TEST_INT_UI_SOURCES)
    BLITZAR_add_gtest(blitzarQtMainWindowGTests
        LABELS ui_integration integration_real
        TIMEOUT 45
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_UI_SOURCES}
            ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
            "${BLITZAR_ROOT_DIR}/tests/support/qt_test_utils.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/Graph.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/graphs/Paint.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/Controller.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/core/Window.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/core/Widgets.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/config/WindowConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/Controls.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/actions/FileActions.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/Layout.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/layout/State.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/Presenter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/Telemetry.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/workspace/Persistence.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/window/workspace/Shell.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/MultiView.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/Octree.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/Painter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/Particle.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/viewport/Color.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/types/Enums.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/performance/Throughput.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/theme/Theme.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/geometry/ViewMath.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/support/storage/LayoutStore.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/Physics.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/Render.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/Run.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/src/panels/control/SceneSetup.cpp"
            ${BLITZAR_GRAPHICS_SOURCES}
        LIBS
            ${BLITZAR_TEST_RUST_LIBS}
            Qt6::Widgets
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
    BLITZAR_configure_qt_runtime_deploy(blitzarQtMainWindowGTests)
endif()

BLITZAR_add_gtest(blitzarGraphicsGTests
    LABELS unit
    SOURCES
        "${BLITZAR_ROOT_DIR}/tests/unit/graphics/TST_UNT_GRA_GraphicsTests.cpp"
        ${BLITZAR_GRAPHICS_SOURCES}
)
