# @file tests/cmake/targets_gtests.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

if(WIN32)
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_win.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgBase.cpp"
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgWin.cpp"
    )
else()
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_posix.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgBase.cpp"
        "${BLITZAR_ROOT_DIR}/engine/config/env/platform/CfgPosix.cpp"
    )
endif()

if(BLITZAR_TEST_UNIT_CONFIG_SOURCES)
    BLITZAR_add_gtest(blitzarConfigArgsGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_CONFIG_SOURCES}
            ${BLITZAR_TEST_ENV_UTILS_SOURCES}
            
            "${BLITZAR_ROOT_DIR}/engine/config/args/parsing/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/parsing/CfgParse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgCoreOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgClientOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgInitOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgInitStateOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/args/options/CfgFluidOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/runtime/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyScalar.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyNormalized.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesClient.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesInitState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesFluid.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgPerformance.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenario.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioRuntime.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioCosmology.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/physics/CfgPhysics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/render/CfgRender.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgParser.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgLegacyScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgWrite.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgSceneWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgStreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/core/configuration/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/modes/normalization/CfgNormalize.cpp"
            "${BLITZAR_ROOT_DIR}/engine/server/simulation/runtime/SrvSimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/text/parsing/CfgParse.cpp"
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
            "${BLITZAR_ROOT_DIR}/engine/config/text/parsing/CfgParse.cpp"
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
            "${BLITZAR_ROOT_DIR}/engine/config/args/parsing/CfgParse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/runtime/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyScalar.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyNormalized.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesClient.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesInitState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesFluid.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgPerformance.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgMain.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenario.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioRuntime.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioCosmology.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/physics/CfgPhysics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/validation/render/CfgRender.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgParser.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgLegacyScene.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgWrite.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgCore.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgInitialState.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgSceneWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgStreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/core/configuration/CfgConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/config/modes/normalization/CfgNormalize.cpp"
            "${BLITZAR_ROOT_DIR}/engine/server/simulation/runtime/SrvSimulationInitConfig.cpp"
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
            "${BLITZAR_ROOT_DIR}/engine/config/text/parsing/CfgParse.cpp"
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
    "${BLITZAR_ROOT_DIR}/engine/config/args/parsing/CfgParse.cpp"
    ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/config/text/parsing/CfgParse.cpp"
)
set(BLITZAR_TEST_CONFIG_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/config/registry/runtime/CfgMain.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyScalar.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/application/CfgApplyNormalized.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesCore.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesClient.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesInitState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/registry/entries/CfgEntriesFluid.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgPerformance.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/profile/profile/CfgMain.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenario.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioRuntime.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioInitialState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/scenario/CfgScenarioCosmology.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/physics/CfgPhysics.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/validation/render/CfgRender.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgOptions.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgParser.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgLegacyScene.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgWrite.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgCore.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/parsing/CfgInitialState.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/scene/CfgSceneWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgStreamWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/directive/write/CfgValueFormatter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/core/configuration/CfgConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/config/modes/normalization/CfgNormalize.cpp"
    "${BLITZAR_ROOT_DIR}/engine/server/simulation/runtime/SrvSimulationInitConfig.cpp"
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
    "${BLITZAR_ROOT_DIR}/modules/qt/window/control/GuiController.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/window/presentation/GuiPresenter.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/widgets/overlays/GuiOctree.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/support/types/GuiEnums.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/support/performance/GuiThroughput.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/support/storage/GuiLayoutStore.cpp"
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
