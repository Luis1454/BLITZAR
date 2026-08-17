# @file cmake/apps.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

add_library(blitzarPlatform STATIC ${BLITZAR_PLATFORM_SOURCES})
set_target_properties(blitzarPlatform PROPERTIES POSITION_INDEPENDENT_CODE ON)
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
    "${BLITZAR_ROOT_DIR}/runtime/command/execution/CmdBatchRunner.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/command/catalog/CmdCatalog.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/command/execution/CmdExecutor.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/command/parsing/CmdParser.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/command/transport/CmdTransport.cpp"
)
if(WIN32)
    set(BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES ${BLITZAR_CONFIG_ENV_SOURCE})
else()
    set(BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES ${BLITZAR_CONFIG_ENV_SOURCE})
endif()
set(BLITZAR_COMMAND_CONFIG_SOURCES
    ${BLITZAR_CONFIG_COMMAND_SOURCES}
    ${BLITZAR_SERVER_INIT_SOURCE}
)
add_library(blitzarCoreFfi STATIC
    ${BLITZAR_CORE_FFI_SOURCES}
    ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    ${BLITZAR_SERVER_SOURCES}
)
set_target_properties(blitzarCoreFfi PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(BLITZAR_ENABLE_CUDA)
    configure_BLITZAR_cuda_target(blitzarCoreFfi)
else()
    configure_BLITZAR_cpp_target(blitzarCoreFfi)
endif()
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
    apps/launcher/src/main.cpp
)
configure_BLITZAR_cpp_target(${APP_NAME})

if(BLITZAR_BUILD_CLIENT_HOST AND BLITZAR_BUILD_CLIENT_MODULES)
    if(WIN32)
        add_executable(${DESKTOP_GUI_NAME} WIN32
            apps/desktop/src/Main.cpp
            apps/desktop/src/WindowsMain.cpp
        )
    else()
        add_executable(${DESKTOP_GUI_NAME}
            apps/desktop/src/Main.cpp
            apps/desktop/src/PosixMain.cpp
        )
    endif()
    configure_BLITZAR_cpp_target(${DESKTOP_GUI_NAME})
endif()

if(BLITZAR_BUILD_HEADLESS_BINARY)
    add_executable(${HEADLESS_NAME}
        apps/headless/src/main.cpp
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
        ${BLITZAR_BATCH_SOURCES}
    )
    if(BLITZAR_ENABLE_CUDA)
        configure_BLITZAR_cuda_target(${HEADLESS_NAME})
    else()
        configure_BLITZAR_cpp_target(${HEADLESS_NAME})
    endif()
    target_link_libraries(${HEADLESS_NAME} PRIVATE blitzarPlatform OpenMP::OpenMP_CXX)
    target_compile_options(${HEADLESS_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-Xptxas=-v>
    )
endif()

if(BLITZAR_BUILD_SERVER_DAEMON)
    add_executable(${SERVER_DAEMON_NAME}
        apps/server-service/src/main.cpp
        apps/server-service/src/Args.cpp
        runtime/server/core/SrvDaemon.cpp
        runtime/server/core/SrvDaemonCommands.cpp
        runtime/server/core/SrvDaemonPhysics.cpp
        runtime/server/core/SrvDaemonPersistence.cpp
        runtime/server/core/SrvDaemonTransport.cpp
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
        ${BLITZAR_SERVER_SOURCES}
    )
    if(BLITZAR_ENABLE_CUDA)
        configure_BLITZAR_cuda_target(${SERVER_DAEMON_NAME})
    else()
        configure_BLITZAR_cpp_target(${SERVER_DAEMON_NAME})
    endif()
    target_link_libraries(${SERVER_DAEMON_NAME} PRIVATE blitzarPlatform OpenMP::OpenMP_CXX)
endif()
if(BLITZAR_BUILD_WEB_GATEWAY)
    BLITZAR_ensure_rust_web_gateway_target()
endif()
if(BLITZAR_BUILD_CLIENT_HOST)
    add_executable(${CLIENT_HOST_NAME}
        apps/client-host/src/main.cpp
        apps/client-host/src/Cli.cpp
        apps/client-host/src/CliArgs.cpp
        apps/client-host/src/CliText.cpp
        apps/client-host/src/ModuleOps.cpp
        ${BLITZAR_RUNTIME_COMMAND_SOURCES}
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
        ${BLITZAR_COMMAND_CONFIG_SOURCES}
        runtime/client/module/CliBoundary.cpp
        runtime/client/module/CliHash.cpp
        runtime/client/module/CliHandle.cpp
        runtime/client/module/CliLoad.cpp
        runtime/client/module/CliApi.cpp
        runtime/client/common/ClientCommon.cpp
        runtime/client/module/CliManifest.cpp
        ${BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES}
        engine/config/text/CfgParse.cpp
    )
    configure_BLITZAR_cpp_target(${CLIENT_HOST_NAME})
endif()

if(BLITZAR_BUILD_CLIENT_MODULES)
    add_library(${CLIENT_MODULE_CLI_NAME} MODULE
        modules/cli/Module.cpp
        modules/cli/State.cpp
        modules/cli/Text.cpp
        modules/cli/ServerOps.cpp
        modules/cli/Commands.cpp
        modules/cli/Lifecycle.cpp
        ${BLITZAR_RUNTIME_COMMAND_SOURCES}
        ${BLITZAR_COMMAND_CONFIG_SOURCES}
        runtime/client/diagnostics/CliErrorBuffer.cpp
        runtime/client/module/CliBoundary.cpp
        runtime/client/module/CliApi.cpp
        runtime/client/common/ClientCommon.cpp
        ${BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES}
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
    )
    configure_BLITZAR_cpp_target(${CLIENT_MODULE_CLI_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_CLI_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    BLITZAR_add_client_module_manifest(${CLIENT_MODULE_CLI_NAME} cli)

    add_library(${CLIENT_MODULE_ECHO_NAME} MODULE
        modules/echo/Module.cpp
        runtime/client/diagnostics/CliErrorBuffer.cpp
        runtime/client/module/CliBoundary.cpp
        runtime/client/module/CliApi.cpp
    )
    configure_BLITZAR_cpp_target(${CLIENT_MODULE_ECHO_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_ECHO_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    BLITZAR_add_client_module_manifest(${CLIENT_MODULE_ECHO_NAME} echo)

    add_library(${CLIENT_MODULE_GUI_PROXY_NAME} MODULE
        modules/proxy/Module.cpp
        modules/proxy/Support.cpp
        runtime/client/diagnostics/CliErrorBuffer.cpp
        runtime/client/module/CliBoundary.cpp
        runtime/client/module/CliApi.cpp
    )
    configure_BLITZAR_cpp_target(${CLIENT_MODULE_GUI_PROXY_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_GUI_PROXY_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    BLITZAR_add_client_module_manifest(${CLIENT_MODULE_GUI_PROXY_NAME} gui)

    BLITZAR_find_qt6_widgets()

    if(TARGET Qt6::Widgets AND TARGET Qt6::OpenGLWidgets)
        BLITZAR_ensure_rust_runtime_target()
        add_library(${CLIENT_MODULE_QT_INPROC_NAME} MODULE
            modules/qt/GuiModule.cpp
            runtime/client/diagnostics/CliErrorBuffer.cpp
            runtime/client/module/CliBoundary.cpp
            runtime/client/module/CliApi.cpp
            runtime/client/runtime/CliBridge.cpp
            runtime/client/runtime/CliCommands.cpp
            runtime/client/runtime/CliInitialState.cpp
            runtime/client/runtime/CliRemoteSession.cpp
            runtime/client/common/ClientCommon.cpp
            runtime/client/runtime/CliRuntime.cpp
            runtime/client/runtime/CliBridgeState.cpp
            runtime/ffi/bridge/FfiApi.cpp
            ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
            ${BLITZAR_SERVER_SOURCES}
            modules/qt/src/widgets/graphs/GuiGraph.cpp
            modules/qt/src/widgets/graphs/GuiPaint.cpp
            modules/qt/src/widgets/graphs/GuiSpectrumGraph.cpp
            modules/qt/src/window/control/GuiController.cpp
            modules/qt/src/window/core/GuiWindow.cpp
            modules/qt/src/window/core/GuiWidgets.cpp
            modules/qt/src/window/config/GuiWindowConfig.cpp
            modules/qt/src/window/config/GuiWindowConfigUi.cpp
            modules/qt/src/window/config/GuiConfigurationEditor.cpp
            modules/qt/src/window/scene/GuiSceneEditor.cpp
            modules/qt/src/window/scene/GuiSceneEditorFields.cpp
            modules/qt/src/window/scene/GuiSceneEditorProperties.cpp
            modules/qt/src/window/scene/GuiSceneEditorState.cpp
            modules/qt/src/window/control/GuiControls.cpp
            modules/qt/src/window/actions/GuiFileActions.cpp
            modules/qt/src/window/layout/GuiLayout.cpp
            modules/qt/src/window/layout/GuiState.cpp
            modules/qt/src/window/layout/GuiStateDefaults.cpp
            modules/qt/src/window/presentation/GuiPresenter.cpp
            modules/qt/src/window/presentation/GuiTelemetry.cpp
            modules/qt/src/window/workspace/GuiPersistence.cpp
            modules/qt/src/window/workspace/GuiShell.cpp
            modules/qt/src/widgets/viewport/GuiMultiView.cpp
            modules/qt/src/widgets/viewport/GuiRenderSnapshot.cpp
            modules/qt/src/widgets/viewport/GuiGpuView.cpp
            modules/qt/src/widgets/viewport/GuiGpuViewInput.cpp
            modules/qt/src/widgets/viewport/GuiShaderSources.cpp
            modules/qt/src/widgets/overlays/GuiOctree.cpp
            modules/qt/src/widgets/overlays/GuiPainter.cpp
            modules/qt/src/widgets/viewport/GuiParticle.cpp
            modules/qt/src/widgets/viewport/GuiColor.cpp
            modules/qt/src/support/types/GuiEnums.cpp
            modules/qt/src/support/performance/GuiThroughput.cpp
            modules/qt/src/support/theme/GuiTheme.cpp
            modules/qt/src/support/geometry/GuiViewMath.cpp
            modules/qt/src/support/storage/GuiLayoutStore.cpp
            modules/qt/src/panels/control/GuiPhysics.cpp
            modules/qt/src/panels/control/GuiDisclosure.cpp
            modules/qt/src/panels/control/GuiRender.cpp
            modules/qt/src/panels/control/GuiRun.cpp
            ${BLITZAR_GRAPHICS_SOURCES}
        )
        if(BLITZAR_ENABLE_CUDA)
            configure_BLITZAR_cuda_target(${CLIENT_MODULE_QT_INPROC_NAME})
        else()
            configure_BLITZAR_cpp_target(${CLIENT_MODULE_QT_INPROC_NAME})
        endif()
        if(WIN32)
            target_compile_definitions(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
        endif()
        target_link_libraries(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE Qt6::Widgets Qt6::OpenGL
            Qt6::OpenGLWidgets OpenMP::OpenMP_CXX)
        if(TARGET blitzarRustRuntime)
            target_link_libraries(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE blitzarRustRuntime)
        endif()
        BLITZAR_configure_qt_runtime_deploy(${CLIENT_MODULE_QT_INPROC_NAME})
        BLITZAR_add_client_module_manifest(${CLIENT_MODULE_QT_INPROC_NAME} qt)
    else()
        message(STATUS "Qt6 not found. Qt client module is disabled.")
    endif()
endif()
