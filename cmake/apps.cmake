# @file cmake/apps.cmake
# @author Luis1454
# @project BLITZAR
# @brief CMake build orchestration for BLITZAR targets and tooling.

set(BLITZAR_PLATFORM_SOURCES
    engine/src/platform/Errors.cpp
    engine/src/platform/common/DynamicLibrary.cpp
    engine/src/platform/common/Process.cpp
    engine/src/platform/common/ProcessImpl.cpp
    engine/src/platform/common/Socket.cpp
)
if(WIN32)
    list(APPEND BLITZAR_PLATFORM_SOURCES
        engine/src/platform/win/DynamicLibrary.cpp
        engine/src/platform/win/Paths.cpp
        engine/src/platform/win/Process.cpp
        engine/src/platform/win/Socket.cpp
    )
else()
    list(APPEND BLITZAR_PLATFORM_SOURCES
        engine/src/platform/posix/DynamicLibrary.cpp
        engine/src/platform/posix/Paths.cpp
        engine/src/platform/posix/Process.cpp
        engine/src/platform/posix/SocketOps.cpp
    )
endif()
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
    "${BLITZAR_ROOT_DIR}/runtime/src/command/execution/BatchRunner.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/catalog/Catalog.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/execution/Executor.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/parsing/Parser.cpp"
    "${BLITZAR_ROOT_DIR}/runtime/src/command/transport/Transport.cpp"
)
if(WIN32)
    set(BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Base.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Win.cpp"
    )
else()
    set(BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Base.cpp"
        "${BLITZAR_ROOT_DIR}/engine/src/config/env/Posix.cpp"
    )
endif()
set(BLITZAR_COMMAND_CONFIG_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/src/config/args/Parse.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Main.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Apply.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/registry/Entries.cpp"
    "${BLITZAR_ROOT_DIR}/engine/src/config/profile/Performance.cpp"
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
if(BLITZAR_BUILD_HEADLESS_BINARY)
    add_executable(${HEADLESS_NAME}
        apps/headless/src/main.cpp
        ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
        ${BLITZAR_SERVER_SOURCES}
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
        runtime/src/server/core/Daemon.cpp
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
        runtime/src/client/module/Boundary.cpp
        runtime/src/client/module/Hash.cpp
        runtime/src/client/module/Handle.cpp
        runtime/src/client/module/Load.cpp
        runtime/src/client/module/Api.cpp
        runtime/src/client/common/ClientCommon.cpp
        runtime/src/client/module/Manifest.cpp
        ${BLITZAR_CLIENT_COMMON_SUPPORT_SOURCES}
        engine/src/config/text/Parse.cpp
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
        runtime/src/client/diagnostics/ErrorBuffer.cpp
        runtime/src/client/module/Boundary.cpp
        runtime/src/client/module/Api.cpp
        runtime/src/client/common/ClientCommon.cpp
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
        runtime/src/client/diagnostics/ErrorBuffer.cpp
        runtime/src/client/module/Boundary.cpp
        runtime/src/client/module/Api.cpp
    )
    configure_BLITZAR_cpp_target(${CLIENT_MODULE_ECHO_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_ECHO_NAME} PRIVATE BLITZAR_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    BLITZAR_add_client_module_manifest(${CLIENT_MODULE_ECHO_NAME} echo)

    add_library(${CLIENT_MODULE_GUI_PROXY_NAME} MODULE
        modules/proxy/Module.cpp
        modules/proxy/Support.cpp
        runtime/src/client/diagnostics/ErrorBuffer.cpp
        runtime/src/client/module/Boundary.cpp
        runtime/src/client/module/Api.cpp
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
            modules/qt/Module.cpp
            runtime/src/client/diagnostics/ErrorBuffer.cpp
            runtime/src/client/module/Boundary.cpp
            runtime/src/client/module/Api.cpp
            runtime/src/client/runtime/Bridge.cpp
            runtime/src/client/common/ClientCommon.cpp
            runtime/src/client/runtime/Runtime.cpp
            runtime/src/client/runtime/BridgeState.cpp
            runtime/src/ffi/bridge/Api.cpp
            ${BLITZAR_RUNTIME_PROTOCOL_SOURCES}
            ${BLITZAR_SERVER_SOURCES}
            modules/qt/src/widgets/graphs/Graph.cpp
            modules/qt/src/widgets/graphs/Paint.cpp
            modules/qt/src/window/control/Controller.cpp
            modules/qt/src/window/core/Window.cpp
            modules/qt/src/window/core/Widgets.cpp
            modules/qt/src/window/config/WindowConfig.cpp
            modules/qt/src/window/control/Controls.cpp
            modules/qt/src/window/actions/FileActions.cpp
            modules/qt/src/window/layout/Layout.cpp
            modules/qt/src/window/layout/State.cpp
            modules/qt/src/window/presentation/Presenter.cpp
            modules/qt/src/window/presentation/Telemetry.cpp
            modules/qt/src/window/workspace/Persistence.cpp
            modules/qt/src/window/workspace/Shell.cpp
            modules/qt/src/widgets/viewport/MultiView.cpp
            modules/qt/src/widgets/overlays/Octree.cpp
            modules/qt/src/widgets/overlays/Painter.cpp
            modules/qt/src/widgets/viewport/Particle.cpp
            modules/qt/src/widgets/viewport/Color.cpp
            modules/qt/src/support/types/Enums.cpp
            modules/qt/src/support/performance/Throughput.cpp
            modules/qt/src/support/theme/Theme.cpp
            modules/qt/src/support/geometry/ViewMath.cpp
            modules/qt/src/support/storage/LayoutStore.cpp
            modules/qt/src/panels/control/Physics.cpp
            modules/qt/src/panels/control/Render.cpp
            modules/qt/src/panels/control/Run.cpp
            modules/qt/src/panels/control/SceneSetup.cpp
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
        target_link_libraries(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE Qt6::Widgets)
        if(TARGET blitzarRustRuntime)
            target_link_libraries(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE blitzarRustRuntime)
        endif()
        BLITZAR_configure_qt_runtime_deploy(${CLIENT_MODULE_QT_INPROC_NAME})
        BLITZAR_add_client_module_manifest(${CLIENT_MODULE_QT_INPROC_NAME} qt)
    else()
        message(STATUS "Qt6 not found. Qt client module is disabled.")
    endif()
endif()
