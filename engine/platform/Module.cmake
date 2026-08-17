# @file engine/platform/Module.cmake
# @brief Platform abstraction sources.

set(BLITZAR_PLATFORM_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/platform")
set(BLITZAR_PLATFORM_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/platform/PltErrors.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/common/PltDynamicLibrary.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/common/PltProcess.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/common/PltProcessImpl.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/common/PltSocket.cpp"
)
if(WIN32)
    list(APPEND BLITZAR_PLATFORM_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/platform/win/PltDynamicLibrary.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/win/PltPaths.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/win/PltProcess.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/win/PltSocket.cpp"
    )
else()
    list(APPEND BLITZAR_PLATFORM_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/platform/posix/PltDynamicLibrary.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/posix/PltPaths.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/posix/PltProcess.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/posix/PltSocketOps.cpp"
    )
endif()
