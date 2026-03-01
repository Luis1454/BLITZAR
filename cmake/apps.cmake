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
        ${GRAVITY_BACKEND_SOURCES}
    )
    configure_gravity_cuda_target(${HEADLESS_NAME})
endif()

if(GRAVITY_BUILD_BACKEND_DAEMON)
    add_executable(${BACKEND_DAEMON_NAME}
        apps/backend-service/main.cpp
        apps/backend-service/backend_args.cpp
        runtime/src/backend/BackendServer.cpp
        ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
        ${GRAVITY_BACKEND_SOURCES}
    )
    configure_gravity_cuda_target(${BACKEND_DAEMON_NAME})
endif()

if(GRAVITY_BUILD_FRONTEND_MODULE_HOST)
    add_executable(${MODULE_HOST_NAME}
        apps/module-host/main.cpp
        apps/module-host/module_host_cli.cpp
        runtime/src/frontend/FrontendModuleHandle.cpp
        runtime/src/frontend/FrontendModuleHandleLoad.cpp
        runtime/src/frontend/FrontendModuleApi.cpp
    )
    configure_gravity_cpp_target(${MODULE_HOST_NAME})
endif()

if(GRAVITY_BUILD_FRONTEND_MODULES)
    add_library(${FRONTEND_MODULE_CLI_NAME} MODULE
        modules/cli/module.cpp
        runtime/src/frontend/ErrorBuffer.cpp
        runtime/src/frontend/FrontendModuleApi.cpp
        ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
        engine/src/config/TextParse.cpp
    )
    configure_gravity_cpp_target(${FRONTEND_MODULE_CLI_NAME})

    add_library(${FRONTEND_MODULE_ECHO_NAME} MODULE
        modules/echo/module.cpp
        runtime/src/frontend/ErrorBuffer.cpp
        runtime/src/frontend/FrontendModuleApi.cpp
    )
    configure_gravity_cpp_target(${FRONTEND_MODULE_ECHO_NAME})

    add_library(${FRONTEND_MODULE_GUI_PROXY_NAME} MODULE
        modules/proxy/module.cpp
        runtime/src/frontend/ErrorBuffer.cpp
        runtime/src/frontend/FrontendModuleApi.cpp
    )
    configure_gravity_cpp_target(${FRONTEND_MODULE_GUI_PROXY_NAME})
endif()

if(GRAVITY_BUILD_FRONTEND_MODULES)
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
        add_library(${FRONTEND_MODULE_QT_INPROC_NAME} MODULE
            modules/qt/module.cpp
            runtime/src/frontend/ErrorBuffer.cpp
            runtime/src/frontend/FrontendModuleApi.cpp
            runtime/src/frontend/FrontendBackendBridge.cpp
            runtime/src/frontend/FrontendCommon.cpp
            runtime/src/frontend/FrontendRuntime.cpp
            runtime/src/frontend/LocalBackendFactory.cpp
            ${GRAVITY_RUNTIME_PROTOCOL_SOURCES}
            ${GRAVITY_BACKEND_SOURCES}
            modules/qt/ui/EnergyGraphWidget.cpp
            modules/qt/ui/MainWindow.cpp
            modules/qt/ui/MultiViewWidget.cpp
            modules/qt/ui/ParticleView.cpp
            modules/qt/ui/ParticleViewColor.cpp
            modules/qt/ui/QtViewMath.cpp
        )
        configure_gravity_cuda_target(${FRONTEND_MODULE_QT_INPROC_NAME})
        target_link_libraries(${FRONTEND_MODULE_QT_INPROC_NAME}
            PRIVATE
                Qt6::Widgets
        )
    else()
        message(STATUS "Qt6 not found. Qt in-process frontend module is disabled.")
    endif()
endif()
