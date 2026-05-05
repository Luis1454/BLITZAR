# @file cmake/apps.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

set(BLITZAR_PLATFORM_SOURCES
    engine/src/platform/PlatformErrors.cpp
    engine/src/platform/common/DynamicLibraryCommon.cpp
    engine/src/platform/common/PlatformProcessCommon.cpp
    engine/src/platform/common/PlatformProcessCommonImpl.cpp
    engine/src/platform/common/SocketPlatformCommon.cpp
)
if(WIN32)
    list(APPEND BLITZAR_PLATFORM_SOURCES
        engine/src/platform/win/DynamicLibraryWin.cpp
        engine/src/platform/win/PlatformPathsWin.cpp
        engine/src/platform/win/PlatformProcessWin.cpp
        engine/src/platform/win/SocketPlatformWin.cpp
    )
else()
    list(APPEND BLITZAR_PLATFORM_SOURCES
        engine/src/platform/posix/DynamicLibraryPosix.cpp
        engine/src/platform/posix/PlatformPathsPosix.cpp
        engine/src/platform/posix/PlatformProcessPosix.cpp
        engine/src/platform/posix/SocketPlatformPosix.cpp
    )
endif()
add_library(blitzarPlatform STATIC ${BLITZAR_PLATFORM_SOURCES})
configure_BLITZAR_cpp_target(blitzarPlatform)
if(APPLE)
    target_compile_definitions(blitzarPlatform
        PRIVATE
            BLITZAR_PLATFORM_DYLIB_EXT=".dylib"
    )
endif()

function(BLITZAR_add_client_module_manifest target_name module_id)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DMODULE_FILE=$<TARGET_FILE:${target_name}>
            -DMODULE_ID=${module_id}
            -DMODULE_NAME=${target_name}
            -DAPI_VERSION=1
            -DPRODUCT_NAME=BLITZAR
            -DPRODUCT_VERSION=0.0.0-dev
            -P ${BLITZAR_ROOT_DIR}/scripts/generate_client_module_manifest.cmake
        VERBATIM
    )
endfunction()

include("${BLITZAR_ROOT_DIR}/cmake/qt_paths.cmake")

set(BLITZAR_RUNTIME_COMMAND_SOURCES
    "${BLITZAR_ROOT_DIR}/runtime/src/command/CommandBatchRunner.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/CommandCatalog.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/CommandExecutor.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/CommandParser.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/CommandTransport.cpp"
)
if(WIN32)
    set(BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtilsWin.cpp"
    )
else()
    set(BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtils.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/EnvUtilsPosix.cpp"
    )
endif()
set(BLITZAR_COMMAND_CONFIG_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationArgsParse.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistry.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryApply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationOptionRegistryEntries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/SimulationPerformanceProfile.cpp"
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
add_library(blitzarCoreFfi STATIC
    ${BLITZAR_CORE_FFI_SOURCES}
    ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    ${BLITZAR_SERVER_SOURCES}
)
configure_BLITZAR_cuda_target(blitzarCoreFfi)
set_target_properties(blitzarCoreFfi PROPERTIES OUTPUT_NAME "blitzar-core-ffi")
if(WIN32)
    target_link_libraries(blitzarPlatform
        PUBLIC
            ws2_32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(blitzarPlatform
        PUBLIC
            dl
    )
endif()

add_executable(${APP_NAME}
    apps/launcher/main.cpp
)
configure_BLITZAR_cpp_target(${APP_NAME})
if(BLITZAR_BUILD_HEADLESS_BINARY)
    add_executable(${HEADLESS_NAME}
        apps/headless/main.cu
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
        ${BLITZAR_SERVER_SOURCES}
    )
    configure_BLITZAR_cuda_target(${HEADLESS_NAME})
    target_compile_options(${HEADLESS_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-Xptxas=-v>
    )
endif()

if(BLITZAR_BUILD_SERVER_DAEMON)
    add_executable(${SERVER_DAEMON_NAME}
        apps/server-service/main.cpp
        apps/server-service/server_args.cpp
        runtime/src/server/daemon/daemon.cpp
        runtime/src/server/daemon/lifecycle.cpp
        runtime/src/server/daemon/acceptance.cpp
        runtime/src/server/daemon/handler.cpp
        runtime/src/server/daemon/requestProcessor.cpp
        runtime/src/server/daemon/protocol/parser.cpp
        runtime/src/server/daemon/protocol/dispatcher.cpp
        runtime/src/server/daemon/protocol/stateCommands.cpp
        runtime/src/server/daemon/protocol/configCommands.cpp
        runtime/src/server/daemon/helpers.cpp
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
        ${BLITZAR_SERVER_SOURCES}
    )
    configure_BLITZAR_cuda_target(${SERVER_DAEMON_NAME})
endif()
if(BLITZAR_BUILD_WEB_GATEWAY)
    BLITZAR_ensure_rust_web_gateway_target()
endif()
if(BLITZAR_BUILD_CLIENT_HOST)
    add_executable(${CLIENT_HOST_NAME}
        apps/client-host/main.cpp
        apps/client-host/client_host_cli.cpp
        apps/client-host/client_host_cli_args.cpp
        apps/client-host/client_host_cli_text.cpp
        apps/client-host/client_host_module_ops.cpp
        ${BLITZAR_RUNTIME_COMMAND_SOURCES}
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
        ${BLITZAR_COMMAND_CONFIG_SOURCES}
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleHash.cpp
        runtime/src/client/ClientModuleHandle.cpp
        runtime/src/client/ClientModuleHandleLoad.cpp
        runtime/src/client/ClientModuleApi.cpp
        runtime/src/client/ClientCommon.cpp
        runtime/src/client/ClientModuleManifest.cpp
        ${BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES}
        engine/src/config/TextParse.cpp
    )
    configure_BLITZAR_cpp_target(${CLIENT_HOST_NAME})
endif()

if(BLITZAR_BUILD_CLIENT_MODULES)
    add_library(${CLIENT_MODULE_CLI_NAME} MODULE
        modules/cli/module.cpp
        modules/cli/module_cli_state.cpp
        modules/cli/module_cli_text.cpp
        modules/cli/module_cli_server_ops.cpp
        modules/cli/module_cli_commands.cpp
        modules/cli/module_cli_lifecycle.cpp
        ${BLITZAR_RUNTIME_COMMAND_SOURCES}
        ${BLITZAR_COMMAND_CONFIG_SOURCES}
        runtime/src/client/ErrorBuffer.cpp
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleApi.cpp
        runtime/src/client/ClientCommon.cpp
        ${BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES}
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    )
    configure_BLITZAR_cpp_target(${CLIENT_MODULE_CLI_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_CLI_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    BLITZAR_add_client_module_manifest(${CLIENT_MODULE_CLI_NAME} cli)

    add_library(${CLIENT_MODULE_ECHO_NAME} MODULE
        modules/echo/module.cpp
        runtime/src/client/ErrorBuffer.cpp
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleApi.cpp
    )
    configure_BLITZAR_cpp_target(${CLIENT_MODULE_ECHO_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_ECHO_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    BLITZAR_add_client_module_manifest(${CLIENT_MODULE_ECHO_NAME} echo)

    add_library(${CLIENT_MODULE_GUI_PROXY_NAME} MODULE
        modules/proxy/module.cpp
        runtime/src/client/ErrorBuffer.cpp
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleApi.cpp
    )
    configure_BLITZAR_cpp_target(${CLIENT_MODULE_GUI_PROXY_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_GUI_PROXY_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    BLITZAR_add_client_module_manifest(${CLIENT_MODULE_GUI_PROXY_NAME} gui)

    BLITZAR_find_qt6_widgets()

    if(TARGET Qt6::Widgets)
        BLITZAR_ensure_rust_runtime_target()
        add_library(${CLIENT_MODULE_QT_INPROC_NAME} MODULE
            modules/qt/module.cpp
            runtime/src/client/ErrorBuffer.cpp
            runtime/src/client/ClientModuleBoundary.cpp
            runtime/src/client/ClientModuleApi.cpp
            runtime/src/client/ClientServerBridge.cpp
            runtime/src/client/ClientCommon.cpp
            runtime/src/client/ClientRuntime.cpp
            runtime/src/client/RustRuntimeBridgeState.cpp
            ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
            ${BLITZAR_SERVER_SOURCES}
            modules/qt/ui/EnergyGraphWidget.cpp
            modules/qt/ui/EnergyGraphWidgetPaint.cpp
            modules/qt/ui/energyGraph/renderer.cpp
            modules/qt/ui/energyGraph/plotting.cpp
            modules/qt/ui/energyGraph/theme.cpp
            modules/qt/ui/energyGraph/layout.cpp
            modules/qt/ui/energyGraph/data.cpp
            modules/qt/ui/mainWindow/presenter/formatters.cpp
            modules/qt/ui/mainWindow/presenter/telemetryAggregator.cpp
            modules/qt/ui/mainWindow/presenter/stateComputers.cpp
            modules/qt/ui/mainWindow/shell/dockBuilder.cpp
            modules/qt/ui/mainWindow/shell/menuBuilder.cpp
            modules/qt/ui/mainWindow/shell/themeMenuHandler.cpp
            modules/qt/ui/mainWindow/layout/comboInitializer.cpp
            modules/qt/ui/mainWindow/layout/numericInitializer.cpp
            modules/qt/ui/mainWindow/layout/propertyInitializer.cpp
            modules/qt/ui/mainWindow/fileActions/fileDialogs.cpp
            modules/qt/ui/mainWindow/fileActions/connectorManager.cpp
            modules/qt/ui/mainWindow/fileActions/formatHelpers.cpp
            modules/qt/ui/particleView/gimbalController.cpp
            modules/qt/ui/particleView/particleRasterizer.cpp
            modules/qt/ui/particleView/overlayRenderer.cpp
            modules/qt/ui/MainWindowController.cpp
            modules/qt/ui/MainWindow.cpp
            modules/qt/ui/MainWindowConfig.cpp
            modules/qt/ui/mainWindow/config/apply.cpp
            modules/qt/ui/MainWindowControls.cpp
            modules/qt/ui/MainWindowFileActions.cpp
            modules/qt/ui/MainWindowLayout.cpp
            modules/qt/ui/MainWindowLayoutState.cpp
            modules/qt/ui/MainWindowPresenter.cpp
            modules/qt/ui/MainWindowTelemetry.cpp
            modules/qt/ui/MainWindowWorkspacePersistence.cpp
            modules/qt/ui/MainWindowWorkspaceShell.cpp
            modules/qt/ui/MultiViewWidget.cpp
            modules/qt/ui/OctreeOverlay.cpp
            modules/qt/ui/OctreeOverlayPainter.cpp
            modules/qt/ui/ParticleView.cpp
            modules/qt/ui/ParticleViewColor.cpp
            modules/qt/ui/UiEnums.cpp
            modules/qt/ui/ThroughputAdvisor.cpp
            modules/qt/ui/QtTheme.cpp
            modules/qt/ui/QtViewMath.cpp
            modules/qt/ui/WorkspaceLayoutStore.cpp
            modules/qt/ui/panels/PhysicsControlPanel.cpp
            modules/qt/ui/panels/RenderControlPanel.cpp
            modules/qt/ui/panels/RunControlPanel.cpp
            modules/qt/ui/panels/SceneSetupPanel.cpp
            ${BLITZAR_GRAPHICS_SOURCES}
        )
        configure_BLITZAR_cuda_target(${CLIENT_MODULE_QT_INPROC_NAME})
        if(WIN32)
            target_compile_definitions(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
        endif()
        target_link_libraries(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE blitzarRustRuntime Qt6::Widgets)
        BLITZAR_configure_qt_runtime_deploy(${CLIENT_MODULE_QT_INPROC_NAME})
        BLITZAR_add_client_module_manifest(${CLIENT_MODULE_QT_INPROC_NAME} qt)
    else()
        message(STATUS "Qt6 not found. Qt client module is disabled.")
    endif()
endif()
