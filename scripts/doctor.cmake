cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED QT_DIR OR QT_DIR STREQUAL "")
    set(QT_DIR "C:/Qt/6.8.2/msvc2022_64")
endif()
if(NOT DEFINED BUILD_DIR OR BUILD_DIR STREQUAL "")
    set(BUILD_DIR "build")
endif()

message(STATUS "[doctor] cmake version: ${CMAKE_VERSION}")

function(gravity_doctor_program name)
    unset(_prog CACHE)
    unset(_prog)
    find_program(_prog NAMES ${name})
    if(_prog)
        message(STATUS "[doctor] ${name}: ${_prog}")
    else()
        message(WARNING "[doctor] ${name}: not found in PATH")
    endif()
endfunction()

gravity_doctor_program(ninja)
gravity_doctor_program(nvcc)
gravity_doctor_program(cl)

set(_qt_deploy "${QT_DIR}/bin/windeployqt.exe")
if(EXISTS "${_qt_deploy}")
    message(STATUS "[doctor] qt deploy: ${_qt_deploy}")
else()
    message(WARNING "[doctor] qt deploy missing: ${_qt_deploy}")
endif()

set(_client_host "${BUILD_DIR}/blitzar-client.exe")
if(EXISTS "${_client_host}")
    message(STATUS "[doctor] client host: ${_client_host}")
else()
    message(WARNING "[doctor] client host missing: ${_client_host}")
endif()

set(_qt_module "${BUILD_DIR}/gravityClientModuleQtInProc.dll")
if(EXISTS "${_qt_module}")
    message(STATUS "[doctor] qt module: ${_qt_module}")
else()
    message(WARNING "[doctor] qt module missing: ${_qt_module}")
endif()

file(GLOB _qt_platform_plugins
    "${BUILD_DIR}/platforms/qwindows.dll"
    "${BUILD_DIR}/platforms/qwindowsd.dll"
)
if(_qt_platform_plugins)
    list(GET _qt_platform_plugins 0 _qt_platform_plugin)
    message(STATUS "[doctor] qt platform plugin: ${_qt_platform_plugin}")
else()
    message(WARNING "[doctor] qt platform plugin missing under: ${BUILD_DIR}/platforms")
endif()

file(GLOB _qt_core_dlls
    "${BUILD_DIR}/Qt6Core.dll"
    "${BUILD_DIR}/Qt6Cored.dll"
)
if(_qt_core_dlls)
    list(GET _qt_core_dlls 0 _qt_core_dll)
    message(STATUS "[doctor] qt core dll: ${_qt_core_dll}")
else()
    message(WARNING "[doctor] qt core dll missing under: ${BUILD_DIR}")
endif()
