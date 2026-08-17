# @file tests/cmake/targets_gtests.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

if(WIN32)
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_win.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/config/env/CfgBase.cpp"
        "${BLITZAR_ROOT_DIR}/engine/config/env/CfgWin.cpp"
    )
else()
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_posix.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/config/env/CfgBase.cpp"
        "${BLITZAR_ROOT_DIR}/engine/config/env/CfgPosix.cpp"
    )
endif()

if(BLITZAR_TEST_UNIT_CONFIG_SOURCES)
    BLITZAR_add_gtest(blitzarConfigArgsGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_CONFIG_SOURCES}
            ${BLITZAR_TEST_ENV_UTILS_SOURCES}
            
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgParse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgCoreOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgClientOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgInitOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgInitStateOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgFluidOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApplyScalar.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApplyNormalized.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesClient.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesInitState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesFluid.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/CfgPerformance.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenario.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioRuntime.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioCosmology.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgPhysics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgRender.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgParser.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgLegacyScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgWrite.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgSceneWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgStreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/core/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/modes/CfgNormalize.cpp"
            "${BLITZAR_ROOT_DIR}/engine/server/SrvSimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/text/CfgParse.cpp"
    )
endif()

if(BLITZAR_TEST_UNIT_PROTOCOL_SOURCES)
    BLITZAR_add_gtest(blitzarProtocolCodecGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_PROTOCOL_SOURCES}
            "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/PtcJsonCodec.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcParser.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcStatus.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcSnapshot.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/protocol/codec/parser/PtcNumber.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/protocol/PtcProtocol.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/text/CfgParse.cpp"
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
            "${BLITZAR_ROOT_DIR}/engine/config/args/CfgParse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApplyScalar.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApplyNormalized.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesClient.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesInitState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesFluid.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/CfgPerformance.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenario.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioRuntime.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioCosmology.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgPhysics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgRender.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgParser.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgLegacyScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgWrite.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgSceneWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgStreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/core/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/modes/CfgNormalize.cpp"
            "${BLITZAR_ROOT_DIR}/engine/server/SrvSimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/diagnostics/CliErrorBuffer.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/module/CliApi.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/module/CliBoundary.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/module/CliHash.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/module/CliHandle.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/module/CliLoad.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/module/CliManifest.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/client/common/ClientCommon.cpp"
            ${BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE}
            ${BLITZAR_TEST_ENV_UTILS_SOURCES}
            ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
            "${BLITZAR_ROOT_DIR}/engine/config/text/CfgParse.cpp"
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
    "${BLITZAR_ROOT_DIR}/engine/config/args/CfgParse.cpp"
    ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/config/text/CfgParse.cpp"
)
set(BLITZAR_TEST_CONFIG_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgMain.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApplyScalar.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgApplyNormalized.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesCore.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesClient.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesInitState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/CfgEntriesFluid.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/profile/CfgPerformance.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/profile/CfgMain.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenario.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioRuntime.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioInitialState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgScenarioCosmology.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgPhysics.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/CfgRender.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgParser.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgLegacyScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgWrite.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgCore.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgInitialState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgSceneWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgStreamWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/CfgValueFormatter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/core/CfgConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/modes/CfgNormalize.cpp"
    "${BLITZAR_ROOT_DIR}/engine/server/SrvSimulationInitConfig.cpp"
)
set(BLITZAR_TEST_BASE_BRIDGE_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/poll_utils.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/client/runtime/CliBridgeState.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/client/runtime/CliBridge.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/client/runtime/CliCommands.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/client/runtime/CliInitialState.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/client/runtime/CliRemoteSession.cpp"
    ${BLITZAR_TEST_CONFIG_SOURCES}
    ${BLITZAR_TEST_BASE_REAL_SOURCES}
)
set(BLITZAR_TEST_BASE_RUNTIME_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/client_utils.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/client/runtime/CliRuntime.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/client/common/ClientCommon.cpp"
    ${BLITZAR_TEST_BASE_BRIDGE_SOURCES}
)
set(BLITZAR_TEST_BASE_QT_LOGIC_SOURCES
    ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
    "${BLITZAR_ROOT_DIR}/modules/qt/src/window/control/GuiController.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/window/presentation/GuiPresenter.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/widgets/overlays/GuiOctree.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/support/types/GuiEnums.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/support/performance/GuiThroughput.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/src/support/storage/GuiLayoutStore.cpp"
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

include("${CMAKE_CURRENT_LIST_DIR}/targets_qt_gtests.cmake")

BLITZAR_add_gtest(blitzarGraphicsGTests
    LABELS unit
    SOURCES
        "${BLITZAR_ROOT_DIR}/tests/unit/graphics/TST_UNT_GRA_GraphicsTests.cpp"
        ${BLITZAR_GRAPHICS_SOURCES}
)
