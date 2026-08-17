# @file engine/platform/Module.cmake
# @brief Platform abstraction sources.

set(BLITZAR_PLATFORM_INCLUDE_DIR "${BLITZAR_ROOT_DIR}/engine/platform/include")
set(BLITZAR_PLATFORM_SOURCES
    "${BLITZAR_ROOT_DIR}/engine/platform/src/Errors.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/src/common/DynamicLibrary.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/src/common/Process.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/src/common/ProcessImpl.cpp"
    "${BLITZAR_ROOT_DIR}/engine/platform/src/common/Socket.cpp"
)
if(WIN32)
    list(APPEND BLITZAR_PLATFORM_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/platform/src/win/DynamicLibrary.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/src/win/Paths.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/src/win/Process.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/src/win/Socket.cpp"
    )
else()
    list(APPEND BLITZAR_PLATFORM_SOURCES
        "${BLITZAR_ROOT_DIR}/engine/platform/src/posix/DynamicLibrary.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/src/posix/Paths.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/src/posix/Process.cpp"
        "${BLITZAR_ROOT_DIR}/engine/platform/src/posix/SocketOps.cpp"
    )
endif()
