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
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationConfig.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationModes.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/backend/SimulationInitConfig.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
    )
endif()

if(GRAVITY_TEST_UNIT_PROTOCOL_SOURCES)
    gravity_add_gtest(gravityProtocolCodecGTests
        LABELS unit
        SOURCES
            ${GRAVITY_TEST_UNIT_PROTOCOL_SOURCES}
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/BackendJsonCodec.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/BackendJsonCodecParse.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/BackendJsonCodecParseStatus.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/BackendJsonCodecParseSnapshot.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/BackendJsonCodecReadNumber.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/protocol/BackendProtocol.cpp"
            "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
    )
endif()

set(GRAVITY_TEST_UNIT_MODULE_SOURCES
    ${GRAVITY_TEST_UNIT_MODULE_CLI_SOURCES}
    ${GRAVITY_TEST_UNIT_MODULE_HOST_SOURCES}
)
if(GRAVITY_TEST_UNIT_MODULE_SOURCES)
    gravity_add_gtest(gravityModuleCliHostGTests
        LABELS unit
        SOURCES
            ${GRAVITY_TEST_UNIT_MODULE_SOURCES}
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_state.cpp"
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_text.cpp"
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_backend_ops.cpp"
            "${GRAVITY_ROOT_DIR}/modules/cli/module_cli_commands.cpp"
            "${GRAVITY_ROOT_DIR}/apps/module-host/module_host_cli_args.cpp"
            "${GRAVITY_ROOT_DIR}/apps/module-host/module_host_cli_text.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/frontend/ErrorBuffer.cpp"
            "${GRAVITY_ROOT_DIR}/runtime/src/frontend/FrontendModuleBoundary.cpp"
            ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
            "${GRAVITY_ROOT_DIR}/engine/src/config/TextParse.cpp"
        LIBS
            ${GRAVITY_TEST_PLATFORM_TARGET}
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
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
    "${GRAVITY_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
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
