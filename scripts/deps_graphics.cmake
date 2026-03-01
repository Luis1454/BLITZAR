cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED TRIPLET OR TRIPLET STREQUAL "")
    set(TRIPLET "x64-windows")
endif()
if(NOT DEFINED BUILD_QT_WITH_VCPKG)
    set(BUILD_QT_WITH_VCPKG OFF)
endif()
if(NOT DEFINED VCPKG_EXE OR VCPKG_EXE STREQUAL "")
    set(VCPKG_EXE "C:/vcpkg/vcpkg.exe")
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
message(STATUS "[deps] installing sfml:${TRIPLET}")
execute_process(
    COMMAND "${VCPKG_EXE}" install "sfml:${TRIPLET}" --recurse
    RESULT_VARIABLE _sfml_result
)
if(NOT _sfml_result EQUAL 0)
    message(FATAL_ERROR "vcpkg install sfml failed with exit code ${_sfml_result}")
endif()

if(BUILD_QT_WITH_VCPKG)
    message(STATUS "[deps] installing qtbase:${TRIPLET}")
    execute_process(
        COMMAND "${VCPKG_EXE}" install "qtbase:${TRIPLET}" --recurse
        RESULT_VARIABLE _qt_result
    )
    if(NOT _qt_result EQUAL 0)
        message(FATAL_ERROR "vcpkg install qtbase failed with exit code ${_qt_result}")
    endif()
endif()

message(STATUS "[deps] done")
