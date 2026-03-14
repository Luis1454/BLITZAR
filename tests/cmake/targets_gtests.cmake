if(GRAVITY_TEST_UNIT_CONFIG_SOURCES)
    gravity_add_gtest(gravityConfigArgsGTests
        LABELS unit
        SOURCES
            ${GRAVITY_TEST_UNIT_CONFIG_SOURCES}
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgs.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsCoreOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsClientOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsInitStateOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsFluidOptions.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
    )
endif()

if(GRAVITY_TEST_UNIT_PROTOCOL_SOURCES)
    gravity_add_gtest(gravityProtocolCodecGTests
        LABELS unit
        SOURCES
            ${GRAVITY_TEST_UNIT_PROTOCOL_SOURCES}
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodec.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParse.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseStatus.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecParseSnapshot.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerJsonCodecReadNumber.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/ServerProtocol.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
    )
endif()

set(GRAVITY_TEST_UNIT_MODULE_SOURCES
    ${GRAVITY_TEST_UNIT_MODULE_CLI_SOURCES}
    ${GRAVITY_TEST_UNIT_CLIENT_HOST_SOURCES}
)
if(GRAVITY_TEST_UNIT_MODULE_SOURCES)
    gravity_add_gtest(gravityClientCliHostGTests
        LABELS unit
        SOURCES
            ${GRAVITY_TEST_UNIT_MODULE_SOURCES}
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_state.cpp"
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_text.cpp"
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_server_ops.cpp"
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_commands.cpp"
            "${GRAVITY_ROOT_DIR}/apps/client-host/client_host_cli.cpp"
            "${GRAVITY_ROOT_DIR}/apps/client-host/client_host_cli_args.cpp"
            "${GRAVITY_ROOT_DIR}/apps/client-host/client_host_module_ops.cpp"
            "${GRAVITY_ROOT_DIR}/apps/client-host/client_host_cli_text.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/ErrorBuffer.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientModuleApi.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientModuleBoundary.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientModuleHash.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientModuleHandle.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientModuleHandleLoad.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientModuleManifest.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/client/RustRuntimeBridgeState.cpp"
            ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
            "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
        LIBS
            gravityRustRuntime
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

if(WIN32)
    set(GRAVITY_TEST_SCOPED_ENV_VAR_SOURCE "${GRAVITY_ROOT_DIR}/tests/support/scoped_env_var_win.cpp")
    set(GRAVITY_TEST_ENV_UTILS_SOURCES
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtilsWin.cpp"
    )
else()
    set(GRAVITY_TEST_SCOPED_ENV_VAR_SOURCE "${GRAVITY_ROOT_DIR}/tests/support/scoped_env_var_posix.cpp")
    set(GRAVITY_TEST_ENV_UTILS_SOURCES
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${GRAVITY_ROOT_DIR}/engine/src/config/EnvUtilsPosix.cpp"
    )
endif()

set(GRAVITY_TEST_BASE_REAL_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/support/server_harness.cpp"
    "${GRAVITY_ROOT_DIR}/tests/support/server_harness_runtime.cpp"
    "${GRAVITY_TEST_SCOPED_ENV_VAR_SOURCE}"
    ${GRAVITY_TEST_ENV_UTILS_SOURCES}
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
    ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
    "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
)
set(GRAVITY_TEST_BASE_BRIDGE_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/support/poll_utils.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/client/RustRuntimeBridgeState.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientServerBridge.cpp"
    ${GRAVITY_TEST_BASE_REAL_SOURCES}
)
set(GRAVITY_TEST_BASE_RUNTIME_SOURCES
    "${GRAVITY_ROOT_DIR}/tests/support/client_utils.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientRuntime.cpp"
    "${GRAVITY_ROOT_DIR}/runtime/src/client/ClientCommon.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirective.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfigDirectiveWrite.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
    ${GRAVITY_TEST_BASE_BRIDGE_SOURCES}
)

if(GRAVITY_TEST_INT_PROTOCOL_SOURCES)
    gravity_add_gtest(gravityServerProtocolGTests
        LABELS integration integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_PROTOCOL_SOURCES}
            ${GRAVITY_TEST_BASE_REAL_SOURCES}
        LIBS
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

if(GRAVITY_TEST_INT_BRIDGE_SOURCES)
    gravity_add_gtest(gravityClientServerBridgeGTests
        LABELS integration integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_BRIDGE_SOURCES}
            ${GRAVITY_TEST_BASE_BRIDGE_SOURCES}
        LIBS
            gravityRustRuntime
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

if(GRAVITY_TEST_INT_RUNTIME_SOURCES)
    gravity_add_gtest(gravityClientRuntimeGTests
        LABELS contract integration_real
        TIMEOUT 30
        SERVER_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_RUNTIME_SOURCES}
            ${GRAVITY_TEST_BASE_RUNTIME_SOURCES}
        LIBS
            gravityRustRuntime
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()

if(TARGET Qt6::Widgets AND GRAVITY_TEST_INT_UI_SOURCES)
    gravity_add_gtest(gravityQtMainWindowGTests
        LABELS ui_integration integration_real
        TIMEOUT 45
        SERVER_LOCK
        SOURCES
            ${GRAVITY_TEST_INT_UI_SOURCES}
            ${GRAVITY_TEST_BASE_RUNTIME_SOURCES}
            "${GRAVITY_ROOT_DIR}/tests/support/qt_test_utils.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/server/SimulationInitConfig.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/EnergyGraphWidget.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/MainWindow.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/MultiViewWidget.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/ParticleView.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/ParticleViewColor.cpp"
            "${GRAVITY_ROOT_DIR}/modules/qt/ui/QtViewMath.cpp"
        LIBS
            gravityRustRuntime
            Qt6::Widgets
            ${GRAVITY_TEST_PLATFORM_TARGET}
    )
endif()
