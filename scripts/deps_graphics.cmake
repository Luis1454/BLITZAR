# File: scripts/deps_graphics.cmake
# Purpose: Automation script for BLITZAR build, release, or operations tasks.

cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED TRIPLET OR TRIPLET STREQUAL "")
    set(TRIPLET "x64-windows")
endif()
if(NOT DEFINED BUILD_QT_WITH_VCPKG)
    set(BUILD_QT_WITH_VCPKG OFF)
endif()
if(NOT DEFINED VCPKG_EXE OR VCPKG_EXE STREQUAL "")
    if(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_EXE "$ENV{VCPKG_ROOT}/vcpkg.exe")
    elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
        set(VCPKG_EXE "$ENV{VCPKG_INSTALLATION_ROOT}/vcpkg.exe")
    else()
        set(VCPKG_EXE "$ENV{SystemDrive}/vcpkg/vcpkg.exe")
    endif()
endif()

if(NOT EXISTS "${VCPKG_EXE}")
    find_program(_vcpkg_fallback NAMES vcpkg vcpkg.exe)
    if(_vcpkg_fallback)
        set(VCPKG_EXE "${_vcpkg_fallback}")
    endif()
endif()

if(NOT EXISTS "${VCPKG_EXE}")
    message(FATAL_ERROR "vcpkg executable not found. Set VCPKG_EXE or install vcpkg.")
endif()

message(STATUS "[deps] using vcpkg: ${VCPKG_EXE}")

if(BUILD_QT_WITH_VCPKG)
    message(STATUS "[deps] installing qtbase:${TRIPLET}")
    execute_process(
        COMMAND "${VCPKG_EXE}" install "qtbase:${TRIPLET}" --recurse
        RESULT_VARIABLE _qt_result
    )
    if(NOT _qt_result EQUAL 0)
        message(FATAL_ERROR "vcpkg install qtbase failed with exit code ${_qt_result}")
    endif()
else()
    message(STATUS "[deps] no graphics packages requested (Qt install disabled)")
endif()

message(STATUS "[deps] done")
