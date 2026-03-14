set(GRAVITY_PLATFORM_SOURCES
    engine/src/platform/PlatformErrors.cpp
    engine/src/platform/common/DynamicLibraryCommon.cpp
    engine/src/platform/common/PlatformProcessCommon.cpp
    engine/src/platform/common/PlatformProcessCommonImpl.cpp
    engine/src/platform/common/SocketPlatformCommon.cpp
)
if(WIN32)
    list(APPEND GRAVITY_PLATFORM_SOURCES
        engine/src/platform/win/DynamicLibraryWin.cpp
        engine/src/platform/win/PlatformPathsWin.cpp
        engine/src/platform/win/PlatformProcessWin.cpp
        engine/src/platform/win/SocketPlatformWin.cpp
    )
else()
    list(APPEND GRAVITY_PLATFORM_SOURCES
        engine/src/platform/posix/DynamicLibraryPosix.cpp
        engine/src/platform/posix/PlatformPathsPosix.cpp
        engine/src/platform/posix/PlatformProcessPosix.cpp
        engine/src/platform/posix/SocketPlatformPosix.cpp
    )
endif()

add_library(gravityPlatform STATIC ${GRAVITY_PLATFORM_SOURCES})
configure_gravity_cpp_target(gravityPlatform)
if(APPLE)
    target_compile_definitions(gravityPlatform
        PRIVATE
            GRAVITY_PLATFORM_DYLIB_EXT=".dylib"
    )
endif()

function(gravity_add_client_module_manifest target_name module_id)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DMODULE_FILE=$<TARGET_FILE:${target_name}>
            -DMODULE_ID=${module_id}
            -DMODULE_NAME=${target_name}
            -DAPI_VERSION=1
            -DPRODUCT_NAME=BLITZAR
            -DPRODUCT_VERSION=0.0.0-dev
            -P ${CMAKE_SOURCE_DIR}/scripts/generate_client_module_manifest.cmake
        VERBATIM
    )
endfunction()

add_library(gravityCoreFfi STATIC
    ${GRAVITY_CORE_FFI_SOURCES}
    ${GRAVITY_SERVER_SOURCES}
)
configure_gravity_cuda_target(gravityCoreFfi)
set_target_properties(gravityCoreFfi PROPERTIES OUTPUT_NAME "blitzar-core-ffi")

if(WIN32)
    target_link_libraries(gravityPlatform
        PUBLIC
            ws2_32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(gravityPlatform
        PUBLIC
            dl
    )
endif()

add_executable(${APP_NAME}
    apps/launcher/main.cpp
)
configure_gravity_cpp_target(${APP_NAME})

if(GRAVITY_BUILD_HEADLESS_BINARY)
    add_executable(${HEADLESS_NAME}
        apps/headless/main.cu
        ${GRAVITY_SERVER_SOURCES}
    )
    configure_gravity_cuda_target(${HEADLESS_NAME})
endif()

if(GRAVITY_BUILD_SERVER_DAEMON)
    add_executable(${SERVER_DAEMON_NAME}
        apps/server-service/main.cpp
        apps/server-service/server_args.cpp
        runtime/src/server/ServerDaemon.cpp
        ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
        ${GRAVITY_SERVER_SOURCES}
    )
    configure_gravity_cuda_target(${SERVER_DAEMON_NAME})
endif()

if(GRAVITY_BUILD_CLIENT_HOST)
    add_executable(${CLIENT_HOST_NAME}
        apps/client-host/main.cpp
        apps/client-host/client_host_cli.cpp
        apps/client-host/client_host_cli_args.cpp
        apps/client-host/client_host_cli_text.cpp
        apps/client-host/client_host_module_ops.cpp
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleHash.cpp
        runtime/src/client/ClientModuleHandle.cpp
        runtime/src/client/ClientModuleHandleLoad.cpp
        runtime/src/client/ClientModuleApi.cpp
        runtime/src/client/ClientModuleManifest.cpp
        engine/src/config/TextParse.cpp
    )
    configure_gravity_cpp_target(${CLIENT_HOST_NAME})
endif()

if(GRAVITY_BUILD_CLIENT_MODULES)
    add_library(${CLIENT_MODULE_CLI_NAME} MODULE
        modules/cli/module.cpp
        modules/cli/module_cli_state.cpp
        modules/cli/module_cli_text.cpp
        modules/cli/module_cli_server_ops.cpp
        modules/cli/module_cli_commands.cpp
        modules/cli/module_cli_lifecycle.cpp
        runtime/src/client/ErrorBuffer.cpp
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleApi.cpp
        ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
        engine/src/config/TextParse.cpp
    )
    configure_gravity_cpp_target(${CLIENT_MODULE_CLI_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_CLI_NAME} PRIVATE GRAVITY_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    gravity_add_client_module_manifest(${CLIENT_MODULE_CLI_NAME} cli)

    add_library(${CLIENT_MODULE_ECHO_NAME} MODULE
        modules/echo/module.cpp
        runtime/src/client/ErrorBuffer.cpp
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleApi.cpp
    )
    configure_gravity_cpp_target(${CLIENT_MODULE_ECHO_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_ECHO_NAME} PRIVATE GRAVITY_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    gravity_add_client_module_manifest(${CLIENT_MODULE_ECHO_NAME} echo)

    add_library(${CLIENT_MODULE_GUI_PROXY_NAME} MODULE
        modules/proxy/module.cpp
        runtime/src/client/ErrorBuffer.cpp
        runtime/src/client/ClientModuleBoundary.cpp
        runtime/src/client/ClientModuleApi.cpp
    )
    configure_gravity_cpp_target(${CLIENT_MODULE_GUI_PROXY_NAME})
    if(WIN32)
        target_compile_definitions(${CLIENT_MODULE_GUI_PROXY_NAME} PRIVATE GRAVITY_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
    endif()
    gravity_add_client_module_manifest(${CLIENT_MODULE_GUI_PROXY_NAME} gui)
endif()

if(GRAVITY_BUILD_CLIENT_MODULES)
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if(NOT Qt6_FOUND AND WIN32)
        file(GLOB _qt_local_roots "C:/Qt/*/msvc*_64")
        list(SORT _qt_local_roots COMPARE NATURAL ORDER DESCENDING)
        foreach(_qt_root IN LISTS _qt_local_roots)
            if(EXISTS "${_qt_root}/lib/cmake/Qt6/Qt6Config.cmake")
                list(PREPEND CMAKE_PREFIX_PATH "${_qt_root}")
                break()
            endif()
        endforeach()
        find_package(Qt6 QUIET COMPONENTS Widgets)
    endif()

    if(TARGET Qt6::Widgets)
        gravity_ensure_rust_runtime_target()
        add_library(${CLIENT_MODULE_QT_INPROC_NAME} MODULE
            modules/qt/module.cpp
            runtime/src/client/ErrorBuffer.cpp
            runtime/src/client/ClientModuleBoundary.cpp
            runtime/src/client/ClientModuleApi.cpp
            runtime/src/client/ClientServerBridge.cpp
            runtime/src/client/ClientCommon.cpp
            runtime/src/client/ClientRuntime.cpp
            runtime/src/client/RustRuntimeBridgeState.cpp
            ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
            ${GRAVITY_SERVER_SOURCES}
            modules/qt/ui/EnergyGraphWidget.cpp
            modules/qt/ui/MainWindow.cpp
            modules/qt/ui/MultiViewWidget.cpp
            modules/qt/ui/ParticleView.cpp
            modules/qt/ui/ParticleViewColor.cpp
            modules/qt/ui/QtViewMath.cpp
        )
        configure_gravity_cuda_target(${CLIENT_MODULE_QT_INPROC_NAME})
        if(WIN32)
            target_compile_definitions(${CLIENT_MODULE_QT_INPROC_NAME} PRIVATE GRAVITY_CLIENT_MODULE_EXPORT_ATTR=__declspec\(dllexport\))
        endif()
        target_link_libraries(${CLIENT_MODULE_QT_INPROC_NAME}
            PRIVATE
                gravityRustRuntime
                Qt6::Widgets
        )
        gravity_add_client_module_manifest(${CLIENT_MODULE_QT_INPROC_NAME} qt)
    else()
        message(STATUS "Qt6 not found. Qt client module is disabled.")
    endif()
endif()
