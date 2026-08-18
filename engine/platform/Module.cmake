# @file engine/platform/Module.cmake
# @brief Platform abstraction sources.

set(BLITZAR_PLATFORM_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/platform")
set(BLITZAR_PLATFORM_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/platform/errors/PltErrors.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/dynamic_library/PltDynamicLibrary.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcess.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcessImpl.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/socket/PltSocket.cpp"
)
if(WIN32)
    list(APPEND BLITZAR_PLATFORM_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/platform/dynamic_library/PltDynamicLibraryWin.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/paths/PltPathsWin.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcessWin.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/socket/PltSocketWin.cpp"
    )
else()
    list(APPEND BLITZAR_PLATFORM_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/platform/dynamic_library/PltDynamicLibraryPosix.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/paths/PltPathsPosix.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/process/PltProcessPosix.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/socket/PltSocketPosix.cpp"
    )
endif()
