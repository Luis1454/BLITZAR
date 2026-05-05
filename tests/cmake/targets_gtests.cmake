# @file tests/cmake/targets_gtests.cmake
# @author Luis1454
# @project BLITZAR
# @brief Automated verification assets for BLITZAR quality gates.

if(WIN32)
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_win.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtilsWin.cpp"
    )
else()
    set(BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE "${BLITZAR_ROOT_DIR}/tests/support/scoped_env_var_posix.cpp")
    set(BLITZAR_TEST_ENV_UTILS_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtilsPosix.cpp"
    )
endif()

if(BLITZAR_TEST_UNIT_CONFIG_SOURCES)
    BLITZAR_add_gtest(blitzarConfigArgsGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_CONFIG_SOURCES}
            ${BLITZAR_TEST_ENV_UTILS_SOURCES}
            
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgs.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsCoreOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsClientOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsInitOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsInitStateOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsFluidOptions.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationProfile.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidation.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationPhysics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationRender.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveStreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/TextParse.cpp"
    )
endif()

if(BLITZAR_TEST_UNIT_PROTOCOL_SOURCES)
    BLITZAR_add_gtest(blitzarProtocolCodecGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_PROTOCOL_SOURCES}
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodec.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParse.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseStatus.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseSnapshot.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecReadNumber.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/protocol/ServerProtocol.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/TextParse.cpp"
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
            "${BLITZAR_ROOT_DIR}/modules/cli/module_cli_state.cpp"
            "${BLITZAR_ROOT_DIR}/modules/cli/module_cli_text.cpp"
            "${BLITZAR_ROOT_DIR}/modules/cli/module_cli_server_ops.cpp"
            "${BLITZAR_ROOT_DIR}/modules/cli/module_cli_commands.cpp"
            ${BLITZAR_RUNTIME_COMMAND_SOURCES}
            "${BLITZAR_ROOT_DIR}/apps/client-host/client_host_cli.cpp"
            "${BLITZAR_ROOT_DIR}/apps/client-host/client_host_cli_args.cpp"
            "${BLITZAR_ROOT_DIR}/apps/client-host/client_host_module_ops.cpp"
            "${BLITZAR_ROOT_DIR}/apps/client-host/client_host_cli_text.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationProfile.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidation.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationPhysics.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationRender.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveStreamWriter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveValueFormatter.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ErrorBuffer.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientModuleApi.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientModuleBoundary.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientModuleHash.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientModuleHandle.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientModuleHandleLoad.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientModuleManifest.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientCommon.cpp"
            "${BLITZAR_ROOT_DIR}/runtime/src/client/RustRuntimeBridgeState.cpp"
            ${BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE}
            ${BLITZAR_ENV_UTILS_SOURCES}
            ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
            "${BLITZAR_ROOT_DIR}/engine/src/config/TextParse.cpp"
        LIBS
            blitzarRustRuntime
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()
set(BLITZAR_TEST_BASE_REAL_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/server_harness.cpp"
    "${BLITZAR_ROOT_DIR}/tests/support/server_harness_runtime.cpp"
    "${BLITZAR_TEST_SCOPED_ENV_VAR_SOURCE}"
    ${BLITZAR_TEST_ENV_UTILS_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
    ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    "${BLITZAR_ROOT_DIR}/engine/src/config/TextParse.cpp"
)
set(BLITZAR_TEST_BASE_BRIDGE_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/poll_utils.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/RustRuntimeBridgeState.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientServerBridge.cpp"
    ${BLITZAR_TEST_BASE_REAL_SOURCES}
)
set(BLITZAR_TEST_BASE_RUNTIME_SOURCES
    "${BLITZAR_ROOT_DIR}/tests/support/client_utils.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientRuntime.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/client/ClientCommon.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationProfile.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidation.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationPhysics.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationScenarioValidationRender.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveStreamWriter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/DirectiveValueFormatter.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
    ${BLITZAR_TEST_BASE_BRIDGE_SOURCES}
)
set(BLITZAR_TEST_BASE_QT_LOGIC_SOURCES
    ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
    "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowController.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowPresenter.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/ui/OctreeOverlay.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/ui/UiEnums.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/ui/ThroughputAdvisor.cpp"
    "${BLITZAR_ROOT_DIR}/modules/qt/ui/WorkspaceLayoutStore.cpp"
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

if(BLITZAR_TEST_INT_BRIDGE_SOURCES)
    BLITZAR_add_gtest(blitzarClientServerBridgeGTests
        LABELS integration integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_BRIDGE_SOURCES}
            ${BLITZAR_TEST_BASE_BRIDGE_SOURCES}
        LIBS
            blitzarRustRuntime
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()

if(BLITZAR_TEST_INT_RUNTIME_SOURCES)
    BLITZAR_add_gtest(blitzarClientRuntimeGTests
        LABELS contract integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_RUNTIME_SOURCES}
            ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
        LIBS
            blitzarRustRuntime
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()

if(BLITZAR_TEST_UNIT_UI_SOURCES)
    BLITZAR_add_gtest(blitzarQtUiLogicGTests
        LABELS unit
        SOURCES
            ${BLITZAR_TEST_UNIT_UI_SOURCES}
            ${BLITZAR_TEST_BASE_QT_LOGIC_SOURCES}
        LIBS
            blitzarRustRuntime
            ${BLITZAR_TEST_PLATFORM_TARGET}
    )
endif()

if(TARGET Qt6::Widgets AND BLITZAR_TEST_INT_UI_SOURCES)
    BLITZAR_add_gtest(blitzarQtMainWindowGTests
        LABELS ui_integration integration_real
        TIMEOUT 45
        SERVER_LOCK
        SOURCES
            ${BLITZAR_TEST_INT_UI_SOURCES}
            ${BLITZAR_TEST_BASE_RUNTIME_SOURCES}
            "${BLITZAR_ROOT_DIR}/tests/support/qt_test_utils.cpp"
            "${BLITZAR_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/EnergyGraphWidget.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/EnergyGraphWidgetPaint.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/energyGraph/renderer.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/energyGraph/plotting.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/energyGraph/theme.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/energyGraph/layout.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/energyGraph/data.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/presenter/formatters.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/presenter/telemetryAggregator.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/presenter/stateComputers.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/shell/dockBuilder.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/shell/menuBuilder.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/shell/themeMenuHandler.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/layout/comboInitializer.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/layout/numericInitializer.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/layout/propertyInitializer.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/fileActions/fileDialogs.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/fileActions/connectorManager.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/fileActions/formatHelpers.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/particleView/gimbalController.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/particleView/particleRasterizer.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/particleView/overlayRenderer.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowController.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindow.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowConfig.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/mainWindow/config/apply.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowControls.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowFileActions.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowLayout.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowLayoutState.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowPresenter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowTelemetry.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowWorkspacePersistence.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MainWindowWorkspaceShell.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/MultiViewWidget.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/OctreeOverlay.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/OctreeOverlayPainter.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/ParticleView.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/ParticleViewColor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/UiEnums.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/ThroughputAdvisor.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/QtTheme.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/QtViewMath.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/WorkspaceLayoutStore.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/panels/PhysicsControlPanel.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/panels/RenderControlPanel.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/panels/RunControlPanel.cpp"
            "${BLITZAR_ROOT_DIR}/modules/qt/ui/panels/SceneSetupPanel.cpp"
            ${BLITZAR_GRAPHICS_SOURCES}
        LIBS
            blitzarRustRuntime
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

